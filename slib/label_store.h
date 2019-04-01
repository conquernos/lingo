#ifndef _LABEL_STORE_H
#define _LABEL_STORE_H

#include "svdlib.h"
#include "phrase_match.h"
#include "phrase_store.h"
#include "abstract_concept.h"

#define ISPHRASE	0x00
#define ISTERM		0x01

/* M matrix
   row count : K
   column count : feature + term */
typedef struct _M_MATRIX{
	DMat mtrx;
	int phrase_cnt;
} M_MATRIX;

typedef struct _LABEL{
	int ptid;			// phrase id or term id
	char phrase_term;	// ISPHRASE or ISTERM
	char pruned;		// pruned(low score):2 pruned(similar):1 or not:0
	double score;
	int term_cnt;
} LABEL;

typedef struct _LABEL_TABLE{
	LABEL *labels;
	int count;
	int phrase_cnt;
	int term_cnt;
	int pruned_cnt;
} LABEL_TABLE;

// M matrix
int makeMMatrix(M_MATRIX *mmtrx, P_MATRIX *pmtrx, ABS_CONCEPT *abs);
int makeMMatrix_Phrase(M_MATRIX *mmtrx, P_MATRIX *pmtrx, ABS_CONCEPT *abs);

// label table
LABEL_TABLE *createLabelTable();
int makeLabelTable(LABEL_TABLE *label_table, M_MATRIX *mmtrx, PHRASE_TABLE *phrase_table, int featcnt);
void freeLabelTable(LABEL_TABLE *table);

#endif
