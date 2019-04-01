#include <stdio.h>
#include <math.h>

#include "term_doc_matrix.h"
#include "inssort.h"

static const int termfrq_size = sizeof(TERM_FRQ);

#ifndef ROUND
#define ROUND(X,Y)	floor((X)*pow(10,(Y))+0.5)/pow(10,(Y))
#endif

static int _cmp_termfrq(const void *elmt, const void *key);

// make a doc-term table
int makeDocTermTable(DOC_FEAT_TABLE *dftable, FEATURE_TABLE *feattable, TERM_TABLE *termtable, DOC_TERM_TABLE *dttable, int *valcnt)
{
	FEATURE_LIST *pfeatlist;
	FEATURE_FRQ *pfeatfrq;
	FEATURE *pfeat;
	TERM_FRQ *termfrqs, termfrq;
	TERM_LIST *termlists, *ptermlist;
	INSSORT *ins_term;
	int termid;

	int docnum, featnum, termnum;
	int termcnt, term_curcnt;

	termlists = malloc(sizeof(TERM_LIST)*dftable->count);
	memset(termlists, 0, sizeof(TERM_LIST)*dftable->count);
	*valcnt = 0;

	for(docnum=0; docnum<dftable->count; docnum++){
		pfeatlist = dftable->featlists + docnum;
		ptermlist = termlists + docnum;

		// count the number of terms that each features of the document have
		for(featnum=0, termcnt=0; featnum<pfeatlist->count; featnum++){
			pfeatfrq = pfeatlist->featfrqs + featnum;
			pfeat = feattable->features + pfeatfrq->featid;
			if(pfeat->qualify_flg) termcnt += pfeat->termcnt;
		}
		termfrqs = malloc(sizeof(TERM_FRQ)*termcnt);
		ins_term = openInsertSort(&termfrqs, termfrq_size, 0, termcnt, 0, _cmp_termfrq);

		// insert terms to the doc-term table
		for(featnum=0, term_curcnt=0; featnum<pfeatlist->count; featnum++){
			pfeatfrq = pfeatlist->featfrqs + featnum;
			pfeat = feattable->features + pfeatfrq->featid;
			if(!pfeat->qualify_flg) continue;

			for(termnum=0; termnum<pfeat->termcnt; termnum++){
				termid = *(pfeat->termids + termnum);
				termfrq.termid = termid;
				termfrq.termfrq = pfeatfrq->featfrq;

				if(insertSort(ins_term, &termfrq) == 0){
					term_curcnt++;
					termtable->terms[termid].docfrq++;
				}
			}
		}
		/*
		// for memory saving
		if(termcnt != term_curcnt){
		termfrqs = realloc(termfrqs, termfrq_size*term_curcnt);
		}
		*/

		ptermlist->termfrqs = termfrqs;
		*valcnt += ptermlist->count = term_curcnt;
		closeInsertSort(ins_term);
	}

	dttable->termlists = termlists;
	dttable->count = dftable->count;

	return 0;
}

// make a term-doc matrix
int makeTermDocMatrix(DOC_FEAT_TABLE *dftable, FEATURE_TABLE *feattable, TERM_TABLE *termtable, SMat *tdmtrx, enum weight_type type)
{
	DOC_TERM_TABLE dttable;
	TERM_LIST *termlists, *ptermlist;
	TERM_FRQ *termfrqs, *ptermfrq;
	int rows, cols, vals = 0;
	int docnum, pointr, termnum, sop, eop;
	int termid;
	int termfrq, docfrq;
	double veclen, *val;

	rows = termtable->count;
	cols = dftable->count;

	// make a doc-term table
	makeDocTermTable(dftable, feattable, termtable, &dttable, &vals);
	termlists = dttable.termlists;

	if((*tdmtrx = svdNewSMat(rows, cols, vals)) == NULL) return -1;

	// make a sparse term-doc matrix using the doc-term table
	for(docnum=0, pointr=0; docnum<cols; docnum++){
		(*tdmtrx)->pointr[docnum] = pointr;
		ptermlist = termlists + docnum;
		termfrqs = ptermlist->termfrqs;
		veclen = 0.0;

		sop = pointr;
		for(termnum=0; termnum<ptermlist->count; termnum++){
			ptermfrq = termfrqs + termnum;
			termid = ptermfrq->termid;
			termfrq = ptermfrq->termfrq;
			docfrq = termtable->terms[termid].docfrq;

			(*tdmtrx)->rowind[pointr] = termid;

			// set the value weight
			switch(type){
				case TF:	// TF weighting
					(*tdmtrx)->value[pointr] = termfrq;
					break;
				case DF:	// DF weighting
					(*tdmtrx)->value[pointr] = docfrq;
					break;
				case TF_IDF:	// TF-IDF weighting
					(*tdmtrx)->value[pointr] = termfrq * (log(cols/docfrq)+0.01);
					break;
			}

			veclen += pow((*tdmtrx)->value[pointr], 2);
			pointr++;
		}
		eop = pointr;
		veclen = sqrt(veclen);
		// vector length normalization
		for(termnum=sop; termnum<eop; termnum++){
			val = (*tdmtrx)->value + termnum;
			if(veclen != 0) *val /= veclen;

			*val = ROUND(*val, 2);
		}
	}
	(*tdmtrx)->pointr[cols] = vals;

	for(docnum=0; docnum<dttable.count; docnum++){
		ptermlist = termlists + docnum;

		free(ptermlist->termfrqs);
	}
	free(termlists);

	return 0;
}

static int _cmp_termfrq(const void *elmt, const void *key)
{
	TERM_FRQ *elmt_termfrq, *key_termfrq;

	elmt_termfrq = (TERM_FRQ*)elmt;
	key_termfrq = (TERM_FRQ*)key;

	if(elmt_termfrq->termid == key_termfrq->termid){
		elmt_termfrq->termfrq += key_termfrq->termfrq;
	}

	return elmt_termfrq->termid - key_termfrq->termid;
}
