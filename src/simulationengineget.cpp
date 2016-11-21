#include <iostream>
#include <fstream>
#include <map>
#include <cmath>
#if 0
#include <CellmlSimulator.hpp>
#endif

#include "common.hpp"
#include "molecule.hpp"
#include "GeneralModel.hpp"
#include "cvodes.hpp"
#include "kinsol.hpp"
#include "dataset.hpp"
#include "simulationengineget.hpp"

SimulationEngineGet::SimulationEngineGet()
{
#if 0
    mCsim = new CellmlSimulator();
#endif
}

SimulationEngineGet::~SimulationEngineGet()
{
#if 0
    if (mCsim) delete mCsim;
#endif
}

int SimulationEngineGet::loadModel(const std::string& modelUrl)
{
#if 0
    std::string flattenedModel = mCsim->serialiseCellmlFromUrl(modelUrl);
    if (flattenedModel == "")
    {
        std::cerr << "Error serializing model: " << modelUrl << std::endl;
        return -1;
    }
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
#endif
    return -1;
}

int SimulationEngineGet::addOutputVariable(const MyVariable &variable, int columnIndex)
{
#if 0
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
#endif
    return -1;
}

int SimulationEngineGet::initialiseSimulation()
{
    GeneralModel model;
    setDebugLevel(0);

    // define our model
    Molecule molecule;
    // Cl - just change the defaults...
    molecule.typeId = "http://cellml.sourceforge.net/ns/ion/Cl";
    molecule.z = -1.0;
    molecule.C_a = molecule.C_b = 102.0; molecule.C_c = 16.0;
    molecule.P_a = 0.0; molecule.P_b = 541.0e-9; molecule.P_j = 3.0e-9;
    model.addMolecule(molecule);
    // Na - just change the defaults...
    molecule.typeId = "http://cellml.sourceforge.net/ns/ion/Na";
    molecule.z = 1.0;
    molecule.C_a = molecule.C_b = 104.0; molecule.C_c = 7.0;
    molecule.P_a = 100.0e-9; molecule.P_b = 20.0e-9; molecule.P_j = 3.0e-9;
    model.addMolecule(molecule);
    // K - just change the defaults...
    molecule.typeId = "http://cellml.sourceforge.net/ns/ion/K";
    molecule.z = 1.0;
    molecule.C_a = molecule.C_b = 5.3; molecule.C_c = 72.0;
    molecule.P_a = 50.0e-9; molecule.P_b = 463.0e-9; molecule.P_j = 3.0e-9;
    model.addMolecule(molecule);
    // X1 - just change the defaults...
    molecule.typeId = "http://cellml.sourceforge.net/ns/ion/X1";
    molecule.z = -1.0;
    molecule.C_a = molecule.C_b = 7.3; molecule.C_c = 63.0;
    molecule.P_a = 0.0; molecule.P_b = 0.0; molecule.P_j = 0.0;
    model.addMolecule(molecule);
    // Cl - just change the defaults...
    molecule.typeId = "http://cellml.sourceforge.net/ns/ion/X2";
    molecule.z = 1.0;
    molecule.C_a = molecule.C_b = 81.4; molecule.C_c = 142.0;
    molecule.P_a = 0.0; molecule.P_b = 0.0; molecule.P_j = 0.0;
    model.addMolecule(molecule);

    // cell parameters
    // data from Table 1 in Latta et al (1984)
    model.Lp_a = 1e-12; model.Lp_b = 1e-11; // cm^3/(dyne second)
    model.A_a = 1.8; model.A_b = 8.8; // cm^2/cm^2 tissue
    model.V = 0.001; // cm^3/cm^2 tissue

    // initial guesses for membrane potentials
    model.E_a = -20.0;
    model.E_b = -60.0;
    model.E_t = -40.0;

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
              /*<< "C_c[Na] = " << model.C_c[GeneralModel::Na]
              << "; C_c[K] = " << model.C_c[GeneralModel::K]
              << "; C_c[Cl] = " << model.C_c[GeneralModel::Cl] << "\n"*/
              << "E_t = " << model.E_t << "; E_a = " << model.E_a
              << "; E_b = " << model.E_b << "\n"
              << "V/V(t=0) = " << model.V / initialVolume << std::endl;

    return 0;
}

std::vector<double> SimulationEngineGet::getOutputValues()
{
    return std::vector<double>();
}
