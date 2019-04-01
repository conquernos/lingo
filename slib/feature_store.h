#ifndef _FEATURE_STORE_H
#define _FEATURE_STORE_H

#include "libtrie.h"
#include "libucs.h"

/* Trie for searching a feature
   Feature -> FeatureID */
typedef struct _FEATURE_TREE{
	mTRIE16 *tree;	// key : UCS2 *feature
	int max_featid;
} FEATURE_TREE;

typedef struct _FEATURE_TREE_VAL{
	char *words;
	int featid;
	int docfrq;
	int last_docid; // for calculating the doc frequency
} FEATURE_TREE_VAL;

/* Feature table 
   FeatureID -> Feature */
typedef struct _FEATURE{
	char *feature;		// "한국 최초로 노벨상을 수상"
	int *termids;
	int docfrq;
	int termcnt;

	char *words;		// "한국 최초 노벨상 수상"
	int weight;
	char qualify_flg;	// 1(yes) or 0(no)
} FEATURE;

typedef struct _FEATURE_TABLE{
	FEATURE *features;
	int threshold_docfrq;
	int count;
} FEATURE_TABLE;

/* DocID -> FeatureID List */
typedef struct _FEATURE_FRQ{
	int featid;
	int featfrq;
} FEATURE_FRQ;

typedef struct _FEATURE_LIST{
	FEATURE_FRQ *featfrqs;
	int count;
} FEATURE_LIST;

typedef struct _DOC_FEAT_TABLE{
	FEATURE_LIST *featlists;
	int count;
} DOC_FEAT_TABLE;


/* feature tree */
FEATURE_TREE * createFeatureTree();
int insertToFeatureTree(FEATURE_TREE *tree, UCS2 *feature, char *words, int docid, int *featid);
void freeFeatureTree(FEATURE_TREE *tree);

/* feature table */
FEATURE_TABLE * createFeatureTable(int docfrq);
int makeFeatureTable(FEATURE_TABLE *table, FEATURE_TREE *tree);
int setTermidOfFeatureTable(FEATURE_TABLE *table, int featid, int *termids, int count);
void freeFeatureTable(FEATURE_TABLE *table);

/* doc-feature table */
DOC_FEAT_TABLE * createDocFeatTable(int doccnt);
int addInFeatureList(FEATURE_FRQ *featfrqs, int featid, int *filcnt);
int insertToDocFeatTable(DOC_FEAT_TABLE *table, int docid, FEATURE_FRQ *featfrqs, int featcnt);
void freeDocFeatTable(DOC_FEAT_TABLE *table);

#endif
