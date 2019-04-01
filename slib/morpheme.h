#ifndef MORPHEME_H_
#define MORPHEME_H_

#include "nlplib.h"
#include "nlpsys/libtrie.h"

/* 명사
nng		일반명사
nnp		고유명사
ncp
nnb		의존명사
nnK		성
nnk		이름
nsp		복합명사구
ne		명칭
np		대명사
nr		수사
nf		계사(명사+술어)
   */

/* 동사
vv		동사
va		형용사
vx		보조동사
vxp		보조긍정동사
vxn		보조부정동사
vxc		보조연결동사
vcp		긍정지정사
vcn		부정지정사
vf		수식형 서술어
   */

/* 수식
mm		관형사
mag		일반부사
mar		부정부사
ma+		긍정부사
maj		접속부사
mf		
   */

/* 독립
ic		감탄사
   */

/* 관계
jks		주격조사
jkg		관형격조사
jkc		보격조사
jko		목적격조사
jkb		부사격조사
jkx		부정확한 격조사
jc		접속조사
jx		보조사
   */

/* 의존
ep		선어말어미
ef		종결어미
ec		연결어미
ecm
ecb
ecc
etn		명사형전성어미
etm		관형형어미
xpn		체언접두사
xsn		명사파생접미사
xsv		동사파생접미사
xsa		형용사파생접미사
   */

/* 기호
sf		문장끝부호
sp		쉼표,콜론,빗금
ss		인용부호
se		줄임표
so		붙임표
sn		숫자
sw		기타기호
   */

/* 기타
eng 	영어
chn 	한자
jpn		일어
unk		unknown
   */

typedef struct _morphword{
	UCS2 *word;
	int bom;
	int eom;
} MORPHWORD;

typedef struct _morphword_list{
	MORPHWORD *list;
	int count;
} MORPHWORD_LIST;

UCS2 *getWord(NLPDOC *doc, int bom);
UCS2 *getNoun(NLPDOC *doc, int bom);
UCS2 *getCompNoun(NLPDOC *nlp, int bom);
UCS2 *getVerb(NLPDOC *nlp, int bom);
UCS2 *getAdverb(NLPDOC *nlp, int bom);

MORPHWORD_LIST *getNounlist(NLPDOC *doc);
MORPHWORD_LIST *getCompNounlist(NLPDOC *nlp);
MORPHWORD_LIST *getVerblist(NLPDOC *doc);
MORPHWORD_LIST *getAdverblist(NLPDOC *nlp);
void freeWordlist(MORPHWORD_LIST *nounlist);

int findVerb(NLPDOC *nlp, int bom, int *fndm, int *eom, int *type);
int findNoun(NLPDOC *nlp, int bom, int direct, int *fndm);

void morph_ucs2_strcpy(MORPH_STRUCT *Morph, UCS2 *buf, int bom, int eom);
char * getWords(MORPH_STRUCT *Morph, int bom, int eom);

#endif

