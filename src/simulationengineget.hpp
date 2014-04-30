#ifndef SIMULATIONENGINEGET_HPP
#define SIMULATIONENGINEGET_HPP

#include <vector>

class CellmlSimulator;

class SimulationEngineGet
{
public:
    SimulationEngineGet();
    ~SimulationEngineGet();

    /**
     * @brief Load the specified model into this instance of the GET simulation tool.
     * @param modelUrl The CellML model to load.
     * @return zero on success.
     */
    int loadModel(const std::string& modelUrl);

    /**
     * @brief Add the given data to this GET instance's list of output variables.
     * @param data The data to register as an output variable.
     * @param columnIndex The index of this variable in the output array (first index = 1).
     * @return zero on success, non-zero on failure.
     */
    int addOutputVariable(const MyData& data, int columnIndex);

    /**
     * @brief Initialise this instance of the GET simulator.
     * @return zero on success.
     */
    int initialiseSimulation();

    /**
     * @brief Get the current value of the output variables registered for this instance of the GET simulator.
     * @return A vector containing the values of the output variable values.
     */
    std::vector<double> getOutputValues();

private:
    std::string mModelUrl;
    CellmlSimulator* mCsim;
};

#endif // SIMULATIONENGINEGET_HPP
