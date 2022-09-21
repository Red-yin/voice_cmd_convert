#ifndef __MY_SYNTAX_H__
#define __MY_SYNTAX_H__

#include "cJSON.h"
enum data_type{
	CONST = 1,
	VARIABLE
};

typedef struct node_data{
	enum data_type type;
	char *name;
}node_data;

typedef struct tree_node{
	node_data *data;
	struct tree_node *father;
	struct tree_node *child;
	struct tree_node *next;
}tree_node;

cJSON *run_grammar(const char *code, const cJSON *data);
#endif
