/* Created by Oleksandra Baga
 * Created for Eptecon Berlin
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "adc_app.h"
#include "spi_app.h"
#include "interrupt_app.h"
#include "mqtt_publish.h"
#include "esp_event.h"
#include "queue_int.h"
#include "interrupt.h"
#include "mqtt_subscribe.h"
#include "battery.h"
#include "nvs.h"

gpio_config_t io_int_conf; // Interrupt from Noise Click

static xQueueHandle gpio_noise_interrupt_queue = NULL;

int volatile detected_interrupt = 0;
int volatile buttonPushed = 0;
int count = 1;
int detected_voltage = 0;
clock_t detected_time;

volatile int numberOfButtonInterrupts = 0;
volatile uint32_t lastDebounceTime;

void remove_outdated_interrupts(TQueue *q, clock_t now, int threshold)
{

    TQ_node *node = q->start;
    while (1) {
        clock_t it = node->interrupt->interruptTime;
        double dt = (double) (now - it) / CLOCKS_PER_SEC;

        if (dt > threshold) {
            TInterrupt *t = q_remove(q);
            free(t);
            node = q->start;
        } else {
            break;
        }
    }

}

int check_threshold(TQueue *q, TInterrupt *ir){
    int flag = 0;

    if (noiseThreshold == -1) {
    	noiseThreshold = THRESHOLD;
    }

    remove_outdated_interrupts(q, ir->interruptTime, T_TH3);

    int amountInterrupts = q_length(q);
    TInterrupt *tmpLastInt = q_getlast(q);
    // calculating the elapsed time
    double elapsedTime = ((double) (ir->interruptTime - tmpLastInt->interruptTime)) / CLOCKS_PER_SEC;

    switch(noiseThreshold){
        case 1:
            // we got more interrupts during the time slot than are allowed
            if (amountInterrupts >= L_TH1 && elapsedTime <= T_TH1) {
                flag = 1;
            }
                // we did't exceed the amount of interrupts and time is still running
            else if (amountInterrupts < L_TH1 && elapsedTime < T_TH1){
                flag = 0;
            }
            break;
        case 2:
            // we got more interrupts during the time slot than are allowed
            if (amountInterrupts >= L_TH2 && elapsedTime <= T_TH2) {
                flag = 1;
            }
                // we did't exceed the amount of interrupts and time is still running
            else if (amountInterrupts < L_TH2 && elapsedTime < T_TH2){
                flag = 0;
            }
            break;
        case 3:
            // we got more interrupts during the time slot than are allowed
            if (amountInterrupts >= L_TH3 && elapsedTime <= T_TH3) {
                flag = 1;
            }
            // we did't exceed the amount of interrupts and time is still running
            else if (amountInterrupts < L_TH3 && elapsedTime < T_TH3){
                flag = 0;
            }
            break;
        case 4:
            // we got more interrupts during the time slot than are allowed
            if (amountInterrupts >= L_TH4 && elapsedTime <= T_TH4) {
                flag = 1;
            }
                // we did't exceed the amount of interrupts and time is still running
            else if (amountInterrupts < L_TH4 && elapsedTime < T_TH4){
                flag = 0;
            }
            break;
        case 5:
            // we got more interrupts during the time slot than are allowed
            if (amountInterrupts >= L_TH5 && elapsedTime <= T_TH5) {
                flag = 1;
            }
                // we did't exceed the amount of interrupts and time is still running
            else if (amountInterrupts < L_TH5 && elapsedTime < T_TH5){
                flag = 0;
            }
            break;
        default:
            printf("Unrecognized Number");
    }
    printf("AOI: %i, ET: %f, Flag is %i\n", amountInterrupts, elapsedTime, flag);
    return flag;
}

/**********************************************************************************
 * ISR Handler
**********************************************************************************/
void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_noise_interrupt_queue, &gpio_num, NULL);
}

/**********************************************************************************
 * GPIO Task
 * Detects an interrupt from the ESP32 inner queue ->
 * reads the voltage on the moment of interrupt
 * sets the interrupt flag ->
 * the RTOS tasks can detect the global variable (interrupt flag) and start perform
 * their own interrupt routines. Separately tasks for each hardware and software task
 * allow to switch on/off some functionality regarding to the client needs.
**********************************************************************************/
void gpio_noise_interrupt_task(void* arg) {
    uint32_t io_num;
    while(1) {
        if(xQueueReceive(gpio_noise_interrupt_queue, &io_num, portMAX_DELAY) && io_num == GPIO_INPUT_INTERRUPT) {
            // get voltage from ADC
            detected_voltage = adc1_to_voltage(ADC1_TEST_CHANNEL, &characteristics);
            printf("%d mV\n", detected_voltage);
            printf("GPIO[%d]: #%d Interrupt\n", io_num, count);

            // add the new interrupt into the queue
            count++;
            detected_time = clock();
            printf("Detected time: %i\n", (int)detected_time);
            TInterrupt *new_interrupt = register_interrupt(count, detected_voltage, detected_time);
            q_add(queueInterrupt, new_interrupt);

            // check threshold and setting/resetting the interrupt flag
            detected_interrupt = check_threshold(queueInterrupt, new_interrupt);
        }
        else {
            // resetting interrupt flag if no interrupt occurs
            detected_interrupt = 0;
        }
    }
}

/**********************************************************************************
 * config GPIO as Input
**********************************************************************************/
void gpio_config_input(gpio_config_t *io_int_conf){

    //interrupt of rising edge
    io_int_conf->intr_type = GPIO_INTR_POSEDGE;
    //bit mask of the pins
    io_int_conf->pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_int_conf->mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_int_conf->pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(io_int_conf);
}

/**********************************************************************************
 * config GPIO as Input for blinking LED
**********************************************************************************/
void blink_interupt_init(){
    gpio_pad_select_gpio(GPIO_OUTPUT_BLINK);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(GPIO_OUTPUT_BLINK, GPIO_MODE_OUTPUT);
    // turn off the led before interrupt arises
    gpio_set_level(GPIO_OUTPUT_BLINK, 0);
}

/**********************************************************************************
 * Blink Function
**********************************************************************************/
void blink_interrupt_task(void* arg){
    while(1) {
        if (detected_interrupt) {
            // Turn off the LED
            gpio_set_level(GPIO_OUTPUT_BLINK, 1);
            // Delay
            vTaskDelay(INTERRUPT_DELAY / portTICK_RATE_MS);
            // Turn off
            gpio_set_level(GPIO_OUTPUT_BLINK, 0);
            detected_interrupt = 0;
        }
        else {
            detected_interrupt = 0;
            vTaskDelay(10 / portTICK_RATE_MS);
        }
    }
}

/**********************************************************************************
 * MQTT Update JSON-File if Interrupt occurs
**********************************************************************************/
void mqtt_interrupt_task(void *arg){
    while(1) {
        if (detected_interrupt) {
            mqtt_update_json_voltage(detected_voltage);
            mqtt_update_json_battery(); // BATTERY TEST
	    mqtt_publish(TOPIC2, mqtt_msg);
            detected_interrupt = 0;
        }
        else {
            detected_interrupt = 0;
            vTaskDelay(10 / portTICK_RATE_MS);
        }
    }
}

/**********************************************************************************
 * config GPIO Interrupt Queue and Handler
**********************************************************************************/
void gpio_init_noise_ISR(){

    //create a queue to handle gpio event from isr
    gpio_noise_interrupt_queue = xQueueCreate(10, sizeof(uint32_t));
    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_INTERRUPT, gpio_isr_handler, (void*) GPIO_INPUT_INTERRUPT);
    gpio_isr_handler_add(GPIO_BUTTON_SUBSCRIBE_INTERRUPT, gpio_isr_handler, (void*) GPIO_BUTTON_SUBSCRIBE_INTERRUPT);

}


/**********************************************************************************
 * GPIO BUTTON Task with Critical Section using FreeRTOS MUTEX
 * the task that called taskENTER_CRITICAL() is guaranteed to remain in the Running state
 * until the critical section is exited, unless the task explicitly attempts to block or yield
 * (which it should not do from inside a critical section).
 *
 * Detects an interrupt on the button GPIO
 * Starts the subscription function to read the settings
 * from the ThingsBoard account
**********************************************************************************/
void gpio_button_subscribe_interrupt_task(void* arg) {
    uint32_t io_num;
    while(1) {
    	//This is the critical IF statement
        if(xQueueReceive(gpio_noise_interrupt_queue, &io_num, portMAX_DELAY) && io_num == GPIO_BUTTON_SUBSCRIBE_INTERRUPT) {
        	numberOfButtonInterrupts++;
        	printf("Button Int# %d\n", numberOfButtonInterrupts);

        	//filter out any noise by setting a time buffer
        	double buttonTime = (clock() - lastDebounceTime) / CLOCKS_PER_SEC;
        	if(buttonTime > BUTTON_DEBOUNCE_DELAY){
        		printf("BUTTON PRESSED Int# %d\n", numberOfButtonInterrupts);
        		lastDebounceTime = clock();
            	buttonPushed = 1; // set the button interrupt flag

            	// RECEIVING THE SETTING FROM THING BOARD VIA MQTT
            	mqtt_get_json_settings();
        	}
        	else{
        		lastDebounceTime = clock();
        	}
        }

        else {
               vTaskDelay(10 / portTICK_RATE_MS);
               buttonPushed = 0; // clear the button interrupt flag
         }
    }
}

