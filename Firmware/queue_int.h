// Created by Oleksandra Baga on 10.12.17.
// Updated for Eptecon by Oleksandra Baga on 20.03.18

// Queue FIFO (First In First Out)

#pragma once

#include "interrupt_app.h"
#include "interrupt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SQ_node
{
    struct SQ_node *next;
    TInterrupt *interrupt;
} TQ_node;

typedef struct SQueue
{
    TQ_node *start;
    TQ_node *end;
} TQueue;

TQueue *q_new();
void q_add(TQueue *q, TInterrupt *ir);
TInterrupt *q_remove(TQueue *q);
TInterrupt *q_getlast(TQueue *q);
void q_print(TQueue *q);
int q_length(TQueue *q);

extern TQueue *queueInterrupt;

#ifdef __cplusplus
}
#endif