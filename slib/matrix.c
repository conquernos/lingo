#include <stdio.h>
#include <stdlib.h>
#include "matrix.h"

// multiply two matrixes
int multiplyMatrix(DMat Amat, DMat Bmat, DMat *result_mat)
{
	int i, j, k;
	double val = 0.0;

	if(Amat->cols > Bmat->rows || !result_mat){
		printf("[ERR] multiplyMatrix arg error\n");
		return -1;
	}

	*result_mat = svdNewDMat(Amat->rows, Bmat->cols);
	if(!*result_mat){
		printf("[ERR] svdNewDMat failed\n");
		return -1;
	}

	for(i=0; i<Amat->rows; i++){	// row
		for(j=0; j<Bmat->cols; j++){	// column
			val = 0.0;
			for(k=0; k<Amat->cols; k++){	// Amat`s cols or Bmat`s rows
				val += Amat->value[i][k] * Bmat->value[k][j];
			}
			(*result_mat)->value[i][j] = val;
		}
	}

	return 0;
}

DMat multiplyMatrixWithS(double *s, DMat mat, int dimension)
{
	int i,j;

	if(sizeof(s)/sizeof(double) > mat->rows){
		printf("[ERR] multiplyMatrix arg error\n");
		return NULL;
	}

	//printf("-->size:%ldbyte\n", dimension*mat->cols*sizeof(double));
	DMat result_mat = svdNewDMat(dimension, mat->cols);
	if(!result_mat){
		printf("[ERR] svdNewDMat failed\n");
		return NULL;
	}

	for(i=0; i<dimension; i++){
		for(j=0; j<mat->cols; j++){
			result_mat->value[i][j] = s[i] * mat->value[i][j];
		}
	}

	return result_mat;
}

int multiplySelfMatrixWithS(double *s, DMat mat, int dimension)
{
	int i,j;

	if( !s || !mat || (sizeof(s)/sizeof(double) > mat->rows) ){
		printf("[ERR] multiplyMatrix arg error\n");
		return -1;
	}

	for(i=0; i<dimension; i++){
		for(j=0; j<mat->cols; j++){
			mat->value[i][j] *= s[i];
		}
	}

	return 0;
}
