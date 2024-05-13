#include "cache.h"
#include "csapp.h"
#include "proxy_utils.h"
#include <stddef.h>

// 初始化整个缓存:
void cache_initialize(Cache *cache, size_t segment_number, size_t max_cache_size, size_t max_value_size) {
    cache->segment_number = segment_number;

    cache->max_segment_size = max_cache_size / cache->segment_number;
    cache->max_value_size = max_value_size;
    cache->current_segment_sizes = (size_t *)Malloc(sizeof(size_t) * cache->segment_number);

    // 分配存储双向链表头结点的指针的数组:
    cache->segments = (Cache **)Malloc(sizeof(Cache *) * cache->segment_number);

    // 分配读者-写者模型信号量:
    cache->reader_counts = (int *)Malloc(sizeof(int) * cache->segment_number);
    cache->reader_semaphores = (sem_t *)Malloc(sizeof(sem_t) * cache->segment_number);
    cache->readers_writers_mutexes = (sem_t *)Malloc(sizeof(sem_t) * cache->segment_number);

    // 分配只读读者-欲修改读者模型信号量:
    cache->read_only_reader_counts = (int *)Malloc(sizeof(int) * cache->segment_number);
    cache->read_only_reader_semaphores = (sem_t *)Malloc(sizeof(sem_t) * cache->segment_number);
    cache->read_only_readers_try_modify_readers_mutexes = (sem_t *)Malloc(sizeof(sem_t) * cache->segment_number);

    int segment_index;
    for (segment_index = 0; segment_index < cache->segment_number; segment_index++) {
        // 初始化每个段的总大小为零:
        cache->current_segment_sizes[segment_index] = 0;

        // 分配双向链表头结点:
        cache->segments[segment_index] = (Cache *)Malloc(sizeof(Cache));

        // 头结点的前一个结点指针和后一个结点指针初始时均指向自身:
        cache->segments[segment_index]->previous_node = cache->segments[segment_index];
        cache->segments[segment_index]->next_node = cache->segments[segment_index];

        // 初始化读者-写者模型信号量:
        cache->reader_counts[segment_index] = 0;
        Sem_init(cache->reader_semaphores + segment_index, 0, 0);
        Sem_init(cache->readers_writers_mutexes + segment_index, 0, 1);

        // 初始化只读读者-欲修改读者模型信号量:
        cache->read_only_reader_counts[segment_index] = 0;
        Sem_init(cache->read_only_reader_semaphores + segment_index, 0, 0);
        Sem_init(cache->read_only_readers_try_modify_readers_mutexes + segment_index, 0, 1);
    }
}

// 释放缓存中的单个双向链表:
static void cache_free_segment(CacheNode *linked_list) {
    if (!linked_list) {
        return;
    }

    CacheNode *current_node = linked_list;
    CacheNode *next_node = current_node->next_node;
    while (1) {
        Free(current_node);
        if (next_node == linked_list) {
            break;
        }
        current_node = next_node;
        next_node = current_node->next_node;
    }
}

// 释放整个缓存:
void cache_free(Cache *cache) {
    size_t segment_index;

    // 释放段大小数组:
    Free(cache->current_segment_sizes);

    // 释放每个双向链表:
    for (segment_index = 0; segment_index < cache->segment_number; segment_index++) {
        cache_free_segment(cache->segments[segment_index]);
    }

    // 释放双向链表头结点指针数组:
    Free(cache->segments);

    // 释放读者-写者模型信号量:
    Free(cache->reader_counts);
    Free(cache->reader_semaphores);
    Free(cache->readers_writers_mutexes);

    // 释放只读读者-欲修改读者模型信号量:
    Free(cache->read_only_reader_counts);
    Free(cache->read_only_reader_semaphores);
    Free(cache->read_only_readers_try_modify_readers_mutexes);
}

// 将数据复制到缓存中, 并覆盖旧值, 若成功则返回非零值, 否则返回零:
int cache_put(Cache *cache, char *key, size_t key_size, void *new_value, size_t new_value_size) {
    if (!cache || !key || !new_value) {
        return 0;
    }

    if (new_value_size > cache->max_value_size) {
        return 0;
    }

    size_t segment_index = (size_t)hash(key) % cache->segment_number;
    CacheNode *linked_list = cache->segments[segment_index];

    P(cache->readers_writers_mutexes + segment_index);

    // 遍历链表, 检查是否存在给定的键:
    CacheNode *current_node = linked_list->next_node;
    while (current_node != linked_list && strcmp(key, current_node->key) != 0) {
        current_node = current_node->next_node;
    }

    // 如果没有找到结点:
    if (current_node == linked_list) {
        // 新建结点:
        current_node = (CacheNode *)Malloc(sizeof(CacheNode));

        // 存储键:
        current_node->key = (char *)Malloc(sizeof(char) * key_size);
        memcpy(current_node->key, key, key_size);
    }

    // 如果找到结点:
    else {
        // 将其从链表中摘出:
        current_node->previous_node->next_node = current_node->next_node;
        current_node->next_node->previous_node = current_node->previous_node;

        // 释放旧值:
        Free(current_node->value);
        cache->current_segment_sizes[segment_index] -= current_node->value_size;
    }

    // 如果缓存大小不足, 还需要弹出足够多的末尾结点以腾出空间:
    while (cache->current_segment_sizes[segment_index] + new_value_size > cache->max_segment_size) {
        CacheNode *tail_node = linked_list->previous_node;
        tail_node->previous_node->next_node = linked_list;
        linked_list->previous_node = tail_node->previous_node;
        Free(tail_node->key);
        Free(tail_node->value);
        cache->current_segment_sizes[segment_index] -= tail_node->value_size;
    }

    // 存储新值:
    current_node->value = Malloc(sizeof(void) * new_value_size);
    memcpy(current_node->value, new_value, new_value_size);
    current_node->value_size = new_value_size;
    cache->current_segment_sizes[segment_index] += current_node->value_size;

    // 然后将其插入头结点之后 (即 LRU):
    linked_list->next_node->previous_node = current_node;
    current_node->next_node = linked_list->next_node;
    linked_list->next_node = current_node;
    current_node->previous_node = linked_list;

    V(cache->readers_writers_mutexes + segment_index);
}

// 从缓存中复制数据至外部缓冲区, 若成功则返回非零值, 否则返回零:
int cache_get(Cache *cache, char *key, void *destination) {
    if (!cache || !key || !destination) {
        return 0;
    }

    size_t segment_index = (size_t)hash(key) % cache->segment_number;
    CacheNode *linked_list = cache->segments[segment_index];

    P(cache->reader_semaphores + segment_index);
    if (cache->reader_counts[segment_index] == 0) {
        P(cache->readers_writers_mutexes + segment_index);
    }
    cache->reader_counts[segment_index]++;
    V(cache->reader_semaphores + segment_index);

    P(cache->read_only_reader_semaphores + segment_index);
    if (cache->read_only_reader_counts[segment_index] == 0) {
        P(cache->read_only_readers_try_modify_readers_mutexes + segment_index);
    }
    cache->read_only_reader_counts[segment_index]++;
    V(cache->read_only_reader_semaphores + segment_index);

    // 只读阶段, 遍历链表, 检查是否存在给定的键:
    CacheNode *current_node = linked_list->next_node;
    while (current_node != linked_list && strcmp(key, current_node->key) != 0) {
        current_node = current_node->next_node;
    }

    // 查找结束后立即退出只读阶段:
    P(cache->read_only_reader_semaphores + segment_index);
    cache->read_only_reader_counts[segment_index]--;
    if (cache->read_only_reader_counts[segment_index] == 0) {
        V(cache->read_only_readers_try_modify_readers_mutexes + segment_index);
    }
    V(cache->read_only_reader_semaphores + segment_index);

    // 如果没找到则直接返回 False:
    if (current_node == linked_list) {
        return 0;
    }

    // 如果找到则将数据复制到调用者提供的缓冲区:
    memcpy(destination, current_node->value, current_node->value_size);

    // 然后进入欲修改阶段, 将其插入头结点之后 (即 LRU):
    P(cache->read_only_readers_try_modify_readers_mutexes + segment_index);

    linked_list->next_node->previous_node = current_node;
    current_node->next_node = linked_list->next_node;
    linked_list->next_node = current_node;
    current_node->previous_node = linked_list;

    V(cache->read_only_readers_try_modify_readers_mutexes + segment_index);
}