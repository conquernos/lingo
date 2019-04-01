#ifndef _DOC_TERM_MATRIX_H
#define _DOC_TERM_MATRIX_H

#include "svdlib.h"
#include "feature_store.h"
#include "term_store.h"
#include "cossim.h"

typedef struct _TERM_FRQ{
	int termid;
	int termfrq;
} TERM_FRQ;

typedef struct _TERM_LIST{
	TERM_FRQ *termfrqs;
	int count;
} TERM_LIST;

typedef struct _DOC_TERM_TABLE{
	TERM_LIST *termlists;
	int count;
} DOC_TERM_TABLE;


/* term-doc matrix */
int makeDocTermTable(DOC_FEAT_TABLE *dftable, FEATURE_TABLE *feattable, TERM_TABLE *termtable, DOC_TERM_TABLE *dttable, int *valcnt);
int makeTermDocMatrix(DOC_FEAT_TABLE *dftable, FEATURE_TABLE *feattable, TERM_TABLE *termtable, SMat *tdmtrx, enum weight_type type);

#endif
