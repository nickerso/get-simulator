/*
 * kinsol.cpp
 *
 *  Created on: Sep 13, 2012
 *      Author: dnic019
 */
#include <iostream>

#include <kinsol/kinsol.h>
#include <kinsol/kinsol_dense.h>
#include <nvector/nvector_serial.h>
#include <sundials/sundials_types.h>
#include <sundials/sundials_math.h>

#include "common.hpp"
#include "GeneralModel.hpp"

#define ZERO   RCONST(0.0)
#define ONE    RCONST(1.0)

#define FTOL   RCONST(1.e-7)  /* function tolerance */
#define STOL   RCONST(1.e-5)  /* step tolerance */

/* Accessor macro */
#define Ith(v,i)    NV_Ith_S(v,i-1)

/* Functions Called by the KINSOL Solver */
static int func(N_Vector u, N_Vector f, void *user_data);

/* Private Helper Functions */
static int SolveIt(void *kmem, N_Vector u, N_Vector s, int glstr, int mset);
//static void PrintHeader(int globalstrategy, realtype fnormtol,
//		realtype scsteptol);
static void PrintOutput(N_Vector u);
static void PrintFinalStats(void *kmem);
static int check_flag(void *flagvalue, const char *funcname, int opt);

typedef struct
{
    GeneralModel* model;
    double minimumValue;
    double maximumValue;
} UserData;

// just to filter out error messages from KINSol that we can handle internally
void handleKinsolError(int code, const char *module, const char *function, char *msg, void *dat)
{
    // TODO: are there any errors we want to return to the user?
    // maybe for debugging?
}

/**
 * mode == 0: voltage clamp (solving for U_a)
 * mode != 0: open-circuit (solving for U_t)
 * include the constraint that minimumValue <= U <= maximumValue to ensure that physiological potentials
 * only are solved (i.e., try and restrain unrealistic values for the exp(U) functions).
 *
 * The treatment of the bound constraints on U is done using
 * the additional variables
 *    l = U - minimumValue >= 0
 *    L = U - maximumValue <= 0
 *
 * and using the constraint feature in KINSOL to impose
 *    l >= 0
 *    L <= 0
 */
int solveOneVariable(GeneralModel* model, double minimuimValue, double maximumValue)
{
    int NEQ = 1 /* variable, U */ + 2 /* constraints, l and L */;
	realtype fnormtol, scsteptol;
    N_Vector u, s, c;
	int glstr, mset, flag;
	void *kmem;
    UserData* data;

	u = NULL;
	s = NULL;
    c = NULL;
	kmem = NULL;
	glstr = KIN_NONE;

    // set up the user data
    data = (UserData*)malloc(sizeof(UserData));
    data->maximumValue = maximumValue;
    data->minimumValue = minimuimValue;
    data->model = model;

	/* Create serial vectors of length NEQ */
	u = N_VNew_Serial(NEQ);
	if (check_flag((void *) u, "N_VNew_Serial", 0))
		return (1);
    s = N_VNew_Serial(NEQ);
    if (check_flag((void *) s, "N_VNew_Serial", 0))
        return (1);
    c = N_VNew_Serial(NEQ);
    if (check_flag((void *) c, "N_VNew_Serial", 0))
        return (1);

	// set initial guess
	realtype *udata = NV_DATA_S(u);
	if (model->mode)
		udata[0] = model->U_t;
	else
        udata[0] = model->U_a;
    udata[1] = udata[0] - minimuimValue; // lower bound constraint initial guess
    udata[2] = udata[0] - maximumValue;  // upper     "       "       "

	N_VConst_Serial(RCONST(1.0), s); /* no scaling */

    Ith(c,1) =  ZERO;   /* no constraint on U */
    Ith(c,2) =  ONE;    /* l = U - minimumValue >= 0 */
    Ith(c,3) = -ONE;    /* L = U - maximumValue <= 0 */

	fnormtol = FTOL;
	scsteptol = STOL;

	kmem = KINCreate();
	if (check_flag((void *) kmem, "KINCreate", 0))
		return (1);

    flag = KINSetUserData(kmem, static_cast<void*>(data));
	if (check_flag(&flag, "KINSetUserData", 1))
		return (1);
    flag = KINSetConstraints(kmem, c);
    if (check_flag(&flag, "KINSetConstraints", 1)) return(1);
    flag = KINSetFuncNormTol(kmem, fnormtol);
	if (check_flag(&flag, "KINSetFuncNormTol", 1))
		return (1);
	flag = KINSetScaledStepTol(kmem, scsteptol);
	if (check_flag(&flag, "KINSetScaledStepTol", 1))
		return (1);

	flag = KINInit(kmem, func, u);
	if (check_flag(&flag, "KINInit", 1))
		return (1);

	/* Call KINDense to specify the linear solver */
	flag = KINDense(kmem, NEQ);
	if (check_flag(&flag, "KINDense", 1))
		return (1);

    // Level of output from Kinsol (default/no output = 0, most output = 3)
    KINSetPrintLevel(kmem, 0);

    // we want to control the errors that get output
    KINSetErrHandlerFn(kmem, handleKinsolError, NULL);

	/* Print out the problem size, solution parameters, initial guess. */
    //PrintHeader(glstr, fnormtol, scsteptol);

	/* --------------------------- */

    /*printf("\n------------------------------------------\n");
	printf("\nInitial guess\n");
	printf("  [x1,x2] = ");
    PrintOutput(u);*/

    glstr = KIN_LINESEARCH;
    mset = 0;
    int returnCode = SolveIt(kmem, u, s, glstr, mset);

	//glstr = KIN_LINESEARCH;
	//mset = 1;
	//SolveIt(kmem, u, s, glstr, mset);

	//glstr = KIN_NONE;
	//mset = 0;
	//SolveIt(kmem, u, s, glstr, mset);

	//glstr = KIN_LINESEARCH;
	//mset = 0;
	//SolveIt(kmem, u, s, glstr, mset);

    /**
      @todo Is this needed???
      */
	// assign final value
	udata = NV_DATA_S(u);
	if (model->mode)
    {
		model->U_t = udata[0];
        int errorFlag;
        model->solveOpenCircuitPotentials(errorFlag, true);
    }
	else
    {
		model->U_a = udata[0];
        model->solveVoltageClampPotentials(true);
    }


    /* Free memory */
	N_VDestroy_Serial(u);
	N_VDestroy_Serial(s);
    N_VDestroy_Serial(c);
	KINFree(&kmem);

    return (returnCode);
}

static int SolveIt(void *kmem, N_Vector u, N_Vector s, int glstr, int mset)
{
	int flag;

    /*
	printf("\n");
	if (mset == 1)
		printf("Exact Newton");
	else
		printf("Modified Newton");
	if (glstr == KIN_NONE)
		printf("\n");
	else
		printf(" with line search\n");
    */

	flag = KINSetMaxSetupCalls(kmem, mset);
	if (check_flag(&flag, "KINSetMaxSetupCalls", 1))
		return (1);

    int counter = 0;
    do
    {
        flag = KINSol(kmem, u, glstr, s, s);
        /*
         * KIN SUCCESS = 0, KIN INITIAL GUESS OK = 1, and KIN STEP LT STPTOL = 2. All
        * remaining return values are negative and therefore a test flag < 0 will trap all KINSol
        * failures.
        */
        if (flag >= 0) break;
        // try perturbing 'u' to see if we can find a solution - might want to restrict under
        // what error conditions to do this? (conditions will sort themselves out?)
        //NV_Ith_S(u, 0) = NV_Ith_S(u, 0) + 1.1;
        // perturbing doesn't seem to help, but switching off the linesearch at least lets the Latta
        // protocol complete...
        glstr = KIN_NONE;
    }
    while (++counter < 2);

    if (flag < 0)
    {
        std::cerr << "KINSol failed with error code: " << flag << std::endl;
        return (1);
    }

    if (debugLevel() > 15)
    {
        printf("Solution:\n  U = ");
        PrintOutput(u);
    }

    if (debugLevel() > 30) PrintFinalStats(kmem);

	return (0);

}

/*
 *--------------------------------------------------------------------
 * FUNCTIONS CALLED BY KINSOL
 *--------------------------------------------------------------------
 */

static int func(N_Vector u, N_Vector f, void *user_data)
{
	realtype *udata, *fdata;
    UserData* data;

    data = static_cast<UserData*>(user_data);

	udata = NV_DATA_S(u);
	fdata = NV_DATA_S(f);

    int errorFlag = 0;
    if (data->model->mode)
	{
		// Open-circuit case (solving for E_t)
        data->model->U_t = udata[0];
        fdata[0] = data->model->solveOpenCircuitPotentials(errorFlag);
        if (errorFlag != 0)
        {
            std::cerr << "failure reported from solveForElectroneutrality" << std::endl;
            return 1; // return negative to indicate non-recoverable failure (positive = recoverable failure)
        }
    }
	else
	{
		// Voltage-clamp case (solving for E_a)
        data->model->U_a = udata[0];
        fdata[0] = data->model->solveVoltageClampPotentials();
	}

    // constraints
    fdata[1] = udata[1] - udata[0] + data->minimumValue;
    fdata[2] = udata[2] - udata[0] + data->maximumValue;

	return (0);
}

/*
 *--------------------------------------------------------------------
 * PRIVATE FUNCTIONS
 *--------------------------------------------------------------------
 */

/*
 * Print first lines of output (problem description)
 */

//static void PrintHeader(int globalstrategy, realtype fnormtol,
//		realtype scsteptol)
//{
//	printf("\nBobby\n");
//	printf("Tolerance parameters:\n");
//#if defined(SUNDIALS_EXTENDED_PRECISION)
//	printf("  fnormtol  = %10.6Lg\n  scsteptol = %10.6Lg\n",
//			fnormtol, scsteptol);
//#elif defined(SUNDIALS_DOUBLE_PRECISION)
//	printf("  fnormtol  = %10.6lg\n  scsteptol = %10.6lg\n", fnormtol,
//			scsteptol);
//#else
//	printf("  fnormtol  = %10.6g\n  scsteptol = %10.6g\n",
//			fnormtol, scsteptol);
//#endif
//}

/*
 * Print solution
 */

static void PrintOutput(N_Vector u)
{
#if defined(SUNDIALS_EXTENDED_PRECISION)
	printf(" %8.6Lg\n", Ith(u,1));
#elif defined(SUNDIALS_DOUBLE_PRECISION)
	printf(" %8.6lg\n", Ith(u,1));
#else
	printf(" %8.6g\n", Ith(u,1));
#endif
}

/*
 * Print final statistics contained in iopt
 */

static void PrintFinalStats(void *kmem)
{
    long int nni, nfe, nje, nfeD;
    int flag;

    flag = KINGetNumNonlinSolvIters(kmem, &nni);
    check_flag(&flag, "KINGetNumNonlinSolvIters", 1);
    flag = KINGetNumFuncEvals(kmem, &nfe);
    check_flag(&flag, "KINGetNumFuncEvals", 1);

    flag = KINDlsGetNumJacEvals(kmem, &nje);
    check_flag(&flag, "KINDlsGetNumJacEvals", 1);
    flag = KINDlsGetNumFuncEvals(kmem, &nfeD);
    check_flag(&flag, "KINDlsGetNumFuncEvals", 1);

    printf("Final Statistics:\n");
    printf("  nni = %5ld    nfe  = %5ld \n", nni, nfe);
    printf("  nje = %5ld    nfeD = %5ld \n", nje, nfeD);
}

/*
 * Check function return value...
 *    opt == 0 means SUNDIALS function allocates memory so check if
 *             returned NULL pointer
 *    opt == 1 means SUNDIALS function returns a flag so check if
 *             flag >= 0
 *    opt == 2 means function allocates memory so check if returned
 *             NULL pointer
 */

static int check_flag(void *flagvalue, const char *funcname, int opt)
{
	int *errflag;

	/* Check if SUNDIALS function returned NULL pointer - no memory allocated */
	if (opt == 0 && flagvalue == NULL)
	{
		fprintf(stderr,
				"\nSUNDIALS_ERROR: %s() failed - returned NULL pointer\n\n",
				funcname);
		return (1);
	}

	/* Check if flag < 0 */
	else if (opt == 1)
	{
		errflag = (int *) flagvalue;
		if (*errflag < 0)
		{
			fprintf(stderr, "\nSUNDIALS_ERROR: %s() failed with flag = %d\n\n",
					funcname, *errflag);
			return (1);
		}
	}

	/* Check if function returned NULL pointer - no memory allocated */
	else if (opt == 2 && flagvalue == NULL)
	{
		fprintf(stderr,
				"\nMEMORY_ERROR: %s() failed - returned NULL pointer\n\n",
				funcname);
		return (1);
	}

	return (0);
}

