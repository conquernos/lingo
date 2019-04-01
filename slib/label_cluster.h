#ifndef _LABEL_CLUSTER_H
#define _LABEL_CLUSTER_H

#include "svdlib.h"
#include "label_store.h"
#include "phrase_match.h"

/* column:label, row:term, value:similarity
   Q = P
 */
typedef struct _Q_MATRIX{
	SMat matrix;
} Q_MATRIX;

/* column:doc, row:label, value:strength of membership
   C = QtㆍA
 */
typedef struct _C_MATRIX{
	DMat matrix;
} C_MATRIX;

/* table maching the used label and the total label */
typedef struct _LABEL_MATCH{
	int *label_ids;
	int count;
} LABEL_MATCH;

typedef struct _CLUSTER{
	char *label;
	int *docids;
	int count;
} CLUSTER;

/* Cluster table
   ClusterID -> Label, DocIDs */
typedef struct _CLUSTER_TABLE{
	CLUSTER *clusters;
	int count;
} CLUSTER_TABLE;

/* Trie for searching a label
   Label -> Cluster */
typedef struct _LABEL_TREE{
	mTRIE8 *tree;	// key : UTF8 *label, value : CLUSTER
	int count;
} LABEL_TREE;

// Q matrix
Q_MATRIX * createQMatrix(void);
int makeQMatrix(Q_MATRIX *q_matrix, LABEL_TABLE *label_table, P_MATRIX *p_matrix, LABEL_MATCH *match);
double getQMatrixValue(Q_MATRIX *matrix, long row, long col);
void freeQMatrix(Q_MATRIX *matrix);

// C matrix
C_MATRIX * createCMatrix(void);
int makeCMatrix(C_MATRIX *c_matrix, Q_MATRIX *q_matrix, SMat term_doc_matrix);
void freeCMatrix(C_MATRIX *matrix);

// cluster
CLUSTER_TABLE * createClusterTable(void);
int assignDocument(CLUSTER_TABLE *table, C_MATRIX *c_matrix, double threshold_assignment);
int makeClusterTable(CLUSTER_TABLE *table, LABEL_TREE *tree);
void freeClusterTable(CLUSTER_TABLE *table);

/* label tree */
LABEL_TREE * createLabelTree(void);
int insertToLabelTree(LABEL_TREE *tree, UTF8 *label, int *docids, int count);
void freeLabelTree(LABEL_TREE *tree);


#endif
