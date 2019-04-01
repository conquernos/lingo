#ifndef _FEATURE_EXTRACT_H
#define _FEATURE_EXTRACT_H

#include "nlplib.h"

/* feature morpheme number */
typedef struct _FEATURE_MORPHEME{
	int bom;
	int eom;
} FEATURE_MORPHEME;

/* feature morpheme number table */
typedef struct _FEATURE_MORPHEME_TABLE{
	FEATURE_MORPHEME *feattexts;
	int count;
	int maxcnt;
} FEATURE_MORPHEME_TABLE;

enum allowed_morph_type {VARIETY, NOUN};

/* feature morpheme table */
FEATURE_MORPHEME_TABLE * createFeatureTextList(int maxcnt);
int extractFeatureMorph(NLPDOC *nlp, FEATURE_MORPHEME_TABLE *featlist, enum allowed_morph_type);
void freeFeatureTextList(FEATURE_MORPHEME_TABLE *featlist);

#endif
