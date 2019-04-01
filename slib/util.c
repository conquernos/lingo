#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "xten_user.h"
#include "util.h"
#include "strlib.h"

int getSectionVals(int dlocnum, int docid, char *secnames[], char *secvals[], char *secbuf, int seccnt, int secbuf_size)
{
	char *sp = NULL;
	int secidx = 0;
	int len = 0;

	if(xten_ReadDocMultSec(dlocnum, docid, seccnt, secnames, secbuf, secbuf_size) < 0) return -1;
	strchgchr(secbuf,'\n', ' ');

	sp = secbuf;
	for(secidx=0; secidx<seccnt ; secidx++){
		memcpy(&len, sp, sizeof(int));
		*sp='\n';
		*(sp+1)=0x00;
		sp += sizeof(int);
		secvals[secidx] = sp;
		sp += len + 1;
	}

	return 0;
}


float timer(void)
{
	long elapsed_time;
	struct rusage mytime;
	getrusage(RUSAGE_SELF,&mytime);

	/* convert elapsed time to milliseconds */
	elapsed_time = (mytime.ru_utime.tv_sec * 1000 + mytime.ru_utime.tv_usec / 1000);

	/* return elapsed time in seconds */
	return((float)elapsed_time/1000.0);
}

