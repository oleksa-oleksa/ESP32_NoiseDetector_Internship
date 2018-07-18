#ifndef JSON_UTIL
#define JSON_UTIL

#include "cJSON.h"


#ifdef __cplusplus
extern "C" {
#endif

char* json_create(void);
char* json_add_string(char *json, char *name, char *value);
char* json_add_int(char *json, char *name, int value);
char* json_add_bool(char *json, char *name, int flag_value);
char* json_update(char *json, char *name, char *value);
char* json_update_bool(char *json, char *name, cJSON *value);

#ifdef __cplusplus
}
#endif

#endif /* JSON_UTIL  */