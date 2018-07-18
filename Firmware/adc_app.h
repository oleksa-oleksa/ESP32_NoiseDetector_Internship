// Created by Oleksandra Baga on 09.02.18.
// Created for Eptecon Berlin

#pragma once
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define V_REF   1100
#define ADC1_TEST_CHANNEL (ADC1_CHANNEL_6)  //GPIO 34
#define ADC_ATTEN ADC_ATTEN_DB_0

/**********************************************************************************
 * ADC Settings
**********************************************************************************/
extern esp_adc_cal_characteristics_t characteristics;
extern uint32_t adc1_voltage;

void adc_init(esp_adc_cal_characteristics_t *characteristics);
