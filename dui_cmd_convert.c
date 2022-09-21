#include "cJSON.h"
#include "zlog.h"
#include <stdio.h>
#include <string.h>
#include "my_syntax.h"
#include "dui_cmd_convert.h"

int str_larger(char *s1, char *s2)
{
	int l1 = strlen(s1);
	int l2 = strlen(s2);
	int len = l1 < l2 ? l1: l2;
	int i = 0;
	for(i = 0; i < len; i++){
		if(s1[i] > s2[i]){
			return 1;
		}else if(s1[i] < s2[i]){
			return 0;
		}
	}
	if(i == len){
		return l1 > l2;
	}
    return 0;
}

void swap(char **st1, char **st2)
{
	char *t = *st1;
	*st1 = *st2;
	*st2 = t;
}

int partition(char *str[], int low, int high)
{
	char *pivot = str[high];
	int i = low -1;
	for(int j = low; j < high; j++){
		if(0 == str_larger(str[j], pivot)){
			i++;
			swap(&str[i], &str[j]);
		}
	}
	swap(&str[i+1], &str[high]);
	return (i+1);
}

void quick_sort(char *str[], int low, int high)
{
	if(low < high){
		int pi = partition(str, low, high);
		quick_sort(str, low, pi-1);
		quick_sort(str, pi+1, high);
	}
}

void print_str(char *str[], int count)
{
	for(int i = 0; i < count; i++){
		printf("%s\t", str[i]);
	}
	printf("\n");
}


void sort(char *str[], int count)
{
    quick_sort(str, 0, count-1);
}

//json数据相同，返回0，不同，返回1，流程问题，返回-1
int json_compare(const cJSON *src, const cJSON *dst)
{
    cJSON *child = NULL;
    int src_count = 0;
    for(child = src->child; child != NULL; child = child->next, src_count++);
    int dst_count = 0;
    for(child = dst->child; child != NULL; child = child->next, dst_count++);
    if(src_count != dst_count || src_count == 0){
        //dzlog_info("json is not same");
        return 1;
    }
    char **src_keys = (char **)calloc(src_count, sizeof(char *));
    char **dst_keys = (char **)calloc(dst_count, sizeof(char *));
    if(src_keys == NULL || dst_keys == NULL){
        dzlog_error("%d memory calloc failed", src_count);
        return -1;
    }

    int i = 0;
    for(i = 0, child = src->child; child != NULL; child = child->next, i++){
        src_keys[i] = child->string;
        //dzlog_info("key[%d]: %s", i, src_keys[i]);
    }
    sort(src_keys, src_count);
    for(i = 0, child = dst->child; child != NULL; child = child->next, i++){
        dst_keys[i] = child->string;
        //dzlog_info("key[%d]: %s", i, dst_keys[i]);
    }
    sort(dst_keys, dst_count);
    cJSON *src_sorted = cJSON_CreateObject();
    cJSON *dst_sorted = cJSON_CreateObject();
    for(i = 0; i < src_count; i++){
        cJSON_AddItemToObject(src_sorted, src_keys[i], cJSON_Duplicate(cJSON_GetObjectItem(src, src_keys[i]), 1));
        cJSON_AddItemToObject(dst_sorted, dst_keys[i], cJSON_Duplicate(cJSON_GetObjectItem(dst, dst_keys[i]), 1));
    }
    char *src_str = cJSON_PrintUnformatted(src_sorted);
    char *dst_str = cJSON_PrintUnformatted(dst_sorted);
    cJSON_Delete(src_sorted);
    cJSON_Delete(dst_sorted);
    if(src_str && dst_str){
        if(strcmp(src_str, dst_str) == 0){
            //dzlog_info("json is the same: %s, %s", src_str, dst_str);
            free(src_str);
            free(dst_str);
            return 0;
        }else{
            //dzlog_info("json is not the same");
            free(src_str);
            free(dst_str);
            return 1;
        }
    }
    //dzlog_info("json is null :not the same");
    if(src_str){
        free(src_str);
    }
    if(dst_str){
        free(dst_str);
    }
    return 1;
}

cJSON *data_variable_format(const cJSON *class_intent, const cJSON *vispeech_intent)
{
    cJSON *dui_format = cJSON_CreateObject();
    cJSON *child = NULL;
    for(child = vispeech_intent->child; child != NULL; child = child->next){
        cJSON *cmd_item = cJSON_GetObjectItem(class_intent, child->string);
        if(cmd_item && cmd_item->type == cJSON_String && cmd_item->valuestring[0] == '$'){
            cJSON_AddItemToObject(dui_format, child->string, cJSON_Duplicate(cmd_item, 1));
        }else{
            cJSON_AddItemToObject(dui_format, child->string, cJSON_Duplicate(child, 1));
        }
    }
    return dui_format;
}

//json数据相同，返回0，不同，返回1，流程问题，返回-1
int dui_data_compare(const cJSON *class_intent, const cJSON *vispeech_intent)
{
    if(class_intent == NULL || vispeech_intent == NULL){
        dzlog_warn("param error: dui: %p, cmd: %p", class_intent, vispeech_intent);
        return -1;
    }

    cJSON *dui_format = data_variable_format(class_intent, vispeech_intent);
    int ret = json_compare(dui_format, class_intent);
    cJSON_Delete(dui_format);
    return ret;
}

cJSON *json_variable_extend(const cJSON *variable, const cJSON *data_src)
{
    if(variable == NULL){
        return NULL;
    }
    if(data_src == NULL){
        return cJSON_Duplicate(variable, 1);
    }
    cJSON *ret = NULL;
    if(variable->type == cJSON_Object){
        ret = cJSON_CreateObject();
    }else if(variable->type == cJSON_Array){
        ret = cJSON_CreateArray();
    }
    cJSON *child = NULL;
    for(child = variable->child; child != NULL; child = child->next){
        if(child->type == cJSON_String){
            cJSON *key = NULL, *value = NULL;
            char *key_str = child->string;
            if(child->valuestring && child->valuestring[0] == '$'){
                value = run_grammar((const char *)child->valuestring, data_src);
            }
            if(child->string && child->string[0] == '$'){
                key = run_grammar((const char *)child->string, data_src);
            }
            if(key && key->type == cJSON_String){
                key_str = key->valuestring;
            }

            if(value){
                if(ret->type == cJSON_Object){
                    cJSON_AddItemToObject(ret, key_str, value);
                }else if(ret->type == cJSON_Array){
                    cJSON_AddItemToArray(ret, value);
                }else{
                    cJSON_Delete(value);
                }
            }else{
                if(ret->type == cJSON_Object){
                    cJSON_AddItemToObject(ret, key_str, cJSON_Duplicate(child, 0));
                }else if(ret->type == cJSON_Array){
                    cJSON_AddItemToArray(ret, cJSON_Duplicate(child, 0));
                }
            }
            if(key){
                cJSON_Delete(key);
            }
        }else if(child->type == cJSON_Object || child->type == cJSON_Array){
            cJSON *d = json_variable_extend(child, data_src);
            if(d && ret->type == cJSON_Object){
                cJSON_AddItemToObject(ret, child->string, d);
            }else if(d && ret->type == cJSON_Array){
                cJSON_AddItemToArray(ret, d);
            }else if(d){
                cJSON_Delete(d);
            }
        }else{
            if(ret->type == cJSON_Object){
                cJSON_AddItemToObject(ret, child->string, cJSON_Duplicate(child, 0));
            }else if(ret->type == cJSON_Array){
                cJSON_AddItemToArray(ret, cJSON_Duplicate(child, 0));
            }
        }
    }
    return ret;
}

#if 0
cJSON *travel_json(const cJSON *item)
{
    if(item->type == cJSON_Object){
        travel_json(item->child);
        travel_json(item->next);
    }else if(item->type == cJSON_String){
        variable_replace(item)
    }else{
        //do nothing;
    }
}
						
cJSON *cmd_variable_replace(const cJSON *cmd, const cJSON *dui_param)
{
    if(cmd == NULL || dui_param == NULL){
        dzlog_warn("param error: cmd : %p, dui_param: %p", cmd, dui_param);
        return NULL;
    }

}
#endif

cJSON *get_cmd_device_param(const cJSON *cmd_param)
{
    if(cmd_param == NULL){
        return NULL;
    }
    char *name_list[] = {"roomName", "floorName", "deviceType", "deviceName", "scope", "deviceId", "roomId", "floorId", "online"};
    cJSON *root = NULL;
    for(int i = 0; i < sizeof(name_list)/sizeof(char *); i++){
        cJSON *name = cJSON_GetObjectItem(cmd_param, name_list[i]);
        if(name && name->type != cJSON_NULL){
            if(root == NULL){
                root = cJSON_CreateObject();
            }
            cJSON_AddItemToObject(root, name_list[i], cJSON_Duplicate(name, 0));
        }
    }
    return root;
}

cJSON *get_cmd_property_param(cJSON *cmd_param)
{
    if(cmd_param == NULL){
        return NULL;
    }
    char *name_list[] = {"property", "mode", "determiner", "unit"};
    cJSON *root = NULL;
    for(int i = 0; i < sizeof(name_list)/sizeof(char *); i++){
        cJSON *name = cJSON_GetObjectItem(cmd_param, name_list[i]);
        if(name && name->type != cJSON_NULL){
            if(root == NULL){
                root = cJSON_CreateObject();
            }
            cJSON_AddItemToObject(root, name_list[i], cJSON_Duplicate(name, 0));
        }
    }
    return root;
}

void cmd_json_content_check(cJSON *root)
{
    if(root == NULL){
        return;
    }
    cJSON *p = root->child;
    while(p){
        cJSON *n = p->next;
        if(p->type == cJSON_NULL){
            cJSON_DeleteItemFromObject(root, p->string);
        }else if(p->type == cJSON_Object){
            cmd_json_content_check(p);
        }
        p = n;
    }
}

int is_name_exist(const cJSON *list, const char *name)
{
	int len = cJSON_GetArraySize(list);
	for(int i = 0; i < len; i++){
		cJSON *item = cJSON_GetArrayItem(list, i);
		if(strcmp(item->valuestring, name) == 0){
			return 1;
		}
	}
	return 0;
}

int is_alias_exist(const cJSON *item, const char *name)
{
	if(item == NULL || name == NULL){
		return -1;
	}
	cJSON *alias = cJSON_GetObjectItem(item, "alias");
	if(alias == NULL){
		//dzlog_warn("alias is not exist");
		return 0;
	}else{
		return is_name_exist((const cJSON *)alias, name);
	}
}

//determiner:0:-, 1:+
int search_voice_determiner(const cJSON *voice_properties, const char *property, const char *determiner)
{
	if(property == NULL || determiner == NULL){
		return -1;
	}
	int len = voice_properties == NULL ? 0: cJSON_GetArraySize(voice_properties);
	for(int i = 0; i < len; i++){
		cJSON *prop = cJSON_GetArrayItem(voice_properties, i);
		cJSON *name = cJSON_GetObjectItem(prop, "name");
		if(name && strcmp(name->valuestring, property) == 0){
			cJSON *determiner_down = cJSON_GetObjectItem(prop, "determiner_down");
			cJSON *determiner_up = cJSON_GetObjectItem(prop, "determiner_up");
			int determiner_flag = -1;
			if(determiner_down != NULL && determiner_up != NULL){
				determiner_flag = is_name_exist((const cJSON *)determiner_down, determiner);
				if(determiner_flag == 1){
					return 0;
				}
				determiner_flag = is_name_exist((const cJSON*)determiner_up, determiner);
				if(determiner_flag == 1){
					return 1;
				}
				return -1;
			}
		}
	}
	return -1;
}

const char *search_voice_mode(const cJSON *voice_properties, const char *property, const char *mode_alias)
{
	if(property == NULL || mode_alias == NULL){
		return NULL;
	}
	int len = voice_properties == NULL ? 0: cJSON_GetArraySize(voice_properties);
	for(int i = 0; i < len; i++){
		cJSON *prop = cJSON_GetArrayItem(voice_properties, i);
		cJSON *name = cJSON_GetObjectItem(prop, "name");
		if(name && strcmp(name->valuestring, property) == 0){
			cJSON *mode = cJSON_GetObjectItem(prop, "mode");
			if(mode != NULL){
                if(mode->type == cJSON_Array){
                    int l = cJSON_GetArraySize(mode);
                    for(int j = 0; j < l; j++){
                        cJSON *item = cJSON_GetArrayItem(mode, j);
                        cJSON *alias = cJSON_GetObjectItem(item, "alias");
                        int n = alias != NULL ? cJSON_GetArraySize(alias): 0;
                        for(int k = 0; k < n; k++){
                            cJSON *a = cJSON_GetArrayItem(alias, k);
                            if(strcmp(a->valuestring, mode_alias) == 0){
                                cJSON *ret = cJSON_GetObjectItem(item, "name");
                                if(ret){
                                    return ret->valuestring;
                                }else{
                                    return NULL;
                                }
                            }
                        }
                    }
                }else if(mode->type == cJSON_String){
                    if(strcmp(mode->valuestring, "UNLIMITED") == 0){
                        return mode_alias;
                    }
                }
			}
		}
	}
	return NULL;
}

cJSON *get_voice_properties(const cJSON *voice_properties, const char *property, const char *mode, const char *unit, const char *determiner)
{
	if(property == NULL && mode == NULL && unit == NULL && determiner == NULL){
		dzlog_info("all params are null");
		return NULL;
	}
	cJSON *ret = cJSON_CreateArray();
	int property_flag = -1, mode_flag = -1, unit_flag = -1, determiner_flag = -1;
	int len = voice_properties == NULL ? 0: cJSON_GetArraySize(voice_properties);
	for(int i = 0; i < len; i++){
		cJSON *prop = cJSON_GetArrayItem(voice_properties, i);
		cJSON *name = cJSON_GetObjectItem(prop, "name");
		if(name == NULL){
			dzlog_warn("voice properties json item has no name");
			continue;
		}
		//dzlog_info("property name: %s", name->valuestring);
		if(property){
			property_flag = is_alias_exist(prop, property);
		}

		if(mode != NULL){
            //如果property不存在，property可以通过mode得到，但是要避免mode==UNLIMITED这种情况
			cJSON *mo = cJSON_GetObjectItem(prop, "mode");
			if(mo != NULL){
                if(mo->type == cJSON_Array){
                    int mode_len = cJSON_GetArraySize(mo);
                    for(int j = 0; j < mode_len; j++){
                        cJSON *mode_item = cJSON_GetArrayItem(mo, j);
                        mode_flag = is_alias_exist(mode_item, mode);
                        if(mode_flag == 1){
                            break;
                        }
                    }
                }else if(mo->type == cJSON_String){
                    if(property_flag == 1 && strcmp(mo->valuestring, "UNLIMITED") == 0){
                        mode_flag = 1;
                    }
                }
			}else{
				mode_flag = 0;
			}
		}
		if(unit != NULL){
			cJSON *un = cJSON_GetObjectItem(prop, "unit");
			if(un != NULL){
				unit_flag = is_name_exist((const cJSON *)un, unit);
			}else{
				unit_flag = 0;
			}
		}
		if(determiner != NULL){
			cJSON *determiner_down = cJSON_GetObjectItem(prop, "determiner_down");
			cJSON *determiner_up = cJSON_GetObjectItem(prop, "determiner_up");
			if(determiner_down != NULL && determiner_up != NULL){
				determiner_flag = is_name_exist((const cJSON *)determiner_down, determiner);
				if(determiner_flag != 1){
					determiner_flag = is_name_exist((const cJSON*)determiner_up, determiner);
				}
			}else{
				determiner_flag = 0;
			}
		}
		if(property_flag != 0 && mode_flag != 0 && unit_flag != 0 && determiner_flag != 0){
			cJSON_AddItemToArray(ret, cJSON_Duplicate(name, 0));
		}
	}
	len = cJSON_GetArraySize(ret);
	if(len == 0){
		cJSON_Delete(ret);
		ret = NULL;
	}
	return ret;
}