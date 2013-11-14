#include <iostream>
#include <string>

#include "molecule.hpp"

Molecule::Molecule()
{
    // set reasonable defaults that the user can override
    typeId = "undefined";
    z = 1.0;
    serosalRateEquation = mucosalRateEquation = intracellularRateEquation = false;
    P_a = P_b = P_j = 0.0;
    sigma_a = sigma_b = 1.0;
}

Molecule::~Molecule()
{
    // nothing to do
}

