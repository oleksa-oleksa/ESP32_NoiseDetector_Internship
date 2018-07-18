#include <stdio.h>
#include <stdlib.h>
#include "interrupt.h"

/**********************************************************************************
 * Interrupt structure and functions independent from ESP32 Architecture
**********************************************************************************/
TInterrupt *register_interrupt(int no, int voltage, clock_t reg_time){
    TInterrupt *tmpInterrupt = (TInterrupt*) malloc(sizeof(TInterrupt));

    if (tmpInterrupt){
        tmpInterrupt->interruptNo = no;
        tmpInterrupt->interruptVoltage = voltage;
        tmpInterrupt->interruptTime = reg_time;
        return tmpInterrupt;
    }

    else {
        return NULL;
    }
}