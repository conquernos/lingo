#ifndef MATRIX_H_
#define MATRIX_H_

#include "svdlib.h"

int multiplyMatrix(DMat Amat, DMat Bmat, DMat *result_mat);
DMat multiplyMatrixWithS(double *s, DMat mat, int dimension);
int multiplySelfMatrixWithS(double *s, DMat mat, int dimension);

#endif
