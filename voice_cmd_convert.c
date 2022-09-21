#include "cJSON.h"
#include "zlog.h"
#include "device_info_format.h"
#include <string.h>

typedef void *(*cbk_type)(void *param);
typedef struct global_func{
	cJSON *(*search_device)(cJSON *param);
	cJSON *(*get_class_voice_config)(cJSON *param);
	cJSON *(*get_class_voice_properties)(cJSON *param);
}global_func;
static global_func g_func = {0};

typedef struct cbk_t{
	char *name;
	void *(**cbk)(void *param);
}cbk_t;
cbk_t g_cbk[] = {
	{"search_device", (cbk_type *)&(g_func.search_device)},
	{"get_class_voice_config", (cbk_type *)&(g_func.get_class_voice_config)},
	{"get_class_voice_properties", (cbk_type *)&(g_func.get_class_voice_properties)}
};

void register_callback(const char *name, void *(*cbk)(void *param))
{
	for(int i = 0; i < sizeof(g_cbk)/sizeof(cbk_t); i++){
		if(strcmp(g_cbk[i].name, name) == 0){
			*g_cbk[i].cbk = cbk;
			break;
		}
	}
}

void print_json(const cJSON *js)
{
	char *str = cJSON_PrintUnformatted(js);
	if(str){
		dzlog_info("%s", str);
		free(str);
	}
}

void print_device_number(cJSON *dev_list)
{
	if(dev_list == NULL){
		dzlog_info("dev list is null");
		return;
	}
	print_json(dev_list);
	int dev_count = cJSON_GetArraySize(dev_list);
	dzlog_info("dev number: %d", dev_count);
}

#include "dui_cmd_convert.h"
#if 0
cJSON *device_info_format_in_voice(cJSON *data, logic_dev_t *dev)
{
	//如果设备信息格式转换成功，返回一个新的JSON数据；如果转换失败，返回传入的data
	if(data && dev->dev_class->vui->device_info){
		cJSON *format_dev_js = voice_device_info_format(dev->dev_class->vui->device_info, (const cJSON *)data);
		if(format_dev_js != NULL){
			dzlog_info("device_control %s data formated", dev->name);
			return format_dev_js;
		}else{
			dzlog_info("device_control %s data not formated: format result is null", dev->name);
			return data;
		}
	}else{
		dzlog_info("device_control %s data not formated: device info is null", dev->name);
		return data;
	}
}
#endif

cJSON *get_device_cmd(cJSON *class_list, cJSON *class_prop_js)
{
	if(class_list == NULL || class_prop_js == NULL){
		dzlog_warn("class list is %p, class prop js is %p", class_list, class_prop_js);
		return NULL;
	}
	int len = cJSON_GetArraySize(class_prop_js);
	int class_list_len = cJSON_GetArraySize(class_list);
	cJSON *ret = NULL;
	for(int i = 0; i < len; i++){
		cJSON *prop_item = cJSON_GetArrayItem(class_prop_js, i);
		cJSON *intent = cJSON_GetObjectItem(prop_item, "intent");
		cJSON *class = cJSON_GetObjectItem(prop_item, "class");
		if(intent == NULL || class == NULL){
			dzlog_warn("class prop js has no intent or class");
			continue;
		}
		for(int j = 0; j < class_list_len; j++){
			cJSON *class_info = cJSON_GetArrayItem(class_list, j);
			cJSON *class_name = cJSON_GetObjectItem(class_info, "class");
			if(class_name && strcmp(class_name->valuestring, class->valuestring) == 0){
				cJSON *map_list = cJSON_GetObjectItem(class_info, "map_list");
				if(map_list){
					cJSON *map_item = NULL;
					int flag = 0;
					cJSON_ArrayForEach(map_item, map_list){
						cJSON *map_intent = cJSON_GetObjectItem(map_item, "intent");
						if(map_intent != NULL && 0 == dui_data_compare((const cJSON *)map_intent, intent)){
							cJSON *cmd = cJSON_GetObjectItem(map_item, "cmd");
							if(cmd){
								if(ret == NULL){
									ret = cJSON_CreateArray();
								}
								flag = 1;
								cJSON *final_cmd = json_variable_extend((const cJSON *)cmd, (const cJSON *)intent);
								cJSON *obj = cJSON_CreateObject();
								cJSON_AddItemToObject(obj, "class", cJSON_Duplicate(class, 0));
								cJSON_AddItemToObject(obj, "intent", cJSON_Duplicate(intent, 1));
								cJSON_AddItemToObject(obj, "cmd", final_cmd);
								cJSON_AddItemToArray(ret, obj);
							}else{
								dzlog_warn("%s map list has no cmd", class_name->valuestring);
							}
							break;
						}
					}
					if(flag == 0){
						dzlog_info("intent is not exist in %s, %d", class->valuestring, j);
					}
				}
			}
		}
	}
	return ret;
}

#if 0
void class_prop_check_by_map_list(orb_list_t *dev_list, cJSON *class_prop_js, cJSON *dui_param)
{
	if (dev_list == NULL || class_prop_js == NULL || dui_param == NULL){
		dzlog_warn("%s dev_list is %p, class_prop_js is %p", __func__, dev_list, class_prop_js);
		return ;
	}

	char *debug = cJSON_PrintUnformatted(dui_param);
	if(debug){
		dzlog_info("duiparam: %s", debug);
		free(debug);
	}
	cJSON *cmd_param = cJSON_CreateObject();
	cJSON *property = cJSON_GetObjectItem(dui_param, "property");
	if(property && property->type != cJSON_NULL){
		cJSON_AddItemToObject(cmd_param, "property", cJSON_Duplicate(property, 1));
	}
	cJSON *operate = cJSON_GetObjectItem(dui_param, "operate");
	if(operate && operate->type != cJSON_NULL){
		cJSON_AddItemToObject(cmd_param, "operate", cJSON_Duplicate(operate, 1));
	}
	cJSON *mode = cJSON_GetObjectItem(dui_param, "mode");
	if(mode && mode->type != cJSON_NULL){
		cJSON_AddItemToObject(cmd_param, "mode", cJSON_CreateString("$mode"));
	}
	cJSON *number = cJSON_GetObjectItem(dui_param, "number");
	if(number && number->type != cJSON_NULL){
		cJSON_AddItemToObject(cmd_param, "number", cJSON_CreateString("$number"));
	}

	cJSON *class_item_prop_js = NULL;
	cJSON_ArrayForEach(class_item_prop_js,class_prop_js){
		cJSON *class = cJSON_GetObjectItem(class_item_prop_js, "class");
		if(class == NULL){
			dzlog_error("class prop has no class");
			continue;
		}
		dzlog_info("class: %s", class->valuestring);
		orb_list_item_t *item = orb_list_item_create(dev_list);
		logic_dev_t *dev = orb_list_item_next(item);
		while(dev){
			dzlog_info("dev class: %s", dev->dev_class->name);
			if(dev->dev_class && dev->dev_class->vui && strcmp(dev->dev_class->name, class->valuestring) == 0 && dev->dev_class->vui->map_list != NULL){
				cJSON *map_item = NULL;
				cJSON_ArrayForEach(map_item, dev->dev_class->vui->map_list){
					cJSON *dui = cJSON_GetObjectItem(map_item, "dui");
					debug = cJSON_PrintUnformatted(dev->dev_class->vui->map_list);
					if(debug){
						dzlog_info("maplist: %s", debug);
						free(debug);
					}
					if(dui != NULL && 0 == dui_data_compare((const cJSON *)dui, cmd_param)){
					//如果DUI命令和配置的内容一样，用map_list里面的cmd把class_prop的命令替换掉
						cJSON_DeleteItemFromObject(class_item_prop_js, "properties");
						debug = cJSON_PrintUnformatted(class_item_prop_js);
						if(debug){
							dzlog_info("class item: %s", debug);
							free(debug);
						}
						cJSON *cmd = cJSON_GetObjectItem(map_item, "cmd");
						if(cmd == NULL){
							dzlog_error("map list has no cmd");
							break;
						}
						//cJSON *new_cmd = cmd_variable_replace(cmd, dui_param);
						//cJSON_AddItemToArray(properties, new_cmd);
						cJSON_AddItemToObject(class_item_prop_js, "properties", cJSON_Duplicate(cmd, 1));
						dzlog_info("[%s] class prop replace by map list cmd ", dev->dev_class->name);
						debug = cJSON_PrintUnformatted(class_item_prop_js);
						if(debug){
							dzlog_info("class item: %s", debug);
							free(debug);
						}
						break;
					}
				}
				break;
			}
			dev = orb_list_item_next(item);
		}
		orb_list_item_destroy(item);
	}
	if(cmd_param){
		cJSON_Delete(cmd_param);
	}
	return;
}
#endif

int get_device_intent(cJSON *class_prop, cJSON *cmd_js)
{
	if(class_prop == NULL){
		dzlog_warn("class prop is NULL");
		return 0;
	}
	
	cJSON *mode = cJSON_GetObjectItem(cmd_js, "mode");
	cJSON *number = cJSON_GetObjectItem(cmd_js, "number");
	cJSON *determiner = cJSON_GetObjectItem(cmd_js, "determiner");
	cJSON *operate = cJSON_GetObjectItem(cmd_js, "operate");
	int len = cJSON_GetArraySize(class_prop);
	int ret = 0;
	for(int i = 0; i < len; i++){
		cJSON *item = cJSON_GetArrayItem(class_prop, i);
		cJSON *intent = cJSON_CreateObject();
		cJSON *property = cJSON_GetObjectItem(item, "property");
		if(property){
			cJSON_AddItemToObject(intent, "property", cJSON_Duplicate(property, 0));
		}
		if(property && determiner && determiner->type != cJSON_NULL){
			int flag = search_voice_determiner(g_func.get_class_voice_properties(NULL), property->valuestring, determiner->valuestring);
			//int flag = search_determiner(property->valuestring, determiner->valuestring);
			if(flag == 0){
				//decrease
				if(operate && strcmp(operate->valuestring, "limit") == 0){
					cJSON_AddItemToObject(intent, "operate", cJSON_CreateString("min"));
				}else if(operate && strcmp(operate->valuestring, "stepdown") == 0){
					cJSON_AddItemToObject(intent, "operate", cJSON_CreateString("increase"));
				}else{
					cJSON_AddItemToObject(intent, "operate", cJSON_CreateString("decrease"));
				}
			}else if(flag == 1){
				//increase
				if(operate && strcmp(operate->valuestring, "limit") == 0){
					cJSON_AddItemToObject(intent, "operate", cJSON_CreateString("max"));
				}else if(operate && strcmp(operate->valuestring, "stepdown") == 0){
					cJSON_AddItemToObject(intent, "operate", cJSON_CreateString("decrease"));
				}else{
					cJSON_AddItemToObject(intent, "operate", cJSON_CreateString("increase"));
				}
			}else{
				dzlog_warn("%s has no determiner named :%s", property->valuestring, determiner->valuestring);
				ret = -1;
			}
		}else if(operate){
			cJSON_AddItemToObject(intent, "operate", cJSON_Duplicate(operate, 0));
		}
		if(number){
			cJSON_AddItemToObject(intent, "value", cJSON_Duplicate(number, 1));
		}else if(mode){
			char *p = property == NULL ? NULL: property->valuestring;
			const char *m = search_voice_mode(g_func.get_class_voice_properties(NULL), p, mode->valuestring);
			if(m){
				cJSON_AddItemToObject(intent, "value", cJSON_CreateString(m));
			}else{
				ret = -2;
			}
		}
		cJSON_DeleteItemFromObject(item, "property");
		cJSON_AddItemToObject(item, "intent", intent);
	}
	return ret;
}

void print_json(const cJSON *js);
cJSON *get_device_property(cJSON *class_list, cJSON *class_prop_js)
{
	cJSON *property = cJSON_GetObjectItem(class_prop_js, "property");
	cJSON *mode = cJSON_GetObjectItem(class_prop_js, "mode");
	cJSON *unit = cJSON_GetObjectItem(class_prop_js, "unit");
	cJSON *determiner = cJSON_GetObjectItem(class_prop_js, "determiner");
	char *p = property == NULL ? NULL: property->valuestring;
	char *m = mode == NULL ? NULL: mode->valuestring;
	char *u = unit == NULL ? NULL: unit->valuestring;
	char *d = determiner == NULL ? NULL: determiner->valuestring;
	cJSON *props = get_voice_properties(g_func.get_class_voice_properties(NULL), p, m, u, d);
	dzlog_info("voice param search props: ");
	print_json(props);
	if(0 != ((long)p | (long)m | (long)u | (long)d) && props == NULL){
		dzlog_warn("not found property in voice_property file");
		return NULL;
	}
	cJSON *ret = NULL;
	if(class_list && props){
		int len = cJSON_GetArraySize(class_list);
		for(int class_num = 0; class_num < len; class_num++){
			cJSON *class_info = cJSON_GetArrayItem(class_list, class_num);
			cJSON *property_voice = cJSON_GetObjectItem(class_info, "property_voice");
			cJSON *class = cJSON_GetObjectItem(class_info, "class");
			if(property_voice && class){
				int len = cJSON_GetArraySize(property_voice);
				int flag = 0;
				for(int i = 0; i < len; i++){
					cJSON *item = cJSON_GetArrayItem(property_voice, i);
					int l = cJSON_GetArraySize(props);
					for(int j = 0; j < l; j++){
						cJSON *prop_item = cJSON_GetArrayItem(props, j);
						if(strcmp(prop_item->valuestring, item->valuestring) == 0){
							if(ret == NULL){
								ret = cJSON_CreateArray();
							}
							cJSON *d = cJSON_CreateObject();
							cJSON_AddItemToObject(d, "property", cJSON_Duplicate(prop_item, 0));
							cJSON_AddItemToObject(d, "class", cJSON_CreateString(class->valuestring));
							cJSON_AddItemToArray(ret, d);
							flag = 1;
							break;
						}
					}
					if(flag == 1){
						break;
					}
				}
			}else{
				dzlog_warn("error: property voice : %p , class: %p", property_voice, class);
			}
		}
	}else if(props != NULL){
		int len = cJSON_GetArraySize(props);
		for(int i = 0; i < len; i++){
			cJSON *prop_item = cJSON_GetArrayItem(props, i);
			if(ret == NULL){
				ret = cJSON_CreateArray();
			}
			cJSON *d = cJSON_CreateObject();
			cJSON_AddItemToObject(d, "property", cJSON_Duplicate(prop_item, 0));
			cJSON_AddItemToArray(ret, d);
		}
	}else if(class_list != NULL){
		int len = cJSON_GetArraySize(class_list);
		for(int i = 0; i < len; i++){
			cJSON *class_item = cJSON_GetArrayItem(class_list, i);
			cJSON *class = cJSON_GetObjectItem(class_item, "class");
			if(class){
				cJSON *d = cJSON_CreateObject();
				cJSON_AddItemToObject(d, "class", cJSON_Duplicate(class, 0));
				if(ret == NULL){
					ret = cJSON_CreateArray();
				}
				cJSON_AddItemToArray(ret, d);
			}
		}
	}
	return ret;
}

static const cJSON *get_class_voice_config(const char *class, const char *item)
{
	cJSON *param = cJSON_CreateObject();
	cJSON_AddItemToObject(param, "class", cJSON_CreateString(class));
	cJSON_AddItemToObject(param, "item", cJSON_CreateString(item));
	cJSON *ret = NULL;
	if(g_func.get_class_voice_config){
		ret = g_func.get_class_voice_config(param);
	}
	if(param){
		cJSON_Delete(param);
	}
	return ret ;
}

#include "hashmap.h"
cJSON *get_device_class_vui(cJSON *dev_list)
{
	if(dev_list == NULL){
		return NULL;
	}
	int list_len = cJSON_GetArraySize(dev_list);
	cJSON *ret = NULL;
	map_t class_name_list = hashmap_new();
	int count = 0;
	for(int i = 0; i < list_len; i++){
		cJSON *dev = cJSON_GetArrayItem(dev_list, i);
		if(ret == NULL){
			ret = cJSON_CreateArray();
		}
		int map_ok = -1;
		void *t;
		cJSON *name = cJSON_GetObjectItem(dev, "className");
		if(name == NULL){
			continue;
		}
		map_ok = hashmap_get(class_name_list, name->valuestring, &t);
		if(map_ok == 0){
			//dzlog_info("%s exist in ret", dev->dev_class->name);
			continue;
		}
		hashmap_put(class_name_list, name->valuestring, NULL);
		cJSON *class_item = cJSON_CreateObject();
		count++;
		cJSON_AddItemToObject(class_item, "class", cJSON_CreateString(name->valuestring));
		cJSON *property_voice = get_class_voice_config(name->valuestring, "property_voice");
		if(property_voice)
			cJSON_AddItemToObject(class_item, "property_voice", cJSON_Duplicate(property_voice, 1));
		cJSON *map_list = get_class_voice_config(name->valuestring, "map_list");
		if(map_list)
			cJSON_AddItemToObject(class_item, "map_list", cJSON_Duplicate(map_list, 1));
		cJSON_AddItemToArray(ret, class_item);
	}
	dzlog_info("class name number: %d", count);
	hashmap_free(class_name_list);
	return ret;
}

#if 0
/********************************************************
 *该函数功能通过属性过滤设备列表
 *dev: 设备信息
 *返回值:	 设备信息json数据
 * ******************************************************/
cJSON *DS_GetDevicesByClass(orb_list_t * dev_list, cJSON *class_prop_js)
{
	if(dev_list == NULL){
		return NULL;
	}

	int class_prop_len = 0;
	if(class_prop_js != NULL){
		class_prop_len = cJSON_GetArraySize(class_prop_js);
	}

	cJSON *dev_list_js = NULL;
	orb_list_item_t *item = orb_list_item_create(dev_list);
	logic_dev_t *dev = orb_list_item_next(item);
	while(dev){
		int flag = 0;
		//如果class_prop_js为null, 将设备列表中所有设备转换成json；
		//若class_prop_js不为null, 将设备列表中class类型相同的设备转换成json
		for(int i = 0; i < class_prop_len; i++){
			cJSON *class_prop_item = cJSON_GetArrayItem(class_prop_js, i);
			cJSON *class = cJSON_GetObjectItem(class_prop_item, "class");
			if(class && strcmp(class->valuestring, dev->dev_class->name) == 0){
				flag = 0;
				break;
			}else{
				flag = 1;
			}
		}
		if(flag == 0){
			cJSON *dev_js = LDI_GetDevInfoJson (dev);
			if(dev_js && dev->dev_class->version > 0){
				if(dev->room){
					cJSON_AddStringToObject(dev_js, "roomName", dev->room->name);
					cJSON_AddStringToObject(dev_js, "roomId", dev->room->id);
					if(dev->room->floor){
						cJSON_AddStringToObject(dev_js, "floorName", dev->room->floor->name);
						cJSON_AddStringToObject(dev_js, "floorId", dev->room->floor->id);
					}
				}
			}
			cJSON *f = device_info_format_in_voice(dev_js, dev);
			if(f != dev_js){
				cJSON_Delete(dev_js);
			}
			if(dev_list_js == NULL){
				dev_list_js = cJSON_CreateArray();
			}
			cJSON_AddItemToArray(dev_list_js, f);
		}
		dev = orb_list_item_next(item);
	}
	orb_list_item_destroy(item);
	return dev_list_js;
}
#endif

cJSON *dev_info_format(cJSON *non_voice_dev)
{
	if(non_voice_dev == NULL){
		return NULL;
	}
	cJSON *className = cJSON_GetObjectItem(non_voice_dev, "className");
	if(className == NULL){
		return NULL;
	}
	cJSON *device_info = get_class_voice_config(className->valuestring, "device_info");
	cJSON *new_dev = voice_device_info_format(device_info, non_voice_dev);
	return new_dev;
}

cJSON *dev_list_filter(cJSON *dev_list, cJSON *class_name_list)
{
	if(dev_list == NULL){
		return NULL;
	}
	int dev_len = cJSON_GetArraySize(dev_list);
	int class_len = 0;
	if(class_name_list != NULL){
		class_len = cJSON_GetArraySize(class_name_list);
	}
	int count = 0;
	cJSON *ret = NULL;
	for(int i = 0; i < dev_len; i++){
		cJSON *dev = cJSON_GetArrayItem(dev_list, i);
		cJSON *className = cJSON_GetObjectItem(dev, "className");
		if(className == NULL){
			dzlog_warn("device info has no className");
			continue;
		}
		if(class_len > 0){
			for(int j = 0; j < class_len; j++){
				cJSON *class = cJSON_GetArrayItem(class_name_list, j);
				cJSON *name = cJSON_GetObjectItem(class, "class");
				if(name == NULL){
					dzlog_warn("intent_cmd has no class");
					return NULL;
				}
				if(strcmp(name->valuestring, className->valuestring) == 0){
					cJSON *new_dev = dev_info_format(dev);
					if(ret == NULL){
						ret = cJSON_CreateArray();
					}
					count++;
					if(new_dev == NULL){
						cJSON_AddItemToArray(ret, cJSON_Duplicate(dev, 1));
					}else{
						cJSON_AddItemToArray(ret, new_dev);
					}
				}
			}
		}else{
			cJSON *new_dev = dev_info_format(dev);
			if(ret == NULL){
				ret = cJSON_CreateArray();
			}
			count++;
			if(new_dev == NULL){
				cJSON_AddItemToArray(ret, cJSON_Duplicate(dev, 1));
			}else{
				cJSON_AddItemToArray(ret, new_dev);
			}
		}
	}
	dzlog_info("dev number: %d", count);
	return ret;	
}

#define TIME_DEBUG 1
#include <sys/time.h>
char *search_device_and_cmd(const char *param, int *error_code)
{
	if(g_func.search_device == NULL){
		dzlog_warn("search function is null");
		return NULL;
	}
	#if TIME_DEBUG
	struct timeval start_time = {0}, end_time = {0}, mid_time = {0};
	gettimeofday(&start_time, NULL);
	#endif
	cJSON *ret = NULL, *intent_cmd = NULL;
	char *buf = NULL;
	cJSON* cmd_js = cJSON_Parse(param);
	if (cmd_js == NULL){
		dzlog_warn("%s cmd->value cJSON_Parse failed", __func__);
		*error_code = -1;
		return NULL;
	}
	int ret_code = 0;
	cmd_json_content_check(cmd_js);
	print_json(cmd_js);
	//step 1:根据位置信息、设备信息、范围等与操作无关的参数搜索设备列表
	cJSON *voice_device_param = get_cmd_device_param(cmd_js);
	dzlog_info("device param get: ");
	print_json(voice_device_param);
	//如果搜索设备参数为空，会获取整个设备列表中所有设备
	cJSON *dev_list = g_func.search_device(voice_device_param);
	if(voice_device_param != NULL){
		cJSON_Delete(voice_device_param);
	}
	dzlog_info("search device done");
	print_device_number(dev_list);
	#if TIME_DEBUG
	gettimeofday(&end_time, NULL);
	long long timeuse = (end_time.tv_sec-start_time.tv_sec)*1000000 + end_time.tv_usec - start_time.tv_usec;
	dzlog_info("search device use time: search_device %lld ms", timeuse/1000);
	#endif

	cJSON *class_list = get_device_class_vui(dev_list);
	//dzlog_info("class list: ");
	//print_json(class_list);
	#if TIME_DEBUG
	gettimeofday(&mid_time, NULL);
	timeuse = (mid_time.tv_sec-end_time.tv_sec)*1000000 + mid_time.tv_usec - end_time.tv_usec;
	dzlog_info("search device use time: get_device_class_vui %lld ms", timeuse/1000);
	#endif
	//step 2:以设备列表中的属性列表、语音参数中与操作相关的属性参数搜索属性列表
	cJSON *voice_property_param = get_cmd_property_param(cmd_js);
	//[{"class":"name", "property":"prop"}]
	dzlog_info("property param: ");
	print_json(voice_property_param);
	#if TIME_DEBUG
	gettimeofday(&end_time, NULL);
	timeuse = (end_time.tv_sec-mid_time.tv_sec)*1000000 + end_time.tv_usec - mid_time.tv_usec;
	dzlog_info("search device use time: get_cmd_property_param %lld ms", timeuse/1000);
	#endif
	cJSON *class_prop = get_device_property(class_list, voice_property_param);
	if(voice_property_param != NULL){
		cJSON_Delete(voice_property_param);
		if(class_prop == NULL){
			dzlog_warn("property param is not exist in property_voice file");
			ret_code = -2;
			goto end;
		}
	}
	#if TIME_DEBUG
	gettimeofday(&mid_time, NULL);
	timeuse = (mid_time.tv_sec-end_time.tv_sec)*1000000 + mid_time.tv_usec - end_time.tv_usec;
	dzlog_info("search device use time: get_device_property %lld ms", timeuse/1000);
	#endif
	//如果voice_property_param != NULL && class_prop == NULL && class_list != NULL，则属性未被设备所支持
	dzlog_info("device property: ");
	print_json(class_prop);
	//step 3:以语音命令中与操作相关的参数调整操作值，完善语音参数列表
	//[{"class":"name", "intent":{"property":"prop","operate":"op", "value":"val"}}]
	int intent_ret = get_device_intent(class_prop, cmd_js);
	//ret == -1 不支持的determiner, ret == -2 不支持的mode
	dzlog_info("device intent: ");
	print_json(class_prop);
	//step 4：以完善后的语音参数列表和设备列表取交集，得到最终的设备列表和控制命令
	//[{"class":"name", "intent":{"property":"prop","operate":"op", "value":"val"}, "cmd":{"curtain":{}}}}]
	intent_cmd = get_device_cmd(class_list, class_prop);
	//intent_cmd == NULL 则intent未被设备支持
	dzlog_info("cmd: ");
	print_json(intent_cmd);
	#if TIME_DEBUG
	gettimeofday(&end_time, NULL);
	timeuse = (end_time.tv_sec-mid_time.tv_sec)*1000000 + end_time.tv_usec - mid_time.tv_usec;
	dzlog_info("search device use time: get_device_cmd %lld ms", timeuse/1000);
	#endif

	//step 5: 筛选dev_list并转换为语音使用的json格式
	cJSON *dev_js = NULL;
	if(class_prop){
		dev_js = dev_list_filter(dev_list, intent_cmd);
	}
	#if TIME_DEBUG
	gettimeofday(&mid_time, NULL);
	timeuse = (mid_time.tv_sec-end_time.tv_sec)*1000000 + mid_time.tv_usec - end_time.tv_usec;
	dzlog_info("search device use time: dev_list_filter %lld ms", timeuse/1000);
	#endif

	ret = cJSON_CreateObject();
	cJSON_AddItemToObject(ret, "class_prop", intent_cmd);
	cJSON_AddItemToObject(ret, "device_list", dev_js);
	
end:
	//dzlog_info("device cmd: ");
	//print_json(ret);
	//step 6: 定义错误码
	if(class_list == NULL){
		//没有搜索到设备
		ret_code = -1;
		dzlog_info("no device found");
	}else if(voice_property_param != NULL && class_prop == NULL){
		//设备列表存在，但是属性未能和设备匹配上
		ret_code = -2;
		dzlog_info("property is not supported");
	}else if(intent_ret == -1 || intent_ret == -2){
		ret_code = -3;
		dzlog_info("operate is not suppoerted");
	}else if(intent_cmd == NULL){
		ret_code = -4;
		dzlog_info("intent is not supported");
	}
	*error_code = ret_code;

	buf = cJSON_PrintUnformatted(ret);
	#if TIME_DEBUG
	gettimeofday(&end_time, NULL);
	timeuse = (end_time.tv_sec-start_time.tv_sec)*1000000 + end_time.tv_usec - start_time.tv_usec;
	dzlog_info("search device use time: %lld ms", timeuse/1000);
	#endif
	if(ret){
		cJSON_Delete(ret);
	}
	if(class_list){
		cJSON_Delete(class_list);
	}
	if(class_prop){
		cJSON_Delete(class_prop);
	}
	if(cmd_js){
		cJSON_Delete(cmd_js);
	}
	if (dev_list){
		cJSON_Delete(dev_list);
	}	

	return buf;
}
