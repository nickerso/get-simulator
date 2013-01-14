
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <math.h>

#include <ida/ida.h>
#include <ida/ida_dense.h>
#include <nvector/nvector_serial.h>
#include <sundials/sundials_math.h>
#include <sundials/sundials_types.h>

#include "GeneralModel.hpp"

#define ZERO RCONST(0.0);
#define ONE  RCONST(1.0);

/* Prototypes of functions called by IDA */

int residualCalculate(realtype tres, N_Vector yy, N_Vector yp,
                        N_Vector resval, void *user_data);

/* Prototypes of private functions */
static void PrintHeader(realtype rtol, N_Vector avtol, N_Vector y);
static void PrintOutput(void *mem, realtype t, N_Vector y);
static void PrintFinalStats(void *mem);
static int check_flag(void *flagvalue, const char *funcname, int opt);

/*
 *--------------------------------------------------------------------
 * Main Program
 *--------------------------------------------------------------------
 */

int main(void)
{
    GeneralModel model;

    // set up output
    std::ofstream output;
    output.open("results.data");
    output.precision(5);
    output.setf(std::ios_base::uppercase | std::ios_base::scientific);
    // set the way numbers should be streamed out
    std::cout.precision(5);
    std::cout.setf(std::ios_base::uppercase | std::ios_base::scientific);
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

    double initialVolume = model.V;

    // set up IDA
    int NEQ_diff = model.C_c.size() + 1; // number of species + cell volume
    int NEQ_alg = 2; // E_a & E_t
    int NEQ = NEQ_diff + NEQ_alg;

    void *mem;
    N_Vector yy, yp, avtol, id;
    realtype rtol, *yval, *ypval, *atval, *idval;
    int i, retval;

    mem = NULL;
    yy = yp = avtol = NULL;
    yval = ypval = atval = NULL;

    /* Allocate N-vectors. */
    yy = N_VNew_Serial(NEQ);
    if(check_flag((void *)yy, "N_VNew_Serial", 0)) return(1);
    yp = N_VNew_Serial(NEQ);
    if(check_flag((void *)yp, "N_VNew_Serial", 0)) return(1);
    avtol = N_VNew_Serial(NEQ);
    if(check_flag((void *)avtol, "N_VNew_Serial", 0)) return(1);
    id  = N_VNew_Serial(NEQ);
    if(check_flag((void *)id, "N_VNew_Serial", 0)) return(1);

    /* Create and initialize  y, y', and absolute tolerance vectors. */
    yval  = NV_DATA_S(yy);
    yval[0] = model.V; // initial volume
    for (i=1; i < NEQ_diff; ++i) yval[i] = model.C_c[i-1];
    yval[i++] = model.U_a;
    yval[i] = model.U_t;

    ypval = NV_DATA_S(yp);
    for (i=0; i < NEQ; ++i) ypval[0] = ZERO;

    rtol = RCONST(1.0e-4);

    atval = NV_DATA_S(avtol);
    atval[0] = RCONST(1.0e-6);
    for (i=1; i < NEQ_diff; ++i) yval[i] = RCONST(1.0e-4);
    atval[i++] = RCONST(1.0e-6);
    atval[i] = RCONST(1.0e-6);
    // and the id vector
    idval = NV_DATA_S(id);
    for (i=0; i < NEQ_diff; ++i) yval[i] = ONE;
    for (; i < NEQ; ++i) yval[i] = ZERO;

    PrintHeader(rtol, avtol, yy);

    /* Call IDACreate and IDAInit to initialize IDA memory */
    mem = IDACreate();
    if(check_flag((void *)mem, "IDACreate", 0)) return(1);

    retval = IDASetUserData(mem, &model);
    if(check_flag(&retval, "IDASetUserData", 1)) return(1);

    retval = IDASetId(mem, id);
    if(check_flag(&retval, "IDASetId", 1)) return(1);

    retval = IDAInit(mem, residualCalculate, t, yy, yp);
    if(check_flag(&retval, "IDAInit", 1)) return(1);
    /* Call IDASVtolerances to set tolerances */
    retval = IDASVtolerances(mem, rtol, avtol);
    if(check_flag(&retval, "IDASVtolerances", 1)) return(1);

    /* Free avtol */
    N_VDestroy_Serial(avtol);

    /* Call IDADense and set up the linear solver. */
    retval = IDADense(mem, NEQ);
    if(check_flag(&retval, "IDADense", 1)) return(1);

    /* Call IDACalcIC (with default options) to correct the initial values. */
    double tout = t + delta_t;
    //retval = IDACalcIC(mem, IDA_YA_YDP_INIT, tout);
    //if(check_flag(&retval, "IDACalcIC", 1)) return(1);

    /* In loop, call IDASolve, print results, and test for error.
     Break out of loop when NOUT preset output times have been reached. */

    /*
     * Solve to steady state in the open-circuit mode.
     */
    model.modelMode = GeneralModel::OpenCircuit;
    while (t <= t_final)
    {
        tout = t + delta_t;
        retval = IDASolve(mem, tout, &t, yy, yp, IDA_NORMAL);

        PrintOutput(mem,t,yy);

        if(check_flag(&retval, "IDASolve", 1)) return(1);

        if (retval == IDA_SUCCESS)
        {
            model.printState(output, t);
        }
        else
        {
            std::cerr << "ida: integration failed in steady-state block" << std::endl;
            output.close();
            return 1;
        }

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
        /*
         * 5) additional variables are computed, and all results saved off-line.
         */
        /*
         * 6) t is updated to t+delta_t and a comparison is made with t_final. If t_final
         * has been reached, the computation ends. Otherwise, the program returns to step
         * 2 to continue the integration.
         */
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

    PrintFinalStats(mem);

    /* Free memory */

    IDAFree(&mem);
    N_VDestroy_Serial(yy);
    N_VDestroy_Serial(yp);

    return(0);

}

/*
 *--------------------------------------------------------------------
 * Functions called by IDA
 *--------------------------------------------------------------------
 */

/*
 * Define the system residual function.
 */

int residualCalculate(realtype tres, N_Vector yy, N_Vector yp, N_Vector rr, void *user_data)
{
    realtype *yval, *ypval, *rval;
    yval = NV_DATA_S(yy);
    ypval = NV_DATA_S(yp);
    rval = NV_DATA_S(rr);

    GeneralModel* model = static_cast<GeneralModel*>(user_data);
    // update state variables
    model->V = yval[0];
    int i;
    for (i=0; i < model->C_c.size(); ++i) model->C_c[i] = yval[i+1];
    model->U_a = yval[++i];
    model->U_t = yval[++i];

    int errorFlag = 0;
    std::vector<double> f = model->calculateRHS((double)tres, errorFlag);
    if (errorFlag != 0)
    {
        std::cerr << "IDA-RHS-fcn failed!" << std::endl;
        return -1; // negative value to indicate non-recoverable failure
    }
    for (i=0; i < f.size(); ++i) rval[i] = f[i] - ypval[i];
    rval[i++] = model->solveVoltageClampPotentials(false); // E_a
    rval[i] = model->solveOpenCircuitPotentials(errorFlag, false); // E_t

    return(0);
}

/*
 *--------------------------------------------------------------------
 * Private functions
 *--------------------------------------------------------------------
 */

/*
 * Print first lines of output (problem description)
 */

static void PrintHeader(realtype rtol, N_Vector avtol, N_Vector y)
{
    realtype *atval, *yval;

    atval  = NV_DATA_S(avtol);
    yval  = NV_DATA_S(y);

    printf("\nidaRoberts_dns: Robertson kinetics DAE serial example problem for IDA\n");
    printf("         Three equation chemical kinetics problem.\n\n");
    printf("Linear solver: IDADENSE, with user-supplied Jacobian.\n");
#if defined(SUNDIALS_EXTENDED_PRECISION)
    printf("Tolerance parameters:  rtol = %Lg   atol = %Lg %Lg %Lg \n",
           rtol, atval[0],atval[1],atval[2]);
    printf("Initial conditions y0 = (%Lg %Lg %Lg)\n",
           yval[0], yval[1], yval[2]);
#elif defined(SUNDIALS_DOUBLE_PRECISION)
    printf("Tolerance parameters:  rtol = %lg   atol = %lg %lg \n",
           rtol, atval[0],atval[1]);
    printf("Initial conditions y0 = (%lg %lg)\n",
           yval[0], yval[1]);
#else
    printf("Tolerance parameters:  rtol = %g   atol = %g %g %g \n",
           rtol, atval[0],atval[1],atval[2]);
    printf("Initial conditions y0 = (%g %g %g)\n",
           yval[0], yval[1], yval[2]);
#endif
    printf("Constraints and id not used.\n\n");
    printf("-----------------------------------------------------------------------\n");
    printf("  t             y1           y2");
    printf("      | nst  k      h\n");
    printf("-----------------------------------------------------------------------\n");
}

/*
 * Print Output
 */

static void PrintOutput(void *mem, realtype t, N_Vector y)
{
    realtype *yval;
    int retval, kused;
    long int nst;
    realtype hused;

    yval  = NV_DATA_S(y);

    retval = IDAGetLastOrder(mem, &kused);
    check_flag(&retval, "IDAGetLastOrder", 1);
    retval = IDAGetNumSteps(mem, &nst);
    check_flag(&retval, "IDAGetNumSteps", 1);
    retval = IDAGetLastStep(mem, &hused);
    check_flag(&retval, "IDAGetLastStep", 1);
#if defined(SUNDIALS_EXTENDED_PRECISION)
    printf("%10.4Le %12.4Le %12.4Le %12.4Le | %3ld  %1d %12.4Le\n",
           t, yval[0], yval[1], yval[2], nst, kused, hused);
#elif defined(SUNDIALS_DOUBLE_PRECISION)
    printf("%10.4le %12.4le %12.4le | %3ld  %1d %12.4le\n",
           t, yval[0], yval[1], nst, kused, hused);
#else
    printf("%10.4e %12.4e %12.4e %12.4e | %3ld  %1d %12.4e\n",
           t, yval[0], yval[1], yval[2], nst, kused, hused);
#endif
}

/*
 * Print final integrator statistics
 */

static void PrintFinalStats(void *mem)
{
    int retval;
    long int nst, nni, nje, nre, nreLS, netf, ncfn, nge;

    retval = IDAGetNumSteps(mem, &nst);
    check_flag(&retval, "IDAGetNumSteps", 1);
    retval = IDAGetNumResEvals(mem, &nre);
    check_flag(&retval, "IDAGetNumResEvals", 1);
    retval = IDADlsGetNumJacEvals(mem, &nje);
    check_flag(&retval, "IDADlsGetNumJacEvals", 1);
    retval = IDAGetNumNonlinSolvIters(mem, &nni);
    check_flag(&retval, "IDAGetNumNonlinSolvIters", 1);
    retval = IDAGetNumErrTestFails(mem, &netf);
    check_flag(&retval, "IDAGetNumErrTestFails", 1);
    retval = IDAGetNumNonlinSolvConvFails(mem, &ncfn);
    check_flag(&retval, "IDAGetNumNonlinSolvConvFails", 1);
    retval = IDADlsGetNumResEvals(mem, &nreLS);
    check_flag(&retval, "IDADlsGetNumResEvals", 1);
    retval = IDAGetNumGEvals(mem, &nge);
    check_flag(&retval, "IDAGetNumGEvals", 1);

    printf("\nFinal Run Statistics: \n\n");
    printf("Number of steps                    = %ld\n", nst);
    printf("Number of residual evaluations     = %ld\n", nre+nreLS);
    printf("Number of Jacobian evaluations     = %ld\n", nje);
    printf("Number of nonlinear iterations     = %ld\n", nni);
    printf("Number of error test failures      = %ld\n", netf);
    printf("Number of nonlinear conv. failures = %ld\n", ncfn);
    printf("Number of root fn. evaluations     = %ld\n", nge);
}

/*
 * Check function return value...
 *   opt == 0 means SUNDIALS function allocates memory so check if
 *            returned NULL pointer
 *   opt == 1 means SUNDIALS function returns a flag so check if
 *            flag >= 0
 *   opt == 2 means function allocates memory so check if returned
 *            NULL pointer
 */

static int check_flag(void *flagvalue, const char *funcname, int opt)
{
    int *errflag;
    /* Check if SUNDIALS function returned NULL pointer - no memory allocated */
    if (opt == 0 && flagvalue == NULL) {
        fprintf(stderr,
                "\nSUNDIALS_ERROR: %s() failed - returned NULL pointer\n\n",
                funcname);
        return(1);
    } else if (opt == 1) {
        /* Check if flag < 0 */
        errflag = (int *) flagvalue;
        if (*errflag < 0) {
            fprintf(stderr,
                    "\nSUNDIALS_ERROR: %s() failed with flag = %d\n\n",
                    funcname, *errflag);
            return(1);
        }
    } else if (opt == 2 && flagvalue == NULL) {
        /* Check if function returned NULL pointer - no memory allocated */
        fprintf(stderr,
                "\nMEMORY_ERROR: %s() failed - returned NULL pointer\n\n",
                funcname);
        return(1);
    }

    return(0);
}
