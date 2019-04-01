#ifndef _UTIL_H
#define _UTIL_H

#ifndef MAX
#define MAX(X,Y)	((X)>(Y))? (X):(Y)
#endif

#ifndef MIN
#define MIN(X,Y)	(((X) < (Y))? (X) : (Y))
#endif

int getSectionVals(int dlocnum, int docid, char *secnames[], char *secvals[], char *secbuf, int seccnt, int secbuf_size);
float timer(void);

#endif
