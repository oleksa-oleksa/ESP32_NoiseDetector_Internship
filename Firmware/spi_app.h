// Created by Oleksandra Baga on 05.02.18.
// Created for Eptecon Berlin

#pragma once
#include "driver/spi_master.h"

/**********************************************************************************
 * SPI Settings
**********************************************************************************/
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   18

// Vref = (((2048 * V_REF_NOISE_CLICK * GAIN) / 4096) * 11 / 13,2)
// DAC_A, buffered, Gain 1, Output Power Down Control bit = 0111b
#define COMMAND_SPI 0x7
#define V_IN_NOISE_CLICK 255
#define V_REF_DAC 2.3
#define GAIN_V_REF 1

/**********************************************************************************
 * SPI Settings
**********************************************************************************/
extern spi_device_handle_t spi;
extern spi_bus_config_t buscfg;
extern spi_device_interface_config_t devcfg;

/**********************************************************************************
 * SPI BUS Initialisation
 * The CS pin must be held low for the duration of a write command.
**********************************************************************************/
void spi_to_noise_click_bus_init(spi_bus_config_t *buscfg);


/**********************************************************************************
 * SPI SLAVE Initialisation
**********************************************************************************/
void spi_to_noise_click_slave_init(spi_device_interface_config_t *devcfg, spi_device_handle_t *spi);


/**********************************************************************************
 * Send a data to the Noise Click.
 * Creates control word with a custom ref voltage
**********************************************************************************/
void spi_to_noise_click_transmit_cmd(spi_device_handle_t spi, int vInNoiseClick);
