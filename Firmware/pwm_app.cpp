// Created by Oleksandra Baga on 13.02.18.
// Eptecon Berlin

#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_attr.h"
#include "soc/rtc.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"
#include "pwm_app.h"
#include "nvs.h"

mcpwm_config_t pwm_config;

/**********************************************************************************
 * PWM GPIO Init
**********************************************************************************/
void pwm_gpio_init(){

    printf("initializing mcpwm gpio\n");
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT);
    mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A);

}

/**********************************************************************************
 * PWM Config FOR NOISE ALARM INTERRUPT
**********************************************************************************/
void pwm_modul_config_noiseAlarm(mcpwm_config_t *pwm_config){
    pwm_config->frequency = FREQUENCY_NOISE_ALARM;  // default no case frequency
    pwm_config->cmpr_a = DUTY_CYCLE;
    pwm_config->counter_mode = MCPWM_UP_COUNTER;
    pwm_config->duty_mode = MCPWM_DUTY_MODE_1;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, pwm_config);
    mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);


}

/**********************************************************************************
 * GPIO Buzzer Task
**********************************************************************************/
void pwm_buzzer_task(void *){
    while(1) {
        if (detected_interrupt) {
            mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
            vTaskDelay(INTERRUPT_DELAY / portTICK_RATE_MS);
            mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
            detected_interrupt = 0;
        }

        else {
            mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
            detected_interrupt = 0;
            vTaskDelay(10 / portTICK_RATE_MS);
        }
    }
}


/**********************************************************************************
 * MELODY: WiFI Connected
**********************************************************************************/

void pwm_melody_wifi_connected(mcpwm_config_t *pwm_config) {

    pwm_config->frequency = FREQUENCY_WIFI_CONNECTED;  // default no case frequency
    pwm_config->cmpr_a = DUTY_CYCLE;
    pwm_config->counter_mode = MCPWM_UP_COUNTER;
    pwm_config->duty_mode = MCPWM_DUTY_MODE_1;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, pwm_config);
    mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
    vTaskDelay(200 / portTICK_RATE_MS);
    mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
}
