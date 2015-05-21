#include <iostream>
#include <map>
#include <cmath>
#include <vector>

#include <csim/model.h>
#include <csim/error_codes.h>
#include <csim/executable_functions.h>

#include "dataset.hpp"
#include "simulationenginecsim.hpp"
#include "setvaluechange.hpp"
#include "utils.hpp"

// hide the details from the caller?
class CellmlSimulator
{
public:
    csim::Model model;
    csim::InitialiseFunction initialiseFunction;
    csim::ModelFunction modelFunction;
    double voi;
    std::vector<double> states, rates, outputs, inputs;
    struct
    {
        double voi;
        std::vector<double> states, outputs, inputs;
    } cache;
    void callInitialise()
    {
        initialiseFunction(states.data(), outputs.data(), inputs.data());
    }
    void callModel()
    {
        modelFunction(voi, states.data(), rates.data(), outputs.data(), inputs.data());
    }
    int simulateModelOneStep(double step)
    {
        // hard code Euler for now
        double dt = step / 1000.0; // really arbitrary :)
        double end = voi+step;
        while (fabs(end-voi) > 1.0e-8)
        {
            voi += dt;
            if (fabs(end - voi) > 1.0e-8) voi = end;
            // calculate rates
            callModel();
            // euler update
            for (int i = 0; i < states.size(); ++i) states[i] += rates[i] * dt;
        }
        // make sure the non-state variables are at the correct time
        callModel();
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
        if (data.outputIndex >= mCsim->outputs.size()) mCsim->outputs.resize(data.outputIndex+1);
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
        if (change.inputIndex >= mCsim->inputs.size()) mCsim->inputs.resize(change.inputIndex+1);
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
    mCsim->rates.resize(mCsim->model.numberOfStateVariables());
    mCsim->callInitialise();
    return 0;
}

int SimulationEngineCsim::initialiseSimulation(double initialTime, double startTime)
{
    // set the initial VOI value
    mCsim->voi = initialTime;
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
    if (change.inputIndex >= mCsim->inputs.size())
    {
        std::cerr << "SimulationEngineCsim::applySetValueChange: invalid input index."
                  << " inputIndex = " << change.inputIndex << "; inputs.size() = "
                  << mCsim->inputs.size() << std::endl;
        return -1;
    }
    mCsim->inputs[change.inputIndex] = change.currentRangeValue;
    return 0;
}
