/*
 * GeneralModel.hpp
 *
 *  Created on: Nov 22, 2012
 *      Author: dnic019
 */

/*
  Step 1) Create general model, set number of species, solve with just passive fluxes.
  Step 2) 1 + ability to add extra membrane fluxes.
  Step 3) 1 + ability to add extra membrane fluxes defined in CellML.
  */
#ifndef GENERALMODEL_HPP_
#define GENERALMODEL_HPP_

#include <vector>

/**
 * This class defines a general epithelial transport model based on the Latta et al (1984)
 * modelling of epithelial transport paper.
 */
class GeneralModel
{
public:
    GeneralModel();
    ~GeneralModel();

	/**
	 * Initialise the model
	 */
    void initialise();

    /**
      * Initialise the NaCl stepping protocol to test the sodium pump electrogenicity.
      */
    void initialiseSaltStepper();

	/**
	 * Solve for electroneutrality in the open-circuit case (fig 4 in Latta paper).
	 */
    double solveOpenCircuitPotentials(int& errorFlag, const bool updateOnly = false);

	/**
	 * Solve for electroneutrality in the voltage-clamp case (fig 3 in Latta paper).
	 */
    double solveVoltageClampPotentials(const bool updateOnly = false);

    /**
     * Compute the current flux values for this model.
     */
    void calculateSoluteMembraneFluxes();

    /**
     * Compute the paracellular flux values for this model.
     */
    void calculateSoluteParacellularFluxes();

    /**
      * Generic method for evaluating passive fluxes.
      */
    void calculatePassiveFluxes(std::vector<double>& J, const std::vector<double>& P,
                                const std::vector<double>& z, const std::vector<double>& C1,
                                const std::vector<double>& C2, const double U);

    /**
      * Compute the water fluxes for the current state of the cell.
      */
    void calculateWaterFluxes();

    /**
      Print the current state of the model.
      */
    void printState(std::ostream& s, double& time);

    /**
      Print the header for the states in this model.
      */
    void printStateHeader(std::ostream& s);

    /**
      Calculate the RHS of the differential equation system.
      */
    std::vector<double> calculateRHS(double time, int& errorFlag);

	void compute_I_a();
    void compute_I_b();
    void compute_I_j();

	// the current mode for Kinsol
	int mode;

    // the current mode for the model
    enum ModelMode
    {
        OpenCircuit = 0, // I_t = 0
        ShortCircuit = 1, // E_t = 0
        SaltStepper = 2 // indicates the protocol to test out the sodium pump
    };
    enum ModelMode modelMode;

	// Species indices
	enum
	{
		Na = 0, K = 1, Cl = 2, X1 = 3, X2 = 4, X3 = 5
	};

	// Concentrations - intracellular (c); apical membrane (mucosal) (a);
	//                  basolateral membrane (serosal) (b)
	std::vector<double> C_c;
	std::vector<double> C_a;
	std::vector<double> C_b;

	// permeabilities - paracellular pathway (j)
	std::vector<double> P_a;
	std::vector<double> P_b;
	std::vector<double> P_j;

	// valences
	std::vector<double> z;

	// reflection coefficients
	std::vector<double> sigma_a;
	std::vector<double> sigma_b;

	// hydraulic conductances
	double Lp_a;
	double Lp_b;

	// Na-K pump kinetic parameters
	double K_Na, K_K, n_Na, n_K, Imax;

	// Geometrical parameters
	double A_a, A_b;
	double V;

	// membrane potentials - transepithelial (t)
	double E_a, U_a;
	double E_b, U_b;
	double E_t, U_t;

	// current values for various things
    double I_t, I_a, I_b, I_j; // currents
    std::vector<double> J_a; // solute fluxes
	std::vector<double> J_b;
	std::vector<double> J_j;
	std::vector<double> J_p; // Na-K pump flux
    double Jw_a, Jw_b; // water fluxes

    // the bounds to put on the membrane potentials for KINSOL
    double maximumPotentialValue, minimumPotentialValue;
};

#endif /* GENERALMODEL_HPP_ */
