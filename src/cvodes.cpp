/* Wrapper around CVODES */

#include <cstdio>
#include <iostream>
#include <vector>

/* Header files with a description of contents used */

#include <cvodes/cvodes.h>           /* prototypes for CVODE fcts. and consts. */
#include <nvector/nvector_serial.h>  /* serial N_Vector types, fcts., and macros */
#include <cvodes/cvodes_dense.h>     /* prototype for CVDense */
#include <sundials/sundials_dense.h> /* definitions DlsMat DENSE_ELEM */
#include <sundials/sundials_types.h> /* definition of type realtype */

#include "common.hpp"
#include "cvodes.hpp"
#include "GeneralModel.hpp"

/* Problem Constants */

#define RTOL  RCONST(1.0e-3)   /* scalar relative tolerance            */
#define ATOL  RCONST(1.0e-4)   /* vector absolute tolerance components */

/* Function Called by the Solver */
static int f(realtype t, N_Vector y, N_Vector ydot, void *user_data);

/* Private functions to output results */
//static void PrintOutput(realtype t, realtype y1, realtype y2, realtype y3);

/* Private function to print final statistics */
static void PrintFinalStats(void *cvode_mem);

/* Private function to check function return values */
static int check_flag(void *flagvalue, const char *funcname, int opt);


/*
 *-------------------------------
 * Main Program
 *-------------------------------
 */
Cvodes::Cvodes()
{
    cvodeMem = NULL;
    y = NULL;
}

int Cvodes::initialise(GeneralModel* model, double initialTime, double maxStep)
{
    int NEQ = model->C_c.size() + 1; // number of species + cell volume
    realtype reltol, abstol;
    int flag, i;
    N_Vector abstolVector;

    /* Create serial vector of length NEQ for I.C. */
    y = N_VNew_Serial(NEQ);
    if (check_flag((void *)y, "N_VNew_Serial", 0)) return(1);

    /* Create serial vector of length NEQ for absolute tolerances */
    abstolVector = N_VNew_Serial(NEQ);
    if (check_flag((void *)abstolVector, "N_VNew_Serial", 0)) return(1);

    /* Initialize y */
    Ith(y,1) = model->V; // initial volume
    for (i=0; i < model->C_c.size(); ++i) Ith(y, i+2) = model->C_c[i];

    /* Set the scalar relative tolerance */
    reltol = RTOL;
    abstol = ATOL; // could use a vector if needed
    Ith(abstolVector, 1) = ATOL / 100.0; // tighter tolerance on volume which is several orders of magnitude
                                         // smaller than the concentrations
    for (i=0; i < model->C_c.size(); ++i) Ith(abstolVector, i+2) = ATOL;

    /* Call CVodeCreate to create the solver memory and specify the
   * Backward Differentiation Formula and the use of a Newton iteration */
    cvodeMem = CVodeCreate(CV_ADAMS, CV_NEWTON);
    if (check_flag((void *)cvodeMem, "CVodeCreate", 0)) return(1);

    /* Call CVodeInit to initialize the integrator memory and specify the
   * user's right hand side function in y'=f(t,y), the inital time T0, and
   * the initial dependent variable vector y. */
    flag = CVodeInit(cvodeMem, f, initialTime, y);
    if (check_flag(&flag, "CVodeInit", 1)) return(1);

    /* Call CVodeSVtolerances to specify the scalar relative tolerance
   * and vector absolute tolerances */
    //flag = CVodeSStolerances(cvodeMem, reltol, abstol);
    flag = CVodeSVtolerances(cvodeMem, reltol, abstolVector);
    if (check_flag(&flag, "CVodeSStolerances", 1)) return(1);

    /* Call CVDense to specify the CVDENSE dense linear solver */
    flag = CVDense(cvodeMem, NEQ);
    if (check_flag(&flag, "CVDense", 1)) return(1);

    // set the maximum step size
    flag = CVodeSetMaxStep(cvodeMem, maxStep);
    if (check_flag(&flag, "CVodeSetMaxStep", 1)) return(1);

    /* pass through the model as the user data */
    flag = CVodeSetUserData(cvodeMem, static_cast<void*>(model));
    if (check_flag(&flag, "CVodeSetUserData", 1)) return(1);

    return 0;
}

Cvodes::~Cvodes()
{
    if (cvodeMem)
    {
        /* Print some final statistics */
        PrintFinalStats(cvodeMem);

        /* Free y vector */
        N_VDestroy_Serial(y);

        /* Free integrator memory */
        CVodeFree(&cvodeMem);
    }
}

int Cvodes::integrate(double& t, double tout)
{
    int flag;
    flag = CVode(cvodeMem, tout, y, &t, CV_NORMAL);
    if (check_flag(&flag, "CVode", 1)) return(1);
    return(0);
}

int Cvodes::reInitialise(double time)
{
    int flag = CVodeReInit(cvodeMem, time, y);
    if (check_flag(&flag, "CVodeReInit", 1)) return(1);

    return 0;
}

/*
 *-------------------------------
 * Functions called by the solver
 *-------------------------------
 */

/*
 * f routine. Compute function f(t,y).
 */

static int f(realtype t, N_Vector y, N_Vector ydot, void *user_data)
{
    GeneralModel* model = static_cast<GeneralModel*>(user_data);
    // update state variables
    model->V = Ith(y,1);
    for (int i=0; i < model->C_c.size(); ++i) model->C_c[i] = Ith(y, i+2);

    int errorFlag = 0;
    std::vector<double> f = model->calculateRHS((double)t, errorFlag);
    if (errorFlag != 0)
    {
        std::cerr << "CVODES-RHS-fcn failed!" << std::endl;
        return -1; // negative value to indicate non-recoverable failure
    }
    for (int i=0; i < f.size(); ++i) NV_Ith_S(ydot, i) = f[i];

    return(0);
}

/*
 *-------------------------------
 * Private helper functions
 *-------------------------------
 */

/*
static void PrintOutput(realtype t, realtype y1, realtype y2, realtype y3)
{
#if defined(SUNDIALS_EXTENDED_PRECISION)
    printf("At t = %0.4Le      y =%14.6Le  %14.6Le  %14.6Le\n", t, y1, y2, y3);
#elif defined(SUNDIALS_DOUBLE_PRECISION)
    printf("At t = %0.4le      y =%14.6le  %14.6le  %14.6le\n", t, y1, y2, y3);
#else
    printf("At t = %0.4e      y =%14.6e  %14.6e  %14.6e\n", t, y1, y2, y3);
#endif

    return;
}
*/

/*
 * Get and print some final statistics
 */

static void PrintFinalStats(void *cvode_mem)
{
    long int nst, nfe, nsetups, nje, nfeLS, nni, ncfn, netf, nge;
    int flag;

    flag = CVodeGetNumSteps(cvode_mem, &nst);
    check_flag(&flag, "CVodeGetNumSteps", 1);
    flag = CVodeGetNumRhsEvals(cvode_mem, &nfe);
    check_flag(&flag, "CVodeGetNumRhsEvals", 1);
    flag = CVodeGetNumLinSolvSetups(cvode_mem, &nsetups);
    check_flag(&flag, "CVodeGetNumLinSolvSetups", 1);
    flag = CVodeGetNumErrTestFails(cvode_mem, &netf);
    check_flag(&flag, "CVodeGetNumErrTestFails", 1);
    flag = CVodeGetNumNonlinSolvIters(cvode_mem, &nni);
    check_flag(&flag, "CVodeGetNumNonlinSolvIters", 1);
    flag = CVodeGetNumNonlinSolvConvFails(cvode_mem, &ncfn);
    check_flag(&flag, "CVodeGetNumNonlinSolvConvFails", 1);

    flag = CVDlsGetNumJacEvals(cvode_mem, &nje);
    check_flag(&flag, "CVDlsGetNumJacEvals", 1);
    flag = CVDlsGetNumRhsEvals(cvode_mem, &nfeLS);
    check_flag(&flag, "CVDlsGetNumRhsEvals", 1);

    flag = CVodeGetNumGEvals(cvode_mem, &nge);
    check_flag(&flag, "CVodeGetNumGEvals", 1);

    printf("\nFinal Statistics:\n");
    printf("nst = %-6ld nfe  = %-6ld nsetups = %-6ld nfeLS = %-6ld nje = %ld\n",
           nst, nfe, nsetups, nfeLS, nje);
    printf("nni = %-6ld ncfn = %-6ld netf = %-6ld nge = %ld\n \n",
           nni, ncfn, netf, nge);
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
        fprintf(stderr, "\nSUNDIALS_ERROR: %s() failed - returned NULL pointer\n\n",
                funcname);
        return(1); }

    /* Check if flag < 0 */
    else if (opt == 1) {
        errflag = (int *) flagvalue;
        if (*errflag < 0) {
            fprintf(stderr, "\nSUNDIALS_ERROR: %s() failed with flag = %d\n\n",
                    funcname, *errflag);
            return(1); }}

    /* Check if function returned NULL pointer - no memory allocated */
    else if (opt == 2 && flagvalue == NULL) {
        fprintf(stderr, "\nMEMORY_ERROR: %s() failed - returned NULL pointer\n\n",
                funcname);
        return(1); }

    return(0);
}

