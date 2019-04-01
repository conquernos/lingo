#ifndef _COSSIM_H
#define _COSSIM_H

#include "phrase_store.h"
#include "term_store.h"

enum weight_type {TF, DF, TF_IDF};

double calcPhraseTermCosSim(PHRASE *phrases, TERM *terms, int phraseid, int termid, int doccnt, enum weight_type type);
double calcPhraseCosSim(PHRASE *phrases, TERM *terms, int phraseid1, int phraseid2, int doccnt, int *samecnt, enum weight_type type);

#endif
