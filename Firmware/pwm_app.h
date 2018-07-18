// Created by Oleksandra Baga on 13.02.18.
// Eptecon Berlin

#pragma once
#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"
#include "pwm_app.h"
#include "interrupt_app.h"

#define GPIO_PWM0A_OUT GPIO_NUM_21   //Set GPIO  as PWM0A
#define FREQUENCY_NOISE_ALARM 1000
#define FREQUENCY_WIFI_CONNECTED 800
#define FREQUENCY_GOT_VALUE 3000
#define FREQUENCY_FAILED_VALUE 200
#define DUTY_CYCLE 50

extern mcpwm_config_t pwm_config;

/**********************************************************************************
 * PWM GPIO Init
**********************************************************************************/
void pwm_gpio_init();

/**********************************************************************************
 * PWM Config
**********************************************************************************/
void pwm_modul_config_noiseAlarm(mcpwm_config_t *pwm_config);

/**********************************************************************************
 * GPIO Buzzer Task
**********************************************************************************/
void pwm_buzzer_task(void *);

/**********************************************************************************
 * MELODY: WiFI Connected
**********************************************************************************/
void pwm_melody_wifi_connected(mcpwm_config_t *pwm_config);
