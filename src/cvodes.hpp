#ifndef CVODES_HPP
#define CVODES_HPP

// might as well make this available externally?
#include <nvector/nvector_serial.h>  /* serial N_Vector types, fcts., and macros */
/* User-defined vector and matrix accessor macros: Ith, IJth */

/* These macros are defined in order to write code which exactly matches
   the mathematical problem description given above.

   Ith(v,i) references the ith component of the vector v, where i is in
   the range [1..NEQ] and NEQ is defined below. The Ith macro is defined
   using the N_VIth macro in nvector.h. N_VIth numbers the components of
   a vector starting from 0.

   IJth(A,i,j) references the (i,j)th element of the dense matrix A, where
   i and j are in the range [1..NEQ]. The IJth macro is defined using the
   DENSE_ELEM macro in dense.h. DENSE_ELEM numbers rows and columns of a
   dense matrix starting from 0. */
#define Ith(v,i)    NV_Ith_S(v,i-1)       /* Ith numbers components 1..NEQ */
#define IJth(A,i,j) DENSE_ELEM(A,i-1,j-1) /* IJth numbers rows,cols 1..NEQ */

class GeneralModel;

class Cvodes
{
public:
    Cvodes();
    ~Cvodes();

    /**
      Initialise CVODES for this model. @returns zero on success.
      */
    int initialise(GeneralModel* model, double initialTime, double maxStep);

    /**
      * Re-initialise CVODES for this model. @returns zero on success.
      */
    int reInitialise(double time);

    /**
      Integrate them model from t to tout. @returns zero on success and t updated to final value.
      */
    int integrate(double& t, double tout);

    void* cvodeMem;
    N_Vector y;
};

#endif // CVODES_HPP
