#include <math.h>

#include "phrase_match.h"

// make a P matrix
int makePMatrix(P_MATRIX *pmtrx, PHRASE_TABLE *phrase_table, TERM_TABLE *term_table, int doccnt, enum weight_type type)
{
	SMat term_phrase_mtrx;
	PHRASE *phrases;
	TERM *terms;
	int phrsnum, termnum;
	int *termids;
	int termid;
	int termcnt;
	int vals, n, point;

	if(!(phrases = phrase_table->phrases)) return -1;
	if(!(terms = term_table->terms)) return -1;

	// count the number of terms
	for(phrsnum=0, vals=0; phrsnum<phrase_table->count; phrsnum++){
		vals += phrases[phrsnum].termcnt;
	}

	if((term_phrase_mtrx = svdNewSMat(term_table->count, phrase_table->count, vals)) == NULL){
		pmtrx->tfmtrx = NULL;
		return -1;
	}

	term_phrase_mtrx->pointr[0] = 0;
	for(phrsnum=0, n=0, point=0; phrsnum<phrase_table->count; phrsnum++){
		termids = phrases[phrsnum].termids;
		termcnt = phrases[phrsnum].termcnt;

		// pointr : 해당 열의 시작 포인트 (이전 열까지의 누적된 0이 아닌 값의 개수)
		term_phrase_mtrx->pointr[phrsnum+1] = point += termcnt;

		for(termnum=0; termnum<termcnt; termnum++){
			termid = termids[termnum];

			// rowind : 0이 아닌 값을 가진 행번호
			term_phrase_mtrx->rowind[n] = termid;

			// value : 0이 아닌 값
			term_phrase_mtrx->value[n] = calcPhraseTermCosSim(phrases, terms, phrsnum, termid, doccnt, type);
			n++;
		}
	}
	pmtrx->tfmtrx = term_phrase_mtrx;
	pmtrx->phrase_cnt = phrase_table->count;
	pmtrx->termcnt = term_table->count;

	return 0;
}

// free memories allocated to a P matrix
void freePMatrix(P_MATRIX *pmtrx)
{
	if(pmtrx && pmtrx->tfmtrx){
		svdFreeSMat(pmtrx->tfmtrx);
	}
}

double getPMatrixValue(P_MATRIX *pmtrx, long row, long col)
{
	if(col < pmtrx->phrase_cnt){
		return getSVal(pmtrx->tfmtrx, row, col);
	}else{
		if(row == col - pmtrx->phrase_cnt) return 1.0;
	}

	return 0.0;
}


