#include "my_syntax.h"
#include "zlog.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

tree_node *create_tree_node();
void destory_tree(tree_node *root);
void tree_data_check(tree_node *root);

int save_to_next(tree_node *root, const char *str, int start, int end)
{
	if(root == NULL || start == end || str == NULL){
		dzlog_info("root: %p, start: %d, end: %d, str: %p\n", root, start, end, str);
		return -1;
	}
	if(root->data && root->data->name == NULL){
		root->data->name = calloc(1, end-start +1);
		if(root->data->name == NULL){
			dzlog_info("memory calloc faile: %d", end-start+1);
			return -1;
		}
		strncpy(root->data->name, &str[start], end-start);
		dzlog_info("[%s %d] node data: %s\n", __func__, __LINE__, root->data->name);
		dzlog_info("next node number is 0\n");
		if(root->father)
			dzlog_info("node father :%s\n", root->father->data->name);
	}else{
		tree_node *p = root;
		int i = 1;
		for(; p->next != NULL; p = p->next, i++);
		dzlog_info("next node number is %d\n", i);
		tree_node *node = create_tree_node();
		if(node == NULL){
			dzlog_info("tree node create failed\n");
			return -1;
		}
		node->data->name = calloc(1, end-start +1);
		if(node->data->name == NULL){
			dzlog_info("memory calloc faile: %d", end-start+1);
			return -1;
		}
		strncpy(node->data->name, &str[start], end-start);
		p->next = node;
		dzlog_info("[%s %d] node data: %s\n", __func__, __LINE__, node->data->name);
	}
	return 0;
}

tree_node *string_analysis(const char *str)
{
	if(str == NULL){
		return NULL;
	}
	int len = strlen(str);
	dzlog_info("string len: %d\n", len);
	int i = 0, point = 0;
	int current_level = 0;
	for(; str[i] == ' ' && i < len; i++);
	if(i < len){
		point = i;
		tree_node *root = NULL;
		for(i++; i < len; i++){
			switch(str[i]){
				case '(':
					dzlog_info("[%s %d] str: %s current level: %d\n", __func__, __LINE__, &str[i], current_level);
					if(root == NULL){
						root = create_tree_node();
					}
					save_to_next(root, str, point, i);
					tree_node *child = create_tree_node();
					if(child == NULL){
						dzlog_info("tree node create failed");
						break;
					}
					if(root->child != NULL){
						dzlog_info("%s child is not null!!!!!!!!!!!!!!!!\n", root->data->name);
						destory_tree(root->child);
					}
					root->child = child;
					child->father = root;
					root = child;
					current_level++;
					dzlog_info("node father 000:%s\n", root->father->data->name);
					point = i+1;
					break;
				case ',':
					dzlog_info("[%s %d] str: %s current level: %d\n", __func__, __LINE__, &str[i], current_level);
					if(root == NULL){
						root = create_tree_node();
					}
					save_to_next(root, str, point, i);
					tree_node *next = create_tree_node();
					if(next == NULL){
						dzlog_info("tree node create failed");
						break;
					}
					if(root->next != NULL){
						dzlog_info("%s next is not null!!!!!!!!!!!!!!!!\n", root->data->name);
						destory_tree(root->next);
					}
					root->next = next;
					next->father = root->father;
	
					root = root->next;
					point = i+1;
					break;
				case ')':
					dzlog_info("[%s %d] str: %s current level: %d\n", __func__, __LINE__, &str[i], current_level);
					if(root == NULL){
						root = create_tree_node();
					}
					save_to_next(root, str, point, i);
					root = root->father;
					current_level--;
					point = i+1;
					break;
			}
		}
		if(root == NULL){
			root = create_tree_node();
			save_to_next(root, str, point, i);
		}
		return root;
	}
	return NULL;
}

tree_node *create_tree_node()
{
	tree_node *ret = calloc(1, sizeof(tree_node));
	if(ret == NULL){
		return NULL;
	}
	ret->data = (node_data *)calloc(1, sizeof(node_data));
	if(ret->data == NULL){
		free(ret);
		return NULL;
	}
	return ret;
}

void destory_tree_data(node_data *data)
{
	if(data){
		if(data->name){
			free(data->name);
		}
		free(data);
	}
}

void destory_tree(tree_node *root)
{
	if(root->data){
		destory_tree_data(root->data);
	}
	if(root->child){
		destory_tree(root->child);
	}
	if(root->next){
		destory_tree(root->next);
	}
	free(root);
}
	
void tree_data_check(tree_node *root)
{
	if(root == NULL){
		return;
	}
	if(root->data && root->data->type == 0 && root->data->name){
		int i = 0;
		int len = strlen(root->data->name);
		for(; i < len && root->data->name[i] == ' '; i++);
		if(root->data->name[i] == '$' || i > 0){
			char *replace_str = calloc(1, len-i);
			if(replace_str == NULL){
				dzlog_info("calloc failed: %d", len-1);
				return;
			}
			if(root->data->name[i] == '$'){
				strncpy(replace_str, &root->data->name[i+1], len-i-1);
				root->data->type = VARIABLE;
			}else{
				strncpy(replace_str, &root->data->name[i], len-i);
				root->data->type = CONST;
			}
			free(root->data->name);
			root->data->name = replace_str;
		}else{
			root->data->type = CONST;
		}
	}
	tree_data_check(root->next);
	tree_data_check(root->child);
}

int level = 0;
void tree_data_show(node_data *data)
{
	if(data){
		dzlog_info("name: %s, type: %d\n", data->name, data->type);
	}
}

void tree_node_show(tree_node *root)
{
	if(root == NULL){
		return;
	}
	tree_data_show(root->data);
	tree_node_show(root->next);
	dzlog_info("%s child: %p\n", root->data->name, root->child);
	if(root->child){
		level++;
		tree_node_show(root->child);
	}
}

#if 0
int main()
{
	char *test_str[] = {"$abc", "$stt($abc)", "$aabc($abcdd, $deff)", "$a($b($c, $d($e, $f), $g), $h($i, $j, $k))", "$1($2(3, $4, 5), 6($7, 8), $9(10,$11))"};
	int i = 0;
	while(1){
		tree_node *tree = string_analysis(test_str[i]);
		tree_data_check(tree);
		tree_node_show(tree);
		destory_tree(tree);
		sleep(1);
		i++;
		i %= sizeof(test_str)/sizeof(char *);
	}
	return 0;
}
#endif

cJSON *test(cJSON *params)
{
	return cJSON_Duplicate(cJSON_GetArrayItem(params, 0), 1);
}

//待优化：依赖外部代码
typedef struct{
    int r;
    int g;
    int b;
}RgbColor;

typedef struct{
    int h;
    int s;
    int l;
}HslColor;
extern int int2Rgb(int rgb_hex, RgbColor *rgb);
extern void RGB2HSL(RgbColor *rgb, HslColor *hsl);

cJSON *std_RGB2HSL(cJSON *params)
{
	if(params == NULL){
		return NULL;
	}
	if(params->type == cJSON_Array){
		int len = cJSON_GetArraySize(params);
		if(len == 0){
			return NULL;
		}
		cJSON *item = cJSON_GetArrayItem(params, 0);
		int color = 0;
		if(item->type == cJSON_Number){
			color = item->valueint;
		}else if(item->type == cJSON_String){
			color = atoi(item->valuestring);
		}
		RgbColor rgb = {0};
		HslColor hsl = {0};
		int2Rgb(color, &rgb);
		RGB2HSL(&rgb, &hsl);
		cJSON *ret = cJSON_CreateObject();
		cJSON_AddNumberToObject(ret,"H",hsl.h);
		cJSON_AddNumberToObject(ret,"S",hsl.s);
		cJSON_AddNumberToObject(ret,"L",hsl.l);
		return ret;
	}
	return NULL;
}

cJSON *HSL(cJSON *params)
{
	if(params == NULL){
		return NULL;
	}
	if(params->type == cJSON_Array){
		int len = cJSON_GetArraySize(params);
		if(len == 0){
			return NULL;
		}
		cJSON *item = cJSON_GetArrayItem(params, 0);
		int color = 0;
		if(item->type == cJSON_Number){
			color = item->valueint;
		}else if(item->type == cJSON_String){
			color = atoi(item->valuestring);
		}
		RgbColor rgb = {0};
		HslColor hsl = {0};
		int2Rgb(color, &rgb);
		RGB2HSL(&rgb, &hsl);
		cJSON *ret = cJSON_CreateObject();
		cJSON_AddNumberToObject(ret,"H",hsl.h);
		cJSON_AddNumberToObject(ret,"S",1000);
		cJSON_AddNumberToObject(ret,"L",1000);
		return ret;
	}
	return NULL;
}

cJSON *negative(cJSON *params)
{
	if(params == NULL){
		return NULL;
	}
	if(params->type == cJSON_Array){
		int len = cJSON_GetArraySize(params);
		if(len == 0){
			return NULL;
		}
		cJSON *item = cJSON_GetArrayItem(params, 0);
		if(item->type == cJSON_Number){
			int value = -item->valueint;
			return cJSON_CreateNumber(value);
		}else if(item->type == cJSON_String){
			int value = -atoi(item->valuestring);
			return cJSON_CreateNumber(value);
		}
	}
	return NULL;
}

typedef struct grammar_cbk{
	const char *name;
	cJSON *(*cbk)(cJSON *params);
}grammar_cbk;

grammar_cbk gc[] = {
	{"negative", negative},
	{"HSL", HSL},
	{"std_RGB2HSL", std_RGB2HSL}
};

cJSON *grammar_tree_travel(tree_node *grammar_tree, const cJSON *data)
{
	if(grammar_tree == NULL){
		return NULL;
	}
	//用先序遍历，从左往右
	//最下层叶子节点数据先扩展为真实值，往上每层根节点扩展为回调函数并执行返回结果给再上层
	tree_node *node = grammar_tree;
	cJSON *child_result = NULL;
	if(node->child){
		child_result = grammar_tree_travel(node->child, data);
	}
	cJSON *ret = cJSON_CreateArray();
	if(child_result == NULL){
		if(node->data && node->data->type == VARIABLE && data){
			cJSON *item = cJSON_GetObjectItem(data, node->data->name);
			if(item){
				cJSON_AddItemToArray(ret, cJSON_Duplicate(item, 1));
			}else{
				cJSON_AddItemToArray(ret, cJSON_CreateString(node->data->name));
			}
		}else{
			cJSON_AddItemToArray(ret, cJSON_CreateString(node->data->name));
		}
	}else{
		if(node->data && node->data->type == VARIABLE){
			int flag = 0;
			for(int i = 0; i < sizeof(gc)/sizeof(grammar_cbk); i++){
				if(strcmp(node->data->name, gc[i].name) == 0){
					cJSON_AddItemToArray(ret, gc[i].cbk(child_result)); 
					flag = 1;
					break;
				}
			}
			if(flag == 0){
				dzlog_warn("%s is not known as callback", node->data->name);
				cJSON_AddItemToArray(ret, cJSON_CreateString(node->data->name));
			}
		}else{
			dzlog_warn("grammar error: child exist, root is not variable");
			if(node->data){
				cJSON_AddItemToArray(ret, cJSON_CreateString(node->data->name));
			}else{
				cJSON_AddItemToArray(ret, cJSON_CreateNull());
			}
		}
		cJSON_Delete(child_result);
	}
	if(node->next){
		cJSON *next_result = grammar_tree_travel(node->next, data);
		if(next_result){
			int len = cJSON_GetArraySize(next_result);
			//注意：组合当前节点和同一层级其他节点的数据时，要维持数组内成员的顺序不变
			for(int i = 0; i < len; i++){
				cJSON *item = cJSON_GetArrayItem(next_result, i);
				cJSON_AddItemToArray(ret, cJSON_Duplicate(item, 1));
			}
			cJSON_Delete(next_result);
		}
	}
	return ret;
}

cJSON *run_grammar(const char *code, const cJSON *data)
{
    tree_node *grammar_tree = string_analysis(code);
	tree_data_check(grammar_tree);

	cJSON *result = grammar_tree_travel(grammar_tree, data);
	destory_tree(grammar_tree);
	if(result == NULL){
		dzlog_error("run grammar has no result");
	}
	char *str = cJSON_PrintUnformatted(result);
	if(str){
		dzlog_info("run grammar result: %s", str);
		free(str);
	}
	if(result->type != cJSON_Array){
		dzlog_error("run grammar result format error");
		cJSON_Delete(result);
		return NULL;
	}
	int len = cJSON_GetArraySize(result);
	if(len > 1){
		dzlog_warn("run grammar result error: result number: %d", len);
		return NULL;
	}else{
		cJSON *item = cJSON_Duplicate(cJSON_GetArrayItem(result, 0), 1);
		cJSON_Delete(result);
		return item;
	}
}