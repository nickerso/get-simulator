#include <cmath>
#include <iostream>

#include "NewtonRaphson.hpp"
#include "LattaNaTransportModel.hpp"

static double f(double u, LattaNaTransportModel* model)
{
    std::cout << "f(u), u = " << u << std::endl;
    double f;
    if (model->mode)
    {
        // Open-circuit case (solving for E_t)
        model->U_t = u;
        f = model->solveForElectroneutrality();
    }
    else
    {
        // Voltage-clamp case (solving for E_a)
        model->U_a = u;
        f = model->solveForElectroneutralityVoltageClamp();
    }
    std::cout << "f(u) = " << f << std::endl;
    return f;
}

int solveOneVariable(LattaNaTransportModel* model)
{
    double dx = 0.1;
    double x;

    // get the current value
    if (model->mode) x = model->U_t;
    else x = model->U_a;

    int iterationCounter = 0;
    double fn, dfndx, error = f(x, model);
    while ((fabs(error) > 0.001) && (iterationCounter < 20)) // as per Latta et al paper
    {
        std::cout << "x = " << x << "; error = " << error << std::endl;
        fn = error;
        dfndx = (f(x+dx, model) - f(x-dx, model)) / (2.0*dx);
        std::cout << "dfndx = " << dfndx << std::endl;
        x -= fn/dfndx;
        std::cout << "new x = " << x << std::endl;
        error = f(x, model);
        iterationCounter++;
    }

    return (0);
}
