//
// Created by Oleksandra Baga on 12.05.18.


/* Variables for saving in non-volatile storage of ESP32
 * Reference voltage for SPI Programming of Mic
 * Buzzer settings on/off
 * LED settings on/of
 * Threshold value
 * SSID and Password for Wifi (done as a part of BootWifi.cpp)
 *
 * READ:
 * Boot, reboot (check if there are any custom variables. Use default defines if no variables found)
 *
 * Write:
 * after each subscription that obtains any of values and this new value is not the same as an old one (avoiding the case when user plays with widgets)
 *
*/

#pragma once

#include "nvs_flash.h"

extern int16_t referenceVoltage;
extern int16_t isBuzzerOn;
extern int16_t isLedOn;
extern int16_t noiseThreshold;


enum NVS_VARIABLE_TYPE {
    NO_VARIABLE = 0,
    VARIABLE_SET_VOLTAGE = 1,
    VARIABLE_SET_SOUND = 2,
    VARIABLE_SET_LED = 3,
    VARIABLE_SET_THRESHOLD = 4
};

void nvs_init();

nvs_handle nvs_esp32_open();

int nvs_read_value(int varType);

void nvs_write_value(int varType, int16_t value);
