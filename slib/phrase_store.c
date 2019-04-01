#include <stdio.h>

#include "phrase_store.h"

// create a phrase table
PHRASE_TABLE *createPhraseTable()
{
	PHRASE_TABLE *table;

	table = malloc(sizeof(PHRASE_TABLE));
	memset(table, 0, sizeof(PHRASE_TABLE));

	return table;
}

// make a phrase table
int makePhraseTable(PHRASE_TABLE *phrase_table, FEATURE_TABLE *feature_table)
{
	FEATURE *feats;
	PHRASE *phrases;
	int featnum, phrsnum;
	int phrase_size = sizeof(PHRASE);

	feats = feature_table->features;
	phrases = phrase_table->phrases = malloc(phrase_size*feature_table->count);

	// insert a feature that qualified and have more than two terms to the phrase table
	for(featnum=0, phrsnum=0; featnum<feature_table->count; featnum++){
		if( (feats[featnum].qualify_flg) && (feats[featnum].termcnt > 1) ){
			memcpy(phrases + phrsnum++, feats + featnum, phrase_size);
		}
	}
	phrase_table->count = phrsnum;
	phrase_table->phrases = realloc(phrase_table->phrases, phrase_size*phrase_table->count);

	return 0;
}

// free memories allocated to a phrase table
void freePhraseTable(PHRASE_TABLE *table)
{
	if(table){
		if(table->phrases) free(table->phrases);
		free(table);
	}
}
