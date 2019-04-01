#include <stdio.h>

#include "term_store.h"

static int _free_termtree(UCS2 *key, char *value, char *args[]);
static int _set_termtable(UCS2 *key, char *value, char *args[]);

// create a term tree
TERM_TREE * createTermTree()
{
	TERM_TREE *tree;

	if((tree = malloc(sizeof(TERM_TREE)))){
		tree->max_termid = -1;
		if(!(tree->tree = new_mtrie16())){
			free(tree); tree = NULL;
		}
	}

	return tree;
}

// insert a term to the term tree (for search)
int insertToTermTree(TERM_TREE *tree, UCS2 *term, int *termid)
{
	TERM_TREE_VAL *value;

	// already exist
	if((value = (TERM_TREE_VAL*)search_mtrie16(tree->tree, term))){
		*termid = value->termid;

	// newly insert
	}else{
		value = malloc(sizeof(TERM_TREE_VAL));
		*termid = value->termid = ++(tree->max_termid);

		if(insert_mtrie16(tree->tree, term, (char*)value, sizeof(value)) < 0){
			free(value);
			return -1;
		}
		croot_addr_mtrie16(tree->tree);
	}

	return 0;
}

// free memories allocated to the term tree
void freeTermTree(TERM_TREE *tree)
{
	int free_tree_value(char *value)
	{
		if(value) free(value);
		return 0;
	}

	if(tree){
		if(tree->tree){
			/*
			traverse_mtrie16(tree->tree, _free_termtree, NULL);
			free(tree->tree);
			*/
			free_mtrie16(tree->tree, free_tree_value);
		}
		free(tree);
	}
}

// create a term table
TERM_TABLE * createTermTable()
{
	TERM_TABLE *table;

	if((table = malloc(sizeof(TERM_TABLE)))){
		table->terms = NULL;
		table->count = 0;
	}

	return table;
}

// make the term table
int makeTermTable(TERM_TABLE *table, TERM_TREE *tree)
{
	int termcnt;

	termcnt = tree->max_termid+1;
	table->terms = malloc(sizeof(TERM_TABLE)*termcnt);
	memset(table->terms, 0, sizeof(TERM_TABLE)*termcnt);
	traverse_mtrie16(tree->tree, _set_termtable, (char**)&table);
	table->count = termcnt;

	return 0;
}

// free memories allocated to a term table
void freeTermTable(TERM_TABLE *table)
{
	int termnum;
	TERM *pterm;
	if(table){
		if(table->terms){
			for(termnum=0; termnum<table->count; termnum++){
				pterm = table->terms + termnum;
				//printf("%d:%d\n", termnum, (int)pterm->term);
				if(pterm->term) free(pterm->term);
			}
			free(table->terms);
		}
		free(table);
	}
}

static int _free_termtree(UCS2 *key, char *value, char *args[])
{
	if(value) free(value);

	return 0;
}

static int _set_termtable(UCS2 *key, char *value, char *args[])
{
	TERM_TABLE *table;
	TERM *terms, *pterm;
	TERM_TREE_VAL *termval;
	int termnum;
	UTF8 utf8_key[1024];
	int utf8_key_len;

	table = (TERM_TABLE*)args[0];
	terms = table->terms;
	termval = (TERM_TREE_VAL*)value;
	termnum = termval->termid;
	pterm = terms + termnum;

	ucs2_to_utf8(key, utf8_key);
	utf8_key_len = strlen((char*)utf8_key);
	if( (utf8_key_len > 0) && (pterm->term = malloc(sizeof(char)*(utf8_key_len+1))) ){
		// save to the structure
		strncpy(pterm->term, (char*)utf8_key, utf8_key_len+1);
		pterm->docfrq = 0;
	}else{
		memset(pterm, 0, sizeof(TERM));
	}

	return 0;
}
