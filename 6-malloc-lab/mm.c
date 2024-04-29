/*
 * mm.c - <待填充>
 *
 * <待填充>
 * 假设页大小为 2 的幂.
 * 假设堆空间的起始地址为 8 的倍数.
 * 假设 `size_t` 类型变量的大小为 4 的倍数.
 * 假设 `void *` 类型变量的大小为 4 的倍数.
 * 一个头部的大小等于 `size_t` 类型变量的大小, 用于存储块大小以及在确保块大小必然为 4 的倍数的前提下在 (必然为零的) 低 2
 * 位存储当前块的分配状态以及前一个块的分配状态. 一个尾部的大小等于头部的大小, 并且当且仅当一个块空闲时设置尾部,
 * 用于边界标记. 一个合法的空闲块由一个右对齐的头部, 中间的由若干个 8 字节区间构成的延伸区域 (因为并不是分配状态,
 * 不存在有效载荷的概念, 所以命名为延伸区域), 以及一个左对齐的尾部共三部分构成.
 * 延伸区域中包含用于形成双向空闲链表的两个指针, 因此延伸区域的大小至少为 8 字节.
 * 一个合法的已分配块由一个合法的空闲块在保持块大小不变的前提下转化而来, 其中头部大小和位置不变,
 * 延伸区域和尾部合并构成有效载荷区域. 综上所述, 一个合法的块的大小必然为 8 的倍数,
 * 并且已分配块的有效载荷的起始地址也必然为 8 的倍数.
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

#define DEBUG ((unsigned int)1)
// #define DEBUG ((unsigned int)0)

// `void *` 类型变量的大小 (假设为 4 的倍数):
static size_t POINTER_SIZE = sizeof(char *);

// `size_t` 类型变量的大小 (假设为 4 的倍数):
static size_t SIZE_T_SIZE = sizeof(size_t);

// 系统页大小 (假设为 2 的幂):
static size_t PAGE_SIZE;

// 对齐大小, 固定为 8, 与官方 malloc 保持一致:
static size_t ALIGN_SIZE = (size_t)8;

// 最小块 (由两个头部以及两个链表指针构成) 的大小:
static size_t MIN_BLOCK_SIZE = 2 * (sizeof(size_t) + sizeof(char *));

// 当前块的分配状态位掩码 (最低有效位):
static size_t CURRENT_BLOCK_ALLOCATE_STATUS_MASK = (size_t)1;

// 前一个块的分配状态位掩码 (次低有效位):
static size_t PREVIOUS_BLOCK_ALLOCATE_STATUS_MASK = (((size_t)1) << 1);

// 块大小掩码 (已知块大小为 8 的倍数, 因此最低 3 位必然为 0):
static size_t BLOCK_SIZE_MASK = ~(size_t)7;

// 最小的等价类上界 (等于 "不小于最小块大小的 2 的幂" 中的最小值):
static size_t MIN_EQUIVALENCE_CLASS_UPPER_BOUND;

// 空闲链表数组 (其元素存储于堆空间的起始位置):
static void *LINKED_LIST_ARRAY_POINTER;

// 结尾块大小 (结尾块仅由一个头部构成):
static size_t EPILOGUE_BLOCK_SIZE = sizeof(size_t);

#define ALIGN(size) (((size_t)(size) - (size_t)(1) + (size_t)(ALIGN_SIZE)) & (size_t)(BLOCK_SIZE_MASK))
#define GET_CURRENT_BLOCK_ALLOCATE_STATUS_FROM_BP(bp) ((*(size_t *)(bp)) & (size_t)(CURRENT_BLOCK_ALLOCATE_STATUS_MASK))
#define GET_PREVIOUS_BLOCK_ALLOCATE_STATUS_FROM_BP(bp)                                                                 \
    ((*(size_t *)(bp)) & (size_t)(PREVIOUS_BLOCK_ALLOCATE_STATUS_MASK))
#define GET_BLOCK_SIZE_FROM_BP(bp) ((size_t)((*(size_t *)(bp)) & (size_t)(BLOCK_SIZE_MASK)))
#define GET_BP_FROM_P(p) ((void *)((char *)(p) - (size_t)(SIZE_T_SIZE)))
#define GET_P_FROM_BP(bp) ((void *)((char *)(bp) + (size_t)(SIZE_T_SIZE)))
#define SET_BLOCK_SIZE_BY_BP(bp, block_size)                                                                           \
    ((*(size_t *)(bp)) =                                                                                               \
         ((size_t)(block_size) & (size_t)BLOCK_SIZE_MASK) | ((*(size_t *)(bp)) & (~(size_t)BLOCK_SIZE_MASK)))
#define SET_CURRENT_BLOCK_ALLOCATE_STATUS_BY_BP(bp) ((*(size_t *)(bp)) |= (size_t)(CURRENT_BLOCK_ALLOCATE_STATUS_MASK))
#define UNSET_CURRENT_BLOCK_ALLOCATE_STATUS_BY_BP(bp)                                                                  \
    ((*(size_t *)(bp)) &= ~(size_t)(CURRENT_BLOCK_ALLOCATE_STATUS_MASK))
#define SET_PREVIOUS_BLOCK_ALLOCATE_STATUS_BY_BP(bp)                                                                   \
    ((*(size_t *)(bp)) |= (size_t)(PREVIOUS_BLOCK_ALLOCATE_STATUS_MASK))
#define UNSET_PREVIOUS_BLOCK_ALLOCATE_STATUS_BY_BP(bp)                                                                 \
    ((*(size_t *)(bp)) &= ~(size_t)(PREVIOUS_BLOCK_ALLOCATE_STATUS_MASK))
#define GET_PREVIOUS_BP_FROM_BP(bp)                                                                                    \
    ((void *)((char *)(bp)                                                                                             \
              - ((size_t)((*(size_t *)((char *)(bp) - (size_t)(SIZE_T_SIZE))) & (size_t)(BLOCK_SIZE_MASK)))))
#define GET_NEXT_BP_FROM_BP(bp) ((void *)((char *)(bp) + GET_BLOCK_SIZE_FROM_BP(bp)))
#define GET_LINKED_LIST_POINTER_BY_INDEX(idx)                                                                          \
    ((void *)((char *)(LINKED_LIST_ARRAY_POINTER) + (size_t)(idx) * (size_t)MIN_BLOCK_SIZE))
#define SET_FOOTER_BY_BP(bp)                                                                                           \
    (*(size_t *)((char *)(bp) + (size_t)GET_BLOCK_SIZE_FROM_BP(bp) - (size_t)(SIZE_T_SIZE)) =                          \
         *(size_t *)(bp)) // 在设置好头部的前提下将头部复制到尾部
#define GET_PREVIOUS_LINKED_LIST_BP_FROM_BP(bp) ((void *)(*(char **)((char *)(bp) + (size_t)(SIZE_T_SIZE))))
#define SET_PREVIOUS_LINKED_LIST_BP_BY_BP(bp, previous_bp)                                                             \
    ((*(char **)((char *)(bp) + (size_t)(SIZE_T_SIZE))) = (char *)(previous_bp))
#define GET_NEXT_LINKED_LIST_BP_FROM_BP(bp)                                                                            \
    ((void *)(*(char **)((char *)(bp) + (size_t)(SIZE_T_SIZE) + (size_t)(POINTER_SIZE))))
#define SET_NEXT_LINKED_LIST_BP_BY_BP(bp, next_bp)                                                                     \
    ((*(char **)((char *)(bp) + (size_t)(SIZE_T_SIZE) + (size_t)(POINTER_SIZE))) = (char *)(next_bp))
#define GET_BLOCK_SIZE_FROM_PAYLOAD_SIZE(payload_size) (ALIGN((size_t)(SIZE_T_SIZE) + (size_t)(payload_size)))

static void _format_free_block(void *bp) {
    // 设置当前块的状态为 "空闲":
    UNSET_CURRENT_BLOCK_ALLOCATE_STATUS_BY_BP(bp);

    // 将头部复制至尾部:
    SET_FOOTER_BY_BP(bp);

    // 将下一个块的 "前一个块的分配状态位" 设置为 "空闲":
    UNSET_PREVIOUS_BLOCK_ALLOCATE_STATUS_BY_BP(GET_NEXT_BP_FROM_BP(bp));
}

static void *_get_previous_free_block(void *bp) {
    // 前一个块已分配的话返回 NULL, 否则返回前一个块的指针:
    void *previous_free_block = GET_PREVIOUS_BLOCK_ALLOCATE_STATUS_FROM_BP(bp) ? NULL : GET_PREVIOUS_BP_FROM_BP(bp);

    return previous_free_block;
}

static void *_get_next_free_block(void *bp) {
    void *next_block = GET_NEXT_BP_FROM_BP(bp);

    // 后一个块已分配的话返回 NULL, 否则返回后一个块的指针:
    void *next_free_block = GET_CURRENT_BLOCK_ALLOCATE_STATUS_FROM_BP(next_block) ? NULL : next_block;

    return next_free_block;
}

static void _pick_from_free_list(void *bp) {
    DEBUG &&printf("[_pick_from_free_list]\n");
    DEBUG &&printf("bp: %x\n", (unsigned int)bp);
    // 前块:
    void *previous_linked_list_bp = GET_PREVIOUS_LINKED_LIST_BP_FROM_BP(bp);
    DEBUG &&printf("pre_bp: %x\n", (unsigned int)previous_linked_list_bp);

    DEBUG &&printf("111\n");

    // 后块:
    void *next_linked_list_bp = GET_NEXT_LINKED_LIST_BP_FROM_BP(bp);
    DEBUG &&printf("next_bp: %x\n", (unsigned int)next_linked_list_bp);

    DEBUG &&printf("112\n");

    // 将前块连接至后块:
    SET_NEXT_LINKED_LIST_BP_BY_BP(previous_linked_list_bp, next_linked_list_bp);

    DEBUG &&printf("113\n");

    // 将后块连接至前块:
    SET_PREVIOUS_LINKED_LIST_BP_BY_BP(next_linked_list_bp, previous_linked_list_bp);

    DEBUG &&printf("114\n");
}

static void _coalesce_current_and_next_free_block(void *bp, void *next_bp) {
    size_t total_size = GET_BLOCK_SIZE_FROM_BP(bp);
    total_size += GET_BLOCK_SIZE_FROM_BP(next_bp);
    SET_BLOCK_SIZE_BY_BP(bp, total_size);
    SET_FOOTER_BY_BP(bp);
}

static void _insert_to_free_lists(void *bp) {
    size_t block_size = GET_BLOCK_SIZE_FROM_BP(bp);
    int current_equivalence_class_upper_bound;
    int idx = 0;

    // 根据等价类上界寻找正确的链表:
    for (current_equivalence_class_upper_bound = MIN_EQUIVALENCE_CLASS_UPPER_BOUND;
         current_equivalence_class_upper_bound <= PAGE_SIZE; current_equivalence_class_upper_bound <<= 1, ++idx) {
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

static void *_coalesce(void *bp) {
    void *free_block;

    // 合并当前块和后一个块:
    if ((free_block = _get_next_free_block(bp))) {

        DEBUG &&printf("11\n");
        // 将后一个块从链表中摘出:
        _pick_from_free_list(free_block);

        DEBUG &&printf("12\n");

        // 合并两个块:
        _coalesce_current_and_next_free_block(bp, free_block);

        DEBUG &&printf("13\n");
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

static void *_find_in_free_lists(size_t block_size) {
    // 合理性检查, 确保块大小向上对齐为 8 字节形成合法块大小:
    block_size = ALIGN(block_size);

    int current_equivalence_class_upper_bound;
    int idx = 0;

    // 查找最小等价类到次大等价类:
    for (current_equivalence_class_upper_bound = MIN_EQUIVALENCE_CLASS_UPPER_BOUND;
         current_equivalence_class_upper_bound <= PAGE_SIZE; current_equivalence_class_upper_bound <<= 1, ++idx) {
        if (current_equivalence_class_upper_bound < block_size) {
            continue;
        }

        void *head_node = GET_LINKED_LIST_POINTER_BY_INDEX(idx);
        void *current_node = head_node;
        void *next_node;
        while ((next_node = GET_NEXT_LINKED_LIST_BP_FROM_BP(current_node)) != head_node
               && GET_BLOCK_SIZE_FROM_BP(next_node) < block_size) {
            current_node = next_node;
        }
        if (next_node != head_node) {
            _pick_from_free_list(next_node);
            return next_node;
        }
    }

    // 查找最大等价类:
    {
        void *head_node = GET_LINKED_LIST_POINTER_BY_INDEX(idx);
        void *current_node = head_node;
        void *next_node;
        while ((next_node = GET_NEXT_LINKED_LIST_BP_FROM_BP(current_node)) != head_node
               && GET_BLOCK_SIZE_FROM_BP(next_node) < block_size) {
            current_node = next_node;
        }
        if (next_node != head_node) {
            _pick_from_free_list(next_node);
            return next_node;
        }
    }

    return NULL;
}

static void *_extend(size_t new_free_block_size) {
    // 合理性检查, 确保块大小向上对齐为 8 字节形成合法块大小:
    new_free_block_size = ALIGN(new_free_block_size);

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

    // 设置新的空闲块的大小:
    SET_BLOCK_SIZE_BY_BP(new_free_block, new_free_block_size);

    // 格式化新空闲块:
    _format_free_block(new_free_block);

    // 尝试合并新空闲块 (合并前需确保新结尾块已格式化):
    new_free_block = _coalesce(new_free_block);

    // 返回合并后的新空闲块:
    return new_free_block;
}

void *_place(size_t size, size_t block_size, void *bp) {
    // 传入的空闲块的真实大小:
    size_t total_block_size = GET_BLOCK_SIZE_FROM_BP(bp);

    // 如果有多余的空间则进行分割:
    if (block_size + MIN_BLOCK_SIZE <= total_block_size) {
        size_t free_block_size = total_block_size - block_size;
        void *free_block = (void *)((char *)(bp) + block_size);

        // 设置空闲块大小:
        SET_BLOCK_SIZE_BY_BP(free_block, free_block_size);

        // 格式化空闲块:
        _format_free_block(free_block);

        // 将合并后的空闲块插入空闲链表:
        _insert_to_free_lists(free_block);
    }

    // 设置当前块的大小:
    SET_BLOCK_SIZE_BY_BP(bp, block_size);

    // 设置当前块的状态为 "已分配":
    SET_CURRENT_BLOCK_ALLOCATE_STATUS_BY_BP(bp);

    // 将下一个块的 "前一个块的分配状态位" 设置为 "已分配":
    SET_PREVIOUS_BLOCK_ALLOCATE_STATUS_BY_BP(GET_NEXT_BP_FROM_BP(bp));

    // 返回载荷的首地址:
    return (void *)((char *)(bp) + SIZE_T_SIZE);
}

/*
 * mm_init - <待填充>
 */
int mm_init(void) {
    DEBUG &&printf("[mm_init]\n");

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
        int current_equivalence_class_upper_bound = MIN_EQUIVALENCE_CLASS_UPPER_BOUND;
        int idx = 0;

        // 初始化最小等价类到次大等价类的链表头结点的指针:
        for (; current_equivalence_class_upper_bound <= PAGE_SIZE; current_equivalence_class_upper_bound <<= 1, ++idx) {
            void *current_header_node = mem_sbrk(MIN_BLOCK_SIZE);
            DEBUG &&printf("第 %d 个链表头结点的地址: %x\n", idx, (unsigned int)current_header_node);
            if (current_header_node == (void *)-1) {
                printf("error: out of memory\n");
                exit(1);
            }
            SET_NEXT_LINKED_LIST_BP_BY_BP(current_header_node, current_header_node);
            SET_PREVIOUS_LINKED_LIST_BP_BY_BP(current_header_node, current_header_node);
        }

        // 初始化最大等价类的链表头结点:
        void *current_header_node = mem_sbrk(MIN_BLOCK_SIZE);
        if (current_header_node == (void *)-1) {
            printf("error: out of memory\n");
            exit(1);
        }
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
    }

    return 0;
}

/*
 * mm_malloc - <待填充>
 */
void *mm_malloc(size_t size) {
    DEBUG &&printf("[mm_malloc]\n");
    DEBUG &&printf("size: %d\n", size);

    // 将载荷大小转换为块大小:
    size_t block_size = GET_BLOCK_SIZE_FROM_PAYLOAD_SIZE(size);
    DEBUG &&printf("block size: %d\n", block_size);

    DEBUG &&printf("1\n");

    // 从链表中查找合适的空闲块:
    void *target_block = _find_in_free_lists(block_size);

    DEBUG &&printf("target_block: %s\n", target_block ? "got target block" : "NULL");

    DEBUG &&printf("2\n");

    // 如果没找到则直接分配:
    if (!target_block) {
        // 对于不超过页面大小的块, 一次性请求至少两倍大小的空间, 对于大于页面大小的块则按原样大小请求:
        // size_t allocated_block_size = block_size <= PAGE_SIZE ? 2 * PAGE_SIZE : ALIGN(block_size);
        size_t allocated_block_size = block_size <= PAGE_SIZE ? (2 * block_size) : ALIGN(block_size);
        target_block = _extend(allocated_block_size);

        DEBUG &&printf("3\n");
    }

    // 分割并返回载荷指针:
    void *p = _place(size, block_size, target_block);
    DEBUG &&printf("4\n");

    return p;
}

/*
 * mm_free - <待填充>
 */
void mm_free(void *p) {
    DEBUG &&printf("[mm_free]\n");

    // 通过载荷地址得到块地址:
    void *bp = GET_BP_FROM_P(p);

    DEBUG &&printf("bp: %x\n", (unsigned int)bp);

    // 如果是空闲块的话直接返回:
    if (!GET_CURRENT_BLOCK_ALLOCATE_STATUS_FROM_BP(bp)) {
        return;
    }

    // 将已分配块格式化为空闲块:
    _format_free_block(bp);

    // 尝试合并空闲块:
    _coalesce(bp);

    // 将合并后的空闲块插入空闲链表:
    _insert_to_free_lists(bp);
}

/*
 * mm_realloc - <待填充>
 */
void *mm_realloc(void *ptr, size_t size) {
    return NULL;
}
