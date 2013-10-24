#include <iostream>
#include <map>
#include <CellmlSimulator.hpp>

#include "dataset.hpp"
#include "simulationenginecsim.hpp"

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
    std::string flattenedModel = mCsim->serialiseCellmlFromUrl(modelUrl);
    if (flattenedModel == "")
    {
        std::cerr << "Error serializing model: " << modelUrl.c_str() << std::endl;
        return -1;
    }
    if (mCsim->loadModelString(flattenedModel) != 0)
    {
        std::cerr << "Error loading model string: " << flattenedModel.c_str() << std::endl;
        return -2;
    }
    return 0;
}

double SimulationEngineCsim::getInitialTime() const
{
    return mInitialTime;
}

void SimulationEngineCsim::setInitialTime(double value)
{
    mInitialTime = value;
}
double SimulationEngineCsim::getOutputStartTime() const
{
    return mOutputStartTime;
}

void SimulationEngineCsim::setOutputStartTime(double value)
{
    mOutputStartTime = value;
}
double SimulationEngineCsim::getOutputEndTime() const
{
    return mOutputEndTime;
}

void SimulationEngineCsim::setOutputEndTime(double value)
{
    mOutputEndTime = value;
}
int SimulationEngineCsim::getNumberOfPoints() const
{
    return mNumberOfPoints;
}

void SimulationEngineCsim::setNumberOfPoints(int value)
{
    mNumberOfPoints = value;
}

