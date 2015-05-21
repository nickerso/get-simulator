#ifndef SIMULATIONENGINECSIM_HPP
#define SIMULATIONENGINECSIM_HPP

#include <string>
#include <vector>

#include "dataset.hpp"

class CellmlSimulator;
class MySetValueChange;

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

    /**
     * @brief Add the given data to this CSim instance's list of output variables.
     * This method will update the data object with information on where it is stored in the outputs of the model. All
     * outputs must be flagged prior to initialising the simulation.
     * @sa initialiseSimulation, addOutputVariable
     * @param data The data to register as an output variable.
     * @return zero on success, non-zero on failure.
     */
    int addOutputVariable(MyData& data);

    /**
     * @brief Add the required variable from the given set value change to this CSim's list of input variables.
     * This method will update the change object with the information on where it is stored in the inputs of the model.
     * All inputs must be flagged prior to initialising the simulation.
     * @sa initialiseSimulation, addInputVariable
     * @param change The set value change to flag as an input for this simulation.
     * @return zero on success, non-zero on failure.
     */
    int addInputVariable(MySetValueChange& change);

    /**
     * @brief Instantiate the simulation for this instance of CSim.
     * Will cause the model to be compiled, and thus should only be called once all inputs
     * and outputs have been flagged as they can not be added after the simulation is
     * instantiated.
     * @return zero on success.
     */
    int instantiateSimulation();

    /**
     * @brief Initialise the simulation.
     * Should be called after the simulation is instantiated and after any changes have been
     * applied to the model.
     * @param initialTime The initial value of the variable of integration.
     * @param startTime The final value of the variable of integration, the value at
     * which the user will start collecting output.
     * @return zero on success.
     */
    int initialiseSimulation(double initialTime, double startTime);

    /**
     * @brief Fetch the current values of the output variables for this instance of the simulation engine.
     * @return A vector of the output variable values.
     */
    const std::vector<double>& getOutputValues();

    /**
     * @brief Simulate the model for one time period.
     * The simulation must be initialised prior to calling this method.
     * @param step The size of the step to take.
     * @return zero on success.
     */
    int simulateModelOneStep(double step);

    /**
     * @brief Reset the simulator.
     * @param resetModel If true, the model will be reset back to initial conditions.
     * @return zero on success.
     */
    int resetSimulator(bool resetModel);

    /**
     * @brief Apply the change described by the set value change. Assumes its always a trivial set variable value.
     * @param changes The set-value change to make, from a repeated task.
     * @return zero on success.
     */
    int applySetValueChange(const MySetValueChange& change);

private:
    std::string mModelUrl;
    CellmlSimulator* mCsim;
    // will only be true once the simulation has been initialised.
    bool mInitialised;
};

#endif // SIMULATIONENGINECSIM_HPP
