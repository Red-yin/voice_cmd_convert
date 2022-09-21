#ifndef __VOICE_INTERFACE_H__
#define __VOICE_INTERFACE_H__

char *search_device_and_cmd(const char *param, int *error_code);
void register_callback(const char *name, void *(*cbk)(void *param));
#endif
