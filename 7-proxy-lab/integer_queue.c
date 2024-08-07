#include "integer_queue.h"
#include "csapp.h"
#include <malloc.h>

void integer_queue_initialize(IntegerQueue *integer_queue, size_t max_queue_size) {
    integer_queue->queue = (int *)Malloc(sizeof(int) * max_queue_size);
    integer_queue->max_queue_size = max_queue_size;
    integer_queue->front = integer_queue->rear = 0;
    Sem_init(&integer_queue->slots, 0, max_queue_size);
    Sem_init(&integer_queue->items, 0, 0);
    Sem_init(&integer_queue->mutex, 0, 1);
}

void integer_queue_free(IntegerQueue *integer_queue) {
    Free(integer_queue->queue);
}

void integer_queue_put(IntegerQueue *integer_queue, int value) {
    P(&integer_queue->slots);
    P(&integer_queue->mutex);
    integer_queue->queue[(integer_queue->rear++) % integer_queue->max_queue_size] = value;
    V(&integer_queue->mutex);
    V(&integer_queue->items);
}

int integer_queue_pop(IntegerQueue *integer_queue) {
    P(&integer_queue->items);
    P(&integer_queue->mutex);
    int value = integer_queue->queue[(integer_queue->front++) % integer_queue->max_queue_size];
    V(&integer_queue->mutex);
    V(&integer_queue->slots);

    return value;
}