#include <stdio.h>
#include <stdlib.h>
#include "queue_int.h"
#include "interrupt_app.h"

TQueue *queueInterrupt;

int q_isEmpty(TQueue *q)
{
    return (q->start == NULL) && (q->end == NULL);
}

TQueue *q_new()
{
    TQueue *q = (TQueue *) malloc(sizeof(TQueue));
    q->start = NULL;
    q->end = NULL;
    return q;
}

int q_length(TQueue *q)
{
    int len = 0;
    TQ_node *n = q->start;

    while (n) {
        len++;
        n = n->next;
    }
    return len;
}

void q_add(TQueue *q, TInterrupt *ir)
{
    if (ir == NULL)
    {
        return;
    }
    TQ_node *newNode = (TQ_node *) malloc(sizeof(TQ_node));
    if (newNode == NULL)
    {
        printf("Memory allocation error for interrupt ID %d\n", ir->interruptNo);
        return;
    }
    newNode->next = NULL;
    newNode->interrupt = ir;

    // Queue is empty => the first and only one process will be added
    if (q_isEmpty(q))
    {
        q->start = newNode;
    }
    else // if queue is not empty
    {
        q->end->next = newNode; // moves next on the new p
    }
    q->end = newNode; // places the new end of the queue
}

TInterrupt *q_remove(TQueue *q)
{
    if (q_isEmpty(q))
    {
        return NULL;
    }

    TQ_node *deNode = q->start;
    if (q->start == q->end) {
        q->start = q->end = NULL;
    } else {
        q->start = q->start->next;
    }
    TInterrupt *deInt = deNode->interrupt;
    free(deNode);
    return deInt;
}

TInterrupt *q_getlast(TQueue *q){
    if (q_isEmpty(q)){
        return NULL;
    }

    return q->start->interrupt;
    /*
    if (q->start == q->end) {
        return q->start->interrupt;
    } else {
        q->start = q->start->next;
    }

    TInterrupt *lastInt = lastNode->interrupt;
    return lastInt;
    */
}


void q_print(TQueue *q)
{
    if (q_isEmpty(q))
    {   printf("The queue is empty\n");
        return;
    }

    int count = 1;
    TQ_node *tmpNode = q->start;

    printf("The current queue:\n");
    while (tmpNode != NULL)
    {
        printf("%02d. Interrupt ID %d\n", count++, tmpNode->interrupt->interruptNo);
        tmpNode = tmpNode->next;
    };
}
