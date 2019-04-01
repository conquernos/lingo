#ifndef _PHRASE_MATCH_H
#define _PHRASE_MATCH_H

#include "svdlib.h"
#include "phrase_store.h"
#include "term_store.h"
#include "cossim.h"

typedef struct _P_MATRIX{
	SMat tfmtrx;	// term-feat matrix
	int phrase_cnt;
	int termcnt;
} P_MATRIX;

// P matrix
int makePMatrix(P_MATRIX *pmtrx, PHRASE_TABLE *phrase_table, TERM_TABLE *term_table, int doccnt, enum weight_type type);
double getPMatrixValue(P_MATRIX *pmtrx, long row, long col);

#endif
