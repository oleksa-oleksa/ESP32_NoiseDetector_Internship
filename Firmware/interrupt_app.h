// Created by Oleksandra Baga on 06.02.18.
// Created for Eptecon Berlin
// Updated by Oleksandra Baga on 20.03.18
// Version with time control of interrupts

#pragma once
#include "driver/gpio.h"
#include "time.h"
#include "sdkconfig.h"
#include "mqtt_publish.h"
#include "interrupt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************
 * GPIO Matrix for Interrupt
**********************************************************************************/
#define GPIO_INPUT_INTERRUPT GPIO_NUM_4 // GPIO 33 as Input for detecting the INT from Microphone
#define GPIO_BUTTON_SUBSCRIBE_INTERRUPT GPIO_NUM_5 // GPIO 16 as input GPIO for button for activation of the subscribe function

#define GPIO_INPUT_PIN_SEL  ((1ULL << GPIO_INPUT_INTERRUPT) | (1ULL << GPIO_BUTTON_SUBSCRIBE_INTERRUPT))
#define ESP_INTR_FLAG_DEFAULT 0
#define GPIO_OUTPUT_BLINK GPIO_NUM_32

#define INTERRUPT_DELAY 1000
#define BUTTON_DEBOUNCE_DELAY 1

#define THRESHOLD 5
#define L_TH1 12
#define L_TH2 8
#define L_TH3 6
#define L_TH4 4
#define L_TH5 2

#define T_TH1 8
#define T_TH2 8
#define T_TH3 8
#define T_TH4 8
#define T_TH5 8


/**********************************************************************************
 * Check if voltage exceeds the threshold value
**********************************************************************************/
typedef struct SQueue TQueue;
int check_threshold(TQueue *q, TInterrupt *ir);

/**********************************************************************************
 * Interrupt GPIO
**********************************************************************************/
extern gpio_config_t io_int_conf; // Interrupt from Noise Click
extern volatile int detected_interrupt;
extern volatile int numberOfButtonInterrupts;
extern volatile uint32_t lastDebounceTime;
extern volatile int buttonPushed;

extern TaskHandle_t xBlinkTaskHandle;
extern TaskHandle_t xPWMTaskHandle;

/**********************************************************************************
 * ISR Handler
**********************************************************************************/
void IRAM_ATTR gpio_isr_handler(void* arg);

/**********************************************************************************
 * GPIO Task
**********************************************************************************/
void gpio_noise_interrupt_task(void* arg);

/**********************************************************************************
 * config GPIO as Input
**********************************************************************************/
void gpio_config_input(gpio_config_t *io_conf);

/**********************************************************************************
 * config GPIO as Input for blinking LED
**********************************************************************************/
void blink_interupt_init();

/**********************************************************************************
 * Blink Function
**********************************************************************************/
void blink_interrupt_task(void *);

/**********************************************************************************
 * config GPIO Interrupt Queue and Handler
**********************************************************************************/
void gpio_init_noise_ISR();

/**********************************************************************************
 * MQTT Update JSON-File if Interrupt occurs
**********************************************************************************/
void mqtt_interrupt_task(void *arg);


/**********************************************************************************
 * GPIO Task
 * Detects an interrupt on the button GPIO
 * Starts the subscription function to read the settings
 * from the ThingsBoard account
**********************************************************************************/
void gpio_button_subscribe_interrupt_task(void* arg);


#ifdef __cplusplus
}
#endif
