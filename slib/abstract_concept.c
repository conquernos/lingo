#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "abstract_concept.h"

#ifndef MIN
#define MIN(X,Y)	(((X) < (Y))? (X) : (Y))
#endif

// obtain the number K(rank)
int obtainK(SMat tdmat, double q, int *k)
{
	SVDRec svd;
	double *singular_vals;
	int r;		// the rank of the A matrix
	double Af = 0.0		// Frobenius norm of the original doc-term matrix
		, Akf = 0.0;	// Frobenius norm of the A`s k-rank approximation
	int knum;

	r = MIN(tdmat->cols, tdmat->rows);

	if(!(svd = svdLAS2A_S(tdmat, r))) return -1;
	singular_vals = svd->S;
	
	// Frobenius norm of the original doc-term matrix
	for(knum=0; knum<r; knum++){
		Af += pow(singular_vals[knum], 2);
	}
	Af = sqrt(Af);

	// Frobenius norm of the A`s k-rank approximation
	for(knum=0; knum<r; knum++){
		Akf += pow(singular_vals[knum], 2);

		if(sqrt(Akf)/Af >= q) break;
	}

	// obtain the K
	*k = (knum+1>1)? knum+1:2;

	svdFreeSVDRec(svd);

	return 0;
}

// make the Ut matrix (transpose matrix of the U)
int makeUt(ABS_CONCEPT *abs, SMat tdmat, double q, int dimension)
{
	SVDRec svd;

	if(dimension > 0) abs->k = dimension;
	else if(obtainK(tdmat, q, &abs->k) < 0) return -1;
	svd = svdLAS2A(tdmat, abs->k);
	abs->Ut = svd->Ut;
	abs->k = abs->Ut->rows;

	svd->Ut = NULL;
	svdFreeSVDRec(svd);

	return 0;
}


