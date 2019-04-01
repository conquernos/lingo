#ifndef _ABSTRACT_CONCEPT_H
#define _ABSTRACT_CONCEPT_H

#include "svdlib.h"

typedef struct ABS_CONCEPT{
	int k;		// dimension
	DMat Ut;
} ABS_CONCEPT;

// Ut matrix
int obtainK(SMat tdmat, double q, int *k);
int makeUt(ABS_CONCEPT *abs, SMat tdmat, double q, int dimension);

#endif
