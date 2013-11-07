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

    /**
     * @brief Add the given data to this CSim instance's list of output variables.
     * @param data The data to register as an output variable.
     * @param columnIndex The index of this variable in the output array (first index = 1).
     * @return zero on success, non-zero on failure.
     */
    int addOutputVariable(const MyData& data, int columnIndex);

    /**
     * @brief Compile the model into an executable object. Should be called after all output variables have been added.
     * @return zero on success.
     */
    int compileModel();

private:
    std::string mModelUrl;
    double mInitialTime;
    double mOutputStartTime;
    double mOutputEndTime;
    int mNumberOfPoints;
    CellmlSimulator* mCsim;
};

#endif // SIMULATIONENGINECSIM_HPP
