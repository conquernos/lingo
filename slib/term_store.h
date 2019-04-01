#ifndef _TERM_STORE_H
#define _TERM_STORE_H

#include "libtrie.h"
#include "libucs.h"

/* Trie for searching a feature
   Term -> TermID */
typedef struct _TERM_TREE{
	mTRIE16 *tree;	// key : UCS2 *term
	int max_termid;
} TERM_TREE;

typedef struct _TERM_TREE_VAL{
	int termid;
} TERM_TREE_VAL;

/* Term table
   TermID -> Term */
typedef struct _TERM{
	char *term;
	int docfrq;
} TERM;

typedef struct _TERM_TABLE{
	TERM *terms;
	int count;
} TERM_TABLE;


/* term tree */
TERM_TREE * createTermTree();
int insertToTermTree(TERM_TREE *termtree, UCS2 *term, int *termid);
void freeTermTree(TERM_TREE *termtree);

/* term table */
TERM_TABLE * createTermTable();
int makeTermTable(TERM_TABLE *table, TERM_TREE *tree);
void freeTermTable(TERM_TABLE *table);

#endif
