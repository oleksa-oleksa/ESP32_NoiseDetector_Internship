/* MQTT Publisher
 *
 *
 * Original Copyright (C) 2006-2016, ARM Limited, All Rights Reserved, Apache 2.0 License.
 * Additions Copyright (C) Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD, Apache 2.0 License.
 * Additions Copyright (C) Copyright 2017 pcbreflux, Apache 2.0 License.
 * Additions Copyright (C) Copyright 2018 eptecon, Apache 2.0 License.
 *
 */
#include <string.h>
#include <stdlib.h>

#include "sdkconfig.h"

#include "MQTTClient.h"
#include "BootWiFi.h"
#include "WiFiEventHandler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "json_util.h"
#include "interrupt_app.h"
#include "mqtt_publish.h"
#include "cJSON.h"
#include "json_util.h"
#include "battery.h"
#include "spi_app.h"
#include "nvs.h"

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

static const char *TAG = "MQTT";

// JSON Values
char noise_name[] = "noise_level";
char led_name[] = "getLEDValue";
char buzzer_name[] = "getSoundValue";
char battery_name[] = "getBatteryValue";
char threshold_name[] = "getThresholdValue";
char *mqtt_msg;

unsigned char mqtt_sendBuf[MQTT_BUF_SIZE];
unsigned char mqtt_readBuf[MQTT_BUF_SIZE];
int mqtt_connection_status = SUCCESS;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
EventGroupHandle_t wifi_event_group = xEventGroupCreate();

char* mqtt_create_json(void){

    mqtt_msg = json_create();
    printf("JSON created.\n");
    return mqtt_msg;
}

char* mqtt_create_json_voltage(void){

    mqtt_msg = json_add_int(mqtt_msg, noise_name, 0);
    printf("Voltage record added to JSON.\n");
    return mqtt_msg;
}

char* mqtt_create_json_led() {
    mqtt_msg = json_add_bool(mqtt_msg, led_name, (xBlinkTaskHandle != NULL));
    printf("LED status record added to JSON.\n");
    return mqtt_msg;
}

char* mqtt_create_json_buzzer() {
    mqtt_msg = json_add_bool(mqtt_msg, buzzer_name, (xPWMTaskHandle != NULL));
    printf("Buzzer status record added to JSON.\n");
    return mqtt_msg;
}

char* mqtt_create_json_battery(void){

    mqtt_msg = json_add_int(mqtt_msg, battery_name, battery_level);
    printf("Battery record added to JSON.\n");
    return mqtt_msg;
}

char* mqtt_create_json_threshold(void){

    mqtt_msg = json_add_int(mqtt_msg, threshold_name, noiseThreshold);
    printf("Threshold record added to JSON.\n");
    return mqtt_msg;
}

/*********************************************************************************
 * Part of ISR: After Interrupt occurred the detected voltage (the level of noise)
 * should be added to JSON and sent to ThingsBoard Dashboard.
 * Access Point with an external value from outside for update the existing JSON
*********************************************************************************/
void mqtt_update_json_voltage(int detected_voltage){

    /*Update JSON Object*/
    char noise_value[32];
    sprintf(noise_value, "%d", detected_voltage);
    mqtt_msg = json_update(mqtt_msg, noise_name, noise_value);
    //printf("%s\n", mqtt_msg);
    printf("Voltage record updated.\n");
}


/*********************************************************************************
 * Publishing info about LED Status for correct Control Widget view
*********************************************************************************/
void mqtt_update_json_led(){

    cJSON *led_status = NULL;
    if (xBlinkTaskHandle != NULL) {
        led_status = cJSON_CreateBool(1);
    }
    else {
        led_status = cJSON_CreateBool(0);
    }

    /*Update JSON Object*/
    mqtt_msg = json_update_bool(mqtt_msg, led_name, led_status);
    printf("Current LED Status updated.\n");
}


/*********************************************************************************
 * Publishing info about Buzzer Status for correct Control Widget view
*********************************************************************************/
void mqtt_update_json_buzzer(){

    cJSON *buzzer_status = NULL;
    if (xPWMTaskHandle != NULL) {
        buzzer_status = cJSON_CreateBool(1);
    }
    else {
        buzzer_status = cJSON_CreateBool(0);
    }

    /*Update JSON Object*/
    mqtt_msg = json_update_bool(mqtt_msg, buzzer_name, buzzer_status);
    printf("Current Buzzer Status updated.\n");
}

/*********************************************************************************
 * Publishing info about Battery Status
*********************************************************************************/
void mqtt_update_json_battery() { // the int value as arg in the future

    // test case:
    int battery_level = get_battery_level();

    /*Update JSON Object*/
    char battery_value[32];
    sprintf(battery_value, "%d", battery_level);
    mqtt_msg = json_update(mqtt_msg, battery_name, battery_value);
    printf("Battery level updated.\n");
}

/*********************************************************************************
 * Publishing info about Noise Threshold for correct Control Widget view
*********************************************************************************/
void mqtt_update_json_threshold(int16_t threshold) {

    /*Update JSON Object*/
    char threshold_value[32];
    sprintf(threshold_value, "%d", threshold);
    mqtt_msg = json_update(mqtt_msg, threshold_name, threshold_value);
    printf("Noise threshold value updated.\n");
}

/*********************************************************************************
 * Publishing Function: the char[] for JSON should be created and updated in advance, the function
 * will convert the object into MQTT message and send into related topic with related token
 * NOTE: change token to access another dashboard
*********************************************************************************/
int mqtt_publish(char *topic, char *msgbuf){
    int ret = 0;
    MQTTClient client;
    MQTTString clientId;
    MQTTPacket_connectData data;
    MQTTString user;
    MQTTMessage message;

    Network network;
    NetworkInit(&network);
    network.websocket = 0;
    
    ESP_LOGD(TAG, "Start MQTT Publish...");

    ESP_LOGD(TAG,"NetworkConnect %s:%d ...", MQTT_SERVER, MQTT_PORT);
    ret = NetworkConnect(&network,  MQTT_SERVER, MQTT_PORT);
    if (ret != 0) {
        ESP_LOGI(TAG, "NetworkConnect not SUCCESS: %d", ret);
        goto exit;
    }

    ESP_LOGI(TAG,"MQTTClientInit  ...");
    MQTTClientInit(&client, &network,
                   2000,                 // command_timeout_ms
                   mqtt_sendBuf,         //sendbuf,
                   MQTT_BUF_SIZE,        //sendbuf_size,
                   mqtt_readBuf,         //readbuf,
                   MQTT_BUF_SIZE         //readbuf_size
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

    ESP_LOGI(TAG, "MQTTPublish  ... %s", msgbuf); // 
    message.qos = QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)msgbuf;
    message.payloadlen = strlen(msgbuf);

    ret = MQTTPublish(&client, topic, &message);
    if (ret == SUCCESS) {
        ESP_LOGI(TAG, "MQTTPublish SUCCES: JSON PUBLISHED");
    }
    else if (ret != SUCCESS) {
        ESP_LOGI(TAG, "MQTTPublish not SUCCESS: %d", ret);
        goto exit;
    }
    exit:
    MQTTDisconnect(&client);
    NetworkDisconnect(&network);

    ESP_LOGI(TAG, "MQTTDisconnect ...");

    printf("Disconnected!\n");
    return ret;
}

void print_publish_status(int status) {
	if (status == SUCCESS) {
		    	printf("Successfully published!\n");
	}
	else if (status == FAILURE) {
	    printf("\nNo connection to the server! Publishing skipped...\n\n");
	}
	else if (status == BUFFER_OVERFLOW) {
	    printf("\nBuffer overflow! Publishing skipped...\n\n");
	}
}


// void mqtt_publish_task_body(void *arg)
// {
//   if (!arg) {
//     printf("mqtt_publish_task called but with no arguments\n");
//     goto exit;
//   }

//   mqtt_publish_t *ps = (mqtt_publish_t *)arg;
//   int rv = mqtt_publish(ps->topic, ps->message);
//   print_publish_status(rv);

//   free(ps->message);
//   free(ps);

//  exit:
//   vTaskDelete( NULL );
// }


// void mqtt_publis_task(const char *topic, const char *msg)
// {
//   mqtt_publish_t *ps = (mqtt_publish_t *)malloc(sizeof(mqtt_publish_t));
  
//   ps->topic = TOPIC2;
//   ps->message = strdup(mqtt_msg);

//   xTaskCreate(mqtt_publish_task_body, "Publish MQTT message", 16338, ps, 5, NULL);
// }
