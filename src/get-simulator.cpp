/*
 * get.cpp
 *
 *  Created on: Sep 11, 2012
 *      Author: dnic019
 */

#include <iostream>
#include <fstream>

#include "common.hpp"
#include "GeneralModel.hpp"
#include "cvodes.hpp"
#include "kinsol.hpp"

/*
  Can GET be a collection of code that gets combined with the generated code from CellML models and then compiled by LLVM at run time? or is GET an application that calls code generated from CellML models as required?
 */

/*
 * Given a "complete" CellML model describing the whole epithelial cell model configuration, we can
 * create a GeneralModel with the required configuration by interrogating the CellML model directly.
 *
 * FIXME: eventually this would be done using the annotations, but for now assume predefined
 *        component/variable names.
 *
 * Once we have the CellML model (code) we can put the required values into the places where they are needed
 * and perform 'standard' model evaluations...in theory. We will also want to configure the simulation to be
 * performed using SED-ML - but that is step 2.
 */

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <CellML model>" << std::endl;
        return -1;
    }

    // Main algorithm from Latta et al (1984), Figure 2.
    GeneralModel model(argv[1]);

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

    /*
     * and now switch to the short-circuit case
     */
    cvodes.reInitialise(t); // reset the integrator
    model.modelMode = GeneralModel::ShortCircuit;
    model.U_t = 0.0;
    t_final += 1500;
    while (t <= t_final)
    {
        double tout = t + delta_t;
        if (cvodes.integrate(t, tout) != 0)
        {
            std::cerr << "get: integration failed in short-circuit block" << std::endl;
            output.close();
            return 2;
        }
        model.printState(output, t);
    }

    /*
     * and the salt stepper mode to test the sodium pump
     */
    //setDebugLevel(51);
    model.initialiseSaltStepper(); // change the model mode and parameters
    // integrate to get a steady state
    cvodes.reInitialise(t); // reset the integrator
    t_final += 500;
    while (t <= t_final)
    {
        double tout = t + delta_t;
        if (cvodes.integrate(t, tout) != 0)
        {
            std::cerr << "get: integration failed in salt-stepper block" << std::endl;
            output.close();
            return 3;
        }
        model.printState(output, t);
    }
    // and now the steps in NaCl
    std::vector<double> NaCl;
    NaCl.push_back(13.3);
    NaCl.push_back(26.7);
    NaCl.push_back(40.0);
    NaCl.push_back(53.3);
    NaCl.push_back(66.7);
    double duration = 300.0;
    //setDebugLevel(51);
    for (unsigned int i = 0; i < NaCl.size(); ++i)
    {
        model.C_b[GeneralModel::Na] = NaCl[i];
        model.C_b[GeneralModel::Cl] = NaCl[i];
        double serosalChangeTime = t;
        t_final += duration;
        bool mucosalSet = false;
        while (t <= t_final)
        {
            if (!mucosalSet && (t >= (serosalChangeTime + 10.0)))
            {
                // 10 seconds later
                model.C_a[GeneralModel::Na] = NaCl[i];
                model.C_a[GeneralModel::Cl] = NaCl[i];
                mucosalSet = true;
            }
            double tout = t + delta_t;
            if (cvodes.integrate(t, tout) != 0)
            {
                std::cerr << "get: integration failed in salt-stepper block 2 part " << i << std::endl;
                output.close();
                return 4;
            }
            model.printState(output, t);
        }
    }
    // and the final step puts Imax=0 simulating the addition of ouabain to the serosal solution
    model.Imax = 0.0;
    t_final += 500.0;
    while (t <= t_final)
    {
        double tout = t + delta_t;
        if (cvodes.integrate(t, tout) != 0)
        {
            std::cerr << "get: integration failed in final salt-stepper block" << std::endl;
            output.close();
            return 5;
        }
        model.printState(output, t);
    }

    output.close();
    return 0;
}


