#ifndef MOLECULE_HPP
#define MOLECULE_HPP

#include <string>

/**
 * @brief The Molecule class manages each molecule...
 *
 * The compartment/membrane indicies are:
 *    a - mucosal (lumen) compartment, apical membrane
 *    b - serosal (interstitial) compartment, basolateral membrane
 *    j - paracellular pathway, junctional membrane
 *    c - intracellular compartment
 *
 * FIXME: need to eventually look into units and stuff.
 */

class Molecule
{
public:
    Molecule();
    ~Molecule();

    /**
     * @brief The type ID for this molecule, should be an appropriate ontological term?
     */
    std::string typeId;

    /**
     * @brief The current concentration value for this molecule in each of the compartments.
     */
    double C_a, C_b, C_c;

    /**
     * @brief The valence of this molecule.
     */
    double z;

    /**
     * @brief Define to true if there is a rate equation defined for this molecule in each compartment. If false,
     * this molecule is defined to be constant in that compartment.
     */
    bool mucosalRateEquation, serosalRateEquation, intracellularRateEquation;

    /**
     * (Membrane) permeabilities for this molecule.
     */
    double P_a, P_b, P_j;

    /**
     * Membrane reflection coefficients for this molecule (sigma's).
     */
    double sigma_a, sigma_b;

    /**
     * Membrane fluxes.
     */
    double J_a, J_b, J_j;
};

#endif // MOLECULE_HPP
