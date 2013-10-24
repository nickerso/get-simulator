#ifndef SIMULATIONENGINECSIM_HPP
#define SIMULATIONENGINECSIM_HPP

#include <string>

class CellmlSimulator;

class SimulationEngineCsim
{
public:
    SimulationEngineCsim();
    ~SimulationEngineCsim();

    /**
     * @brief Load the model from the given <modelUrl> into this CSim simulation engine.
     * @param modelUrl The URL of the CellML model to load.
     * @return zero on success, non-zero on failure.
     */
    int loadModel(const std::string& modelUrl);

    double getInitialTime() const;
    void setInitialTime(double value);

    double getOutputStartTime() const;
    void setOutputStartTime(double value);

    double getOutputEndTime() const;
    void setOutputEndTime(double value);

    int getNumberOfPoints() const;
    void setNumberOfPoints(int value);

private:
    std::string mModelUrl;
    double mInitialTime;
    double mOutputStartTime;
    double mOutputEndTime;
    int mNumberOfPoints;
    CellmlSimulator* mCsim;
};

#endif // SIMULATIONENGINECSIM_HPP
