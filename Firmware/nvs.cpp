//
// Created by Oleksandra Baga on 12.05.18.
//

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_err_to_name.c"
#include "nvs.h"

int16_t referenceVoltage = -1;
int16_t isBuzzerOn = -1;
int16_t isLedOn = -1;
int16_t noiseThreshold = -1;
esp_err_t err;

/******************************************************/

void nvs_init() {
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

nvs_handle nvs_esp32_open() {
    nvs_handle nvc_open_handle;
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    err = nvs_open("storage", NVS_READWRITE, &nvc_open_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");
    }
    return nvc_open_handle;
}

int nvs_read_value(int varType) {
    int16_t savedValue;

    nvs_handle handle = nvs_esp32_open();

    switch (varType) {
        case NO_VARIABLE:
            break;
        case VARIABLE_SET_VOLTAGE:
            printf("Reading reference voltage from NVS ...");
            err = nvs_get_i16(handle, "refVoltage", &savedValue);
            break;
        case VARIABLE_SET_SOUND:
            printf("Reading buzzer settings from NVS ...");
            err = nvs_get_i16(handle, "buzzer", &savedValue);
            break;
        case VARIABLE_SET_LED:
            printf("Reading LED settings from NVS ...");
            err = nvs_get_i16(handle, "led", &savedValue);
            break;
        case VARIABLE_SET_THRESHOLD:
            printf("Reading noise threshold from NVS ...");
            err = nvs_get_i16(handle, "threshold", &savedValue);
            break;
        default:
            break;
        }

    switch (err) {
        case ESP_OK:
            printf("Done\n");
            printf("NVS Value = %d\n\n", savedValue);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            printf("The value is not initialized yet!\n");
            savedValue = -1;
            break;
        default :
            printf("Error (%s) reading!\n", esp_err_to_name(err));
            savedValue = -1;
    }

    nvs_close(handle);

    return savedValue;

}

void nvs_write_value(int varType, int16_t value) {

    nvs_handle handle = nvs_esp32_open();

    switch (varType) {
        case NO_VARIABLE:
            break;
        case VARIABLE_SET_VOLTAGE:
            printf("Updating reference voltage in NVS ... ");
            err = nvs_set_i16(handle, "refVoltage", value);
            break;
        case VARIABLE_SET_SOUND:
            printf("Reading reference voltage from NVS ...");
            err = nvs_set_i16(handle, "buzzer", value);
            break;
        case VARIABLE_SET_LED:
            printf("Reading reference voltage from NVS ...");
            err = nvs_set_i16(handle, "led", value);
            break;
        case VARIABLE_SET_THRESHOLD:
            printf("Reading reference voltage from NVS ...");
            err = nvs_set_i16(handle, "threshold", value);
            break;
        default:
            break;
    }

    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    printf("Committing updates in NVS ... ");
    err = nvs_commit(handle);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

    nvs_close(handle);
}
