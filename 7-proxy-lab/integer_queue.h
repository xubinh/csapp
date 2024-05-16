#include <semaphore.h>
#include <stddef.h>

typedef struct {
    int *queue;
    size_t max_queue_size;
    size_t front;
    size_t rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
} IntegerQueue;

void integer_queue_initialize(IntegerQueue *integer_queue, size_t size);
void integer_queue_free(IntegerQueue *integer_queue);
void integer_queue_put(IntegerQueue *integer_queue, int value);
int integer_queue_pop(IntegerQueue *integer_queue);
