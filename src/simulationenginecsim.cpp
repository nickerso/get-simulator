#include <iostream>
#include <map>
#include <cmath>
#include <vector>

#include <csim/model.h>
#include <csim/error_codes.h>
#include <csim/executable_functions.h>

#include <cvode/cvode.h>             /* main integrator header file */
#include <nvector/nvector_serial.h>  /* serial N_Vector types, fct. and macros */
#include <sundials/sundials_types.h> /* definition of realtype */

#include "dataset.hpp"
#include "simulationenginecsim.hpp"
#include "setvaluechange.hpp"
#include "utils.hpp"

/* Functions called by CVODE */
static int f(realtype t, N_Vector y, N_Vector ydot, void *user_data);
/* Private function to check function return values */
static int check_flag(void *flagvalue, const char *funcname, int opt);

/* Shared Problem Constants */
//#define ATOL RCONST(1.0e-6)
//#define RTOL RCONST(0.0)
#define ZERO_TOL RCONST(1.0e-7)

// hide the details from the caller?
class CellmlSimulator
{
public:
    CellmlSimulator() : mCvode(0), mMethod(UNKOWN_ALG)
    {

    }
    ~CellmlSimulator()
    {
        if (mCvode) CVodeFree(&mCvode);
    }

    csim::Model model;
    csim::InitialiseFunction initialiseFunction;
    csim::ModelFunction modelFunction;
    double voi, maxStepSize;
    std::vector<double> states, outputs, inputs;
    N_Vector nv_states, nv_rates;
    struct
    {
        double voi;
        std::vector<double> states, outputs, inputs;
    } cache;

    int createIntegrator(const MySimulation& simulation, double x0)
    {
        if (simulation.mMethod == "KISAO:0000019") mMethod = CVODE_ALG;
        else if (simulation.mMethod == "KISAO:0000030") mMethod = EULER_ALG;
        // initialise our variable of integration
        voi = x0;
        // create and initialise our CVODE integrator
        //double reltol = RTOL, abstol = ATOL;
        if (mMethod == CVODE_ALG)
        {
            mCvode = CVodeCreate(CV_ADAMS, CV_FUNCTIONAL);
            if(check_flag(mCvode, "CVodeCreate", 0)) return(1);
            nv_states = N_VMake_Serial(states.size(), states.data());
            nv_rates = N_VNew_Serial(states.size());
            int flag = CVodeInit(mCvode, f, x0, nv_states);
            if(check_flag(&flag, "CVodeInit", 1)) return(1);
            flag = CVodeSStolerances(mCvode, simulation.relativeTolerance,
                                     simulation.absoluteTolerance);
            if(check_flag(&flag, "CVodeSStolerances", 1)) return(1);
            flag = CVodeSetMaxStep(mCvode, simulation.maximumStepSize);
            if(check_flag(&flag, "CVodeSetMaxStep", 1)) return(1);
            // add our user data
            flag = CVodeSetUserData(mCvode, (void*)(this));
            if (check_flag(&flag,"CVodeSetUserData",1)) return(1);
        }
        else
        {
            nv_states = N_VMake_Serial(states.size(), states.data());
            nv_rates = N_VNew_Serial(states.size());
            maxStepSize = simulation.maximumStepSize;
        }
        return 0;
    }

    void callInitialise()
    {
        initialiseFunction(states.data(), outputs.data(), inputs.data());
    }
    void callModel()
    {
        modelFunction(voi, NV_DATA_S(nv_states), NV_DATA_S(nv_rates), outputs.data(), inputs.data());
    }
    int simulateModelOneStep(double step)
    {
        if (fabs(step) < ZERO_TOL) return 0; // nothing to do
        double xout = voi + step;
        if (mMethod == CVODE_ALG)
        {
            int flag = CVodeSetStopTime(mCvode, xout);
            if (check_flag(&flag,"CVodeSetStopStime",1)) return(1);
            flag = CVode(mCvode, xout, nv_states, &voi, CV_NORMAL);
            if (check_flag(&flag,"CVode",1)) return(1);
            // make sure the non-state variables are at the correct time
            callModel();
        }
        else if (mMethod == EULER_ALG)
        {
            // FIXME: for now assume always a +'ve direction in stepping
            while (voi < xout)
            {
                voi += maxStepSize;
                std::cout << "voi = " << voi << "; xout = " << xout << std::endl;
                if (voi > xout) voi = xout;
                callModel();
                for (unsigned i = 0; i < states.size(); ++i)
                {
                    NV_Ith_S(nv_states, i) += maxStepSize * NV_Ith_S(nv_rates, i);
                }
            }
            callModel();
        }
        return 0;
    }
    void checkpointModelValues()
    {
        cache.voi = voi;
        cache.inputs = inputs;
        cache.outputs = outputs;
        cache.states = states;
    }
    void updateModelFromCheckpoint()
    {
        voi = cache.voi;
        inputs = cache.inputs;
        outputs = cache.outputs;
        states = cache.states;
    }
private:
    void* mCvode;
    enum Method {
        CVODE_ALG = 1,
        EULER_ALG = 2,
        UNKOWN_ALG = -1
    };
    int mMethod;
};

SimulationEngineCsim::SimulationEngineCsim()
{
    mCsim = new CellmlSimulator();
    mInitialised = false;
}

SimulationEngineCsim::~SimulationEngineCsim()
{
    if (mCsim) delete mCsim;
}

int SimulationEngineCsim::loadModel(const std::string &modelUrl)
{
    if (mCsim->model.loadCellmlModel(modelUrl) != csim::CSIM_OK)
    {
        std::cerr << "Error loading CellML model: " << modelUrl << std::endl;
        return -2;
    }
    return 0;
}

int SimulationEngineCsim::addOutputVariable(MyData &data)
{
    int numberOfErrors = 0;
    std::string variableId = mCsim->model.mapXpathToVariableId(data.target, data.namespaces);
    data.outputIndex = mCsim->model.setVariableAsOutput(variableId);
    if (data.outputIndex < 0)
    {
        std::cerr << "Unable to map output variable target to a variable in the model: " << data.target
                  << "(id: " << variableId << ")" << "; error code: " << data.outputIndex << std::endl;
        ++numberOfErrors;
    }
    else
    {
        // make sure the output vector is big enough
        if (data.outputIndex >= (int)mCsim->outputs.size()) mCsim->outputs.resize(data.outputIndex+1);
    }
    return numberOfErrors;
}

int SimulationEngineCsim::addInputVariable(MySetValueChange& change)
{
    int numberOfErrors = 0;
    std::string variableId = mCsim->model.mapXpathToVariableId(change.targetXpath, change.namespaces);
    change.inputIndex = mCsim->model.setVariableAsInput(variableId);
    if (change.inputIndex < 0)
    {
        std::cerr << "Unable to map input variable target to a variable in the model: " << change.targetXpath
                  << " (id: " << variableId << ")" << "; error code: " << change.inputIndex << std::endl;
        ++numberOfErrors;
    }
    else
    {
        // make sure the inputs vector is large enough
        if (change.inputIndex >= (int)mCsim->inputs.size()) mCsim->inputs.resize(change.inputIndex+1);
    }
    return numberOfErrors;
}

int SimulationEngineCsim::instantiateSimulation()
{
    if (mCsim->model.instantiate() != csim::CSIM_OK)
    {
        std::cerr <<"SimulationEngineCsim::initialiseSimulation - Error compiling model." << std::endl;
        return -1;
    }
    mCsim->initialiseFunction = mCsim->model.getInitialiseFunction();
    mCsim->modelFunction = mCsim->model.getModelFunction();
    mCsim->states.resize(mCsim->model.numberOfStateVariables());
    mCsim->callInitialise();
    return 0;
}

int SimulationEngineCsim::initialiseSimulation(const MySimulation& simulation, double initialTime, double startTime)
{
    // create our integrator
    if (mCsim->createIntegrator(simulation, initialTime) != 0)
    {
        std::cerr << "SimulationEngineCsim::initialiseSimulation: error creating CVODE integrator." << std::endl;
        return -1;
    }
    // ensure the model is correct for the given initial value
    mCsim->callModel();
    // and checkpoint the model at the initial point
    mCsim->checkpointModelValues();
    // and then integrate to the start time in "one" step
    mCsim->simulateModelOneStep(startTime-initialTime);
    mInitialised = true;
    return 0;
}

const std::vector<double>& SimulationEngineCsim::getOutputValues()
{
    return mCsim->outputs;
}

int SimulationEngineCsim::simulateModelOneStep(double step)
{
    if (mInitialised) return mCsim->simulateModelOneStep(step);
    std::cerr << "SimulationEngineCsim::simulateModelOneStep: must initialise "
                 "simulation prior to calling this method." << std::endl;
    return -1;
}

int SimulationEngineCsim::resetSimulator(bool resetModel)
{
    // nothing to do with the "integrator" at present.
    // the current checkpoint is the initial state of the model.
    if (resetModel) mCsim->updateModelFromCheckpoint();
    return 0;
}

int SimulationEngineCsim::applySetValueChange(const MySetValueChange& change)
{
    if (change.inputIndex >= (int)mCsim->inputs.size())
    {
        std::cerr << "SimulationEngineCsim::applySetValueChange: invalid input index."
                  << " inputIndex = " << change.inputIndex << "; inputs.size() = "
                  << mCsim->inputs.size() << std::endl;
        return -1;
    }
    mCsim->inputs[change.inputIndex] = change.currentRangeValue;
    return 0;
}

int f(realtype x, N_Vector y, N_Vector ydot, void *user_data)
{
    CellmlSimulator* ud = (CellmlSimulator*)user_data;
    ud->modelFunction(x, NV_DATA_S(y), NV_DATA_S(ydot), ud->outputs.data(), ud->inputs.data());
    return 0;
}

static int check_flag(void *flagvalue, const char *funcname, int opt)
{
  int *errflag;

  /* Check if SUNDIALS function returned NULL pointer - no memory allocated */
  if (opt == 0 && flagvalue == NULL) {
    fprintf(stderr, "\nSUNDIALS_ERROR: %s() failed - returned NULL pointer\n\n",
            funcname);
    return(1); }

  /* Check if flag < 0 */
  else if (opt == 1) {
    errflag = (int *) flagvalue;
    if (*errflag < 0) {
      fprintf(stderr, "\nSUNDIALS_ERROR: %s() failed with flag = %d\n\n",
              funcname, *errflag);
      return(1); }}

  /* Check if function returned NULL pointer - no memory allocated */
  else if (opt == 2 && flagvalue == NULL) {
    fprintf(stderr, "\nMEMORY_ERROR: %s() failed - returned NULL pointer\n\n",
            funcname);
    return(1); }

  return(0);
}
