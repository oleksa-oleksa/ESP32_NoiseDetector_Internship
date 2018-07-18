// Created by Oleksandra Baga on 06.02.18.
// Eptecon Berlin

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include "driver/spi_master.h"
#include "esp_system.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "BootWiFi.h"
#include "sdkconfig.h"
#include "spi_app.h"
#include "interrupt_app.h"
#include "adc_app.h"
#include "pwm_app.h"
#include "mqtt_publish.h"
#include "mqtt_subscribe.h"
#include "MQTTClient.h"
#include "queue_int.h"
#include "interrupt.h"
#include "battery.h"
#include "nvs.h"


BootWiFi *boot;
clock_t start_t;

TaskHandle_t xBlinkTaskHandle = NULL;
TaskHandle_t xPWMTaskHandle = NULL;


extern "C" {
void app_main(void);
}

void app_main(void){

    /*********************************************************************************
    * GPIOs Matrix Init
    *********************************************************************************/
    // Config GPIOs for ISR as Input:
    gpio_config_input(&io_int_conf);
    // create a Queue for ISR and hook an ISR Handler
    gpio_init_noise_ISR();
    // LED GPIO as Output without ISR
    blink_interupt_init();
    // PWM GPIO without ISR
    pwm_gpio_init();


    /*********************************************************************************
    * Non-Volatile Storage
    *********************************************************************************/

    nvs_init();

    /*********************************************************************************
    * WI-FI
    *********************************************************************************/
    boot = new BootWiFi();
    boot->boot();
    // pwm_melody_wifi_connected(&pwm_config);

    /*********************************************************************************
    * Queue for Interrupts and Time Scheduling
    *********************************************************************************/
    queueInterrupt = q_new();
    printf("Starting of the program, start_t = %ld\n", clock());

    /*********************************************************************************
     * ADC Init
     *********************************************************************************/
    adc_init(&characteristics);

    /*********************************************************************************
     * SPI Init
     *********************************************************************************/
    spi_to_noise_click_bus_init(&buscfg);
    spi_to_noise_click_slave_init(&devcfg, &spi);

    // Reference Voltage from NVC if exists
    referenceVoltage = nvs_read_value(VARIABLE_SET_VOLTAGE);

    if (referenceVoltage == -1) {
        // Transmit the default to the SPI
        spi_to_noise_click_transmit_cmd(spi, V_IN_NOISE_CLICK);
    }
    else {
        // Transmit custom value to the SPI
        spi_to_noise_click_transmit_cmd(spi, referenceVoltage);
    }

    /*********************************************************************************
     * Noise Threshold
     *********************************************************************************/
    // from NVC if exists
    noiseThreshold = nvs_read_value(VARIABLE_SET_THRESHOLD);

    if (noiseThreshold == -1) {
        noiseThreshold = THRESHOLD; // default threshold
    }

    /*********************************************************************************
    * JSON Creation and Tracking
    *********************************************************************************/
    mqtt_msg = mqtt_create_json();
    mqtt_msg = mqtt_create_json_voltage();
    mqtt_msg = mqtt_create_json_led();
    mqtt_msg = mqtt_create_json_buzzer();
    mqtt_msg = mqtt_create_json_battery();
    mqtt_msg = mqtt_create_json_threshold();

    /*********************************************************************************
     * RTOS Tasks
     *********************************************************************************/
    // ISR Tasks: Noise Interrupt and Button Detection
    xTaskCreate(gpio_noise_interrupt_task, "Interrupt from the Noise Click on the GPIO", 16384, NULL, 10, NULL);
    xTaskCreate(gpio_button_subscribe_interrupt_task, "Interrupt from button", 16384, NULL, 10, NULL);


    /** Creation of LED and Buzzer tasks only if LED and Buzzer set to 1 **/
    // Blink LED Task
    isLedOn = nvs_read_value(VARIABLE_SET_LED);
    // if LED is turned on or default
    if (isLedOn == 1 || isLedOn == -1) {
        printf("LED alarm signal is turned on.\n");
        xTaskCreate(blink_interrupt_task, "blink task", configMINIMAL_STACK_SIZE, NULL, 5, &xBlinkTaskHandle);
    }
    else if (isLedOn == 0) {
        printf("LED alarm signal is turned off.\n");
    }
    mqtt_update_json_led();

    // PWM for Buzzer Task
    isBuzzerOn = nvs_read_value(VARIABLE_SET_SOUND);
    // buzzer is turned on or default
    if (isBuzzerOn == 1 || isBuzzerOn == -1) {
        pwm_modul_config_noiseAlarm(&pwm_config);
        printf("Buzzer alarm signal is turned on.\n");
        xTaskCreate(pwm_buzzer_task, "pwm buzzer task", 16384, NULL, 5, &xPWMTaskHandle);
    }
    else if (isBuzzerOn == 0) {
        printf("Buzzer alarm signal is turned off.\n");
    }
    mqtt_update_json_buzzer();

    // Updating JSON via MQTT if interrupt occurred
    xTaskCreate(mqtt_interrupt_task, "JSON via MQTT", 16384, NULL, 5, NULL);

    // Json default factory setting publish
    // mqtt_publish_task(TOPIC2, mqtt_msg);

    mqtt_publish(TOPIC2, mqtt_msg);

    printf("RUNNING!\n");

}
