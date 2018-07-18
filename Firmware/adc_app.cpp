#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_system.h"
#include "esp_adc_cal.h"
#include "adc_app.h"
#include "spi_app.h"

esp_adc_cal_characteristics_t characteristics;
uint32_t adc1_voltage;

void adc_init(esp_adc_cal_characteristics_t *characteristics){

    //Init ADC and Characteristics
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_TEST_CHANNEL, ADC_ATTEN);
    esp_adc_cal_get_characteristics(V_REF, ADC_ATTEN, ADC_WIDTH_BIT_12, characteristics);

}
