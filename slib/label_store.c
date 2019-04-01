#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "label_store.h"

#ifndef MAX
#define MAX(X,Y)	((X)>(Y))? (X):(Y)
#endif

static int _cmp_label(const void *label1, const void *label2);

// make a M matrix
int makeMMatrix(M_MATRIX *mmtrx, P_MATRIX *pmtrx, ABS_CONCEPT *abs)
{
	double **M_value;
	double **Ut_value;
	int rows, cols;
	int rownum, colnum, num;
	double val;

	rows = abs->k;
	cols = pmtrx->phrase_cnt + pmtrx->termcnt;
	Ut_value = abs->Ut->value;

	if(!(mmtrx->mtrx = svdNewDMat(rows, cols))) return -1;
	M_value = mmtrx->mtrx->value;

	// Ut matirx * P matrix
	for(rownum=0; rownum<rows; rownum++){	// M`s row
		for(colnum=0; colnum<cols; colnum++){	// M`s column
			val = 0.0;
			for(num=0; num<pmtrx->termcnt; num++){	// Ut`s row * P`s column
				val += Ut_value[rownum][num] * getPMatrixValue(pmtrx, num, colnum);
			}
			M_value[rownum][colnum] = fabs(val);
		}
	}

	return 0;
}

// make a M matrix with only phrases
int makeMMatrix_Phrase(M_MATRIX *mmtrx, P_MATRIX *pmtrx, ABS_CONCEPT *abs)
{
	double **M_value;
	double **Ut_value;
	int rows, cols;
	int rownum, colnum, num;
	double val;

	rows = abs->k;
	cols = pmtrx->phrase_cnt;
	Ut_value = abs->Ut->value;

	if(!(mmtrx->mtrx = svdNewDMat(rows, cols))) return -1;
	M_value = mmtrx->mtrx->value;

	// Ut matrix * P matrix
	for(rownum=0; rownum<rows; rownum++){	// M`s row
		for(colnum=0; colnum<cols; colnum++){	// M`s column
			val = 0.0;
			for(num=0; num<pmtrx->termcnt; num++){	// Ut`s row * P`s column
				val += Ut_value[rownum][num] * getPMatrixValue(pmtrx, num, colnum);
			}
			M_value[rownum][colnum] = fabs(val);
		}
	}

	return 0;
}

// create a label table
LABEL_TABLE *createLabelTable()
{
	LABEL_TABLE *table;

	table = malloc(sizeof(LABEL_TABLE));
	memset(table, 0, sizeof(LABEL_TABLE));

	return table;
}

// make a label table
int makeLabelTable(LABEL_TABLE *label_table, M_MATRIX *mmtrx, PHRASE_TABLE *phrase_table, int featcnt)
{
	LABEL *labels, *plabel;
	PHRASE *phrases;
	int colnum, rownum;
	int labelcnt;
	double maxval;

	labelcnt = mmtrx->mtrx->cols;
	labels = malloc(sizeof(LABEL)*labelcnt);
	phrases = phrase_table->phrases;

	for(colnum=0; colnum<mmtrx->mtrx->cols; colnum++){
		maxval = 0.0;

		for(rownum=0; rownum<mmtrx->mtrx->rows; rownum++){
			//maxval = MAX(fabs(maxval), fabs(mmtrx->mtrx->value[rownum][colnum]));
			maxval = MAX(maxval, mmtrx->mtrx->value[rownum][colnum]);
		}
	
		// save to the structure
		plabel = labels + colnum;
		plabel->ptid = (colnum<featcnt)? colnum : colnum-featcnt;
		plabel->phrase_term = (colnum<featcnt)? ISPHRASE : ISTERM;
		plabel->score = (maxval>1.0)? 1.0 : maxval;
		plabel->pruned = 0;

		// if the label is a phrase
		if(plabel->phrase_term == ISPHRASE){
			label_table->phrase_cnt++;
			plabel->term_cnt = phrases[colnum].termcnt;

		// if the label is a term
		}else{
			label_table->term_cnt++;
			plabel->term_cnt = 1;
		}
	}

	// sort the labels into order of the score
	qsort(labels, labelcnt, sizeof(LABEL), _cmp_label);
	label_table->count = labelcnt;
	label_table->labels = labels;

	return 0;
}

// free memories allocated to a label table
void freeLabelTable(LABEL_TABLE *table)
{
	if(table){
		if(table->labels){
			free(table->labels);
		}
		free(table);
	}
}

static int _cmp_label(const void *label1, const void *label2)
{
	return (((LABEL*)label1)->score > ((LABEL*)label2)->score)? -1 : 1;
}
