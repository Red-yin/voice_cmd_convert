#include "cJSON.h"
#include "zlog.h"

cJSON *convert(cJSON *list, const cJSON *code, const cJSON *data)
{
	cJSON *ret = cJSON_CreateObject();
	int size = cJSON_GetArraySize(list);
	for(int i = 0; i < size; i++){
		cJSON *item = cJSON_GetArrayItem(list, i);
		if(item->type != cJSON_String){
			dzlog_warn("list member is not string");
			continue;
		}
		cJSON *prop = cJSON_GetObjectItem(code, item->valuestring);
		if(prop == NULL){
			dzlog_error("%s not exist in device info", item->valuestring);
			continue;
		}
		char *key = "KEY";
		cJSON *k = cJSON_GetObjectItem(prop, key);
		#if 0
        if(k->type == cJSON_Object){
			const cJSON *p = data;
			while(k != NULL){
				if(k->type == cJSON_String){
					//data层级逐步下降
					key = k->valuestring;
					k = cJSON_GetObjectItem(prop, key);
					if(k == NULL){
						p = cJSON_GetObjectItem(p, key);
						dzlog_info("%s is end key", key);
						break;
					}else if(k->type == cJSON_String){
						p = cJSON_GetObjectItem(p, key);
					}
				}else if(k->type == cJSON_Object){
					//k->type是object时，key的值需要做一次转换：根据当前key获取data中对应的value，再根据获取到的value，获取code中的对应的value；
					//转换过程中，data的层级不变
					cJSON *tmp = cJSON_GetObjectItem(p, key);
					if(tmp == NULL){
						dzlog_error("%s not exist in %s", key, p->string);
						break;
					}
					key = tmp->valuestring;
					k = cJSON_GetObjectItem(k, key);
					continue;
				}else{
					break;
				}
			}
		    if(k != NULL){
			    continue;
		    }else{
			    cJSON_AddItemToObject(ret, item->valuestring, cJSON_Duplicate(p, 1));
		    }
        }else 
		#endif
		if(k->type == cJSON_Array){
            int len = cJSON_GetArraySize(k);
            const cJSON *p = data;
            const cJSON *next = data;
            for(int i = 0; i < len; i++){
                cJSON *member = cJSON_GetArrayItem(k, i);
                if(member->type == cJSON_String){
                    p = next;
					next = cJSON_GetObjectItem(p, member->valuestring);
                    if(next == NULL){
                        dzlog_error("%s not exist in %s", member->valuestring, p->valuestring);
                        break;
                    }
                }else if(member->type == cJSON_Object){
                    //KEY列表中的成员（member）是Object时，Object是多个K:V形式的数据，此时的物模型设备信息数据（data）对应的值（next）是其中的一个K,
                    //Object的作用就是将next（K）转换为V！
                    member = cJSON_GetObjectItem(member, next->valuestring);
					if(member == NULL){
						dzlog_error("%s not exist in device_info key", next->valuestring);
						next = NULL;
						break;
					}
                    next = cJSON_GetObjectItem(p, member->valuestring);
                    if(next == NULL){
						if(i == len-1){
							next = member;
						}else{
                        	dzlog_error("%s not exist in %s", member->valuestring, p->valuestring);
						}
                        break;
                    }
                }
            }
            p = next;
            if(p){
                key = "property_list";
                k = cJSON_GetObjectItem(prop, key);
                if(k){
                    cJSON *child_p = convert(k, prop, p);
			        cJSON_AddItemToObject(ret, item->valuestring, child_p);
                }else{
			        cJSON_AddItemToObject(ret, item->valuestring, cJSON_Duplicate(p, 1));
                }
            }
        }
	}
	return ret;
}

cJSON *run_code(const cJSON *code, const cJSON *data)
{
	cJSON *property_list = cJSON_GetObjectItem(code, "property_list");
	if(property_list == NULL || property_list->type != cJSON_Array){
		dzlog_error("not find device_info");
		return NULL;
	}
	cJSON *prop = convert(property_list, code, data);
	cJSON *range_list = cJSON_GetObjectItem(code, "range_list");
	if(range_list){
		cJSON *range = convert(range_list, cJSON_GetObjectItem(code, "range"), data);
		cJSON_AddItemToObject(prop, "range", range);
	}

	cJSON *ret = cJSON_CreateObject();
	cJSON_AddItemToObject(ret, "properties", prop);
	//dzlog_info("ret: %s", cJSON_Print(ret));
	return ret;
}

cJSON *read_code(const char *file_path)
{
	if(file_path == NULL){
		return NULL;
	}
	FILE *fp = fopen(file_path, "r");
	cJSON *root = NULL;
	cJSON *ret = NULL;
	char *buf = NULL;
	cJSON *device_info = NULL;
	if(fp == NULL){
		dzlog_error("%s not exist", file_path);
		goto exit;
	}
	fseek(fp, 0L, SEEK_END);
	int len = ftell(fp) + 1;
	fseek(fp, 0L, SEEK_SET);
	if(len > 1024000){
		dzlog_error("%s size %d, too large", file_path, len);
		goto exit;
	}
	dzlog_info("file size: %d", len);
	buf = (char *)calloc(1, len);
	if (NULL == buf) {
		dzlog_error("vocab load calloc error");
		goto exit;
	}
	int read_len = fread(buf, 1, len - 1, fp);
	if(0 > read_len){
		dzlog_error("%s read failed", file_path);
		goto exit;
	}

	root = cJSON_Parse(buf);
	if(root == NULL){
		dzlog_error("%s is not json, %s", file_path, buf);
		goto exit;
	}
	cJSON *vui = cJSON_GetObjectItem(root, "vui");
	if(vui == NULL){
		dzlog_error("%s has no vui", file_path);
		goto exit;
	}
	device_info = cJSON_GetObjectItem(vui, "device_info");
	if(device_info == NULL){
		dzlog_error("%s has no device_info", file_path);
		goto exit;
	}
	ret = cJSON_Duplicate(device_info, 1);

exit:
	if(root != NULL){
		cJSON_Delete(root);
	}
	if(buf != NULL){
		free(buf);
	}
	if(fp != NULL){
		fclose(fp);
	}
	return ret;
}

void basic_device_info_add(const cJSON *src, cJSON *dst)
{
    if(src == NULL || dst == NULL){
        return;
    }
    cJSON *deviceId = cJSON_GetObjectItem(src, "deviceId");
    if(deviceId){
        cJSON_AddItemToObject(dst, "deviceId", cJSON_Duplicate(deviceId, 0));
    }
    cJSON *deviceName = cJSON_GetObjectItem(src, "deviceName");
    if(deviceName){
        cJSON_AddItemToObject(dst, "deviceName", cJSON_Duplicate(deviceName, 0));
    }
    cJSON *roomId = cJSON_GetObjectItem(src, "roomId");
    if(roomId){
        cJSON_AddItemToObject(dst, "roomId", cJSON_Duplicate(roomId, 0));
    }
    cJSON *floorId = cJSON_GetObjectItem(src, "floorId");
    if(floorId){
        cJSON_AddItemToObject(dst, "floorId", cJSON_Duplicate(floorId, 0));
    }
    cJSON *roomName = cJSON_GetObjectItem(src, "roomName");
    if(roomName){
        cJSON_AddItemToObject(dst, "roomName", cJSON_Duplicate(roomName, 0));
    }
    cJSON *floorName = cJSON_GetObjectItem(src, "floorName");
    if(floorName){
        cJSON_AddItemToObject(dst, "floorName", cJSON_Duplicate(floorName, 0));
    }
    cJSON *className = cJSON_GetObjectItem(src, "className");
    if(className){
        cJSON_AddItemToObject(dst, "className", cJSON_Duplicate(className, 0));
    }
    cJSON *classNameDetail = cJSON_GetObjectItem(src, "classNameDetail");
    if(classNameDetail){
        cJSON_AddItemToObject(dst, "classNameDetail", cJSON_Duplicate(classNameDetail, 0));
    }
    cJSON *category = cJSON_GetObjectItem(src, "category");
    if(category){
        cJSON_AddItemToObject(dst, "category", cJSON_Duplicate(category, 0));
    }
}

cJSON *voice_device_info_format(const cJSON *rule, const cJSON *src)
{
    if(src == NULL || rule == NULL){
        dzlog_info("%s param error: src: %p, rule: %p", __func__, src, rule);
        return NULL;
    }
	#if 0
    char *dev_info = cJSON_PrintUnformatted(src);
    if(dev_info){
        dzlog_info("device_control device info: %s", dev_info);
        free(dev_info);
    }
    char *r = cJSON_PrintUnformatted(rule);
    if(r){
        dzlog_info("device_control rule : %s", r);
        free(r);
    }
	#endif
    cJSON *ret = NULL;
    cJSON *properties = cJSON_GetObjectItem(src, "properties");
    if(properties){
        ret = run_code(rule, properties);
        basic_device_info_add(src, ret);
    }
	#if 0
    r = cJSON_PrintUnformatted(ret);
    if(r){
        dzlog_info("device_control format: %s", r);
        free(r);
    }
	#endif
    return ret;
}