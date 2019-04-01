/* Library for the Feature Store 
   the feature id and the doc id begin with 0 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "feature_store.h"

static int _free_feattree(UCS2 *key, char *value, char *args[]);
static int _set_feattable(UCS2 *key, char *value, char *args[]);

// create a feature tree
FEATURE_TREE * createFeatureTree()
{
	FEATURE_TREE *tree;

	if((tree = malloc(sizeof(FEATURE_TREE)))){
		tree->max_featid = -1;
		if(!(tree->tree = new_mtrie16())){
			free(tree); tree = NULL;
		}
	}

	return tree;
}

// insert a feature to the feature tree (for search)
int insertToFeatureTree(FEATURE_TREE *tree, UCS2 *feature, char *words, int docid, int *featid)
{
	FEATURE_TREE_VAL *value;

	// already exist
	if((value = (FEATURE_TREE_VAL*)search_mtrie16(tree->tree, feature))){
		if(value->last_docid < docid) value->docfrq++;
		*featid = value->featid;
		value->last_docid = docid;
		return 1;

	// newly insert
	}else{
		value = malloc(sizeof(FEATURE_TREE_VAL));
		value->words = words;
		*featid = value->featid = ++(tree->max_featid);
		value->docfrq = 1;

		if(insert_mtrie16(tree->tree, feature, (char*)value, sizeof(value)) < 0){
			free(value);
			return -1;
		}
		croot_addr_mtrie16(tree->tree);
		value->last_docid = docid;
	}

	return 0;
}

// free memories allocated to the feature tree
void freeFeatureTree(FEATURE_TREE *tree)
{
	int free_tree_value(char *value)
	{
		if(value) free(value);
		return 0;
	}

	if(tree){
		if(tree->tree){
			//traverse_mtrie16(tree->tree, _free_feattree, NULL);
			//free(tree->tree);
			free_mtrie16(tree->tree, free_tree_value);
		}
		free(tree);
	}
}

// create a feature table
FEATURE_TABLE * createFeatureTable(int docfrq)
{
	FEATURE_TABLE *table;

	if((table = malloc(sizeof(FEATURE_TABLE)))){
		table->features = NULL;
		table->threshold_docfrq = docfrq;
		table->count = 0;
	}

	return table;
}

// make the feature table
int makeFeatureTable(FEATURE_TABLE *table, FEATURE_TREE *tree)
{
	int featcnt;

	featcnt = tree->max_featid+1;
	table->features = malloc(sizeof(FEATURE)*(featcnt));
	traverse_mtrie16(tree->tree, _set_feattable, (char**)&table);
	table->count = featcnt;

	return 0;
}

// set the term IDs of the feature table
int setTermidOfFeatureTable(FEATURE_TABLE *table, int featid, int *termids, int count)
{
	FEATURE *pfeat;

	if(table->count <= featid) return -1;

	pfeat = table->features + featid;

	pfeat->termids = termids;
	pfeat->termcnt = count;

	switch(pfeat->termcnt){
		case 3:
			pfeat->weight = pfeat->docfrq * 10;
			break;
		case 4:
		case 2:
			pfeat->weight = pfeat->docfrq * 8;
			break;
		default:
			pfeat->weight = pfeat->docfrq * 5;
	}

	return 0;
}

// free memories allocated to the feature table
void freeFeatureTable(FEATURE_TABLE *table)
{
	FEATURE *feats;
	int featnum;

	if(table){
		if((feats = table->features)){
			for(featnum=0; featnum<table->count; featnum++){
				free(feats[featnum].feature);
				free(feats[featnum].termids);
				free(feats[featnum].words);
			}
			free(table->features);
		}
		free(table);
	}
}

// create a doc-feature table
DOC_FEAT_TABLE * createDocFeatTable(int doccnt)
{
	DOC_FEAT_TABLE *table = NULL;

	if((table = malloc(sizeof(DOC_FEAT_TABLE)))){
		table->count = doccnt;
		if(!(table->featlists = malloc(sizeof(FEATURE_LIST)*doccnt))){
			free(table);
			table = NULL;
		}
		memset(table->featlists, 0, sizeof(FEATURE_LIST)*doccnt);
	}

	return table;
}

// add a feature in the feature list (FEATURE_FRQ list)
// the argument featfrqs must be allocated
int addInFeatureList(FEATURE_FRQ *featfrqs, int featid, int *filcnt)
{
	FEATURE_FRQ *feat = NULL;
	int featnum;

	for(featnum=0; featnum<*filcnt; featnum++){
		feat = featfrqs+featnum;

		// increase feature frequency
		if(feat->featid == featid){
			feat->featfrq++;
			break;
		}
	}

	// new feature
	if(featnum == *filcnt){
		feat = featfrqs+featnum;

		feat->featid = featid;
		feat->featfrq = 1;
		(*filcnt)++;
	}

	return 0;
}

// insert the feature list to the doc-feature table (for Term-Doc sparse matrix)
int insertToDocFeatTable(DOC_FEAT_TABLE *table, int docid, FEATURE_FRQ *featfrqs, int featcnt)
{
	FEATURE_LIST *featlist = NULL;
	
	featlist = table->featlists + docid;
	featlist->featfrqs = featfrqs;
	featlist->count = featcnt;

	return 0;
}

// free memories allocated to a doc-feature table
void freeDocFeatTable(DOC_FEAT_TABLE *table)
{
	if(table){
		if(table->featlists){
			FEATURE_FRQ *pfeatfrq;
			int featlistnum;
			for(featlistnum=0; featlistnum<table->count; featlistnum++){
				pfeatfrq = table->featlists[featlistnum].featfrqs;
				if(pfeatfrq) free(pfeatfrq);
			}
			free(table->featlists);
		}
		free(table);
	}
}

static int _free_feattree(UCS2 *key, char *value, char *args[])
{
	if(value){
		free(value);
#if 0
		if(((FEATURE_TREE_VAL*)value)->words){
			free(((FEATURE_TREE_VAL*)value)->words);
		}
		free(value);
#endif
	}

	return 0;
}

static int _set_feattable(UCS2 *key, char *value, char *args[])
{
	FEATURE_TABLE *table;
	FEATURE *feats, *pfeat;
	FEATURE_TREE_VAL *featval;
	int featnum;
	UTF8 utf8_key[1024];
	int utf8_key_len;

	table = (FEATURE_TABLE*)args[0];
	feats = table->features;
	featval = (FEATURE_TREE_VAL*)value;
	featnum = featval->featid;
	pfeat = feats + featnum;

	ucs2_to_utf8(key, utf8_key);
	utf8_key_len = strlen((char*)utf8_key);

	if( (utf8_key_len > 0) && (pfeat->feature = malloc(sizeof(char)*(utf8_key_len+1))) ){
		// save to the structure
		strncpy(pfeat->feature, (char*)utf8_key, utf8_key_len+1);
		pfeat->words = featval->words;
		pfeat->docfrq = featval->docfrq;
		pfeat->qualify_flg = (pfeat->docfrq >= table->threshold_docfrq)? 1 : 0;
		pfeat->termids = NULL;
	}else{
		memset(pfeat, 0, sizeof(FEATURE));
	}

	return 0;
}
