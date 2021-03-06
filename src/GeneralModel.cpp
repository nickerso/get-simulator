/*
 * GeneralModel.cpp
 *
 *  Created on: Nov 22, 2012
 *      Author: dnic019
 */
#include <cmath>
#include <iostream>

#if 0
#include <CellmlSimulator.hpp>
#endif

#include "common.hpp"
#include "molecule.hpp"
#include "GeneralModel.hpp"

#include "kinsol.hpp"

// Physical constants
static const double F = 9.6485341e4; // nC.nmol^-1
static const double R = 8.314472e3;  // pJ.nmol^-1.K^-1
static const double T = 310.0;       // K (value?)

static const double zeroTolerance = 1.0e-4;

static double calcU(const double E)
{
    return E * F / (R * T);
}

static double calcE(const double U)
{
    return U * R * T / F;
}

GeneralModel::GeneralModel()
{
    // nothing to do?
}

GeneralModel::~GeneralModel()
{
	// nothing to do?
}

void GeneralModel::initialise()
{
    // set up the mappings
    int numberOfMolecules = mMolecules.size();
    mC_a.resize(numberOfMolecules);
    mC_b.resize(numberOfMolecules);
    mC_c.resize(numberOfMolecules);
    mP_a.resize(numberOfMolecules);
    mP_b.resize(numberOfMolecules);
    mP_j.resize(numberOfMolecules);
    mZ.resize(numberOfMolecules);
    mSigma_a.resize(numberOfMolecules);
    mSigma_b.resize(numberOfMolecules);
    mJ_a.resize(numberOfMolecules);
    mJ_b.resize(numberOfMolecules);
    mJ_j.resize(numberOfMolecules);
    int j = 0;
    for (auto i=mMolecules.begin(); i!=mMolecules.end(); ++i, ++j)
    {
        Molecule& m = i->second;
        mC_a[j] = &(m.C_a); mC_b[j] = &(m.C_b); mC_c[j] = &(m.C_c);
        mP_a[j] = &(m.P_a); mP_b[j] = &(m.P_b); mP_j[j] = &(m.P_j);
        mJ_a[j] = &(m.J_a); mJ_b[j] = &(m.J_b); mJ_j[j] = &(m.J_j);
        mSigma_a[j] = &(m.sigma_a); mSigma_b[j] = &(m.sigma_b);
        mZ[j] = &(m.z);
    }

    U_a = calcU(E_a);
    U_b = calcU(E_b);
    U_t = calcU(E_t);

    minimumPotentialValue = calcU(-200.0);
    maximumPotentialValue = calcU(200.0);
    std::cout << "Potentials restricted to the range: " << minimumPotentialValue << " <= U <= "
              << maximumPotentialValue << std::endl;
}

double GeneralModel::solveCurrentClampPotentials(int& errorFlag, const bool updateOnly)
{
    errorFlag = 0;
	/**
	 * This is the function called by KINSOL when solving for the open-circuit case.
	 */
    if (updateOnly && (debugLevel() > 19)) std::cout << "Updating open-circuit electroneutrality" << std::endl;

	/* solve for electroneutrality as per the open-circuit case presented in Figure 4
	 * of the Latta et al (1984) paper. Talk about E_x but actually using U_x.
	 * This ensures I_j + I_a = I_t
	 */
    I_t = 0.0; // open-circuit condition

	/*
	 * 1) initial value of E_t is estimated. Here we start with the current value of E_t, so
	 * do nothing.
	 */

	/*
	 * 2) solve for E_a as per voltage-clamp conditions with current estimate of E_t
	 */
    if (updateOnly)
    {

    }
    else
    {
        mode = 0;
        if (solveOneVariable(this, minimumPotentialValue, maximumPotentialValue) > 0)
        {
            std::cerr << "Failed to solve voltage clamp case for U_t = " << U_t << std::endl;
            errorFlag = 1;
            return 0.0;
        }
        mode = 1;
    }

	/*
	 * 3) compute I_j and the current value of E_t (for the values of E_a and E_b just solved for)
	 */
    calculateSoluteParacellularFluxes();
    compute_I_j();

	/*
	 * 4) compute I_j + I_a and compare to I_t. If within tolerance, then E_a, E_b, and E_t are consistent
	 * with the requirements of electroneutrality. Otherwise, refine E_t using Newton-Raphson step and
	 * return to step 2.
     */
    double error;
    if (updateOnly)
    {
        error = 0.0;
    }
    else
    {
        error = I_j + I_a - I_t;
        if (debugLevel() > 10) std::cout << "Current open-circuit error = " << error << std::endl;
    }
    return error;
}

double GeneralModel::solveVoltageClampPotentials(const bool updateOnly)
{
	/**
	 * This is the function called by KINSOL when solving for the voltage-clamp case.
	 */

    if (updateOnly && (debugLevel() > 19))
    {
        std::cout << "Updating voltage clamp electroneutrality" << std::endl;
    }

	/* solve for electroneutrality as per the voltage-clamp case presented in Figure 3
	 * of the Latta et al (1984) paper. This ensures I_a = I_b (i.e., current into and out-of
	 * the cell balance). We are clamping the cell at the current value of E_t.
	 */
	/*
	 * 1) estimate E_a. This will be set by KINSOL.
	 */

	/*
	 * 2) compute E_b = E_t - E_a
	 */
	U_b = U_t - U_a;

    if (debugLevel() > 20)
        std::cout << "solving/updating for E_a and E_b, current values: U_a=" << U_a << "; U_b=" << U_b
                  << "; U_t=" << U_t << std::endl;

	/*
	 * 3) compute I_a and I_b.
	 */
	calculateSoluteMembraneFluxes();
    compute_I_a();
	compute_I_b();

	/*
	 * 4) compare I_a and I_b and if they are equal within tolerance then the computed membrane
	 * potentials are consistent with the requirement for cellular electroneutrality. Otherwise,
	 * refine E_a using a Newton-Raphson step and return to step 2.
	 */
    double error;
    if (updateOnly)
    {
        error = 0.0;
    }
    else
    {
        error = I_a - I_b;
        if (debugLevel() > 99) std::cout << "I_a = " << I_a << "; I_b = " << I_b << "; error = " << error << std::endl;
        if (debugLevel() > 20) std::cout << "Current voltage-clamp error = " << error << std::endl;
    }
    return error;
}

void GeneralModel::calculateSoluteMembraneFluxes()
{
    /*
	 * Apical membrane fluxes assumed to be entirely passive (eq 7)
	 */
    if (debugLevel() > 99) std::cout << "apical membrane fluxes";
    calculatePassiveFluxes(mJ_a, mP_a, mZ, mC_a, mC_c, U_a);
	/*
     * Basolateral membrane flux also passive
	 */
    if (debugLevel() > 99) std::cout << "basolateral membrane fluxes";
    calculatePassiveFluxes(mJ_b, mP_b, mZ, mC_c, mC_b, U_b);
}

void GeneralModel::calculateSoluteParacellularFluxes()
{
    /*
     * Paracellular fluxes assumed to be passive (assuming leaky or moderately tight epithelium.
     */
    calculatePassiveFluxes(mJ_j, mP_j, mZ, mC_a, mC_b, U_t);
}

void GeneralModel::compute_I_a()
{
	I_a = 0;
    for (unsigned int i=0; i<mJ_a.size(); ++i) I_a += (*mZ[i]) * (*mJ_a[i]);
    if (debugLevel() > 99) std::cout << "pre-I_a = " << I_a;
	I_a *= F * A_a;
}

void GeneralModel::compute_I_b()
{
    I_b = 0;
    for (unsigned int i=0; i<mJ_b.size(); ++i) I_b += (*mZ[i]) * (*mJ_b[i]);
    if (debugLevel() > 99) std::cout << "; pre-I_b = " << I_b << std::endl;
    I_b *= F * A_b;
}

void GeneralModel::compute_I_j()
{
    I_j = 0;
    for (unsigned int i=0; i<mJ_j.size(); ++i) I_j += (*mZ[i]) * (*mJ_j[i]);
    I_j *= F * A_a;
}

void GeneralModel::calculatePassiveFluxes(std::vector<double*>& J, const std::vector<double*>& P,
                                                   const std::vector<double*>& z, const std::vector<double*>& C1,
                                                   const std::vector<double*>& C2, const double U)
{
    int i, N=J.size();

    for (i=0; i<N; i++)
    {
        if (fabs((*z[i])*U) > zeroTolerance)
        {
            (*J[i]) = (*P[i]) * (*z[i]) * U * ((*C1[i]) - (*C2[i]) * exp(-(*z[i]) * U)) / (1.0 - exp(-(*z[i]) * U));
            if (debugLevel() > 99) std::cout << "; J[" << i << "] = " << (*J[i]);
        }
        else
        {
            // as per my interpretation of footnote 4 of Latta paper, to avoid / by zero and given improved
            // accuracy of double vs float...
            (*J[i]) = (*P[i]) * (*z[i]) * (F / (R * T)) * ((*C1[i]) - (*C2[i]));
            if (debugLevel() > 99) std::cout << "; Japprox[" << i << "] = " << (*J[i]);
        }
    }
    if (debugLevel() > 99) std::cout << std::endl;
}

void GeneralModel::printState(std::ostream& s, double &time)
{
    s << time << "\t" << V;
    E_t = calcE(U_t);
    E_a = calcE(U_a);
    E_b = calcE(U_b);
    s << "\t" << E_t << "\t" << E_a << "\t" << E_b;
    //for (unsigned int i=0; i< C_c.size(); ++i) s << "\t" << C_c[i];
    //double JNa_net = J_a[Na] + J_j[Na];
    //double JK_net = J_a[K] + J_j[K];
    //s << "\t" << JNa_net << "\t" << JK_net << "\t" << I_t << "\t" << I_a << "\t" << I_b << "\t" << I_j;
    s << std::endl;
}

void GeneralModel::printStateHeader(std::ostream& s)
{
    s << "# time";          // 1
    s << "\tV";             // 2
    s << "\tE_t";           // 3
    s << "\tE_a";           // 4
    s << "\tE_b";           // 5
    // needs to match the class enum...
    s << "\tNa";            // 6
    s << "\tK";             // 7
    s << "\tCl";            // 8
    s << "\tX1";            // 9
    s << "\tX2";            // 10
    s << "\tJNa_net";       // 11
    s << "\tJK_net";        // 12
    s << "\tI_t";           // 13
    s << "\tI_a";           // 14
    s << "\tI_b";           // 15
    s << "\tI_j";           // 16
    s << std::endl;
}

std::vector<double> GeneralModel::calculateRHS(double time, int &errorFlag)
{
    std::vector<double> f(mC_c.size() + 1); // number of species + cell volume

    if (debugLevel() > 1) std::cout << "Calculate RHS for time: " << time << std::endl;

    // compute membrane potentials
    if ((modelMode == OpenCircuit) || (modelMode == SaltStepper))
    {
        mode = 1; // we want to solve for membrane potentials at the open-circuit conditions
        if (solveOneVariable(this, minimumPotentialValue, maximumPotentialValue) > 0)
        {
            std::cerr << "calculateRHS: Failed to solve open circuit case" << std::endl;
            errorFlag = 1;
            return f;
        }
    }
    else if (modelMode == ShortCircuit)
    {
        mode = 0; // we want to solve for membrane potentials at the voltage-clamp conditions
        if (solveOneVariable(this, minimumPotentialValue, maximumPotentialValue) > 0)
        {
            std::cerr << "calculateRHS: Failed to solve voltage clamp case" << std::endl;
            errorFlag = 2;
            return f;
        }
    }
    else
    {
        std::cerr << "Doh! invalid modelMode?" << std::endl;
    }

    // dV/dt
    calculateWaterFluxes();
    f[0] = A_a * Jw_a + A_b * Jw_b;

    // solutes
    calculateSoluteMembraneFluxes();
    for (unsigned int i = 0; i < mC_c.size(); ++i)
    {
        f[i+1] = (A_a * (*mJ_a[i]) - A_b * (*mJ_b[i]) - (*mC_c[i]) * f[0]) / V;
    }
    return f;
}

void GeneralModel::calculateWaterFluxes()
{
    Jw_a = Jw_b = 0;
    for (unsigned int i = 0; i < mC_c.size(); ++i)
    {
        Jw_a += (*mSigma_a[i]) * ((*mC_c[i]) - (*mC_a[i]));
        Jw_b += (*mSigma_b[i]) * ((*mC_c[i]) - (*mC_b[i]));
    }
    Jw_a *= Lp_a * R * T;
    Jw_b *= Lp_b * R * T;
}

void GeneralModel::initialiseSaltStepper()
{
#if 0
    // reset the model back to default conditions
    initialise();
    // and then set salt-stepper specific conditions
    modelMode = SaltStepper;
    C_a[Na] = C_b[Na] = 0.0;
    C_a[Cl] = C_b[Cl] = 0.0;
    C_a[K] = C_b[K] = C_a[X1] = C_b[X1] = 109.3; // [mM]
    C_a[X2] = C_b[X2] = 81.4; // [mM]
    P_a[Na] = P_a[K] = 3.0e-5; // [cm/sec]
    P_a[Cl] = 1.0e-5; // [cm/sec]
#endif
}

int GeneralModel::addMolecule(const Molecule &molecule)
{
    if (mMolecules.count(molecule.typeId) > 0)
    {
        std::cerr << "Unable to add a molecule more than once: " << molecule.typeId << "; count: "
                     << mMolecules.count(molecule.typeId) << std::endl;
        return -1;
    }
    mMolecules[molecule.typeId] = molecule;
    return 0;
}
