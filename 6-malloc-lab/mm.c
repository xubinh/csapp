/*
 * mm.c - 一个简单的动态内存分配器的实现, 包含分配 (`mm_alloc`), 释放 (`mm_free`), 以及重分配 (`mm_realloc`). 实现策略为分离适配 + 最佳适配.
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "xubinh-team",
    /* First member's full name */
    "xubinh",
    /* First member's email address */
    "xubinh@email.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    "",
};

// 是否调试:
// #define DEBUG ((unsigned int)1)
#define DEBUG ((unsigned int)0)

// 系统页大小 (假设为 2 的幂):
static size_t PAGE_SIZE;

// `void *` 类型变量的大小 (假设为 4 的倍数):
static size_t POINTER_SIZE = sizeof(char *);

// `size_t` 类型变量的大小 (假设为 4 的倍数):
static size_t SIZE_T_SIZE = sizeof(size_t);

// 对齐大小, 固定为 8, 与官方 malloc 保持一致:
static size_t ALIGN_SIZE = (size_t)8;

// 块大小掩码 (已知块大小为 8 的倍数, 因此最低 3 位必然为 0):
static size_t BLOCK_SIZE_MASK = ~(size_t)7;

// 当前块的分配状态位掩码 (最低有效位):
static size_t CURRENT_BLOCK_ALLOCATE_STATUS_MASK = (size_t)1;

// 前一个块的分配状态位掩码 (次低有效位):
static size_t PREVIOUS_BLOCK_ALLOCATE_STATUS_MASK = (((size_t)1) << 1);

// 最小块 (由两个头部以及两个链表指针构成) 的大小:
static size_t MIN_BLOCK_SIZE = 2 * (sizeof(size_t) + sizeof(char *));

// 最小的等价类上界 (等于 "不小于最小块大小的 2 的幂" 中的最小值):
static size_t MIN_EQUIVALENCE_CLASS_UPPER_BOUND;

// 空闲链表数组 (其元素存储于堆空间的起始位置):
static void *LINKED_LIST_ARRAY_POINTER;

// 结尾块大小 (结尾块仅由一个头部构成):
static size_t EPILOGUE_BLOCK_SIZE = sizeof(size_t);

// 对齐任意长度至合法块长度:
#define ALIGN(size) (((size_t)(size) - (size_t)(1) + (size_t)(ALIGN_SIZE)) & (size_t)(BLOCK_SIZE_MASK))

// 从当前块的头部中获取当前块的大小, 当前块的分配状态, 以及前一个块的分配状态:
#define GET_BLOCK_SIZE_FROM_BP(bp) ((*(size_t *)(bp)) & (size_t)(BLOCK_SIZE_MASK))
#define GET_CURRENT_BLOCK_ALLOCATE_STATUS_FROM_BP(bp) ((*(size_t *)(bp)) & (size_t)(CURRENT_BLOCK_ALLOCATE_STATUS_MASK))
#define GET_PREVIOUS_BLOCK_ALLOCATE_STATUS_FROM_BP(bp) ((*(size_t *)(bp)) & (size_t)(PREVIOUS_BLOCK_ALLOCATE_STATUS_MASK))

// 在当前块的头部中设置当前块的大小:
#define SET_BLOCK_SIZE_BY_BP(bp, block_size) ((*(size_t *)(bp)) = ((size_t)(block_size) & (size_t)BLOCK_SIZE_MASK) | ((*(size_t *)(bp)) & (~(size_t)BLOCK_SIZE_MASK)))

// 在当前块的头部中设置/重置当前块的分配状态, 以及前一个块的分配状态:
#define SET_CURRENT_BLOCK_ALLOCATE_STATUS_BY_BP(bp) ((*(size_t *)(bp)) |= (size_t)(CURRENT_BLOCK_ALLOCATE_STATUS_MASK))
#define UNSET_CURRENT_BLOCK_ALLOCATE_STATUS_BY_BP(bp) ((*(size_t *)(bp)) &= ~(size_t)(CURRENT_BLOCK_ALLOCATE_STATUS_MASK))
#define SET_PREVIOUS_BLOCK_ALLOCATE_STATUS_BY_BP(bp) ((*(size_t *)(bp)) |= (size_t)(PREVIOUS_BLOCK_ALLOCATE_STATUS_MASK))
#define UNSET_PREVIOUS_BLOCK_ALLOCATE_STATUS_BY_BP(bp) ((*(size_t *)(bp)) &= ~(size_t)(PREVIOUS_BLOCK_ALLOCATE_STATUS_MASK))

// 将空闲块的头部复制至尾部:
#define SET_FOOTER_BY_BP(bp) (*(size_t *)((char *)(bp) + (size_t)GET_BLOCK_SIZE_FROM_BP(bp) - (size_t)(SIZE_T_SIZE)) = *(size_t *)(bp))

// 在块地址和载荷地址之间进行切换:
#define GET_BP_FROM_P(p) ((void *)((char *)(p) - (size_t)(SIZE_T_SIZE)))
#define GET_P_FROM_BP(bp) ((void *)((char *)(bp) + (size_t)(SIZE_T_SIZE)))

// 将载荷大小对齐为块大小:
#define GET_BLOCK_SIZE_FROM_PAYLOAD_SIZE(payload_size) (ALIGN((size_t)(SIZE_T_SIZE) + (size_t)(payload_size)))

// 在堆中的块之间进行前后移动:
#define GET_PREVIOUS_BP_FROM_BP(bp) ((void *)((char *)(bp) - ((size_t)((*(size_t *)((char *)(bp) - (size_t)(SIZE_T_SIZE))) & (size_t)(BLOCK_SIZE_MASK)))))
#define GET_NEXT_BP_FROM_BP(bp) ((void *)((char *)(bp) + GET_BLOCK_SIZE_FROM_BP(bp)))

// 双向链表操作:
#define GET_LINKED_LIST_POINTER_BY_INDEX(idx) ((void *)((char *)(LINKED_LIST_ARRAY_POINTER) + (size_t)(idx) * (size_t)MIN_BLOCK_SIZE))
#define GET_PREVIOUS_LINKED_LIST_BP_FROM_BP(bp) ((void *)(*(char **)((char *)(bp) + (size_t)(SIZE_T_SIZE))))
#define SET_PREVIOUS_LINKED_LIST_BP_BY_BP(bp, previous_bp) ((*(char **)((char *)(bp) + (size_t)(SIZE_T_SIZE))) = (char *)(previous_bp))
#define GET_NEXT_LINKED_LIST_BP_FROM_BP(bp) ((void *)(*(char **)((char *)(bp) + (size_t)(SIZE_T_SIZE) + (size_t)(POINTER_SIZE))))
#define SET_NEXT_LINKED_LIST_BP_BY_BP(bp, next_bp) ((*(char **)((char *)(bp) + (size_t)(SIZE_T_SIZE) + (size_t)(POINTER_SIZE))) = (char *)(next_bp))

/**
 * @brief 将任意块格式化为空闲块 (前提是设置好了块大小)
 *
 * @param bp
 */
static void _format_free_block(void *bp) {
    DEBUG &&printf("[_format_free_block]\n");

    // 设置当前块的状态为 "空闲":
    UNSET_CURRENT_BLOCK_ALLOCATE_STATUS_BY_BP(bp);

    // 将头部复制至尾部:
    SET_FOOTER_BY_BP(bp);

    // 将后一个块的 "前一个块的分配状态位" 设置为 "空闲":
    void *next_block = GET_NEXT_BP_FROM_BP(bp);
    UNSET_PREVIOUS_BLOCK_ALLOCATE_STATUS_BY_BP(next_block);

    // 如果后一个块是空闲块那么还需要将头部同步至尾部:
    !GET_CURRENT_BLOCK_ALLOCATE_STATUS_FROM_BP(next_block) && SET_FOOTER_BY_BP(next_block);
}

/**
 * @brief 若堆中的前一个块为空闲块则返回该块, 否则返回 NULL
 *
 * @param bp
 * @return void*
 */
static void *_get_previous_free_block(void *bp) {
    DEBUG &&printf("[_get_previous_free_block]\n");

    // 前一个块已分配的话返回 NULL, 否则返回前一个块的指针:
    void *previous_free_block = GET_PREVIOUS_BLOCK_ALLOCATE_STATUS_FROM_BP(bp) ? NULL : GET_PREVIOUS_BP_FROM_BP(bp);

    return previous_free_block;
}

/**
 * @brief 若堆中的前一个块为空闲块则返回该块, 否则返回 NULL
 *
 * @param bp
 * @return void*
 */
static void *_get_next_free_block(void *bp) {
    DEBUG &&printf("[_get_next_free_block]\n");

    void *next_block = GET_NEXT_BP_FROM_BP(bp);

    // 后一个块已分配的话返回 NULL, 否则返回后一个块的指针:
    void *next_free_block = GET_CURRENT_BLOCK_ALLOCATE_STATUS_FROM_BP(next_block) ? NULL : next_block;

    return next_free_block;
}

/**
 * @brief 将空闲块从链表中摘出
 *
 * @param bp
 */
static void _pick_from_free_list(void *bp) {
    DEBUG &&printf("[_pick_from_free_list]\n");
    DEBUG &&printf("当前块地址: %p\n", bp);
    // 前块:
    void *previous_linked_list_bp = GET_PREVIOUS_LINKED_LIST_BP_FROM_BP(bp);
    DEBUG &&printf("前一个块地址: %p\n", previous_linked_list_bp);

    // 后块:
    void *next_linked_list_bp = GET_NEXT_LINKED_LIST_BP_FROM_BP(bp);
    DEBUG &&printf("后一个块地址: %p\n", next_linked_list_bp);

    // 将前块连接至后块:
    SET_NEXT_LINKED_LIST_BP_BY_BP(previous_linked_list_bp, next_linked_list_bp);

    DEBUG &&printf("成功将前块连接至后块\n");

    // 将后块连接至前块:
    SET_PREVIOUS_LINKED_LIST_BP_BY_BP(next_linked_list_bp, previous_linked_list_bp);

    DEBUG &&printf("成功将后块连接至前块\n");
}

/**
 * @brief 对相邻两个空闲块进行合并
 *
 * @param bp
 * @param next_bp
 */
static void _coalesce_current_and_next_free_block(void *bp, void *next_bp) {
    DEBUG &&printf("[_coalesce_current_and_next_free_block]\n");

    // 获取当前块的大小:
    size_t total_size = GET_BLOCK_SIZE_FROM_BP(bp);

    // 加上下一个块的大小得到总大小:
    total_size += GET_BLOCK_SIZE_FROM_BP(next_bp);

    // 设置当前块的大小:
    SET_BLOCK_SIZE_BY_BP(bp, total_size);

    // 复制头部至尾部:
    SET_FOOTER_BY_BP(bp);
}

/**
 * @brief 对空闲块进行合并, 返回合并后的大空闲块
 *
 * @param bp
 * @return void*
 */
static void *_coalesce(void *bp) {
    DEBUG &&printf("[_coalesce]\n");

    void *free_block;

    // 合并当前块和后一个块:
    if ((free_block = _get_next_free_block(bp))) {
        // 将后一个块从链表中摘出:
        _pick_from_free_list(free_block);

        // 合并两个块:
        _coalesce_current_and_next_free_block(bp, free_block);
    }

    // 合并前一个块和当前块:
    if ((free_block = _get_previous_free_block(bp))) {
        // 将前一个块从链表中摘出:
        _pick_from_free_list(free_block);

        // 合并两个块:
        _coalesce_current_and_next_free_block(free_block, bp);

        // 前一个块的头部成为新空闲块的头部:
        bp = free_block;
    }

    return bp;
}

/**
 * @brief 在链表中查找合适的空闲块, 策略为最佳适配
 *
 * @param block_size
 * @return void*
 */
static void *_find_in_free_lists(size_t block_size) {
    DEBUG &&printf("[_find_in_free_lists]\n");

    size_t current_equivalence_class_upper_bound;
    int idx = 0;

    // 查找最小等价类到次大等价类:
    for (current_equivalence_class_upper_bound = MIN_EQUIVALENCE_CLASS_UPPER_BOUND; current_equivalence_class_upper_bound <= PAGE_SIZE; current_equivalence_class_upper_bound <<= 1, ++idx) {
        if (current_equivalence_class_upper_bound < block_size) {
            continue;
        }

        DEBUG &&printf("当前等价类上界: %d\n", current_equivalence_class_upper_bound);

        void *head_node = GET_LINKED_LIST_POINTER_BY_INDEX(idx);
        void *previous_node = head_node;
        void *current_node;
        size_t current_block_size;
        void *best_node = NULL;
        size_t best_block_size = ((size_t)0) - 1;

        // 不断查找下一个结点, 直至下一个结点重新回到链表:
        while ((current_node = GET_NEXT_LINKED_LIST_BP_FROM_BP(previous_node)) != head_node) {
            if ((current_block_size = GET_BLOCK_SIZE_FROM_BP(current_node)) >= block_size && current_block_size < best_block_size) {
                best_block_size = current_block_size;
                best_node = current_node;
            }
            previous_node = current_node;
        }

        // 如果下一个结点满足放置条件:
        if (best_node) {
            // 将其从链表中摘下:
            _pick_from_free_list(best_node);

            // 返回该结点:
            return best_node;
        }
    }

    // 查找最大等价类 (重复上面的步骤):
    {
        DEBUG &&printf("当前等价类上界: ∞\n");

        void *head_node = GET_LINKED_LIST_POINTER_BY_INDEX(idx);
        void *previous_node = head_node;
        void *current_node;
        size_t current_block_size;
        void *best_node = NULL;
        size_t best_block_size = ((size_t)0) - 1;

        // 不断查找下一个结点, 直至下一个结点重新回到链表:
        while ((current_node = GET_NEXT_LINKED_LIST_BP_FROM_BP(previous_node)) != head_node) {
            if ((current_block_size = GET_BLOCK_SIZE_FROM_BP(current_node)) >= block_size && current_block_size < best_block_size) {
                best_block_size = current_block_size;
                best_node = current_node;
            }
            previous_node = current_node;
        }

        // 如果下一个结点满足放置条件:
        if (best_node) {
            // 将其从链表中摘下:
            _pick_from_free_list(best_node);

            // 返回该结点:
            return best_node;
        }
    }

    // 一个都没有找到那么返回空指针:
    return NULL;
}

/**
 * @brief 拓展堆空间, 将拓展后得到的空间初始化为空闲块, 并且不对其进行合并
 *
 * @param new_free_block_size
 * @return void*
 */
static void *_extend(size_t new_free_block_size) {
    DEBUG &&printf("[_extend]\n");

    // 新的空闲块 (从结尾块后面开始延伸堆空间, 同时在逻辑上将新结尾块移至延伸后的堆空间的末尾,
    // 旧结尾块的位置用于放置新空闲块的头部):
    void *new_free_block = mem_sbrk(new_free_block_size);
    if (new_free_block == (void *)-1) {
        printf("error: out of memory\n");
        exit(1);
    }
    new_free_block = (void *)((char *)(new_free_block)-EPILOGUE_BLOCK_SIZE);

    // 新的结尾块:
    void *new_epilogue_block_pointer = (void *)((char *)(new_free_block) + new_free_block_size);

    // 先格式化新结尾块:
    SET_CURRENT_BLOCK_ALLOCATE_STATUS_BY_BP(new_epilogue_block_pointer); // 结尾块从不释放
    SET_BLOCK_SIZE_BY_BP(new_epilogue_block_pointer, (size_t)0);         // 结尾块长度设置为 0
    // 结尾块的前一个块的分配状态由调用者设置.

    // 设置新的空闲块的大小:
    SET_BLOCK_SIZE_BY_BP(new_free_block, new_free_block_size);

    // 格式化新空闲块:
    _format_free_block(new_free_block);

    // 返回新空闲块:
    return new_free_block;
}

/**
 * @brief 将空闲块插入链表
 *
 * @param bp
 */
static void _insert_to_free_lists(void *bp) {
    DEBUG &&printf("[_insert_to_free_lists]\n");

    size_t block_size = GET_BLOCK_SIZE_FROM_BP(bp);
    int current_equivalence_class_upper_bound;
    int idx = 0;

    // 根据等价类上界寻找正确的链表:
    for (current_equivalence_class_upper_bound = MIN_EQUIVALENCE_CLASS_UPPER_BOUND; current_equivalence_class_upper_bound <= PAGE_SIZE; current_equivalence_class_upper_bound <<= 1, ++idx) {
        if (block_size <= current_equivalence_class_upper_bound) {
            break;
        }
    }

    // 插入链表:
    void *head_node = GET_LINKED_LIST_POINTER_BY_INDEX(idx);
    void *next_node = GET_NEXT_LINKED_LIST_BP_FROM_BP(head_node);
    SET_NEXT_LINKED_LIST_BP_BY_BP(head_node, bp);
    SET_NEXT_LINKED_LIST_BP_BY_BP(bp, next_node);
    SET_PREVIOUS_LINKED_LIST_BP_BY_BP(next_node, bp);
    SET_PREVIOUS_LINKED_LIST_BP_BY_BP(bp, head_node);
}

/**
 * @brief 将载荷分配给空闲块, 并做必要的分割, 返回载荷的首地址
 *
 * @param block_size
 * @param bp
 * @return void*
 */
static void *_place(size_t block_size, void *bp) {
    DEBUG &&printf("[_place]\n");

    // 传入的空闲块的真实大小:
    size_t total_block_size = GET_BLOCK_SIZE_FROM_BP(bp);

    DEBUG &&printf("所需的块大小: %d\n", block_size);

    DEBUG &&printf("传入的块大小: %d\n", total_block_size);

    // 如果有多余的空间则进行分割:
    if (block_size + MIN_BLOCK_SIZE <= total_block_size) {
        DEBUG &&printf("可以分割\n");
        size_t free_block_size = total_block_size - block_size;

        void *free_block = (void *)((char *)(bp) + block_size);

        DEBUG &&printf("空闲块地址: %p\n", free_block);
        DEBUG &&printf("空闲块大小: %d - %d = %d\n", total_block_size, block_size, free_block_size);

        // 设置空闲块大小:
        SET_BLOCK_SIZE_BY_BP(free_block, free_block_size);

        // 设置前一个块的分配状态为 "已分配":
        SET_PREVIOUS_BLOCK_ALLOCATE_STATUS_BY_BP(free_block);

        // 格式化空闲块:
        _format_free_block(free_block);

        // 将合并后的空闲块插入空闲链表:
        _insert_to_free_lists(free_block);
    }

    else {
        block_size = total_block_size;
    }

    DEBUG &&printf("块地址: %p\n", bp);

    // 设置当前块的大小:
    SET_BLOCK_SIZE_BY_BP(bp, block_size);

    // 设置当前块的状态为 "已分配":
    SET_CURRENT_BLOCK_ALLOCATE_STATUS_BY_BP(bp);

    // 将后一个块的 "前一个块的分配状态位" 设置为 "已分配":
    SET_PREVIOUS_BLOCK_ALLOCATE_STATUS_BY_BP(GET_NEXT_BP_FROM_BP(bp));

    // 返回载荷的首地址:
    return (void *)((char *)(bp) + SIZE_T_SIZE);
}

/**
 * @brief 检查堆中所有块的合理性, 返回空闲块数量
 *
 * @return int
 */
static int _check_heap_format(void) {
    int free_block_number = 0;

    int current_equivalence_class_upper_bound;
    int idx = 0;

    // 跳过所有链表:
    for (current_equivalence_class_upper_bound = MIN_EQUIVALENCE_CLASS_UPPER_BOUND; current_equivalence_class_upper_bound <= PAGE_SIZE; current_equivalence_class_upper_bound <<= 1, ++idx)
        ;
    idx++;

    // 获取第一个块的地址:
    void *current_bp = GET_LINKED_LIST_POINTER_BY_INDEX(idx);

    size_t real_previous_block_allocate_status = (size_t)1;

    // 遍历堆中每一个块直至遇到结尾块 (长度为零):
    size_t current_block_size;
    while ((current_block_size = GET_BLOCK_SIZE_FROM_BP(current_bp))) {
        // 获取当前块和前一个块的分配状态:
        size_t current_block_allocate_status = GET_CURRENT_BLOCK_ALLOCATE_STATUS_FROM_BP(current_bp);
        size_t previous_block_allocate_status = GET_PREVIOUS_BLOCK_ALLOCATE_STATUS_FROM_BP(current_bp);

        // 如果连续两个块都是空闲块则报错:
        if (current_block_allocate_status == real_previous_block_allocate_status && !real_previous_block_allocate_status) {
            DEBUG &&printf("连续两个块都是空闲块, 当前块地址: %p\n", current_bp);
            return -1;
        }

        // 检查当前块的头部中记录的前一个块的分配状态是否与前一个块的真实分配状态一致:
        if ((!!previous_block_allocate_status) != (!!real_previous_block_allocate_status)) {
            DEBUG &&printf("头部中记录的前一个块的分配状态与前一个块的真实分配状态不一致, 当前块地址: %p\n", current_bp);
            return -1;
        }

        // 如果是空闲块则还需要检查头部和尾部是否相同:
        if (!current_block_allocate_status) {
            // 空闲块数量增加:
            free_block_number++;

            if (*(size_t *)(current_bp) != *(size_t *)((char *)current_bp + current_block_size - SIZE_T_SIZE)) {
                DEBUG &&printf("头部和尾部不相同, 当前块地址: %p\n", current_bp);
                return -1;
            }
        }

        // 跳到后一个块:
        current_bp = GET_NEXT_BP_FROM_BP(current_bp);
        real_previous_block_allocate_status = current_block_allocate_status;
    }

    return free_block_number;
}

/**
 * @brief 检查空闲链表的合理性, 返回空闲块的数量
 *
 * @return int
 */
static int _check_free_lists(void) {
    int free_block_number = 0;

    int current_equivalence_class_upper_bound;
    int idx = 0;

    // 检查从最小到次大的等价类所对应的空闲链表的合理性:
    for (current_equivalence_class_upper_bound = MIN_EQUIVALENCE_CLASS_UPPER_BOUND; current_equivalence_class_upper_bound <= PAGE_SIZE; current_equivalence_class_upper_bound <<= 1, ++idx) {
        void *head_node = GET_LINKED_LIST_POINTER_BY_INDEX(idx);
        void *current_node = head_node;
        void *previous_node = head_node;
        void *current_previous_node;

        // 不断获取下一个结点, 并在到达链表头时退出循环:
        while ((current_node = GET_NEXT_LINKED_LIST_BP_FROM_BP(current_node)) != head_node) {
            // 如果当前块的状态为 "已分配" 则报错:
            if (GET_CURRENT_BLOCK_ALLOCATE_STATUS_FROM_BP(current_node)) {
                DEBUG &&printf("空闲链表中存在已分配的块, 链表大小: %d, 块地址: %p\n", current_equivalence_class_upper_bound, current_node);
                return -1;
            }

            // 空闲块数量增加:
            free_block_number++;

            // 如果当前块中存储的前一个块地址和真实的前一个块地址不一致则报错:
            if (previous_node != (current_previous_node = GET_PREVIOUS_LINKED_LIST_BP_FROM_BP(current_node))) {
                DEBUG &&printf("空闲链表链接断裂, 链表大小: %d, 当前块地址: %p, 前一个块地址: %p, 当前块中存储的前一个块地址: "
                               "%p\n",
                               current_equivalence_class_upper_bound, current_node, previous_node, current_previous_node);
                return -1;
            }

            previous_node = current_node;
        }
    }

    // 检查最大等价类所对应的空闲链表的合理性:
    {
        void *head_node = GET_LINKED_LIST_POINTER_BY_INDEX(idx);
        void *current_node = head_node;
        void *previous_node = head_node;
        void *current_previous_node;
        // 不断获取下一个结点, 并在到达链表头时退出循环:
        while ((current_node = GET_NEXT_LINKED_LIST_BP_FROM_BP(current_node)) != head_node) {
            // 如果当前块的状态为 "已分配" 则报错:
            if (GET_CURRENT_BLOCK_ALLOCATE_STATUS_FROM_BP(current_node)) {
                DEBUG &&printf("空闲链表中存在已分配的块, 链表大小: %d, 块地址: %p\n", current_equivalence_class_upper_bound, current_node);
                return -1;
            }

            // 空闲块数量增加:
            free_block_number++;

            // 如果当前块中存储的前一个块地址和真实的前一个块地址不一致则报错:
            if (previous_node != (current_previous_node = GET_PREVIOUS_LINKED_LIST_BP_FROM_BP(current_node))) {
                DEBUG &&printf("空闲链表链接断裂, 链表大小: ∞, 当前块地址: %p, 前一个块地址: %p, 当前块中存储的前一个块地址: "
                               "%p\n",
                               current_node, previous_node, current_previous_node);
                return -1;
            }

            previous_node = current_node;
        }
    }

    return free_block_number;
}

/**
 * @brief 对指针数组进行插入排序
 *
 * @param pointer_array
 * @param left
 * @param right
 */
static void _insertion_sort_pointer_array(void **pointer_array, size_t left, size_t right) {
    size_t current_idx;
    for (current_idx = left + 1; current_idx < right; current_idx++) {
        void *current_pointer = pointer_array[current_idx];
        size_t target_idx = current_idx;
        void *target_pointer;
        while (target_idx && ((target_pointer = pointer_array[target_idx - 1]) > current_pointer)) {
            pointer_array[target_idx] = target_pointer;
            target_idx--;
        }
        pointer_array[target_idx] = current_pointer;
    }
}

/**
 * @brief 对指针数组进行快速排序
 *
 * @param pointer_array
 * @param left
 * @param right
 */
static void _quick_sort_pointer_array(void **pointer_array, size_t left, size_t right) {
    // 挑选中位数 pivot:
    void *pivot;
    {
        size_t middle = left + (right - left) / 2;
        void *temp_1, *temp_2;
        if ((temp_1 = pointer_array[left]) > (temp_2 = pointer_array[middle])) {
            pointer_array[left] = temp_2;
            pointer_array[middle] = temp_1;
        }
        if ((temp_1 = pointer_array[middle]) > (temp_2 = pointer_array[right - 1])) {
            pointer_array[middle] = temp_2;
            pointer_array[right - 1] = temp_1;
        }
        if ((temp_1 = pointer_array[left]) > (temp_2 = pointer_array[middle])) {
            pointer_array[left] = temp_2;
            pointer_array[middle] = temp_1;
        }
        pivot = pointer_array[middle];
        pointer_array[middle] = pointer_array[left];
    }

    // 划分:
    size_t current_ptr = left + 1;
    size_t range_ptr = right;
    void *current_value;
    while (current_ptr < range_ptr) {
        if ((current_value = pointer_array[current_ptr]) <= pivot) {
            pointer_array[current_ptr - 1] = current_value;
            current_ptr++;
        } else {
            pointer_array[current_ptr] = pointer_array[--range_ptr];
            pointer_array[range_ptr] = current_value;
        }
    }
    pointer_array[range_ptr - 1] = pivot;

    // 根据情况选择切换至插入排序还是继续快速排序:
    size_t sub_array_size;
    if ((sub_array_size = range_ptr - 1 - left) > 1) {
        if (sub_array_size <= 10) {
            _insertion_sort_pointer_array(pointer_array, left, range_ptr - 1);
        } else {
            _quick_sort_pointer_array(pointer_array, left, range_ptr - 1);
        }
    }
    if ((sub_array_size = right - range_ptr) > 1) {
        if (sub_array_size <= 10) {
            _insertion_sort_pointer_array(pointer_array, range_ptr, right);
        } else {
            _quick_sort_pointer_array(pointer_array, range_ptr, right);
        }
    }
}

/**
 * @brief 对指针数组进行排序
 *
 * @param pointer_array
 * @param pointer_array_size
 */
static void _sort_pointer_array(void **pointer_array, size_t pointer_array_size) {
    if (pointer_array_size > 1) {
        if (pointer_array_size <= 10) {
            _insertion_sort_pointer_array(pointer_array, 0, pointer_array_size);
        } else {
            _quick_sort_pointer_array(pointer_array, 0, pointer_array_size);
        }
    }
}

/**
 * @brief 检查堆中空闲块和空闲链表中空闲块是否一致, 若一致则返回非零值, 否则返回零
 *
 * @param heap_free_block_number
 * @param free_list_block_number
 * @return int
 */
static int _check_free_list_and_heap_consistency(size_t heap_free_block_number, size_t free_list_block_number) {
    if (heap_free_block_number != free_list_block_number) {
        DEBUG &&printf("堆中空闲块与空闲链表中空闲块的数量不一致\n");
        return 0;
    }

    DEBUG &&printf("空闲块数量: %d\n", heap_free_block_number);

    void **heap_free_block_pointers = (void **)malloc((size_t)heap_free_block_number * sizeof(void *));
    {
        size_t current_heap_free_block_number = 0;

        void *current_bp;
        size_t current_block_size;

        int current_equivalence_class_upper_bound;
        int idx = 0;

        for (current_equivalence_class_upper_bound = MIN_EQUIVALENCE_CLASS_UPPER_BOUND; current_equivalence_class_upper_bound <= PAGE_SIZE; current_equivalence_class_upper_bound <<= 1, ++idx) {
        }
        idx++;

        // 直接跳过所有链表, 获取第一个块的地址:
        current_bp = GET_LINKED_LIST_POINTER_BY_INDEX(idx);

        // 遍历堆中每一个块直至遇到结尾块 (长度为零):
        while ((current_block_size = GET_BLOCK_SIZE_FROM_BP(current_bp))) {
            // 如果是空闲块则收集起来:
            if (!GET_CURRENT_BLOCK_ALLOCATE_STATUS_FROM_BP(current_bp)) {
                heap_free_block_pointers[current_heap_free_block_number] = current_bp;
                current_heap_free_block_number++;
            }

            // 跳到后一个块:
            current_bp = GET_NEXT_BP_FROM_BP(current_bp);
        }
    }

    void **free_list_block_pointers = (void **)malloc((size_t)free_list_block_number * sizeof(void *));
    {
        size_t current_heap_free_block_number = 0;

        int current_equivalence_class_upper_bound;
        int idx = 0;
        DEBUG &&printf("链表内部情况:\n");

        // 遍历最小到次大的等价类所对应的空闲链表:
        for (current_equivalence_class_upper_bound = MIN_EQUIVALENCE_CLASS_UPPER_BOUND; current_equivalence_class_upper_bound <= PAGE_SIZE; current_equivalence_class_upper_bound <<= 1, ++idx) {
            DEBUG &&printf("%d", current_equivalence_class_upper_bound);

            void *head_node = GET_LINKED_LIST_POINTER_BY_INDEX(idx);
            void *current_node = head_node;

            // 不断获取下一个结点, 并在到达链表头时退出循环:
            while ((current_node = GET_NEXT_LINKED_LIST_BP_FROM_BP(current_node)) != head_node) {
                // 收集空闲块:
                free_list_block_pointers[current_heap_free_block_number] = current_node;
                current_heap_free_block_number++;

                DEBUG &&printf(" -> %p\t", current_node);
            }
            DEBUG &&printf("\n");
        }

        // 遍历最大等价类所对应的空闲链表:
        {
            DEBUG &&printf("∞\t");

            void *head_node = GET_LINKED_LIST_POINTER_BY_INDEX(idx);
            void *current_node = head_node;

            // 不断获取下一个结点, 并在到达链表头时退出循环:
            while ((current_node = GET_NEXT_LINKED_LIST_BP_FROM_BP(current_node)) != head_node) {
                // 收集空闲块:
                free_list_block_pointers[current_heap_free_block_number] = current_node;
                current_heap_free_block_number++;

                DEBUG &&printf(" -> %p\t", current_node);
            }
            DEBUG &&printf("\n");
        }
    }

    // 打印排序前状态:
    {
        DEBUG &&printf("堆空闲块\t空闲链表块 (排序前)\n");

        size_t idx;
        for (idx = 0; idx < heap_free_block_number; idx++) {
            DEBUG &&printf("%p\t%p\n", heap_free_block_pointers[idx], free_list_block_pointers[idx]);
        }
    }

    // 进行排序:
    _sort_pointer_array(heap_free_block_pointers, heap_free_block_number);
    _sort_pointer_array(free_list_block_pointers, free_list_block_number);

    // 检查排序算法正确性:
    {
        size_t idx;
        for (idx = 1; idx < heap_free_block_number; idx++) {
            if (heap_free_block_pointers[idx - 1] > heap_free_block_pointers[idx] || free_list_block_pointers[idx - 1] > free_list_block_pointers[idx]) {
                DEBUG &&printf("排序算法出错\n");
                exit(1);
            }
        }
    }

    // 检查堆中空闲块与空闲链表中空闲块是否一致:
    int check_result = 1;
    {
        DEBUG &&printf("堆空闲块\t空闲链表块 (排序后)\n");

        size_t idx;
        for (idx = 0; idx < heap_free_block_number; idx++) {
            DEBUG &&printf("%p\t%p\n", heap_free_block_pointers[idx], free_list_block_pointers[idx]);

            if (heap_free_block_pointers[idx] != free_list_block_pointers[idx]) {
                check_result = 0;
            }
        }
    }

    // 释放内存:
    free(heap_free_block_pointers);
    free(free_list_block_pointers);

    return check_result;
}

/**
 * @brief 合理性检查工具, 若一切正常则返回非零值, 否则返回零
 *
 * @return int
 */
static int mm_check(void) {
    // DEBUG &&printf("[mm_check]\n");

    int heap_free_block_number;
    int free_list_block_number;

    // 获取堆中空闲块数量 (如果出错返回 -1):
    if ((heap_free_block_number = _check_heap_format()) == -1) {
        return 0;
    }

    // 获取空闲链表中空闲块数量 (如果出错返回 -1):
    if ((free_list_block_number = _check_free_lists()) == -1) {
        return 0;
    }

    // 检查堆中空闲块与空闲链表中空闲块是否一致:
    if (!_check_free_list_and_heap_consistency(heap_free_block_number, free_list_block_number)) {
        return 0;
    }

    return 1;
}

/**
 * @brief 初始化堆空间
 *
 * @return int
 */
int mm_init(void) {
    DEBUG &&printf("[mm_init]\n");

    DEBUG &&printf("指针大小: %d\n", POINTER_SIZE);
    DEBUG &&printf("头部/尾部大小: %d\n", SIZE_T_SIZE);

    // 系统页大小:
    PAGE_SIZE = (size_t)mem_pagesize();

    // 最小的等价类上界 (等于 "不小于最小块大小的 2 的幂" 中的最小值):
    {
        // 2 的 0 次方幂:
        MIN_EQUIVALENCE_CLASS_UPPER_BOUND = (size_t)1;

        // 如果 2 的 n 次方幂仍然小于最小块大小:
        while (MIN_EQUIVALENCE_CLASS_UPPER_BOUND < MIN_BLOCK_SIZE) {
            // 计算 2 的 n + 1 次方幂:
            MIN_EQUIVALENCE_CLASS_UPPER_BOUND <<= 1;
        }
    }

    // 对齐链表数组的首字节 (这是由于为了统一接口, 链表元素实际上也是块, 既然是块就需要对齐)
    if (mem_sbrk(SIZE_T_SIZE % ALIGN_SIZE) == (void *)-1) {
        printf("error: out of memory\n");
        exit(1);
    }

    // 空闲链表数组的首字节地址:
    LINKED_LIST_ARRAY_POINTER = (void *)((char *)(mem_heap_lo()) + SIZE_T_SIZE % ALIGN_SIZE);

    // 初始化每个链表头结点:
    {
        int current_equivalence_class_upper_bound;
        int idx = 0;

        // 初始化最小等价类到次大等价类的链表头结点的指针:
        for (current_equivalence_class_upper_bound = MIN_EQUIVALENCE_CLASS_UPPER_BOUND; current_equivalence_class_upper_bound <= PAGE_SIZE; current_equivalence_class_upper_bound <<= 1, ++idx) {
            void *current_header_node = mem_sbrk(MIN_BLOCK_SIZE);
            if (current_header_node == (void *)-1) {
                printf("error: out of memory\n");
                exit(1);
            }
            DEBUG &&printf("第 %d 个链表头结点的地址: %p, 上界: %d\n", idx, current_header_node, current_equivalence_class_upper_bound);
            SET_NEXT_LINKED_LIST_BP_BY_BP(current_header_node, current_header_node);
            SET_PREVIOUS_LINKED_LIST_BP_BY_BP(current_header_node, current_header_node);
        }

        // 初始化最大等价类的链表头结点:
        void *current_header_node = mem_sbrk(MIN_BLOCK_SIZE);
        if (current_header_node == (void *)-1) {
            printf("error: out of memory\n");
            exit(1);
        }
        DEBUG &&printf("第 %d 个链表头结点的地址: %p, 上界: ∞\n", idx, current_header_node);
        SET_NEXT_LINKED_LIST_BP_BY_BP(current_header_node, current_header_node);
        SET_PREVIOUS_LINKED_LIST_BP_BY_BP(current_header_node, current_header_node);
    }

    // 初始化结尾块:
    {
        void *epilogue_block_pointer = mem_sbrk(EPILOGUE_BLOCK_SIZE);
        if (epilogue_block_pointer == (void *)-1) {
            printf("error: out of memory\n");
            exit(1);
        }
        SET_CURRENT_BLOCK_ALLOCATE_STATUS_BY_BP(epilogue_block_pointer);  // 从不释放
        SET_PREVIOUS_BLOCK_ALLOCATE_STATUS_BY_BP(epilogue_block_pointer); // 代替序言块的作用
        SET_BLOCK_SIZE_BY_BP(epilogue_block_pointer, (size_t)0);          // 结尾块长度为 0
    }

    if (DEBUG && !mm_check()) {
        exit(1);
    }

    DEBUG &&printf("\n");
    return 0;
}

/**
 * @brief 为所请求的载荷分配合适的块, 必要时向系统申请额外的空间
 *
 * @param size
 * @return void*
 */
void *mm_malloc(size_t size) {
    DEBUG &&printf("[mm_malloc]\n");
    DEBUG &&printf("请求载荷大小: %d\n", size);

    // 将载荷大小转换为块大小:
    size_t block_size = GET_BLOCK_SIZE_FROM_PAYLOAD_SIZE(size);
    DEBUG &&printf("所需块大小: %d\n", block_size);

    // 从链表中查找合适的空闲块:
    void *target_block = _find_in_free_lists(block_size);

    DEBUG
    &&(target_block ? printf("已找到空闲块, 块地址: %p\n", target_block) : printf("未找到空闲块\n"));

    // 如果没找到则直接分配:
    if (!target_block) {
        // 对于小于页面大小的块, 一次性请求一定倍数大小的空间, 对于不低于页面大小的块则按原样大小请求:
        size_t allocated_block_size = block_size < PAGE_SIZE ? (5 * block_size) : ALIGN(block_size);
        target_block = _extend(allocated_block_size);
    }

    // 分割并返回载荷指针:
    void *p = _place(block_size, target_block);

    if (DEBUG && !mm_check()) {
        exit(1);
    }

    DEBUG &&printf("\n");
    return p;
}

/**
 * @brief 释放已分配的块, 必要时合并空闲块
 *
 * @param p
 */
void mm_free(void *p) {
    DEBUG &&printf("[mm_free]\n");

    // 通过载荷地址得到块地址:
    void *bp = GET_BP_FROM_P(p);

    DEBUG &&printf("即将释放块, 块地址: %p, 块大小: %d\n", bp, GET_BLOCK_SIZE_FROM_BP(bp));

    // 将已分配块格式化为空闲块:
    _format_free_block(bp);

    // 尝试合并空闲块:
    bp = _coalesce(bp);

    // 将合并后的空闲块插入空闲链表:
    _insert_to_free_lists(bp);

    if (DEBUG && !mm_check()) {
        exit(1);
    }

    DEBUG &&printf("\n");

    return;
}

/**
 * @brief 视情况对已分配的块进行原地收缩或原地拓展, 必要时进行重新分配并迁移数据
 *
 * @param ptr
 * @param size
 * @return void*
 */
void *mm_realloc(void *ptr, size_t size) {
    DEBUG &&printf("[mm_realloc]\n");

    void *bp = GET_BP_FROM_P(ptr);
    size_t block_size = GET_BLOCK_SIZE_FROM_BP(bp);
    size_t new_block_size = GET_BLOCK_SIZE_FROM_PAYLOAD_SIZE(size);

    // 如果大小不变则直接返回原分配块:
    if (new_block_size == block_size) {
        DEBUG &&printf("大小不变\n");

        if (DEBUG && !mm_check()) {
            exit(1);
        }

        DEBUG &&printf("\n");

        return ptr;
    }

    void *new_ptr = NULL;

    void *next_bp = GET_NEXT_BP_FROM_BP(bp);
    size_t next_block_allocate_status = GET_CURRENT_BLOCK_ALLOCATE_STATUS_FROM_BP(next_bp);
    size_t next_block_size = GET_BLOCK_SIZE_FROM_BP(next_bp);

    // 需要收缩:
    if (new_block_size < block_size) {
        DEBUG &&printf("收缩\n");

        // 下一块为已分配块:
        if (next_block_allocate_status) {
            // 如果剩下的内存空间长度不足以形成新的空闲块:
            if (new_block_size + MIN_BLOCK_SIZE > block_size) {
                // 以内部碎片的形式保留在原分配块中:
                new_ptr = ptr;
            }

            // 否则分割:
            else {
                // 收缩当前块:
                SET_BLOCK_SIZE_BY_BP(bp, new_block_size);

                // 分割形成新空闲块并插入空闲链表:
                void *new_next_bp = (void *)((char *)bp + new_block_size);
                size_t new_next_block_size = block_size - new_block_size;
                SET_BLOCK_SIZE_BY_BP(new_next_bp, new_next_block_size);
                SET_PREVIOUS_BLOCK_ALLOCATE_STATUS_BY_BP(new_next_bp);
                _format_free_block(new_next_bp);
                _insert_to_free_lists(new_next_bp);

                new_ptr = ptr;
            }
        }

        // 下一块为空闲块:
        else {
            // 收缩当前块:
            SET_BLOCK_SIZE_BY_BP(bp, new_block_size);

            // 将下一块从空闲链表中摘出:
            _pick_from_free_list(next_bp);

            // 拓展空闲块形成新空闲块:
            void *new_next_bp = (void *)((char *)bp + new_block_size);
            size_t new_next_block_size = next_block_size + (block_size - new_block_size);
            SET_BLOCK_SIZE_BY_BP(new_next_bp, new_next_block_size);
            SET_PREVIOUS_BLOCK_ALLOCATE_STATUS_BY_BP(new_next_bp);
            _format_free_block(new_next_bp);

            // 重新挂回空闲链表中:
            _insert_to_free_lists(new_next_bp);

            new_ptr = ptr;
        }
    }

    // 需要拓展:
    else {
        DEBUG &&printf("拓展\n");

        size_t available_size;

        // 下一块为已分配块, 或者下一块为空闲块但不足以容纳拓展的空间:
        if (next_block_allocate_status || (available_size = block_size + next_block_size) < new_block_size) {
            // 下一块虽然为已分配块但是为结尾块:
            if (next_block_allocate_status && !next_block_size) {
                DEBUG &&printf("下一块虽然为已分配块但是为结尾块\n");

                // 直接拓展堆:
                _extend(new_block_size - block_size);

                // 延长当前块:
                SET_BLOCK_SIZE_BY_BP(bp, new_block_size);

                // 设置结尾块的前一个块的分配状态:
                SET_PREVIOUS_BLOCK_ALLOCATE_STATUS_BY_BP(GET_NEXT_BP_FROM_BP(bp));

                new_ptr = ptr;
            }

            // 下一块空闲块虽然不足以容纳拓展的空间, 但下下一块为结尾块:
            else if (!next_block_allocate_status && !GET_BLOCK_SIZE_FROM_BP(GET_NEXT_BP_FROM_BP(next_bp))) {
                DEBUG &&printf("下一块空闲块虽然不足以容纳拓展的空间, 但下下一块为结尾块\n");

                // 将下一块从空闲链表中摘出:
                _pick_from_free_list(next_bp);

                // 直接拓展堆:
                _extend(new_block_size - available_size);

                // 延长当前块:
                SET_BLOCK_SIZE_BY_BP(bp, new_block_size);

                // 设置结尾块的前一个块的分配状态:
                SET_PREVIOUS_BLOCK_ALLOCATE_STATUS_BY_BP(GET_NEXT_BP_FROM_BP(bp));

                new_ptr = ptr;
            }

            // 其余情况:
            else {
                DEBUG &&printf("分配新的块\n");

                // 分配新的块:
                new_ptr = mm_malloc(size);

                // 复制内容:
                memcpy(new_ptr, ptr, size);

                // 释放原有块:
                mm_free(ptr);
            }
        }

        // 下一块是空闲块并且能够容纳拓展的空间:
        else {
            DEBUG &&printf("拓展原有块\n");

            // 将下一块从空闲链表中摘出:
            _pick_from_free_list(next_bp);

            // 如果拓展之后原空闲块剩下的空间不足以形成新的空闲块则以内部碎片的形式保留在新分配块中:
            if (available_size - MIN_BLOCK_SIZE < new_block_size) {
                // 拓展当前块为整个可用空间:
                new_block_size = available_size;
                SET_BLOCK_SIZE_BY_BP(bp, new_block_size);

                // 将后一个块的 "前一个块的分配状态位" 设置为 "已分配":
                SET_PREVIOUS_BLOCK_ALLOCATE_STATUS_BY_BP(GET_NEXT_BP_FROM_BP(bp));

                new_ptr = ptr;
            }

            // 否则分割:
            else {
                // 拓展当前块:
                SET_BLOCK_SIZE_BY_BP(bp, new_block_size);

                // 收缩旧空闲块形成新空闲块:
                void *new_next_bp = (void *)((char *)bp + new_block_size);
                size_t new_next_block_size = next_block_size + (block_size - new_block_size);
                SET_BLOCK_SIZE_BY_BP(new_next_bp, new_next_block_size);
                SET_PREVIOUS_BLOCK_ALLOCATE_STATUS_BY_BP(new_next_bp);
                _format_free_block(new_next_bp);
                _insert_to_free_lists(new_next_bp);

                new_ptr = ptr;
            }

            DEBUG &&printf("块大小: %d\n", new_block_size);
        }
    }

    if (DEBUG && !mm_check()) {
        exit(1);
    }

    DEBUG &&printf("\n");

    return new_ptr;
}