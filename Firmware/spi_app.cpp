/* Created by Oleksandra Baga
 * Created for Eptecon Berlin
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include "driver/spi_master.h"
#include "esp_system.h"
#include "spi_app.h"
#include "adc_app.h"
#include "interrupt_app.h"

spi_device_handle_t spi;
spi_bus_config_t buscfg;
spi_device_interface_config_t devcfg;

/**********************************************************************************
 * SPI BUS Initialisation
 * The CS pin must be held low for the duration of a write command.
**********************************************************************************/
void spi_to_noise_click_bus_init(spi_bus_config_t *buscfg){
    //Init SPI
    esp_err_t ret;
    buscfg->miso_io_num = -1;
    buscfg->mosi_io_num = PIN_NUM_MOSI;
    buscfg->sclk_io_num = PIN_NUM_CLK;
    buscfg->quadwp_io_num = -1;
    buscfg->quadhd_io_num = -1;

    //Initialize the SPI bus
    ret = spi_bus_initialize(HSPI_HOST, buscfg, 1);
    assert(ret == ESP_OK);

}

/**********************************************************************************
 * SPI SLAVE Initialisation
**********************************************************************************/
void spi_to_noise_click_slave_init(spi_device_interface_config_t *devcfg, spi_device_handle_t *spi){
    esp_err_t ret;
    //Clock out at 20 MHz
    devcfg->clock_speed_hz = 20 * 1000 * 1000;
    devcfg->mode = 0;
    devcfg->spics_io_num = PIN_NUM_CS;
    devcfg->queue_size = 1;

    //Attach the Slave to the SPI bus
    ret = spi_bus_add_device(HSPI_HOST, devcfg, spi);
    assert(ret == ESP_OK);

}
/**********************************************************************************
 * Sends a data to the Noise Click.
 * Creates control word with a custom ref voltage
**********************************************************************************/
void spi_to_noise_click_transmit_cmd(spi_device_handle_t spi, int vInNoiseClick){

    // creating the CW + Reference Voltage
    uint16_t data = ((COMMAND_SPI & 0xF) << 12) | (vInNoiseClick & 0xFFF);
    // solving the issue with big endian and little endian
    data = ((data & 0xFF) << 8) | ((data >> 8) & 0xFF);

    // calculating decimal value V_out, PIN 8, MCP4921, V_ref
    int V_out = ((V_REF_DAC * vInNoiseClick * GAIN_V_REF) / 4096) * 1000;
    int V_ref = (V_out * 11) / 13.2;

    printf("SPI: transmit decimal: %d\n", vInNoiseClick);
    printf("SPI: transmit HEX: %x\n", data);
    printf("V_out = %d mV\n", V_out);
    printf("V_ref = %d mV\n", V_ref);

    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t)); //Zero out the transaction
    t.length = 16;                  //length is in bits.
    t.rx_buffer = NULL;
    t.tx_buffer = &data;            //Data
    t.user = (void *) 1;

    // Transmit the CW
    ret = spi_device_transmit(spi, &t);
    assert(ret == ESP_OK);
    printf("Transmitted\n\n");
}
