#pragma once

#include "mqtt_publish.h"
#include "MQTTClient.h"
#include "json_util.h"

#define MAX_BUFFER_SIZE 30

extern char *mqtt_recMsg;

void mqtt_get_json_settings();

char* mqtt_subscribe(char *topic, char *msgbuf);

void parseString(char *json, char **name, char **value);