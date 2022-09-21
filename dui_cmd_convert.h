#ifndef __DUI_CMD_CONVERT_H__
#define __DUI_CMD_CONVERT_H__

int dui_data_compare(const cJSON *dui, const cJSON *cmd);
cJSON *get_cmd_property_param(cJSON *cmd_param);
cJSON *get_cmd_device_param(const cJSON *cmd_param);
void cmd_json_content_check(cJSON *root);
cJSON *json_variable_extend(const cJSON *variable, const cJSON *data_src);
cJSON *get_voice_properties(const cJSON *voice_properties, const char *property, const char *mode, const char *unit, const char *determiner);
const char *search_voice_mode(const cJSON *voice_properties, const char *property, const char *mode_alias);
int search_voice_determiner(const cJSON *voice_properties, const char *property, const char *determiner);
#endif