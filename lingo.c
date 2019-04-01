#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "feature_extract.h"
#include "feature_store.h"
#include "term_store.h"
#include "phrase_store.h"
#include "term_doc_matrix.h"
#include "abstract_concept.h"
#include "phrase_match.h"
#include "label_store.h"
#include "label_select.h"
#include "label_cluster.h"
#include "morpheme.h"
#include "cossim.h"
#include "docnum.h"
#include "util.h"

#include "xten_user.h"
#include "strlib.h"

#define MAX_LINE_BUF_SIZE	1024*1024
#define MAX_DOCID_CNT		1024*1024
#define MAX_SECTION_CNT		16
#define USE_WORDS			1

#define REMOVE_NEWLINE(BUF)	\
		{ \
			int len = strlen(BUF); \
			if((len > 0) && ((BUF)[len-1] == '\n')) (BUF)[len-1] = 0x00; \
		}
#define LOOP_FINALIZE {loop_error = 1; goto loop_final;}
#define LOOP_NEXT goto loop_final;
#define MAIN_FINALIZE goto main_final;

enum target_type {ONLY_PHRASE, ONLY_TERM, BOTH_PHRASE_TERM};
enum label_type {FEAT_NOUN, LABEL_NOUN, LABEL_POST};
/*	FEAT_NOUN : use only nouns when extract features
	LABEL_NOUN : use nouns and postpositions when extract features, use nouns when print the label
	LABEL_POST : use nouns and postpositions when extract features, use nouns and postpositions when print the label */

struct {
	// essential option
	char *sysspec;
	char *usrspec;
	char *nlpdic;
	char *docidfile;
	char *secnames[MAX_SECTION_CNT];
	int seccnt;
	char *docnum_secname;

	// threshold option
	int threshold_docfrq;
	double threshold_q;
	int threshold_dimension;
	double threshold_sim;
	double threshold_score;
	double threshold_assignment;

	// method
	enum weight_type wtype;
	enum target_type ttype;	// label target
	enum label_type ltype;	// label type

	// print option
	char print_docid;
	char print_doc;
	char print_feature;
	char print_phrase;
	char print_term;
	char print_label;
	char print_cluster;
	char print_verbose;
} opt;

void print_usage(char *progname);
int get_option(int argc, char **argv);

int getDocIDList(char *buf, int *docids, int *count, int max_count);
int procDocNum(DOCNUM_TABLE *table, int dlocnum, int *docids, int doccnt);
int procFeature(NLPDOC *nlp, int dlocnum, FEATURE_TABLE *feattable, DOC_FEAT_TABLE *dftable, int doccnt, int *docids);
int procTerm(FEATURE_TABLE *feattable, TERM_TABLE *termtable);
int procPhrase(PHRASE_TABLE *phrstable, FEATURE_TABLE *feattable);
int procTDMatrix(DOC_FEAT_TABLE *dftable, FEATURE_TABLE *feattable, TERM_TABLE *termtable, SMat *tdmtrx);
int procAbsConcpt(SMat tdmtrx, ABS_CONCEPT *abs);
int procPhraseMatch(P_MATRIX *pmtrx, PHRASE_TABLE *phrstable, TERM_TABLE *termtable, int doccnt);
int procLabelSelect(LABEL_TABLE *labeltable, M_MATRIX *mmtrx, P_MATRIX *pmtrx, ABS_CONCEPT *abs, PHRASE_TABLE *phrstable, TERM_TABLE *termtable, int doccnt);
int printLabel(LABEL_TABLE *labeltable, PHRASE_TABLE *phrstable, TERM_TABLE *termtable);
int procCluster(CLUSTER_TABLE *cluster, C_MATRIX *cmtrx, Q_MATRIX *qmtrx, LABEL_TABLE *labeltable, P_MATRIX *pmtrx, SMat tdmtrx, DOCNUM_TABLE *docnum_table, LABEL_TREE *label_tree, PHRASE_TABLE *phrase_table, TERM_TABLE * term_table);

int main(int argc, char **argv)
{
	// doc location
	int dlocnum = -1;

	// morpheme analysis
	NLPDOC nlp;

	// feature
	FEATURE_TABLE *feattable;
	DOC_FEAT_TABLE *dftable;
	int tot_doccnt = 0;

	// label
	PHRASE_TABLE *phrstable;	// phrase
	TERM_TABLE *termtable;		// term
	SMat tdmtrx;				// term-doc matrix
	ABS_CONCEPT	abs;			// abstract concept
	P_MATRIX pmtrx;				// phrase matching
	M_MATRIX mmtrx;				// label select
	LABEL_TABLE *labeltable;	// label

	// cluster
	Q_MATRIX *qmtrx;
	C_MATRIX *cmtrx;
	DOCNUM_TABLE *docnum_table;
	CLUSTER_TABLE *cluster, *tot_cluster = NULL;
	LABEL_TREE *label_tree = NULL;

	// input
	FILE *docidfp = NULL;
	char buf[MAX_LINE_BUF_SIZE+1];
	int docids[MAX_DOCID_CNT];

	// time
	double starttime, endtime;

	// option
	if(get_option(argc, argv) < 0){
		printf("[ERR] failed get options\n");
		return -1;
	}

	int loop_error;
	int file_size;

	// initialize
	starttime = timer();
	nlp_Init(&nlp);
	nlp_SetDict(opt.nlpdic);
	nlp_SetBuffer(&nlp, 1024+1024);
	loop_error = 0;
	file_size = 0;

	if(xten_Initialize(opt.sysspec) < 0){
		printf("[ERR] xten initialization fail\n"); MAIN_FINALIZE
	}
	if((dlocnum = xten_OpenDocLoc(opt.usrspec)) < 0){
		printf("[ERR] open document location fail(%d)\n", dlocnum); MAIN_FINALIZE
	}
	if(!(docidfp = fopen(opt.docidfile, "rt"))){
		printf("[ERR] Doc id file name is wrong(%s)\n", opt.docidfile); MAIN_FINALIZE
	}
	if(!(label_tree = createLabelTree())){
		printf("[ERR] create label tree\n"); MAIN_FINALIZE
	}
	if(!(tot_cluster = createClusterTable())){
		printf("[ERR] create cluster table\n"); MAIN_FINALIZE
	}

	fseek(docidfp, 0, SEEK_END);
	file_size = ftell(docidfp);
	fseek(docidfp, 0, SEEK_SET);

	while(!loop_error && fgets(buf, 1024*1024, docidfp)){
		if((buf[0] == 0x00) || (buf[0] == '\n')) continue;
		REMOVE_NEWLINE(buf)
		loop_error = 0;

		printf("%ld", ftell(docidfp)*100/file_size);

		if(opt.print_docid) printf("<DocID list : %s>\n", buf);
		if(getDocIDList(buf, docids, &tot_doccnt, MAX_DOCID_CNT) < 0) continue;

		// initialize
		docnum_table = NULL;
		feattable = NULL;
		dftable = NULL;
		phrstable = NULL;
		termtable = NULL;
		labeltable = NULL;
		qmtrx = NULL;
		cmtrx = NULL;
		cluster= NULL;
		tdmtrx = NULL;
		abs.Ut = NULL;
		pmtrx.tfmtrx = NULL;
		mmtrx.mtrx = NULL;

		if(!(docnum_table = createDocNumTable())) LOOP_FINALIZE
		if(!(feattable = createFeatureTable(opt.threshold_docfrq))) LOOP_FINALIZE
		if(!(dftable = createDocFeatTable(tot_doccnt))) LOOP_FINALIZE
		if(!(phrstable = createPhraseTable())) LOOP_FINALIZE
		if(!(termtable = createTermTable())) LOOP_FINALIZE
		if(!(labeltable = createLabelTable())) LOOP_FINALIZE
		if(!(qmtrx = createQMatrix())) LOOP_FINALIZE
		if(!(cmtrx = createCMatrix())) LOOP_FINALIZE
		if(!(cluster = createClusterTable())) LOOP_FINALIZE

		// main process
		if(procDocNum(docnum_table, dlocnum, docids, tot_doccnt) < 0) LOOP_NEXT
		if(procFeature(&nlp, dlocnum, feattable, dftable, tot_doccnt, docids) < 0) LOOP_NEXT
		if(procTerm(feattable, termtable) < 0) LOOP_NEXT
		if(procPhrase(phrstable, feattable) < 0) LOOP_NEXT
		if(procTDMatrix(dftable, feattable, termtable, &tdmtrx) < 0) LOOP_NEXT
		if( (tdmtrx->rows < 1) || (tdmtrx->cols < 1) ) LOOP_NEXT
		if(procAbsConcpt(tdmtrx, &abs) < 0) LOOP_NEXT
		if(procPhraseMatch(&pmtrx, phrstable, termtable, tot_doccnt) < 0) LOOP_NEXT
		if(procLabelSelect(labeltable, &mmtrx, &pmtrx, &abs, phrstable, termtable, tot_doccnt) < 0) LOOP_NEXT
		if(opt.print_label) printLabel(labeltable, phrstable, termtable);
		//if(procCluster(cluster, cmtrx, qmtrx, labeltable, &pmtrx, tdmtrx, docnum_table, label_tree, phrstable, termtable) < 0) LOOP_NEXT

loop_final:
		// finalize
		freeDocNumTable(docnum_table);
		svdFreeSMat(tdmtrx);
		freeFeatureTable(feattable);
		freeDocFeatTable(dftable);
		freePhraseTable(phrstable);
		freeTermTable(termtable);
		freeLabelTable(labeltable);
		svdFreeDMat(abs.Ut);
		svdFreeSMat(pmtrx.tfmtrx);
		svdFreeDMat(mmtrx.mtrx);
		freeQMatrix(qmtrx);
		freeCMatrix(cmtrx);
		freeClusterTable(cluster);
	}

	if(makeClusterTable(tot_cluster, label_tree) < 0) MAIN_FINALIZE
	if(opt.print_cluster && tot_cluster){
		printf("<CLUSTER>\n");
		int i,j;
		for(i=0; i<tot_cluster->count; i++){
			printf("%s=>", tot_cluster->clusters[i].label);
			for(j=0; j<tot_cluster->clusters[i].count; j++){
				if(j != 0) printf(",");
				printf("%d", tot_cluster->clusters[i].docids[j]);
			}
			printf("\n");
		}
	}

main_final:
	endtime = timer();
	printf("elapsed time = %6g sec.\n", endtime-starttime);

	// finalize
	fclose(docidfp);
	nlp_Final(&nlp);
	xten_CloseDocLoc(dlocnum);
	xten_Finalize();
	freeLabelTree(label_tree);
	freeClusterTable(tot_cluster);

	return 0;
}

void print_usage(char *progname)
{
	printf(
	"<Lingo Labeling & Clustering>\n\n"
	"Essential options:\n"
	"  -s*\tSystem spec file\n"
	"  -u*\tUser spec file\n"
	"  -n*\tNLP dictionary directory\n"
	"  -l*\tDoc ID list file\n"
	"  -i*\tSection name (section1,section2,...)\n\n"
	"Threshold options:\n"
	"  -d\tDoc frequency threshold (int, default:1)\n"
	"  -q\tq (float, default:0.7)\n"
	"  -m\tsvd dimension (int)\n"
	"  -p\tpruning similarity (float, default:0.5)\n"
	"  -c\tlabel score (float, default:0.2)\n"
	"  -g\tcluster assignment (float, default:0.2)\n\n"
	"Other options:\n"
	"  -e\tDoc eigen number section name\n"
	"  -w\tweighting type (tf/df/tfidf, default:tf)\n"
	"  -t\tlabel target type (term/phrase/both, default:both)\n\n"
	/*
	"  -b\tlabel type(unoun/pnoun/ppost, default:pnoun)\n"
	"  \t(unoun:use only nouns when extact features)\n"
	"  \t(pnoun:use nouns and postpositions when extact features, use only nouns when print the label)\n"
	"  \t(ppost:use nouns and postpositions when extact features, use nouns and postpositions when print the label)\n"
	*/
	"Print options:\n"
	"  -I\tprint docid list\n"
	"  -D\tprint document\n"
	"  -F\tprint feature\n"
	"  -R\tprint phrase\n"
	"  -E\tprint term\n"
	"  -L\tprint label\n"
	"  -C\tprint cluster\n"
	"  -V\tprint verbose\n"
	);
}

int get_option(int argc, char **argv)
{
	int c = 0;
	char *psecname;

	if((argc < 2) || (strcmp(argv[1], "--help") == 0)){
		print_usage(argv[0]);
		exit(0);
	}

	memset(&opt, 0, sizeof(opt));
	opt.threshold_docfrq = 1;
	opt.threshold_q = 0.7;
	opt.threshold_dimension = 0;
	opt.threshold_sim = 0.5;
	opt.threshold_score = 0.2;
	opt.threshold_assignment = 0.2;
	opt.wtype = TF;
	opt.ttype = BOTH_PHRASE_TERM;
	opt.ltype = LABEL_NOUN;
	opt.print_docid = 0;
	opt.print_doc = 0;
	opt.print_feature = 0;
	opt.print_phrase = 0;
	opt.print_term = 0;
	opt.print_label = 0;
	opt.print_cluster = 0;
	opt.print_verbose = 0;

	while((c = getopt(argc, argv, "s:u:n:l:i:e:d:q:m:p:c:g:w:t:b:IDFRELCV")) != -1){
		switch(c){
			case 's': // system spec file
				opt.sysspec = optarg;
				break;
			case 'u': // user spec file
				opt.usrspec = optarg;
				break;
			case 'n': // NLP dictionary directory
				opt.nlpdic = optarg;
				break;
			case 'l': // doc id list file
				opt.docidfile = optarg;
				break;
			case 'i': // section
				opt.secnames[0] = strtok(optarg, ",");
				for(opt.seccnt=1; (psecname = strtok(NULL, ",")); opt.seccnt++){
					opt.secnames[opt.seccnt] = psecname;
				}
				break;
			case 'e': // doc eigen number section name
				opt.docnum_secname = optarg;
				break;
			case 'd': // doc frequency threshold
				opt.threshold_docfrq = atoi(optarg);
				break;
			case 'q': // q threshold
				opt.threshold_q = atof(optarg);
				break;
			case 'm': // dimension
				opt.threshold_dimension = atof(optarg);
				break;
			case 'p': // pruning label similarity threshold
				opt.threshold_sim = atof(optarg);
				break;
			case 'c': // label minimum score threshold
				opt.threshold_score = atof(optarg);
				break;
			case 'g': // cluster assignment threshold
				opt.threshold_assignment = atof(optarg);
				break;
			case 'w': // weighting type
				if(strcmp(optarg, "df")==0){
					opt.wtype = DF;
				}else if(strcmp(optarg, "tfidf")==0){
					opt.wtype = TF_IDF;
				}else{
					opt.wtype = TF;
				}
				break;
			case 't': // label target type
				if(strcmp(optarg, "term")==0){
					opt.ttype = ONLY_TERM;
				}else if(strcmp(optarg, "phrase")==0){
					opt.ttype = ONLY_PHRASE;
				}else{
					opt.ttype = BOTH_PHRASE_TERM;
				}
				break;
			case 'b': // label type
				if(strcmp(optarg, "unoun")==0){
					opt.ltype = FEAT_NOUN;
				}else if(strcmp(optarg, "ppost")==0){
					opt.ltype = LABEL_NOUN;
				}else{
					opt.ltype = LABEL_POST;
				}
				break;
			case 'I':
				opt.print_docid = 1;
				break;
			case 'D':
				opt.print_doc = 1;
				break;
			case 'F':
				opt.print_feature = 1;
				break;
			case 'R':
				opt.print_phrase = 1;
				break;
			case 'E':
				opt.print_term = 1;
				break;
			case 'L':
				opt.print_label = 1;
				break;
			case 'C':
				opt.print_cluster = 1;
				break;
			case 'V':
				opt.print_verbose = 1;
				break;
			default:
				printf("[ERR] the option '-%c' is not right option\n", c);
				return -1;
		}
	}

	if(!opt.sysspec) {printf("System spec file(-s) is essential\n"); return -1;}
	if(!opt.usrspec) {printf("User spec file(-u) is essential\n"); return -1;}
	if(!opt.nlpdic) {printf("NLPDIC directory option(-n) is essential\n"); return -1;}
	if(!opt.docidfile) {printf("Doc id file option(-n) is essential\n"); return -1;}
	if(!opt.secnames) {printf("section names option(-n) is essential\n"); return -1;}

	return 0;
}

int getDocIDList(char *buf, int *docids, int *count, int max_count)
{
	char *s_docid, *pstart, *pend;
	int i_docid, startnum, endnum;
	int idx;

	idx = 0;

	if(strstr(buf, "~")){
		pstart = strtok(buf, "~");
		pend = strtok(NULL, "~");
		if(pstart && pend){
			startnum = atoi(pstart);
			endnum = atoi(pend);
			if(endnum - startnum > MAX_DOCID_CNT) return -1;
			for(i_docid=startnum; i_docid<=endnum; i_docid++){
				docids[idx++] = i_docid;
			}
		}else{
			return -1;
		}
	}else{
		s_docid = strtok(buf, " ");
		if(s_docid){
			i_docid = atoi(s_docid);
			docids[idx++] = i_docid;
			while((s_docid = strtok(NULL, " "))){
				i_docid = atoi(s_docid);
				docids[idx++] = i_docid;
				if(idx > max_count) return -1;
			}
		}else{
			return -1;
		}
	}

	*count = idx;

	return 0;
}

int procDocNum(DOCNUM_TABLE *table, int dlocnum, int *docids, int doccnt)
{
	if(makeDocNumTable(table, dlocnum, opt.docnum_secname, docids, doccnt) < 0) return -1;

	if(opt.print_verbose){
		int i;
		printf("<DOC-NUM>\n");
		for(i=0; i<table->count; i++){
			printf("%d:%d-%d\n", table->docnums[i].docid, table->docnums[i].org_docid , table->docnums[i].eigennum);
		}
	}

	return 0;
}

int procFeature(NLPDOC *nlp, int dlocnum, FEATURE_TABLE *feattable, DOC_FEAT_TABLE *dftable, int doccnt, int *docids)
{
	// section
	char *secvals[MAX_SECTION_CNT];
	int secnum;
	char *secbuf;
	secbuf = malloc(1024 * 1024 * (opt.seccnt+1));

	// document
	char *docbuf;

	// feature
	FEATURE_MORPHEME_TABLE *featlist = NULL;
	FEATURE_MORPHEME *feattexts;
	FEATURE_TREE *feattree = NULL;
	FEATURE_FRQ *featfrqs = NULL;
	UCS2 feattext[1024];
	char *featwords;
	int max_featcnt_per_doc = 100, filcnt;
	int featnum, docnum, featid, docid;

	featlist = createFeatureTextList(max_featcnt_per_doc);
	feattree = createFeatureTree();
	docbuf = malloc(opt.seccnt*1024*1024);

	if(opt.print_doc) printf("<TITLE>\n");
	for(docnum=0; docnum<doccnt; docnum++){
		docid = docids[docnum];
		if(getSectionVals(dlocnum, docid, opt.secnames, secvals, secbuf, opt.seccnt, opt.seccnt*1024*1024) < 0){
			freeFeatureTextList(featlist);
			freeFeatureTree(feattree);
			return -1;
		}
		docbuf[0] = '\0';
		for(secnum=0; secnum<opt.seccnt; secnum++){
			if(secnum>0) strcat(docbuf, "\n");
			strcat(docbuf, secvals[secnum]);
		}
		if(docbuf[0] == '\0') continue;
		if(opt.print_doc) printf("%s\n", docbuf);

		// morphological analysis
		nlp_SetText(nlp, docbuf); // set the text buffer for analyzing morpheme
		nlp_Token(nlp);
		nlp_Morph(nlp);
		nlp_Phrase(nlp);
		nlp_Sentence(nlp);

		// extract original texts of features
		extractFeatureMorph(nlp, featlist, VARIETY);

		feattexts = featlist->feattexts;
		featfrqs = malloc(sizeof(FEATURE_FRQ)*featlist->count);
		if(opt.print_verbose){
			int i;
			printf("<FEATURE TEXT>\n");
			for(i=0; i<featlist->count; i++){
				print_morph(&nlp->Morph, feattexts[i].bom, feattexts[i].eom);
				printf("(%d~%d)", feattexts[i].bom, feattexts[i].eom);
				printf("\n");
			}
			printf("\n");
		}

		// make the feature tree
		for(featnum=0, filcnt = 0; featnum<featlist->count; featnum++){
			morph_ucs2_strcpy(&nlp->Morph, feattext, feattexts[featnum].bom, feattexts[featnum].eom);
			featwords = getWords(&nlp->Morph, feattexts[featnum].bom, feattexts[featnum].eom);
			//ucs2_print(feattext);
			//printf("(%s)\n", featwords);
			if(insertToFeatureTree(feattree, feattext, featwords, docnum, &featid) == 1){
				free(featwords);
			}

			// make the feature list (FEATURE_FRQ array)
			addInFeatureList(featfrqs, featid, &filcnt);
		}

		// insert the feature list to the doc-feature table
		insertToDocFeatTable(dftable, docnum, featfrqs, filcnt);
	}

	// make the feature table
	makeFeatureTable(feattable, feattree);

	if(opt.print_feature){
		int i;
		printf("<FEATURE>\n");
		for(i=0; i<feattable->count; i++){
			printf("%s(%s)\t", feattable->features[i].feature, feattable->features[i].words);
			printf("%d\n", feattable->features[i].docfrq);
		}
	}

	if(opt.print_verbose){
		int i,j;
		FEATURE_LIST *list;
		printf("<Doc-Feat Table>\n");
		for(i=0; i<dftable->count; i++){
			list = dftable->featlists + i;

			printf("%d=>", i);
			for(j=0; j<list->count; j++){
				int featid = list->featfrqs[j].featid;
				printf("%d-%s(%d,%d),", featid, feattable->features[featid].feature, list->featfrqs[j].featfrq, feattable->features[featid].docfrq);
			}
			printf("\n");
		}
	}

	// finalize
	free(secbuf);
	free(docbuf);
	freeFeatureTextList(featlist);
	freeFeatureTree(feattree);

	return 0;
}

/*
 * 각 feature를 이루는 term과 term id를 추출하여 feature table에 저장하고
 * 전체 feature들에서 추출된 term들을 term table에 저장한다.
 */
int procTerm(FEATURE_TABLE *feattable, TERM_TABLE *termtable)
{
	TERM_TREE *termtree;
	FEATURE *feats, *pfeat;
	UCS2 ucs2_feature[1024];
	UCS2 *pterm[32];
	int *termids = NULL;

	int featnum, featofs, termnum;
	int termid;
	int termcnt;

	termtree = createTermTree();

	feats = feattable->features;

	for(featnum=0; featnum<feattable->count; featnum++){
		pfeat = feats + featnum;
		termcnt = 0;
		if(pfeat->qualify_flg == 0) continue;

		utf8_to_ucs2((UTF8*)pfeat->words, ucs2_feature);

		// feature를 이루는 term들을 추출
		if(ucs2_feature[0] != 0x0000){
			pterm[termcnt++] = ucs2_feature;
			for(featofs=0; ucs2_feature[featofs] != 0x0000; featofs++){
				if(ucs2_feature[featofs] == 0x0020){
					pterm[termcnt++] = ucs2_feature + featofs + 1;
					ucs2_feature[featofs] = 0x0000;
				}
			}
		}else{
			termcnt = 0;
			termids = NULL;
		}

		if(termcnt > 0) termids = malloc(sizeof(int)*termcnt);

		/*
		 * 각 term들을 termtree에 삽입하면서 termid를 얻어
		 * feature를 이루는 term들의 id 리스트 생성
		 */
		for(termnum=0; termnum<termcnt; termnum++){
			insertToTermTree(termtree, pterm[termnum], &termid);
			termids[termnum] = termid;
		}

		setTermidOfFeatureTable(feattable, featnum, termids, termcnt);
	}

	if(makeTermTable(termtable, termtree) < 0){
		freeTermTree(termtree);
		return -1;
	}


	if(opt.print_verbose){
		int i, j;
		FEATURE *pfeat;
		printf("<FEAT-TERMID>\n");
		for(i=0; i<feattable->count; i++){
			pfeat = feattable->features+i;
			printf("%d(%s)->", i, pfeat->feature);
			for(j=0; j<pfeat->termcnt; j++){
				printf("%d,", pfeat->termids[j]);
			}
			printf("\n");
		}
	}

	freeTermTree(termtree);

	return 0;
}

/*
 * features 중 topic 자격이 되고 두개 이상의 term으로 이뤄진 features로 phrase table을 구성한다.
 */
int procPhrase(PHRASE_TABLE *phrstable, FEATURE_TABLE *feattable)
{
	if(makePhraseTable(phrstable, feattable) < 0) return -1;

	if(opt.print_phrase){
		int i;
		printf("<PHRASE(%d)>\n", phrstable->count);
		for(i=0; i<phrstable->count; i++){
			printf("%s", phrstable->phrases[i].phrase);
			printf("(%d,", phrstable->phrases[i].docfrq);
			printf("%d)\n", phrstable->phrases[i].termcnt);
		}
	}

	return 0;
}

/*
 * make term-doc matrix
 */
int procTDMatrix(DOC_FEAT_TABLE *dftable, FEATURE_TABLE *feattable, TERM_TABLE *termtable, SMat *tdmtrx)
{
	makeTermDocMatrix(dftable, feattable, termtable, tdmtrx, opt.wtype);

	if(opt.print_term){
		int i;
		TERM *pterm;
		printf("<TERM>\n");
		for(i=0; i<termtable->count; i++){
			pterm = termtable->terms + i;
			printf("%d:%s(%d)\n", i, pterm->term, pterm->docfrq);
		}
	}

	//svdWriteSparseMatrix(*tdmtrx, "TD.mtrx", SVD_F_ST);
	if(opt.print_verbose){
		printf("<TD matrix(%ld-%ld-%ld)>\n", (*tdmtrx)->rows, (*tdmtrx)->cols, (*tdmtrx)->vals);
		//printf("%ld-%ld-%ld\n", (*tdmtrx)->rows, (*tdmtrx)->cols, (*tdmtrx)->vals);
		int i,j;
		for(i=0; i<(*tdmtrx)->rows; i++){
			for(j=0; j<(*tdmtrx)->cols; j++){
				printf("%.2f ", getSVal(*tdmtrx, i, j));
			}
			printf("\n");
		}
	}

	return 0;
}

/*
 * term-doc matrix의 Ut 를 얻는다.
 */
int procAbsConcpt(SMat tdmtrx, ABS_CONCEPT *abs)
{
	if(makeUt(abs, tdmtrx, opt.threshold_q, opt.threshold_dimension) < 0) return -1;

	//svdWriteDenseMatrix(abs->Ut, "Ut.mtrx", SVD_F_DT);
	if(opt.print_verbose){
		printf("<Ut matrix(%d)>\n", abs->k);
		int i,j;
		for(i=0; i<abs->Ut->rows; i++){
			for(j=0; j<abs->Ut->cols; j++){
				printf("%.2f ", abs->Ut->value[i][j]);
			}
			printf("\n");
		}
	}

	return 0;
}

int procPhraseMatch(P_MATRIX *pmtrx, PHRASE_TABLE *phrstable, TERM_TABLE *termtable, int doccnt)
{
	if(makePMatrix(pmtrx, phrstable, termtable, doccnt, opt.wtype) < 0) return -1;

	//svdWriteSparseMatrix(pmtrx->tfmtrx, "P.mtrx", SVD_F_DT);

	if(opt.print_verbose){
		printf("<P matrix>\n");
		int i,j;
		for(i=0; i<pmtrx->termcnt; i++){
			for(j=0; j<pmtrx->phrase_cnt+pmtrx->termcnt; j++){
				printf("%.2f ", getPMatrixValue(pmtrx, i, j));
			}
			printf("\n");
		}
	}

	return 0;
}

int procLabelSelect(LABEL_TABLE *labeltable, M_MATRIX *mmtrx, P_MATRIX *pmtrx, ABS_CONCEPT *abs, PHRASE_TABLE *phrstable, TERM_TABLE *termtable, int doccnt)
{
	switch(opt.ttype){
		case ONLY_PHRASE:
			if(makeMMatrix_Phrase(mmtrx, pmtrx, abs) < 0) return -1;
			break;
		default:
			if(makeMMatrix(mmtrx, pmtrx, abs) < 0) return -1;
	}
	if(makeLabelTable(labeltable, mmtrx, phrstable, pmtrx->phrase_cnt) < 0) return -1;
	if(pruneLabel(labeltable, phrstable->phrases, termtable->terms, doccnt, opt.threshold_sim, opt.threshold_score, opt.wtype) < 0) return -1;
	//svdWriteDenseMatrix(mmtrx->mtrx, "M.mtrx", SVD_F_DT);
	if(opt.print_verbose){
		printf("<M matrix>\n");
		int i,j;
		for(i=0; i<mmtrx->mtrx->rows; i++){
			for(j=0; j<mmtrx->mtrx->cols; j++){
				printf("%.2f ", mmtrx->mtrx->value[i][j]);
			}
			printf("\n");
		}
	}

	return 0;
}

int printLabel(LABEL_TABLE *labeltable, PHRASE_TABLE *phrstable, TERM_TABLE *termtable)
{
	LABEL *labels, *plabel;
	PHRASE *phrases;
	TERM *terms;
	int label_num;
	int ptid, term_num;
	int featcnt;

	labels = labeltable->labels;
	phrases = phrstable->phrases;
	terms = termtable->terms;
	featcnt = phrstable->count;

	printf("<LABEL>\n");
	for(label_num=0; label_num<labeltable->count; label_num++){
		plabel = labels + label_num;
		//if(plabel->pruned) continue;

		ptid = plabel->ptid;
		switch(plabel->phrase_term){
			case ISPHRASE:	// phrase
				if(plabel->pruned == 2) break;//printf("%s(low score)\t", phrases[ptid].phrase);
				else if(plabel->pruned == 1) break;//printf("%s(pruned)\t", phrases[ptid].phrase);
				else{
					//printf("%s\t", phrases[ptid].phrase);
					for(term_num=0; term_num<phrases[ptid].termcnt; term_num++){
						if(term_num>0) printf(" ");
						printf("%s", termtable->terms[phrases[ptid].termids[term_num]].term);
					}
					printf("(%.2f)\n", plabel->score);
				}
				break;
			case ISTERM: // term
				printf("%s\t", terms[ptid].term);
				break;
		}
		//printf("%.2f\n", plabel->score);
	}
	printf("\n");

	return 0;
}

int procCluster(CLUSTER_TABLE *cluster, C_MATRIX *cmtrx, Q_MATRIX *qmtrx, LABEL_TABLE *labeltable, P_MATRIX *pmtrx, SMat tdmtrx, DOCNUM_TABLE *docnum_table, LABEL_TREE *label_tree, PHRASE_TABLE *phrase_table, TERM_TABLE * term_table)
{
	LABEL_MATCH label_match;
	LABEL *plabel;
	CLUSTER *clusters, *pcluster;
	DOCNUM *docnums, *pdocnum;
	char *label = NULL;
	int label_num,doc_num, label_id, ptid, docid;
	int *docids, *pdocids;

	docnums = docnum_table->docnums;

	if(makeQMatrix(qmtrx, labeltable, pmtrx, &label_match) < 0) return -1;
	if(makeCMatrix(cmtrx, qmtrx, tdmtrx) < 0) return -1;
	if(assignDocument(cluster, cmtrx, opt.threshold_assignment) < 0) return -1;
	if(label_tree){
		clusters = cluster->clusters;

		for(label_num=0; label_num<label_match.count; label_num++){
			label_id = label_match.label_ids[label_num];
			plabel = labeltable->labels + label_id;
			ptid = plabel->ptid;
			pcluster = clusters + label_num;

			// get label string
			switch(plabel->phrase_term){
				case ISPHRASE:
					label = phrase_table->phrases[ptid].phrase;
					break;
				case ISTERM:
					label = term_table->terms[ptid].term;
					break;
			}

			// get doc eigen numbers
			pdocids = pcluster->docids;
			docids = malloc(sizeof(int)*pcluster->count);
			for(doc_num=0; doc_num<pcluster->count; doc_num++){
				docid = pdocids[doc_num];
				pdocnum = docnums+docid;
				docids[doc_num] = pdocnum->eigennum;
			}
			insertToLabelTree(label_tree, (UTF8*)label, docids, pcluster->count);
			free(docids);
		}
	}

	if(opt.print_verbose && qmtrx->matrix){
		printf("<Q matrix>\n");
		int i,j;
		for(i=0; i<qmtrx->matrix->rows; i++){
			for(j=0; j<qmtrx->matrix->cols; j++){
				printf("%.2f ", getSVal(qmtrx->matrix, i, j));
			}
			printf("\n");
		}
	}

	if(opt.print_verbose && cmtrx->matrix){
		printf("<C matrix>\n");
		int i,j;
		for(i=0; i<cmtrx->matrix->rows; i++){
			for(j=0; j<cmtrx->matrix->cols; j++){
				printf("%.2f ", cmtrx->matrix->value[i][j]);
			}
			printf("\n");
		}
	}

	if(opt.print_verbose){
		printf("<Inner Cluster>\n");
		int i,j;
		for(i=0; i<cluster->count; i++){
			printf("%d=>", i);
			for(j=0; j<cluster->clusters[i].count; j++){
				docid = cluster->clusters[i].docids[j];
				pdocnum = docnums+docid;
				printf("%d(%d,%d),", docid, pdocnum->org_docid, pdocnum->eigennum);
			}
			printf("\n");
		}
	}

	free(label_match.label_ids);

	return 0;
}
