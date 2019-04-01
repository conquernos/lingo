#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "docnum.h"
#include "util.h"
#include "xten_user.h"

#define MAX_DOCNUM_SIZE	1024

// create a doc number table
DOCNUM_TABLE * createDocNumTable(void)
{
	DOCNUM_TABLE *table;

	table = malloc(sizeof(DOCNUM_TABLE));
	if(table) memset(table, 0, sizeof(DOCNUM_TABLE));

	return table;
}

// make the doc number table using the section of the scd file
int makeDocNumTable(DOCNUM_TABLE *table, int dlocnum, char *docnum_secname, int *docids, int doccnt)
{
	// section
	DOCNUM *pdocnum;
	char *secvals[1];
	int docnum, secnum;
	char *secbuf;
	int docid;

	table->docnums = malloc(sizeof(DOCNUM)*doccnt);
	secbuf = malloc(MAX_DOCNUM_SIZE+1);
	secnum = 1;

	for(docnum=0; docnum<doccnt; docnum++){
		docid = docids[docnum];
		pdocnum = table->docnums + docnum;

		// get a eigen number corresponding to the doc id
		if(docnum_secname){
			if(getSectionVals(dlocnum, docid, &docnum_secname, secvals, secbuf, 1, MAX_DOCNUM_SIZE) < 0){
				return -1;
			}
		}

		// save to the structure
		pdocnum->docid = docnum;
		pdocnum->org_docid = docid;
		if(docnum_secname && (secvals[0][0] != '\0')) pdocnum->eigennum = atoi(secvals[0]);
		else pdocnum->eigennum = docid;
	}
	table->count = doccnt;

	free(secbuf);

	return 0;
}

// free memories allocated to the doc number table
void freeDocNumTable(DOCNUM_TABLE *table)
{
	if(table){
		if(table->docnums){
			free(table->docnums);
		}
		free(table);
	}
}
