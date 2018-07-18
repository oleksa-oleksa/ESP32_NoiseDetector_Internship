
/* MQTT Subscriber
 *
 *
 * Original Copyright (C) 2006-2016, ARM Limited, All Rights Reserved, Apache 2.0 License.
 * Additions Copyright (C) Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD, Apache 2.0 License.
 * Additions Copyright (C) Copyright 2017 pcbreflux, Apache 2.0 License.
 * Additions Copyright (C) Copyright 2018 eptecon, Apache 2.0 License.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "sdkconfig.h"
#include "MQTTClient.h"
#include "mqtt_publish.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "json_util.h"
#include "mqtt_subscribe.h"
#include "interrupt_app.h"
#include "spi_app.h"
#include "pwm_app.h"
#include "nvs.h"

unsigned char mqtt_sub_readBuf[MQTT_BUF_SIZE];
unsigned char mqtt_sub_sendBuf[MQTT_BUF_SIZE];
MessageData *received_message;
char *mqtt_recMsg;
int ret_connected;
int ret_subscribed;
static const char *TAG = "MQTT";


enum METHOD_TYPE {
    NO_METHOD = 0,
    METHOD_SET_VOLTAGE = 1,
    METHOD_SET_SOUND = 2,
    METHOD_SET_LED = 3,
    METHOD_SET_THRESHOLD = 4
} ;

typedef struct {
    METHOD_TYPE method;
    union {
        int voltage;
        int set_sound;
        int led;
        int threshold;
    };
} command_holder_t;

command_holder_t active_command;


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void mqtt_message_handler(MessageData *md) {
	ESP_LOGI(TAG, "Topic received!: %.*s %.*s", md->topicName->lenstring.len, md->topicName->lenstring.data, md->message->payloadlen, (char*)md->message->payload);
    mqtt_recMsg = (char *)md->message->payload;
}
#pragma GCC diagnostic pop


// only one method and one value will be obtained from Things Board when Yield ended.
// For the new method and value the button should be pushed again and the new value set after button push
// Function parses json string, extracts command type and command value
// and stores it in a commant_holder_t struct.
void parseJsonCommand(char *json, command_holder_t *ch)
{
    cJSON *monitor_json = cJSON_Parse(json);

    const cJSON *method = NULL;
    const cJSON *param = NULL;
    char *method_name = NULL;

    ch->method = NO_METHOD;

    if (monitor_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }

        goto end;
    }


    method = cJSON_GetObjectItemCaseSensitive(monitor_json, "method");

    if (!cJSON_IsString(method) || (method->valuestring == NULL))
        goto end;

    method_name = method->valuestring;

    if (strcmp(method_name, "setVoltageValue") == 0) {
        param = cJSON_GetObjectItemCaseSensitive(monitor_json, "params");

        if (cJSON_IsString(param) && param->valuestring != NULL) {
            ch->method = METHOD_SET_VOLTAGE;
            ch->voltage = atoi(param->valuestring);
        }
    } else if (strcmp(method_name, "setSoundValue") == 0) {
        param = cJSON_GetObjectItemCaseSensitive(monitor_json, "params");
        if (cJSON_IsBool(param)) {
            ch->method = METHOD_SET_SOUND;
            ch->set_sound = cJSON_IsTrue(param);
        }
    } else if (strcmp(method_name, "setLEDValue") == 0) {
        param = cJSON_GetObjectItemCaseSensitive(monitor_json, "params");
        if (cJSON_IsBool(param)) {
            ch->method = METHOD_SET_LED;
            ch->set_sound = cJSON_IsTrue(param);
        }
    }  else if (strcmp(method_name, "setThresholdValue") == 0) {
        param = cJSON_GetObjectItemCaseSensitive(monitor_json, "params");
        if (cJSON_IsString(param) && param->valuestring != NULL) {
            ch->method = METHOD_SET_THRESHOLD;
            ch->threshold = atoi(param->valuestring);
        }
    }

    end:
    cJSON_Delete(monitor_json);

}

void mqtt_get_json_settings(){

	mqtt_recMsg = mqtt_subscribe((char *) TOPIC1, mqtt_msg);

	// MQTT Queue is not empty, we have got the JSON
	if (mqtt_recMsg != NULL){

		printf("WE GOT: %s\n Parsing...\n", mqtt_recMsg);

		// Parse JSON Object
        parseJsonCommand(mqtt_recMsg, &active_command);

        switch (active_command.method) {
            case METHOD_SET_VOLTAGE:
                printf("Command was to set voltage to %i percent", active_command.voltage);
                vTaskDelay(10 / portTICK_RATE_MS);

                // Updating global NVC variable
                referenceVoltage = active_command.voltage * 3000 / 100;

                // Transmitting new voltage to SPI - new settings applied immediately
                spi_to_noise_click_transmit_cmd(spi, referenceVoltage);

                // Updating JSON File
                mqtt_update_json_battery(); // BATTERY TEST
                mqtt_connection_status = mqtt_publish((char*) TOPIC2, mqtt_msg);
                print_publish_status(mqtt_connection_status);

                // Saving
                nvs_write_value(VARIABLE_SET_VOLTAGE, referenceVoltage);

                break;

            case METHOD_SET_SOUND:
                vTaskDelay(10 / portTICK_RATE_MS);
                printf("Command was to turn sound to %i\n", active_command.set_sound);

                if (active_command.set_sound && xPWMTaskHandle == NULL) {
                    pwm_modul_config_noiseAlarm(&pwm_config);
                    xTaskCreate(pwm_buzzer_task, "pwm buzzer task", 16384, NULL, 5, &xPWMTaskHandle);
                    mqtt_update_json_buzzer();
                    mqtt_update_json_battery(); // BATTERY TEST
                    mqtt_connection_status = mqtt_publish((char*) TOPIC2, mqtt_msg);
                    print_publish_status(mqtt_connection_status);

                    // Updating global NVS Variable
                    isBuzzerOn = 1;
                    // Saving
                    nvs_write_value(VARIABLE_SET_SOUND, isBuzzerOn);
                    vTaskDelay(20 / portTICK_RATE_MS);
                } else if (!active_command.set_sound && xPWMTaskHandle != NULL) {
                    vTaskDelete(xPWMTaskHandle);
                    xPWMTaskHandle = NULL;
                    mqtt_update_json_buzzer();
                    mqtt_update_json_battery(); // BATTERY TEST
                    mqtt_connection_status = mqtt_publish((char*) TOPIC2, mqtt_msg);
                    print_publish_status(mqtt_connection_status);

                    // Updating global NVS Variable
                    isBuzzerOn = 0;
                    // Saving
                    nvs_write_value(VARIABLE_SET_SOUND, isBuzzerOn);
                    vTaskDelay(20 / portTICK_RATE_MS);
                }
                break;

            case METHOD_SET_LED:
                printf("Command was to set LED to %i\n", active_command.led);
                if (active_command.led && xBlinkTaskHandle == NULL) {
                    xTaskCreate(blink_interrupt_task, "blink task", configMINIMAL_STACK_SIZE, NULL, 5, &xBlinkTaskHandle);
                    mqtt_update_json_led();
                    mqtt_update_json_battery(); // BATTERY TEST
                    mqtt_connection_status = mqtt_publish((char*) TOPIC2, mqtt_msg);
                    print_publish_status(mqtt_connection_status);

                    // Updating global NVS Variable
                    isLedOn = 1;
                    // Saving
                    nvs_write_value(VARIABLE_SET_LED, isLedOn);
                    vTaskDelay(20 / portTICK_RATE_MS);
                }
                else if (!active_command.led && xBlinkTaskHandle != NULL) {
                    vTaskDelete(xBlinkTaskHandle);
                    xBlinkTaskHandle = NULL;
                    mqtt_update_json_led();
                    mqtt_update_json_battery(); // BATTERY TEST
                    mqtt_connection_status = mqtt_publish((char*) TOPIC2, mqtt_msg);
                    print_publish_status(mqtt_connection_status);

                    // Updating global NVS Variable
                    isLedOn = 0;
                    // Saving
                    nvs_write_value(VARIABLE_SET_LED, isLedOn);
                    vTaskDelay(20 / portTICK_RATE_MS);
                }
                break;
            case METHOD_SET_THRESHOLD:
                printf("Command was to set threshold to %i\n", active_command.threshold);
                vTaskDelay(10 / portTICK_RATE_MS);

                // Updating global NVS Variable
                noiseThreshold = active_command.threshold;

                mqtt_update_json_threshold(noiseThreshold); // BATTERY TEST
                mqtt_update_json_battery(); // BATTERY TEST
                mqtt_connection_status = mqtt_publish((char*) TOPIC2, mqtt_msg);
                print_publish_status(mqtt_connection_status);

                // Saving
                nvs_write_value(VARIABLE_SET_THRESHOLD, noiseThreshold);
                vTaskDelay(20 / portTICK_RATE_MS);
                break;
            case NO_METHOD:
                printf("Invalid command received\n");
                break;
            default:
                printf("Invalid command received\n");
                break;
        }
    }
	else {
		printf("NULL\n");
	}
}


// only one method and one value will be obtained from Things Board when Yield ended.
// For the new method and value the button should be pushed again and the new value set after button push

char* mqtt_subscribe(char *topic, char *msgbuf){

  int ret;
    MQTTClient client;
    MQTTString clientId;
    MQTTPacket_connectData data;
    MQTTString user;

    Network network;
    NetworkInit(&network);

    
    ESP_LOGD(TAG, "Start MQTT  Subscribe...");

    ESP_LOGD(TAG,"NetworkConnect %s:%d ...", (char *) MQTT_SERVER, MQTT_PORT);
    ret_connected = NetworkConnect(&network, (char *) MQTT_SERVER, MQTT_PORT);
    if (ret_connected != 0) {
        ESP_LOGI(TAG, "NetworkConnect not SUCCESS: %d", ret_connected);
        goto exit;
    }

    ESP_LOGD(TAG,"MQTTClientInit  ...");
    MQTTClientInit(&client, &network,
                   2000,            // command_timeout_ms
                   mqtt_sub_sendBuf,         //sendbuf,
                   MQTT_BUF_SIZE, //sendbuf_size,
                   mqtt_sub_readBuf,         //readbuf,
                   MQTT_BUF_SIZE  //readbuf_size
    );

    clientId = MQTTString_initializer;
    clientId.cstring = (char*) "NoiseDetector";

    user = MQTTString_initializer;
    user.cstring = (char*) TOKEN; // Noise

    data = MQTTPacket_connectData_initializer;
    data.clientID          = clientId;
    data.username		   = user;
    data.willFlag          = 0;
    data.MQTTVersion       = 4; // 3 = 3.1 4 = 3.1.1
    data.keepAliveInterval = 60;
    data.cleansession      = 1;


    ESP_LOGI(TAG,"MQTTConnect  ...");
    ret = MQTTConnect(&client, &data);
    if (ret != SUCCESS) {
        ESP_LOGI(TAG, "MQTTConnect not SUCCESS: %d", ret);
        goto exit;
    }

    ESP_LOGI(TAG, "MQTTSubscribe  ...");
    ret_subscribed = MQTTSubscribe(&client, "v1/devices/me/rpc/request/+", QOS0, mqtt_message_handler);
    if (ret_subscribed != SUCCESS) {
        ESP_LOGI(TAG, "MQTTSubscribe: %d", ret_subscribed);
        goto exit;
    }
    if (ret_subscribed == SUCCESS){
    	printf("Subscrubed\n");
    }

    ESP_LOGI(TAG, "MQTTYield  ...");

    ret = MQTTYield(&client, data.keepAliveInterval);
    if (ret != SUCCESS) {
        ESP_LOGI(TAG, "MQTTYield: %d", ret);
        goto exit;
    }

    exit:
    MQTTDisconnect(&client);
    NetworkDisconnect(&network);

    ESP_LOGI(TAG, "MQTTDisconnect ...");
    printf("Disconnected!\n");

    return(mqtt_recMsg);
}

// void mqtt_subscribe_task_body(void *arg)
// {
//   if (!arg) {
//     printf("Called mqtt_subscribe_task with no arguments, skipping\n");
//     goto exit;
//   }

//   mqtt_subscribe_t *s = (mqtt_subscribe_t *)arg;
//   mqtt_subscribe(s->topic, s->message);
  
//   free(s);
  
//  exit:
//   vTaskDelete( NULL );
// }

// void mqtt_subscribe_task(const char *topic, char *buf)
// {
// }
