#include <stdio.h>
#include <math.h>
#include <string.h>

#include "label_cluster.h"

#ifndef MAX
#define MAX(X,Y)	((X)>(Y)? (X):(Y))
#endif

static int _free_labeltree(UTF8 *key, char *value, char *args[]);
static int _set_clustertable(UTF8 *key, char *value, char *args[]);

// create a Q matrix
Q_MATRIX * createQMatrix(void)
{
	Q_MATRIX *q_matrix;

	q_matrix = malloc(sizeof(Q_MATRIX));
	memset(q_matrix, 0, sizeof(Q_MATRIX));

	return q_matrix;
}

// make a Q matrix
int makeQMatrix(Q_MATRIX *q_matrix, LABEL_TABLE *label_table, P_MATRIX *p_matrix, LABEL_MATCH *match)
{
	LABEL *labels, *plabel;
	SMat tfmtrx, qmtrx;
	int label_id, phrase_id, termid, label_num;
	int *label_ids;

	long *p_pointr, *q_pointr, *p_rowind, *q_rowind;
	double *p_value, *q_value;
	int label_cnt, vals, valcnt, valnum;
	long p_point, q_point;
	
	labels = label_table->labels;
	tfmtrx = p_matrix->tfmtrx;
	p_pointr = tfmtrx->pointr;
	p_rowind = tfmtrx->rowind;
	p_value = tfmtrx->value;
	label_cnt = 0;
	valcnt = 0;
	vals = 0;
	label_ids = malloc(sizeof(int)*label_table->count);

	// the number of values
	for(label_id=0; label_id<label_table->count; label_id++){
		plabel = labels + label_id;
		if(!plabel->pruned){
			vals += plabel->term_cnt;
			label_ids[label_cnt] = label_id;
			label_cnt++;
		}
	}

	match->label_ids = label_ids;
	match->count = label_cnt;

	// make the matrix
	if((qmtrx = svdNewSMat(tfmtrx->rows, label_cnt, vals)) == NULL) return -1;
	q_pointr = qmtrx->pointr;
	q_rowind = qmtrx->rowind;
	q_value = qmtrx->value;

	for(label_num=0, q_point=0; label_num<label_cnt; label_num++){
		plabel = labels + label_ids[label_num];
		if(plabel->phrase_term == ISPHRASE){
			phrase_id = plabel->ptid;
			p_point = p_pointr[phrase_id];
			valcnt = p_pointr[phrase_id+1] - p_point;

			q_pointr[label_num] = q_point;
			for(valnum=0; valnum<valcnt; valnum++){
				q_rowind[q_point+valnum] = p_rowind[p_point+valnum];
				q_value[q_point+valnum] = p_value[p_point+valnum];
			}
		}else{
			termid = plabel->ptid;
			valcnt = 1;

			q_pointr[label_num] = q_point;
			q_rowind[q_point] = termid;
			q_value[q_point] = 1.0;
		}

		q_point += valcnt;
	}
	q_pointr[label_num] = q_point;

	q_matrix->matrix = qmtrx;

	return 0;
}

// get the particular value of a Q matrix
double getQMatrixValue(Q_MATRIX *matrix, long row, long col)
{
	return getSVal(matrix->matrix, row, col);
}

// free memories allocated to a Q matrix
void freeQMatrix(Q_MATRIX *matrix)
{
	if(matrix){
		if(matrix->matrix){
			svdFreeSMat(matrix->matrix);
		}
		free(matrix);
	}
}

// create a C matrix
C_MATRIX * createCMatrix(void)
{
	C_MATRIX *c_matrix;

	c_matrix = malloc(sizeof(C_MATRIX));
	memset(c_matrix, 0, sizeof(C_MATRIX));

	return c_matrix;
}

// make a C matrix
int makeCMatrix(C_MATRIX *c_matrix, Q_MATRIX *q_matrix, SMat term_doc_matrix)
{
	double **c_value;
	SMat qmtrx;
	SMat qtmtrx;
	DMat cmtrx;
	int rows, cols;
	int rownum, colnum, num;
	double val;

	qmtrx = q_matrix->matrix;
	qtmtrx = svdTransposeS(qmtrx);
	rows = qmtrx->cols;
	cols = term_doc_matrix->cols;

	if(!(cmtrx = svdNewDMat(rows, cols))) return -1;
	c_value = cmtrx->value;

	for(colnum=0; colnum<cols; colnum++){	// C`s column

		for(rownum=0; rownum<rows; rownum++){	// C`s row
			val = 0.0;
			for(num=0; num<qtmtrx->cols; num++){	// Qt`s row * TD`s column
				val += getSVal(qtmtrx, rownum, num) * getSVal(term_doc_matrix, num, colnum);
			}
			c_value[rownum][colnum] = fabs(val);

		}
	}

	c_matrix->matrix = cmtrx;
	svdFreeSMat(qtmtrx);

	return 0;
}

// free memories allocated to a C matrix
void freeCMatrix(C_MATRIX *matrix)
{
	if(matrix){
		if(matrix->matrix){
			svdFreeDMat(matrix->matrix);
		}
		free(matrix);
	}
}

// create a cluster table
CLUSTER_TABLE * createClusterTable(void)
{
	CLUSTER_TABLE *table;

	table = malloc(sizeof(CLUSTER_TABLE));
	memset(table, 0, sizeof(CLUSTER_TABLE));

	return table;
}

// make a cluster table using the C matrix
int assignDocument(CLUSTER_TABLE *cluster, C_MATRIX *c_matrix, double threshold_assignment)
{
	CLUSTER *clusters, *pcluster;
	DMat cmtrx;
	double **values;
	double value;
	int doc_id, label_id;
	int doc_cnt, label_cnt;
	int cluster_size;


	cmtrx = c_matrix->matrix;
	values = cmtrx->value;
	doc_cnt = cmtrx->cols;
	label_cnt = cmtrx->rows;

	if(doc_cnt < 1) return -1;
	if(label_cnt < 1) return -1;

	cluster_size = MAX(doc_cnt / label_cnt, 1);
	if(label_cnt > 0){
		clusters = malloc(sizeof(CLUSTER)*label_cnt);
		memset(clusters, 0, sizeof(CLUSTER)*label_cnt);
	}

	for(label_id=0; label_id<label_cnt; label_id++){
		for(doc_id=0; doc_id<doc_cnt; doc_id++){
			value = values[label_id][doc_id];
			if(value >= threshold_assignment){
				pcluster = clusters + label_id;

				if(pcluster->count%cluster_size == 0){
					pcluster->docids = realloc(pcluster->docids, sizeof(int)*(pcluster->count+cluster_size));
				}

				pcluster->docids[pcluster->count++] = doc_id;
			}
		}
		//if(pcluster->count > 0) pcluster->docids = realloc(pcluster->docids, sizeof(int)*pcluster->count);
	}

	cluster->clusters = clusters;
	cluster->count = label_cnt;

	return 0;
}

// make a cluster table using the label tree
int makeClusterTable(CLUSTER_TABLE *table, LABEL_TREE *tree)
{
	char *args[2];
	int cluster_cnt, cluster_num;

	cluster_cnt = tree->count;
	cluster_num = 0;
	table->clusters = malloc(sizeof(CLUSTER)*cluster_cnt);
	args[0] = (char*)table;
	args[1] = (char*)&cluster_num;
	traverse_mtrie8(tree->tree, _set_clustertable, args);
	table->count = cluster_cnt;

	return 0;
}

// free memories allocated to a cluster table
void freeClusterTable(CLUSTER_TABLE *table)
{
	CLUSTER *pcluster;
	int cluster_num;

	if(table){
		if(table->clusters){
			for(cluster_num=0; cluster_num<table->count; cluster_num++){
				pcluster = table->clusters + cluster_num;
				if(pcluster->label) free(pcluster->label);
				if(pcluster->docids) free(pcluster->docids);
			}
			free(table->clusters);
		}
		free(table);
	}
}

// create a label tree
LABEL_TREE * createLabelTree(void)
{
	LABEL_TREE *tree;

	if((tree = malloc(sizeof(LABEL_TREE)))){
		if(!(tree->tree = new_mtrie8())){
			free(tree); tree = NULL;
		}
		tree->count = 0;
	}

	return tree;
}

// insert a label to the label tree (for search)
int insertToLabelTree(LABEL_TREE *tree, UTF8 *label, int *docids, int count)
{
	CLUSTER *value;

	if((value = (CLUSTER *)search_mtrie8(tree->tree, label))){
		value->docids = realloc(value->docids, sizeof(int)*(value->count+count));
		memcpy(value->docids + value->count, docids, sizeof(int)*count);
		value->count += count;
	}else{
		value = malloc(sizeof(CLUSTER));
		value->docids = malloc(sizeof(int)*count);
		memcpy(value->docids, docids, sizeof(int)*count);
		value->count = count;

		if(insert_mtrie8(tree->tree, label, (char*)value, sizeof(value)) < 0){
			free(value);
			return -1;
		}
		tree->count++;
		croot_addr_mtrie8(tree->tree);
	}

	return 0;
}

// free memories allocated to a label tree
void freeLabelTree(LABEL_TREE *tree)
{
	if(tree){
		if(tree->tree){
			traverse_mtrie8(tree->tree, _free_labeltree, NULL);
			free(tree->tree);
		}
		free(tree);
	}
}

static int _free_labeltree(UTF8 *key, char *value, char *args[])
{
	if(value) free(value);

	return 0;
}

static int _set_clustertable(UTF8 *key, char *value, char *args[])
{
	CLUSTER_TABLE *table;
	CLUSTER *clusters, *pcluster, *cluster;
	int cluster_num;

	table = (CLUSTER_TABLE*)args[0];
	cluster_num = (*(int*)args[1])++;
	clusters = table->clusters;
	cluster = (CLUSTER *)value;
	pcluster = clusters + cluster_num;

	pcluster->label = malloc(strlen((char*)key)+1);
	strcpy(pcluster->label, (char*)key);
	pcluster->docids = cluster->docids;
	pcluster->count = cluster->count;

	return 0;
}
