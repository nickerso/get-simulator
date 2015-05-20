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
};

SimulationEngineCsim::SimulationEngineCsim()
{
    mCsim = new CellmlSimulator();
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
    return numberOfErrors;
}

int SimulationEngineCsim::initialiseSimulation(double initialTime, double startTime)
{
    if (mCsim->model.instantiate(true) != csim::CSIM_OK)
    {
        std::cerr <<"SimulationEngineCsim::initialiseSimulation - Error compiling model." << std::endl;
        return -1;
    }
    mCsim->initialiseFunction = mCsim->model.getInitialiseFunction();
    mCsim->modelFunction = mCsim->model.getModelFunction();
#if 0
    if (fabs(startTime-initialTime) > 0.0)
    {
        std::vector<std::vector<double> > results = mCsim->simulateModel(initialTime, startTime, startTime, 1);
        if (results.size() != 2)
        {
            std::cerr << "SimulationEngineCsim::initialiseSimulation - simulation results has more than one entry?! ("
                      << results.size() << ")" << std::endl;
            return -2;
        }
    }
    mCsim->checkpointModelValues();
#endif
    return 0;
}

std::vector<double> SimulationEngineCsim::getOutputValues()
{
#if 0
    return mCsim->getModelOutputs();
#endif
    return std::vector<double>();
}

int SimulationEngineCsim::simulateModelOneStep(double dt)
{
#if 0
    return mCsim->simulateModelOneStep(dt);
#endif
    return -1;
}

int SimulationEngineCsim::resetSimulator(bool resetModel)
{
#if 0
    int returnCode = mCsim->resetIntegrator();
    if (returnCode != 0) return returnCode;
    // the current checkpoint is the initial state of the model.
    if (resetModel) returnCode = mCsim->updateModelFromCheckpoint();
    return returnCode;
#endif
    return -1;
}

int SimulationEngineCsim::applySetValueChange(const MySetValueChange& change)
{
#if 0
    int returnCode = 0;
    std::string modifiedTarget = mapToStandardVariableXpath(change.targetXpath);
    std::string variableId = mCsim->mapXpathToVariableId(modifiedTarget, change.namespaces);
    if (variableId.length() > 0)
    {
        std::cout << "\t\tSetting variable: '" << variableId << "'" << std::endl;
        mCsim->setVariableValue(variableId, change.currentRangeValue);
    }
    else
    {
        std::cerr << "Unable to map output variable target to a variable in the model: " << change.targetXpath << std::endl;
        returnCode = -1;
    }
    return returnCode;
#endif
    return -1;
}
