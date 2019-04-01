#include <stdio.h>

#include "label_select.h"
#include "cossim.h"

// prune labels
int pruneLabel(LABEL_TABLE *labeltable, PHRASE *phrases, TERM *terms, int doccnt, double sim_threshold, double score_threshold, enum weight_type type)
{
	LABEL *labels, *plabel1, *plabel2;
	int labelnum1, labelnum2;
	int labelid1, labelid2;
	double sim = 0.0;
	int samecnt = 0;

	labels = labeltable->labels;

	for(labelnum1=0; labelnum1<labeltable->count; labelnum1++){
		plabel1 = labels + labelnum1;
		labelid1 = plabel1->ptid;

		if(plabel1->score < score_threshold){ plabel1->pruned = 2; continue; }

		for(labelnum2=labelnum1+1; labelnum2<labeltable->count; labelnum2++){
			plabel2 = labels + labelnum2;
			labelid2 = plabel2->ptid;

			// calcurate the similarity between two labels
			if(plabel1->phrase_term == ISPHRASE){
				if(plabel2->phrase_term == ISPHRASE){
					// phrase-phrase
					sim = calcPhraseCosSim(phrases, terms, labelid1, labelid2, doccnt, &samecnt, type);
				}else if(plabel2->phrase_term == ISTERM){
					// phrase-term
					sim = calcPhraseTermCosSim(phrases, terms, labelid1, labelid2, doccnt, type);
				}
			}else if(plabel1->phrase_term == ISTERM){
					// term-phrase
				if(plabel2->phrase_term == ISPHRASE){
					sim = calcPhraseTermCosSim(phrases, terms, labelid2, labelid1, doccnt, type);
					// term-term
				}else if(plabel2->phrase_term == ISTERM){
					if(labelid1 == labelid2) sim = 1.0;
					else sim = 0.0;
				}
			}

			// label pruning (score, term count)
			if((sim >= sim_threshold) && (samecnt > 1)){
				labeltable->pruned_cnt++;
				if(plabel1->score > plabel2->score) plabel2->pruned = 1;
				else if(plabel1->score < plabel2->score) plabel1->pruned = 1;
				else if(plabel1->phrase_term == ISTERM) plabel1->pruned = 1;
				else if(plabel2->phrase_term == ISTERM) plabel2->pruned = 1;
				else if(plabel1->term_cnt < plabel2->term_cnt) plabel2->pruned = 1;
				else if(plabel1->term_cnt > plabel2->term_cnt) plabel1->pruned = 1;
				else plabel2->pruned = 1;
			}
		}
	}

	return 0;
}
