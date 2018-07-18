//
// Created by Oleksandra Baga on 26.03.18.
//

#pragma once

#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************
 * Interrupt structure and functions independent from ESP32 Architecture
**********************************************************************************/
typedef struct
{
    int interruptNo;
    int interruptVoltage;
    int interruptTime;

} TInterrupt;


/**********************************************************************************
 * Creates new interrupt
**********************************************************************************/
TInterrupt *register_interrupt(int no, int voltage, clock_t reg_time);



#ifdef __cplusplus
}
#endif