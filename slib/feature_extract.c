/* Library for the Feature Extrtaction
   It use the korean morphological analysis to extract features 

   1. preprocessing
     - remove from the input documents all characters and terms that can possibly affect the quality of group descriptions.
     - check stop words

   2. necessary condition of the feature
     - Document Frequency > specified number
     - not cross sentence boundaries
     - be a complete phrase
     - not begin nor end with a stop word
     - not be in brackets

   3. removing words
     - number + noun
	 - pronoun
	 - ~ 등
	 - human name
 */

#include <stdio.h>
#include <stdlib.h>

#include "feature_extract.h"
#include "nlplib.h"
#include "nlpsys/libucs.h"
#include "strlib.h"

// create a feature morpheme table
FEATURE_MORPHEME_TABLE * createFeatureTextList(int maxcnt)
{
	FEATURE_MORPHEME_TABLE *featlist;

	featlist = malloc(sizeof(FEATURE_MORPHEME_TABLE));
	if(maxcnt > 0){
		featlist->feattexts = malloc(sizeof(FEATURE_MORPHEME)*(maxcnt));
	}else{
		featlist->feattexts = NULL;
	}
	featlist->count = 0;
	featlist->maxcnt = maxcnt;
	
	return featlist;
}

// extract morpheme numbers(begin~end) of features from a document
int extractFeatureMorph(NLPDOC *nlp, FEATURE_MORPHEME_TABLE *featlist, enum allowed_morph_type type)
{
	// morpheme
	char *allowed_morph;
	char *allowed_morph_variety = 
		" nng nnp nnb nnk nnK ne np nf sn nr eng chn unk mm mag mar ma+ maj jks jkg jkc jko jkb jkx jc jx xsn sp so ";
	char *allowed_morph_noun  = " nng nnp nnb nnk nnK ne np nf sn nr eng chn unk sp sw ";
	char *begin_morph = " nng nnp nnk nnK ne nr sn eng chn unk ss ";
	char *end_morph = " nng nnp nnb nnk ne np nr nf sn sw ";
	char *solo_morph = " nnp nnk ne ";
	MORPH_STRUCT *morpheme = NULL;
	SENTENCE_STRUCT *sentence;
	struct sentence *sents, *psent;
	struct morph *morps;
	char *pos;
	int sentid;

	// feature
	FEATURE_MORPHEME *feattexts;

	// index number
	int sentnum, morpnum, featnum;

	// morpheme number
	int feat_bom, feat_eom;

	// initialization
	if(!(morpheme = &nlp->Morph)) return -1;
	if(!(morps = morpheme->morphs)) return -1;
	if(!(sentence = &nlp->Sentence)) return -1;
	if(!(sents = sentence->sentences)) return -1;
	featlist->count = 0;
	feattexts = featlist->feattexts;
	if(type == VARIETY) allowed_morph = allowed_morph_variety;
	else allowed_morph = allowed_morph_noun;

	for(sentnum=0, featnum=0; sentnum<sentence->count; sentnum++){	// loop for sentences
		psent = sents+sentnum;

		// child sentence
		if(psent->mother_number != -1) continue;

		feat_bom = feat_eom = -1;
		for(morpnum=psent->bom; morpnum<=psent->eom; morpnum++){	// loop for morphemes of a sentence
			if(featnum >= featlist->maxcnt) break;

			sentid = morps[morpnum].sentence_number;
			pos = morps[morpnum].pos;

			// check whether or not it is a noun
			if( ( is_exist(allowed_morph, ' ', pos) || 
				  ((strcmp(pos, "ss")==0) && (is_exist(" ' ` ", ' ', morps[morpnum].utf8))) ||
				  ((strcmp(pos, "sw")==0) && (is_exist(" & ", ' ', morps[morpnum].utf8))) ) &&
				!(sents[sentid].type & SENTENCE_IN_BRACK) ){	// except for the bracket sentence
				// end morpheme
				if(feat_bom > -1){
					if(is_exist(end_morph, ' ', pos)){
						feat_eom = morpnum;
					}
				// begin morpheme
				}else if(is_exist(begin_morph, ' ', pos)){
					feat_bom = morpnum;
					// for the feature with only one morpheme
					if(is_exist(solo_morph, ' ', pos)){
						feat_eom = morpnum;
					}
				}

				// finded a feature
				if(morpnum == psent->eom){
					if( (feat_eom > -1) && (feat_eom-feat_bom < 15) &&
						(strcmp((char*)morps[feat_eom].utf8, "등")) ){
						feattexts[featnum].bom = feat_bom;
						feattexts[featnum].eom = feat_eom;
						featnum++;

						feat_bom = feat_eom = -1;
					}
				}
			}else{
				// finded a feature
				if( (feat_eom > -1) && (feat_eom-feat_bom < 15) && 
					(strcmp((char*)morps[feat_eom].utf8, "등")) ){
					feattexts[featnum].bom = feat_bom;
					feattexts[featnum].eom = feat_eom;
					featnum++;
				}

				feat_bom = feat_eom = -1;
			}
		}
	}

	featlist->count = featnum;

	return 0;
}

void freeFeatureTextList(FEATURE_MORPHEME_TABLE *featlist)
{
	if(featlist){
		if(featlist->feattexts){
			free(featlist->feattexts);
		}
		free(featlist);
	}
}
