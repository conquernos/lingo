#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "morpheme.h"
#include "strlib.h"
#include "linkedlist.h"

#define MORPH_UCS2_LEN(MORPH, UCS2_LEN, BOM, EOM) \
		{ \
			int mnum; \
			for(mnum=(BOM); mnum<=(EOM); mnum++){ \
				*(UCS2_LEN) += (MORPH)->morphs[mnum].length; \
			} \
		}

#define MORPH_UCS2_STRCPY(MORPH, UCS2_STR, BOM, EOM) \
		{ \
			int mnum; \
			(UCS2_STR)[0] = 0; \
			for(mnum=(BOM); mnum<=(EOM); mnum++){ \
				ucs2_strcat((UCS2_STR), (MORPH)->morphs[mnum].text); \
			} \
		}

static int rejectVerb(MORPH_STRUCT *morph, int morph_idx)
{
	int i;
	char *reject[]={
		"대하",
		"의하",
		NULL
	};

	if(!morph || (morph_idx < 0) || morph_idx >= morph->count) return -1;

	for(i=0; reject[i]; i++){
		if(!strcmp(reject[i], (char*)morph->morphs[morph_idx].utf8)) return 1;
	} 

	return 0;
}

/* bom에서 시작하는 명사 추출 (명사구 처리x ) */
UCS2 *getNoun(NLPDOC *nlp, int bom)
{
	UCS2 *noun = NULL;

	TOKEN_STRUCT *token;
	MORPH_STRUCT *morph;
	PHRASE_STRUCT *phrase;

	int ucs2_len = 0;
	int eom, pnum, tnum;
	char *pos = NULL;

	token = &nlp->Token;
	morph= &nlp->Morph;
	phrase = &nlp->Phrase;

	if(!nlp ||(bom < 0)) return NULL;

	// use the pos of the phrase
	pnum = morph->morphs[bom].phrase_number;
	if(pnum != -1){
		pos = phrase->phrases[pnum].pos;
		bom = phrase->phrases[pnum].bom;
		eom = phrase->phrases[pnum].eom;
	}else{
		pos = morph->morphs[bom].pos;
		eom = bom;
	}

	// get the noun
	if(is_exist(" nng nnp nnk nnK eng chn jpn sn ne nsp unk ", ' ', pos)){
		/*
		for(pnum=0; pnum<phrase->count; pnum++){
			if( (phrase->phrases[pnum].bom <= bom) && (phrase->phrases[pnum].eom >= bom) ){
				if(is_exist(" ncp ne nnk ", ' ', phrase->phrases[pnum].pos)){
					bom = phrase->phrases[pnum].bom;
					pos = phrase->phrases[pnum].pos;
					eom = phrase->phrases[pnum].eom;
					break;
				}
			}
		}
		*/

		if(is_exist(" sn ", ' ', pos)){
			tnum = morph->morphs[bom].token_number;
			if(token->tokens[tnum].type ==  TOKEN_DGT) return NULL;
		}
		/*
		if(!is_exist(" nng nnp nnk eng chn jpn sn ne ncp unk ", ' ', pos)){
			return NULL;
		}
		*/

		MORPH_UCS2_LEN(morph, &ucs2_len, bom, eom);
		noun = malloc(sizeof(UCS2)*(ucs2_len+1));
		MORPH_UCS2_STRCPY(morph, noun, bom, eom);
	}

	return noun;
}

/* bom에서 시작하는 명사 추출 (명사구는 복합명사로 추출) */
UCS2 *getCompNoun(NLPDOC *nlp, int bom)
{
	UCS2 *noun = NULL;

	TOKEN_STRUCT *token;
	MORPH_STRUCT *morph;
	PHRASE_STRUCT *phrase;

	int ucs2_len = 0;
	int eom, pnum, tnum;
	char *pos = NULL;

	token = &nlp->Token;
	morph= &nlp->Morph;
	phrase = &nlp->Phrase;

	if(!nlp ||(bom < 0)) return NULL;

	// use the pos of the phrase 
	pnum = morph->morphs[bom].phrase_number;
	if(pnum != -1){
		pos = phrase->phrases[pnum].pos;
		bom = phrase->phrases[pnum].bom;
		eom = phrase->phrases[pnum].eom;
	}else{
		pos = morph->morphs[bom].pos;
		eom = bom;
	}

	// get the noun
	if(is_exist(" nng nnp nnk nnK eng chn jpn sn ne ncp nsp unk ", ' ', pos)){
		if(is_exist(" sn ", ' ', pos)){
			tnum = morph->morphs[bom].token_number;
			if(token->tokens[tnum].type ==  TOKEN_DGT) return NULL;
		}

		MORPH_UCS2_LEN(morph, &ucs2_len, bom, eom);
		noun = malloc(sizeof(UCS2)*(ucs2_len+1));
		MORPH_UCS2_STRCPY(morph, noun, bom, eom);
	}

	return noun;
}

UCS2 *getVerb(NLPDOC *nlp, int bom)
{
	MORPH_STRUCT *morph = NULL;
	PHRASE_STRUCT *phrase = NULL;
	UCS2 *verb = NULL;
	int verb_len = 0;
	int i=0, j=0;

	char *pos, *pos2;
	int eom, eom2;

	if(!nlp || (bom < 0) || (bom > nlp->Morph.count)) return NULL;

	morph = &nlp->Morph;
	phrase = &nlp->Phrase;

	for(i=bom; i<morph->count; i++) {
		if(morph->morphs[i].chunk_number!= -1){
			pos = phrase->phrases[morph->morphs[i].chunk_number].pos;
			eom = phrase->phrases[morph->morphs[i].chunk_number].eom;
		} else {
			pos = morph->morphs[i].pos;
			eom = i;
		}

		// 서술어
		if(is_exist(" vv va vf ", ' ', pos)){
			for(j=eom+1; j<morph->count; j++){
				if(morph->morphs[j].chunk_number!= -1){
					pos2 = phrase->phrases[morph->morphs[j].chunk_number].pos;
					eom2 = phrase->phrases[morph->morphs[j].chunk_number].eom;
				} else {
					pos2 = morph->morphs[j].pos;
					eom2 = j;
				}
				if(is_exist(" vx vxp vxn ", ' ', pos2)){
					eom=eom2; continue;
				}
				break;
			}
			for(j=i, verb_len=2; j<=eom; j++){
				verb_len += morph->morphs[j].length;
			}
			verb = (UCS2*)malloc(sizeof(UCS2) * verb_len);
			verb[0]=0;
			for(j=i; j<=eom; j++){
				ucs2_strcat(verb, morph->morphs[j].text);
			}
			verb[verb_len-2]=0xb2e4;
			verb[verb_len-1]=0;
			return verb;
		}

		// 계사
		if(is_exist(" nng nnp nnb nnk nnK nsp ncp ne np nr nf ", ' ',  pos)){
			for(j=i, verb_len=2; j<=eom; j++){
				verb_len += morph->morphs[j].length;
			}

			if(morph->morphs[eom+1].chunk_number!= -1){
				pos = phrase->phrases[morph->morphs[eom+1].chunk_number].pos;
			} else {
				pos = morph->morphs[eom+1].pos;
			}

			if(is_exist(" vcp vxp ", ' ', pos)){
				verb_len +=1;
				verb = (UCS2*)malloc(sizeof(UCS2) * verb_len);
				verb[0]=0;
				for(j=i; j<=eom; j++){
					ucs2_strcat(verb, morph->morphs[j].text);
				}
				verb[verb_len-3]=0xc774;
				verb[verb_len-2]=0xb2e4;
				verb[verb_len-1]=0;
				return verb;
			} else if(is_exist(" vcn vxn ", ' ', pos)){
				verb_len += 2;
				verb = (UCS2*)malloc(sizeof(UCS2) * verb_len);
				verb[0]=0;
				for(j=i; j<=eom; j++){
					ucs2_strcat(verb, morph->morphs[j].text);
				}
				verb[verb_len-4]=0xc774;
				verb[verb_len-3]=0xb2e4;
				verb[verb_len-2]='X';
				verb[verb_len-1]=0;
				return verb;

			} else if(strstr(" ec ef ", pos)) {
				verb_len += 1;
				verb = (UCS2*)malloc(sizeof(UCS2) * verb_len);
				verb[0]=0;
				for(j=i; j<=eom; j++){
					ucs2_strcat(verb, morph->morphs[j].text);
				}
				verb[verb_len-3]=0xc774;
				verb[verb_len-2]=0xb2e4;
				verb[verb_len-1]=0;
				return verb;
			}
		}
	}

	return NULL;
}

UCS2 *getAdverb(NLPDOC *nlp, int bom)
{
	UCS2 *adverb = NULL;

	TOKEN_STRUCT *token;
	MORPH_STRUCT *morph;
	PHRASE_STRUCT *phrase;

	int ucs2_len = 0;
	int mnum, eom, cnum, cnum2, bom2, eom2;
	char *pos = NULL, *pos2 = NULL;

	if(!nlp || (bom < 0) || (bom > nlp->Morph.count)) return NULL;

	token = &nlp->Token;
	morph= &nlp->Morph;
	phrase = &nlp->Phrase;

	// use the pos of the chunk
	cnum = morph->morphs[bom].chunk_number;
	if(cnum != -1){
		pos = phrase->phrases[cnum].pos;
		eom = phrase->phrases[cnum].eom;
	}else{
		pos = morph->morphs[bom].pos;
		eom = bom;
	}

	// get the noun
	if(strncmp(pos, "ma", 2) == 0){
		for(mnum=bom; mnum<=eom; mnum++){
			ucs2_len += morph->morphs[mnum].length;
		}
	}else{
		cnum2 = morph->morphs[eom+1].chunk_number;
		if(cnum2 != -1){
			pos2 = phrase->phrases[cnum2].pos;
			bom2 = phrase->phrases[cnum2].bom;
			eom2 = phrase->phrases[cnum2].eom;
		}else{
			pos2 = morph->morphs[eom+1].pos;
			bom2 = eom+1;
			eom2 = eom+1;
		}

		if((strncmp(pos2, "ec", 2) == 0) && strstr(" 게 도록 ", morph->morphs[eom2].utf8)){
			eom = eom2;
			for(mnum=bom; mnum<=eom; mnum++){
				ucs2_len += morph->morphs[mnum].length;
			}
		}
	}

	if(ucs2_len > 0){
		adverb = malloc(sizeof(UCS2)*(ucs2_len+1));
		adverb[0] = 0;

		for(mnum=bom; mnum<=eom; mnum++){
			ucs2_strcat(adverb, morph->morphs[mnum].text);
		}
	}

	return adverb;
}

UCS2 *getWord(NLPDOC *nlp, int bom)
{
	UCS2 *word = NULL;

	TOKEN_STRUCT *token;
	MORPH_STRUCT *morph;
	PHRASE_STRUCT *phrase;

	int ucs2_len = 0;
	int eom;
	int chunknum = 0;
	char *pos = NULL;

	if(!nlp || (bom < 0) || (bom > nlp->Morph.count)) return NULL;

	token = &nlp->Token;
	morph= &nlp->Morph;
	phrase = &nlp->Phrase;

	// use the pos of the chunk
	chunknum = morph->morphs[bom].chunk_number;
	if(chunknum != -1){
		pos = phrase->phrases[chunknum].pos;
		eom = phrase->phrases[chunknum].eom;
	}else{
		pos = morph->morphs[bom].pos;
		eom = bom;
	}

	int m;
	for(m=bom; m<=eom; m++){
		ucs2_len += morph->morphs[m].length;
	}

	word = malloc(sizeof(UCS2)*(ucs2_len+1));
	word[0] = 0;

	for(m=bom; m<=eom; m++){
		ucs2_strcat(word, morph->morphs[m].text);
	}

	return word;
}

// get a noun list
MORPHWORD_LIST *getNounlist(NLPDOC *nlp)
{
	MORPHWORD_LIST *nounlist = NULL;

	TOKEN_STRUCT *token;
	MORPH_STRUCT *morph;
	PHRASE_STRUCT *phrase;

	int noun_cnt = 0;
	int ucs2_len = 0;
	int bom, eom, mnum, pnum, tnum;
	char *pos = NULL;

	token = &nlp->Token;
	morph= &nlp->Morph;
	phrase = &nlp->Phrase;

	if(!nlp) return NULL;

	nounlist = (MORPHWORD_LIST*)malloc(sizeof(MORPHWORD_LIST));
	memset(nounlist, 0, sizeof(MORPHWORD_LIST));

	for(mnum=0; mnum<morph->count; mnum++){
		bom = -1;

		// use the pos of the chunk
		pnum = morph->morphs[mnum].chunk_number;
		if(pnum!= -1){
			pos = phrase->phrases[pnum].pos;
			bom = phrase->phrases[pnum].bom;
			eom = phrase->phrases[pnum].eom;
		}else{
			pos = morph->morphs[mnum].pos;
			bom = mnum;
			eom = mnum;
		}

		// get the noun
		if(is_exist(" nng nnp nnk nnK eng chn jpn sn ne nsp unk ", ' ', pos)){
			/*
			for(pnum=0; pnum<phrase->count; pnum++){
				if(phrase->phrases[pnum].bom == mnum){
					if(is_exist(" ncp ne nnk ", ' ', phrase->phrases[pnum].pos)){
						pos = phrase->phrases[pnum].pos;
						eom = phrase->phrases[pnum].eom;
						break;
					}
				}
			}
			*/
			/*
			for(mnum2=eom+1; mnum2<morph->count; mnum2++){
				if(is_exist(" nng nnp nnk nnK eng chn jpn sn ne ncp unk ", ' ', morph->morphs[mnum2].pos)){
					eom = mnum2;
					continue;
				}else{
					break;
				}
			}
			*/

			if(is_exist(" sn ", ' ', pos)){
				tnum = morph->morphs[mnum].token_number;
				if(token->tokens[tnum].type ==  TOKEN_DGT) bom = -1;
			}
			//if(!is_exist(" nng nnp nnk eng chn sn ne ncp unk ", ' ', pos)) bom = -1;

			// 명사 발견시
			if(bom > -1){
				for(mnum=bom; mnum<=eom; mnum++){
					ucs2_len += morph->morphs[mnum].length;
				}

				nounlist->list = (MORPHWORD*)realloc(nounlist->list, sizeof(MORPHWORD)*(++noun_cnt));
				nounlist->list[noun_cnt-1].word = malloc(sizeof(UCS2)*(ucs2_len+1));
				nounlist->list[noun_cnt-1].word[0] = 0;
				nounlist->list[noun_cnt-1].bom = bom;
				nounlist->list[noun_cnt-1].eom = eom;

				for(mnum=bom; mnum<=eom; mnum++){
					ucs2_strcat(nounlist->list[noun_cnt-1].word, morph->morphs[mnum].text);
				}
				/*
				ucs2_print(nounlist->list[noun_cnt-1].word);
				printf("\n");
				*/
			}
			mnum = eom;
		}
	}
	nounlist->count = noun_cnt;

	return nounlist;
}

// get a noun list
MORPHWORD_LIST *getCompNounlist(NLPDOC *nlp)
{
	MORPHWORD_LIST *nounlist = NULL;

	TOKEN_STRUCT *token;
	MORPH_STRUCT *morph;
	PHRASE_STRUCT *phrase;

	int noun_cnt = 0;
	int ucs2_len = 0;
	int bom, eom, mnum, pnum, tnum;
	char *pos = NULL;

	token = &nlp->Token;
	morph= &nlp->Morph;
	phrase = &nlp->Phrase;

	if(!nlp) return NULL;

	nounlist = (MORPHWORD_LIST*)malloc(sizeof(MORPHWORD_LIST));
	memset(nounlist, 0, sizeof(MORPHWORD_LIST));

	for(mnum=0; mnum<morph->count; mnum++){
		bom = -1;

		// use the pos of the phrase
		pnum = morph->morphs[mnum].phrase_number;
		if(pnum!= -1){
			pos = phrase->phrases[pnum].pos;
			bom = phrase->phrases[pnum].bom;
			eom = phrase->phrases[pnum].eom;
		}else{
			pos = morph->morphs[mnum].pos;
			bom = mnum;
			eom = mnum;
		}

		// get the noun
		if(is_exist(" nng nnp nnk nnK eng chn jpn sn ne ncp nsp unk ", ' ', pos)){
			if(is_exist(" sn ", ' ', pos)){
				tnum = morph->morphs[mnum].token_number;
				if(token->tokens[tnum].type ==  TOKEN_DGT) bom = -1;
			}

			// 명사 발견시
			if(bom > -1){
				for(mnum=bom; mnum<=eom; mnum++){
					ucs2_len += morph->morphs[mnum].length;
				}

				nounlist->list = (MORPHWORD*)realloc(nounlist->list, sizeof(MORPHWORD)*(++noun_cnt));
				nounlist->list[noun_cnt-1].word = malloc(sizeof(UCS2)*(ucs2_len+1));
				nounlist->list[noun_cnt-1].word[0] = 0;
				nounlist->list[noun_cnt-1].bom = bom;
				nounlist->list[noun_cnt-1].eom = eom;

				for(mnum=bom; mnum<=eom; mnum++){
					ucs2_strcat(nounlist->list[noun_cnt-1].word, morph->morphs[mnum].text);
				}
				/*
				ucs2_print(nounlist->list[noun_cnt-1].word);
				printf("\n");
				*/
			}
			mnum = eom;
		}
	}
	nounlist->count = noun_cnt;

	return nounlist;
}


// get a verb list
MORPHWORD_LIST *getVerblist(NLPDOC *nlp)
{
	MORPHWORD_LIST *verblist = NULL;

	TOKEN_STRUCT *token;
	MORPH_STRUCT *morph;
	PHRASE_STRUCT *phrase;

	int verb_cnt = 0;
	int ucs2_len = 0;
	int bom, eom, mnum;
	int chunknum = 0;
	char *pos = NULL;
 
	if(!nlp) return NULL;

	token = &nlp->Token;
	morph= &nlp->Morph;
	phrase = &nlp->Phrase;

	verblist = (MORPHWORD_LIST*)malloc(sizeof(MORPHWORD_LIST));
	memset(verblist, 0, sizeof(MORPHWORD_LIST));

	for(mnum=0; mnum<morph->count; mnum++){
		bom = -1;

		// use the pos of the chunk
		chunknum = morph->morphs[mnum].chunk_number;
		if(chunknum != -1){
			pos = phrase->phrases[chunknum].pos;
			eom = phrase->phrases[chunknum].eom;
		}else{
			pos = morph->morphs[mnum].pos;
			eom = mnum;
		}

		// 동사 or 형용사
		if(is_exist(" vv va ", ' ', pos)){
			if(!rejectVerb(morph, mnum)){
				bom = mnum;
			}

		// 명사의 동사형 (명사 + 이다)
		}else if(is_exist(" nng nnp nnb nnk nnK nsp ncp ne np nr nf ", ' ', pos)){
			// see the chunk of the next morpheme
			chunknum = morph->morphs[eom+1].chunk_number;
			if(chunknum != -1){
				pos = phrase->phrases[chunknum].pos;
				eom = phrase->phrases[chunknum].eom;
			} else {
				pos = morph->morphs[eom+1].pos;
				eom = mnum;
			}

			if(is_exist(" vcp vcn ec ep ef vxn vxp ", ' ', pos)){
				bom = mnum;
			}
		}

		// 동사 발견시
		if(bom > -1){
			for(mnum=bom; mnum<=eom; mnum++){
				ucs2_len += morph->morphs[mnum].length;
			}

			verblist->list = (MORPHWORD*)realloc(verblist->list, sizeof(MORPHWORD)*(++verb_cnt));
			verblist->list[verb_cnt-1].word = getVerb(nlp, bom);
			verblist->list[verb_cnt-1].bom = bom;
			verblist->list[verb_cnt-1].eom = eom;
		}
		mnum = eom;
	}
	verblist->count = verb_cnt;

	return verblist;
}

MORPHWORD_LIST *getAdverblist(NLPDOC *nlp)
{
	MORPHWORD_LIST *advlist = NULL;

	TOKEN_STRUCT *token;
	MORPH_STRUCT *morph;
	PHRASE_STRUCT *phrase;

	int adv_cnt = 0;
	int ucs2_len = 0;
	int bom, bom2, eom, eom2, mnum, cnum, cnum2;
	char *pos = NULL, *pos2;

	if(!nlp) return NULL;

	token = &nlp->Token;
	morph= &nlp->Morph;
	phrase = &nlp->Phrase;

	advlist = (MORPHWORD_LIST*)malloc(sizeof(MORPHWORD_LIST));
	memset(advlist, 0, sizeof(MORPHWORD_LIST));

	for(mnum=0; mnum<morph->count; mnum++){
		bom = -1;

		// use the pos of the chunk
		cnum = morph->morphs[mnum].chunk_number;
		if(cnum != -1){
			pos = phrase->phrases[cnum].pos;
			eom = phrase->phrases[cnum].eom;
		}else{
			pos = morph->morphs[mnum].pos;
			eom = mnum;
		}

		if(strncmp(pos, "ma", 2) == 0){
			bom = mnum;
		}else{
			cnum2 = morph->morphs[eom+1].chunk_number;
			if(cnum2 != -1){
				pos2 = phrase->phrases[cnum2].pos;
				bom2 = phrase->phrases[cnum2].bom;
				eom2 = phrase->phrases[cnum2].eom;
			}else{
				pos2 = morph->morphs[eom+1].pos;
				bom2 = eom+1;
				eom2 = eom+1;
			}

			if((strncmp(pos2, "ec", 2) == 0) && strstr(" 게 도록 ", morph->morphs[eom2].utf8)){
				bom = mnum;
				eom = eom2;
			}
		}

		// 부사 발견시
		if(bom > -1){
			for(mnum=bom; mnum<=eom; mnum++){
				ucs2_len += morph->morphs[mnum].length;
			}

			advlist->list = (MORPHWORD*)realloc(advlist->list, sizeof(MORPHWORD)*(++adv_cnt));
			advlist->list[adv_cnt-1].word = malloc(sizeof(UCS2)*(ucs2_len+1));
			advlist->list[adv_cnt-1].word[0] = 0;
			advlist->list[adv_cnt-1].bom = bom;
			advlist->list[adv_cnt-1].eom = eom;

			for(mnum=bom; mnum<=eom; mnum++){
				ucs2_strcat(advlist->list[adv_cnt-1].word, morph->morphs[mnum].text);
			}

		}
		mnum = eom;
	}
	advlist->count = adv_cnt;

	return advlist;
}


// free memories allocated to the noun list
void freeWordlist(MORPHWORD_LIST *nounlist)
{
	int nidx;

	if(nounlist){
		if(nounlist->list){
			for(nidx=0; nidx<nounlist->count; nidx++){
				free(nounlist->list[nidx].word);
			}
			free(nounlist->list);
		}
		free(nounlist);
	}
}

// find a verb
int findVerb(NLPDOC *nlp, int bom, int *fndm, int *eom, int *type)
{
	MORPH_STRUCT *morph = NULL;
	PHRASE_STRUCT *phrase = NULL;

	int chunknum = 0;
	int i, j;
	char *pos;

	if(!nlp || (bom < 0) || (bom >= nlp->Morph.count) || !fndm || !eom || !type) return -1;

	morph = &nlp->Morph;
	phrase = &nlp->Phrase;

	for(i=bom; i<morph->count; i++) {
		// use the pos of the chunk
		chunknum = morph->morphs[i].chunk_number;
		if(chunknum != -1){
			pos = phrase->phrases[chunknum].pos;
			*eom = phrase->phrases[chunknum].eom;
		} else {
			pos = morph->morphs[i].pos;
			*eom = i;
		}

		// 동사 or 형용사
		if(is_exist(" vv va ", ' ', pos)){
			if(!rejectVerb(morph, i)){
				*fndm = i;
				*type = 1;
				for(j=*eom+1; j<morph->count;j++){
					if(strstr("ep", morph->morphs[j].pos)) continue;
					else if(strstr("etm", morph->morphs[j].pos)) *type=2;
					break;
				}
				return 1;
			}
		}

		// 명사의 동사형 (명사 + 이다)
		if(is_exist(" nng nnp nnb nnk nnK nsp ncp ne np nr nf ", ' ', pos)){
			*fndm = i;
			*type = 1;

			// see the chunk of the next morpheme
			chunknum = morph->morphs[*eom+1].chunk_number;
			if(chunknum != -1){
				pos = phrase->phrases[chunknum].pos;
				*eom = phrase->phrases[chunknum].eom;
			} else {
				pos = morph->morphs[*eom+1].pos;
				*eom = i;
			}

			if(is_exist(" vcp vcn ec ep ef vxn vxp ", ' ', pos)){
				return 1;
			}
		}

		i=*eom;
	}

	return 0;
}

// find a noun
int findNoun(NLPDOC *nlp, int bom, int direct, int *fndm)
{
	MORPH_STRUCT *morph = NULL;
	int i=0,j=-1, ncnt=0;

	if(!nlp || (bom < 0) || (bom >= nlp->Morph.count) || !fndm) return 0;
	morph = &nlp->Morph;

	*fndm=0;
	if(direct == 1){ // left 
		for(i=bom; i>=0; i--) {
			if(strstr(",/", morph->morphs[i].utf8)){
				break;
			}
			if(strstr("cr ef etm etn vv va sf unk maj", morph->morphs[i].pos)){
				break;
			}

			if(strstr("nng nnk nnK np nnp nnb sn eng chn jpn nr unk", morph->morphs[i].pos)){
				j = i;
				ncnt++;
				if(ncnt >= 10) break;
			}
		}
		if(j < 0 ) return 0;
		bom = j;
	}

	for(i=bom; i<morph->count; i++) {
		if(strstr("cr ec ef vv va sf maj ", morph->morphs[i].pos)){
			break;
		}
		//if(strstr("np nnb", morph->morphs[i].pos)) continue;
		if(strstr("nng nnk nnK nnp sn eng chn jpn unk", morph->morphs[i].pos)){
			*fndm = i;
			return 1;
		}
	}

	return 0;
}

void morph_ucs2_strcpy(MORPH_STRUCT *Morph, UCS2 *buf, int bom, int eom)
{
	int mnum = 0;
	UCS2 space[2];
	struct morph *pmorp, *nmorp;
	int len;

	if(!Morph || (bom < 0) || (eom < 0) || (bom >= Morph->count) || 
			(eom >= Morph->count) || (bom > eom)) return;

	buf[0] = 0x0000;
	space[0] = 0x0020;
	space[1] = 0x0000;
	for(mnum=bom; mnum<=eom; mnum++){
		pmorp = Morph->morphs + mnum;
		nmorp = pmorp + 1;

		if(!is_exist(" sp ss so ", ' ', pmorp->pos)){
			ucs2_strcat(buf, pmorp->text);
		}
		len = ucs2_strlen(buf);
		if( (mnum != eom) && 
			(((nmorp->token_number - pmorp->token_number) == 2) || 
			(is_exist(" so ss ", ' ', pmorp->pos) && 
			 !(is_exist(" jks jkg jkc jko jkb jkx jc jx ", ' ', nmorp->pos))))  &&
			(buf[len-1] != 0x0020) && (buf[len-1] != 0x0000) ){
			ucs2_strcat(buf, space);
		}
	}
}

char * getWords(MORPH_STRUCT *Morph, int bom, int eom)
{
	char *words;
	int mnum = 0;
	int len = 0;
	struct morph *pmorp, *nmorp;

	if(!Morph || (bom < 0) || (eom < 0) || (bom >= Morph->count) || 
			(eom >= Morph->count) || (bom > eom)) return NULL;

	if(!(words = malloc((eom-bom+1)*100))) return NULL;

	words[0] = 0x00;
	for(mnum=bom; mnum<=eom; mnum++){
		pmorp = Morph->morphs + mnum;
		nmorp = pmorp + 1;

		if(is_exist(" nng nnp nnb nnk nnK sn sw nr eng chn unk ", ' ', pmorp->pos)){
			strcat(words, pmorp->utf8);
			if( (mnum != eom) && 
				( ((nmorp->token_number - pmorp->token_number) == 2) || 
				  !is_exist(" nng nnp nnb nnk nnK sn sw nr eng chn unk ", ' ', nmorp->pos) ) ){
				strcat(words, " ");
			}
		}
	}

	len = strlen(words);
	if(words[len-1] == ' '){ words[len-1] = 0x00; len--; }
	if(!(words = realloc(words, len+1))) return NULL;

	return words;
}
