#include <stdio.h>
#include <math.h>

#include "cossim.h"

double calcPhraseTermCosSim(PHRASE *phrases, TERM *terms, int phraseid, int termid, int doccnt, enum weight_type type)
{
	int *termids;
	int termcnt;
	int termnum;
	double denominator = 0.0, numerator = 0.0, sim = 0.0;

	termids = phrases[phraseid].termids;
	termcnt = phrases[phraseid].termcnt;

	// AㆍB
	for(termnum=0; termnum<termcnt; termnum++){
		if(termids[termnum] == termid){
			if(terms[termid].docfrq == 0) numerator = 0.0;
			else {
				switch(type){
				case TF_IDF: numerator = log(doccnt/terms[termid].docfrq)+0.01; break;
//				case TF_IDF: numerator = pow(log(doccnt/terms[termid].docfrq), 2); break;
				case DF: numerator = terms[termid].docfrq; break;
//				case DF: numerator = pow(terms[termid].docfrq, 2); break;
				case TF: numerator = 1;
				}
			}
			break;
		}
	}

    // |A|ㆍ|B|
    if(numerator > 0.0){
        for(termnum=0; termnum<termcnt; termnum++){
			switch(type){
			case TF_IDF: denominator += pow(log(doccnt/terms[termids[termnum]].docfrq), 2); break;
			case DF: denominator += pow(terms[termids[termnum]].docfrq, 2); break;
			case TF: denominator += 1; break;
			}
        }
        denominator = sqrt(denominator);
        //denominator += fabs(log(doccnt/terms[termid].docfrq));
        if(denominator > 0.0) sim = numerator/denominator;
    }

	return sim;
}

double calcPhraseCosSim(PHRASE *phrases, TERM *terms, int phraseid1, int phraseid2, int doccnt, int *samecnt, enum weight_type type)
{
	int *termids1, *termids2;
	int termcnt1, termcnt2;
	int termnum1, termnum2;
	double denominator = 0.0, denominator1 = 0.0, denominator2 = 0.0, numerator = 0.0, sim = 0.0;

	termids1 = phrases[phraseid1].termids;
	termcnt1 = phrases[phraseid1].termcnt;
	termids2 = phrases[phraseid2].termids;
	termcnt2 = phrases[phraseid2].termcnt;
	*samecnt = 0;

	switch(type){
		case TF_IDF:
			// AㆍB
			for(termnum1=0; termnum1<termcnt1; termnum1++){
				for(termnum2=0; termnum2<termcnt2; termnum2++){
					if(termids1[termnum1] == termids2[termnum2]){
						numerator += pow(log(doccnt/terms[termids1[termnum1]].docfrq)+0.01, 2);
						(*samecnt)++;
					}
				}
			}

			// |A|ㆍ|B|
			if(numerator > 0.0){
				for(termnum1=0; termnum1<termcnt1; termnum1++){
					denominator1 += pow(log(doccnt/terms[termids1[termnum1]].docfrq+0.01), 2);
				}
				for(termnum2=0; termnum2<termcnt2; termnum2++){
					denominator2 += pow(log(doccnt/terms[termids2[termnum2]].docfrq+0.01), 2);
				}
				denominator = sqrt(denominator1*denominator2);
				if(denominator > 0.0) sim = numerator/denominator;
			}
			break;
		case DF:
			// AㆍB
			for(termnum1=0; termnum1<termcnt1; termnum1++){
				for(termnum2=0; termnum2<termcnt2; termnum2++){
					if(termids1[termnum1] == termids2[termnum2]){
						numerator += pow(terms[termids1[termnum1]].docfrq, 2);
						(*samecnt)++;
					}
				}
			}

			// |A|ㆍ|B|
			if(numerator > 0.0){
				for(termnum1=0; termnum1<termcnt1; termnum1++){
					denominator1 += pow(terms[termids1[termnum1]].docfrq, 2);
				}
				for(termnum2=0; termnum2<termcnt2; termnum2++){
					denominator2 += pow(terms[termids2[termnum2]].docfrq, 2);
				}
				denominator = sqrt(denominator1*denominator2);
				if(denominator > 0.0) sim = numerator/denominator;
			}
			break;
		case TF:
			// AㆍB
			for(termnum1=0; termnum1<termcnt1; termnum1++){
				for(termnum2=0; termnum2<termcnt2; termnum2++){
					if(termids1[termnum1] == termids2[termnum2]){
						numerator += 1;
						(*samecnt)++;
					}
				}
			}

			// |A|ㆍ|B|
			if(numerator > 0.0){
				for(termnum1=0; termnum1<termcnt1; termnum1++){
					denominator1 += 1;
				}
				for(termnum2=0; termnum2<termcnt2; termnum2++){
					denominator2 += 1;
				}
				denominator = sqrt(denominator1*denominator2);
				if(denominator > 0.0) sim = numerator/denominator;
			}
			break;
	}

	return sim;
}
