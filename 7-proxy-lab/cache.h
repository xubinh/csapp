#include <semaphore.h>
#include <stddef.h>

typedef struct CacheNode {
    char *key;
    void *value;
    size_t value_size;
    CacheNode *previous_node;
    CacheNode *next_node;
} CacheNode;

typedef struct Cache {
    size_t segment_number;
    size_t max_segment_size;
    size_t max_value_size;
    size_t *current_segment_sizes;
    CacheNode **segments;
    int *reader_counts;
    sem_t *reader_semaphores;
    sem_t *readers_writers_mutexes;
    int *read_only_reader_counts;
    sem_t *read_only_reader_semaphores;
    sem_t *read_only_readers_try_modify_readers_mutexes;
} Cache;

void cache_initialize(Cache *cache, size_t segment_number, size_t max_cache_size, size_t max_value_size);
void cache_free(Cache *cache);
int cache_put(Cache *cache, char *key, size_t key_size, void *new_value, size_t new_value_size);
int cache_get(Cache *cache, char *key, void *destination);