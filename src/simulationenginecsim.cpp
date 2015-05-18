#include <iostream>
#include <map>
#include <cmath>
#include <CellmlSimulator.hpp>
#include <vector>

#include "dataset.hpp"
#include "simulationenginecsim.hpp"
#include "setvaluechange.hpp"
#include "utils.hpp"

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
    // first flatten the model
    std::string flattenedModel = mCsim->serialiseCellmlFromUrl(modelUrl);
    if (flattenedModel == "")
    {
        std::cerr << "Error serializing model: " << modelUrl.c_str() << std::endl;
        return -1;
    }
    // and then apply the changes
    //flattenedModel = mCsim->setVariableValue()
    // and load the model
    if (mCsim->loadModelString(flattenedModel) != 0)
    {
        std::cerr << "Error loading model string: " << flattenedModel.c_str() << std::endl;
        return -2;
    }
    // create the default simulation definition
    if (mCsim->createSimulationDefinition() != 0)
    {
        std::cerr <<"Error creating default simulation definition." << std::endl;
        return -3;
    }
    return 0;
}

int SimulationEngineCsim::addOutputVariable(const MyData &data, int columnIndex)
{
    int numberOfErrors = 0;
    std::string variableId = mCsim->mapXpathToVariableId(data.target, data.namespaces);
    if (variableId.length() > 0)
    {
        std::cout << "\t\tAdding output variable: '" << variableId << "'" << std::endl;
        mCsim->addOutputVariable(variableId, columnIndex);
    }
    else
    {
        std::cerr << "Unable to map output variable target to a variable in the model: " << data.target << std::endl;
        ++numberOfErrors;
    }
    return numberOfErrors;
}

int SimulationEngineCsim::initialiseSimulation(double initialTime, double startTime)
{
    if (mCsim->compileModel() != 0)
    {
        std::cerr <<"SimulationEngineCsim::initialiseSimulation - Error compiling model." << std::endl;
        return -1;
    }
    if (fabs(startTime-initialTime) > 1.0e-8)
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
    return 0;
}

std::vector<double> SimulationEngineCsim::getOutputValues()
{
    return mCsim->getModelOutputs();
}

int SimulationEngineCsim::simulateModelOneStep(double dt)
{
    return mCsim->simulateModelOneStep(dt);
}

int SimulationEngineCsim::resetSimulator(bool resetModel)
{
    int returnCode = mCsim->resetIntegrator();
    if (returnCode != 0) return returnCode;
    // the current checkpoint is the initial state of the model.
    if (resetModel) returnCode = mCsim->updateModelFromCheckpoint();
    return returnCode;
}

int SimulationEngineCsim::applySetValueChange(const MySetValueChange& change)
{
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
}
