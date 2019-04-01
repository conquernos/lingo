#ifndef _PHRASE_STORE_H
#define _PHRASE_STORE_H

#include "feature_store.h"

typedef struct _PHRASE{
	char *phrase;
	int *termids;
	int docfrq;
	int termcnt;
} PHRASE;

typedef struct _PHRASE_TABLE{
	PHRASE *phrases;
	int count;
} PHRASE_TABLE;

// phrase table
PHRASE_TABLE *createPhraseTable();
int makePhraseTable(PHRASE_TABLE *phrase_table, FEATURE_TABLE *feature_table);
void freePhraseTable(PHRASE_TABLE *table);

#endif
