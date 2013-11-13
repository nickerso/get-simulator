#ifndef SIMULATIONENGINEGET_HPP
#define SIMULATIONENGINEGET_HPP

#include <vector>

class SimulationEngineGet
{
public:
    SimulationEngineGet();
    ~SimulationEngineGet();

    /**
     * @brief Dummy method to set up output variables.
     * @return zero on success.
     */
    int setOutputVariables();

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
};

#endif // SIMULATIONENGINEGET_HPP
