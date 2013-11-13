#include <iostream>
#include <fstream>
#include <map>
#include <cmath>
#include <CellmlSimulator.hpp>

#include "common.hpp"
#include "GeneralModel.hpp"
#include "cvodes.hpp"
#include "kinsol.hpp"
#include "dataset.hpp"
#include "simulationengineget.hpp"

SimulationEngineGet::SimulationEngineGet()
{
}

SimulationEngineGet::~SimulationEngineGet()
{
    // nothing to do?
}

int SimulationEngineGet::setOutputVariables()
{
    // FIXME: not implemented yet
    return 0;
}

int SimulationEngineGet::initialiseSimulation()
{
    GeneralModel model();
    setDebugLevel(0);

    // set up output
    std::ofstream output;
    output.open("results.data");
    output.precision(5);
    output.setf(std::ios_base::uppercase | std::ios_base::scientific);
    // set the way numbers should be streamed out
    std::cout.precision(5);
    std::cout.setf(std::ios_base::uppercase | std::ios_base::scientific);

    // we'll use CVODES for integration
    Cvodes cvodes;

    /*
     * 1) transport parameters, initial conditions, t_initial, t_final, and
     * delta_t are specified, and integration variable t is initialised to t_initial.
     */
    double t_initial = 0.0; // seconds
    double t_final = 1500; // seconds
    double delta_t = 1.0; // seconds
    double t = t_initial;
    model.initialise();
    model.printStateHeader(output);
    model.printState(output, t_initial);
    cvodes.initialise(&model, t_initial, delta_t);

    double initialVolume = model.V;

    /*
     * Solve to steady state in the open-circuit mode.
     */
    model.modelMode = GeneralModel::OpenCircuit;
    while (t <= t_final)
    {
        double tout = t + delta_t;
        /*
         * 2) membrane potentials are computed.
         * @FIXME: this needs to be done inside the CVODE RHS function...but maybe ok if small enough time step?
         */
        //        model.mode = 1; // we want to solve for membrane potentials at the open-circuit conditions
        //        if (solveOneVariable(&model) > 0)
        //        {
        //            std::cerr << "Failed to solve open circuit case" << std::endl;
        //            break;
        //        }

        /*
         * 3) solute and water fluxes are computed using the membrane potentials and the
         * respective flux equations.
         */
        /*
         * 4) the state equations are evaluated and are integrated to t+delta_t.
         */
        if (cvodes.integrate(t, tout) != 0)
        {
            std::cerr << "get: integration failed in steady-state block" << std::endl;
            output.close();
            return 1;
        }
        /*
         * 5) additional variables are computed, and all results saved off-line.
         */
        /*
         * 6) t is updated to t+delta_t and a comparison is made with t_final. If t_final
         * has been reached, the computation ends. Otherwise, the program returns to step
         * 2 to continue the integration.
         */

        model.printState(output, t);
    }

    // dump out SS results to compare to table 2 in Latta et al paper.
    std::cout << "Steady state results\n"
              << "====================\n"
              << "C_c[Na] = " << model.C_c[GeneralModel::Na]
              << "; C_c[K] = " << model.C_c[GeneralModel::K]
              << "; C_c[Cl] = " << model.C_c[GeneralModel::Cl] << "\n"
              << "E_t = " << model.E_t << "; E_a = " << model.E_a
              << "; E_b = " << model.E_b << "\n"
              << "V/V(t=0) = " << model.V / initialVolume << std::endl;

    return 0;
}

std::vector<double> SimulationEngineGet::getOutputValues()
{
    return std::vector<double>();
}
