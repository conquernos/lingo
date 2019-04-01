#ifndef _LABEL_SELECT_H
#define _LABEL_SELECT_H

#include "label_store.h"
#include "feature_store.h"
#include "term_store.h"

// label pruning
int pruneLabel(LABEL_TABLE *labeltable, PHRASE *phrases, TERM *terms, int doccnt, double sim_threshold, double score_threshold, enum weight_type type);

#endif
