#ifndef _DOCNUM_H
#define _DOCNUM_H

typedef struct _DOCNUM{
	int docid;		// internal doc id
	int org_docid;	// original doc id
	int eigennum;	// doc eigen number
} DOCNUM;

typedef struct _DOCNUM_TABLE{
	DOCNUM *docnums;
	int count;
} DOCNUM_TABLE;

/* doc number table */
DOCNUM_TABLE * createDocNumTable(void);
int makeDocNumTable(DOCNUM_TABLE *table, int dlocnum, char *docnum_secname, int *docids, int doccnt);
void freeDocNumTable(DOCNUM_TABLE *table);

#endif
