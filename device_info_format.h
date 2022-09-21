#ifndef __DEVICE_INFO_FORMAT_H__
#define __DEVICE_INFO_FORMAT_H__

cJSON *read_code(const char *file_path);
cJSON *run_code(const cJSON *code, const cJSON *data);
cJSON *voice_device_info_format(const cJSON *rule, const cJSON *src);

#endif