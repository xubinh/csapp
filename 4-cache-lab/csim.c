#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// 用于存储 valgrind 输出条目:
typedef struct Trace {
    char type;                  // 访问类型, "L", "S" 或 "M"
    unsigned long long address; // 十六进制无符号地址
    int size;                   // 访问数据大小
    struct Trace *next;         // 用于形成链表
} Trace;

// 爬取 valgrind 输出 trace 文件:
Trace *parse_tracefile(const char *const tracefile) {
    // 打开 trace 文件:
    FILE *file = fopen(tracefile, "r");

    // 常规错误处理:
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    Trace *head = (Trace *)malloc(sizeof(Trace));
    head->next = NULL;
    Trace *tail = head;

    // 每次读一行条目:
    while (fgets(line, sizeof(line), file)) {
        // 新建条目:
        Trace *new_trace = (Trace *)malloc(sizeof(Trace));
        new_trace->next = NULL;

        // 常规错误处理:
        if (new_trace == NULL) {
            while (head != NULL) {
                Trace *temp = head;
                head = head->next;
                free(temp);
            }
            fclose(file);
            perror("Failed to allocate memory");
            exit(EXIT_FAILURE);
        }

        // 跳过指令访问条目:
        if (line[0] == 'I') {
            continue;
        }

        // 爬取访问信息:
        if (sscanf(line, " %c %llx,%d", &new_trace->type, &new_trace->address, &new_trace->size) == 3) {
            // 向链表末尾追加新条目:
            tail->next = new_trace;
            tail = new_trace;
        }

        // 爬取失败则报错:
        else {
            free(new_trace);
            while (head != NULL) {
                Trace *temp = head;
                head = head->next;
                free(temp);
            }
            fclose(file);
            printf("Failed to parse line: %s\n", line);
            exit(EXIT_FAILURE);
        }
    }

    fclose(file);

    if (!head) {
        printf("Empty trace file\n");
        exit(EXIT_FAILURE);
    }

    return head;
}

// 打印帮助信息:
void print_help(char *argv_0) {
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv_0);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\n");
    printf("Examples:\n");
    printf("  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");

    return;
}

// 使用 `getopt` 函数爬取传入参数:
void parse_arguments(const int argc, char *const *const argv, int *const flag_h, int *const flag_v,
                     char **const value_s, char **const value_E, char **const value_b, char **const tracefile) {
    int option_type;
    while ((option_type = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (option_type) {
        case 'h':
            *flag_h = 1;
            break;
        case 'v':
            *flag_v = 1;
            break;
        case 's':
            *value_s = optarg;
            break;
        case 'E':
            *value_E = optarg;
            break;
        case 'b':
            *value_b = optarg;
            break;
        case 't':
            *tracefile = optarg;
            break;
        default:
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (option_type == -1) {
        printf("%s: Missing required command line argument\n", argv[0]);
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }
}

// cache 行:
typedef struct Line {
    unsigned long long tag;
    struct Line *next;
    struct Line *pre;
} Line;

// 使用 Line 的链表的表头表示 cache 组, 链表中的每一个结点表示组中的一行, cache 本身则为所有链表表头的一个数组:
typedef Line *Cache;

// 初始化 cache:
Cache get_cache(const int s, const int E, const int b) {
    int total_set_number = (1 << s);

    Cache cache = (Line *)malloc(total_set_number * sizeof(Line));

    for (int i = 0; i < total_set_number; i++) {
        cache[i].next = NULL;
    }

    return cache;
}

// 释放 cache 组:
void free_set(Line *head) {
    while (head) {
        Line *temp = head->next;
        free(head);
        head = temp;
    }
    return;
}

// 释放整个 cache:
void free_cache(Cache cache, const int s, const int E, const int b) {
    int total_set_number = (1 << s);

    // 释放每个组:
    for (int i = 0; i < total_set_number; i++) {
        free_set(cache[i].next);
    }

    // 释放 cache 本身:
    free(cache);

    return;
}

// 从地址中获取标记 tag 和组序号 set number:
void get_tag_and_set_number(const unsigned long long address, const int s, const int E, const int b,
                            unsigned long long *const tag, int *const set_number) {
    *tag = (address >> (s + b));
    *set_number = (address >> b) & ((1 << s) - 1);
}

// 模拟加载数据:
void simulate_load(Cache cache, const int s, const int E, const int b, const unsigned long long address, int *const hit,
                   int *const miss, int *const eviction) {
    unsigned long long tag;
    int set_number;
    get_tag_and_set_number(address, s, E, b, &tag, &set_number);

    Line *set = cache + set_number;
    Line *line = set->next;
    Line *line_pre = set;
    int line_count = 0;

    // 在组中寻找相同标记的行:
    while (line) {
        // 如果找到:
        if (line->tag == tag) {
            // 将该结点从链表中取出:
            line->pre->next = line->next;
            if (line->next) {
                line->next->pre = line->pre;
            }

            // 将该结点插入到表头之后:
            line->next = set->next;
            if (set->next) {
                set->next->pre = line;
            }
            line->pre = set;
            set->next = line;

            *hit = 1;
            *miss = 0;
            *eviction = 0;

            return;
        }

        // 否则继续寻找:
        line_pre = line;
        line = line->next;
        line_count++;
    }

    // 如果遍历完整个链表都没有找到, 则从内存中加载该数据块到 cache 中 (按 LRU 策略, 直接插入到表头后):
    Line *new_line = (Line *)malloc(sizeof(Line));
    new_line->tag = tag;

    new_line->next = set->next;
    if (set->next) {
        set->next->pre = new_line;
    }
    new_line->pre = set;
    set->next = new_line;

    // 如果组已经满了还需要驱逐一个数据块出去 (直接弹掉表尾结点即可):
    if (line_count == E) {
        line_pre->pre->next = NULL;
        free(line_pre);
    }

    *hit = 0;
    *miss = 1;
    *eviction = (line_count == E);

    return;
}

// 模拟写入数据:
void simulate_store(Cache cache, const int s, const int E, const int b, const unsigned long long address,
                    int *const hit, int *const miss, int *const eviction) {
    // 由于使用的是写分配策略, 所以写入对 cache 的效果等价于加载对 cache 的效果:
    simulate_load(cache, s, E, b, address, hit, miss, eviction);
}

// 模拟修改数据 (handout 明确指出修改一次数据等价于读取 + 写入):
void simulate_modify(Cache cache, const int s, const int E, const int b, const unsigned long long address,
                     int *const hit, int *const miss, int *const eviction) {
    *hit = 0;
    *miss = 0;
    *eviction = 0;

    int hit_t = 0;
    int miss_t = 0;
    int eviction_t = 0;

    // 读取:
    simulate_load(cache, s, E, b, address, &hit_t, &miss_t, &eviction_t);

    *hit += hit_t;
    *miss += miss_t;
    *eviction += eviction_t;

    hit_t = 0;
    miss_t = 0;
    eviction_t = 0;

    // 写入:
    simulate_store(cache, s, E, b, address, &hit_t, &miss_t, &eviction_t);

    *hit += hit_t;
    *miss += miss_t;
    *eviction += eviction_t;
}

// verbose 模式下打印提示信息:
void print_trace(Trace *trace, const int hit, const int miss, const int eviction) {
    char *prompt = hit ? "hit" : "miss";
    printf("%c %llx,%d %s", trace->type, trace->address, trace->size, prompt);

    if (eviction) {
        printf(" eviction");
    }

    printf("\n");

    return;
}

// 模拟整个 cache 过程:
void simulate(const int s, const int E, const int b, Trace *traces, int *const hits, int *const misses,
              int *const evictions, const int flag_verbose) {
    *hits = 0;
    *misses = 0;
    *evictions = 0;

    int hit = 0;
    int miss = 0;
    int eviction = 0;

    Cache cache = get_cache(s, E, b);

    while (traces) {
        Trace *trace = traces;

        hit = 0;
        miss = 0;
        eviction = 0;

        if (trace->type == 'L') {
            simulate_load(cache, s, E, b, trace->address, &hit, &miss, &eviction);
        } else if (trace->type == 'S') {
            simulate_store(cache, s, E, b, trace->address, &hit, &miss, &eviction);
        } else if (trace->type == 'M') {
            simulate_modify(cache, s, E, b, trace->address, &hit, &miss, &eviction);
        } else {
            printf("Unknown trace type\n");
            exit(EXIT_FAILURE);
        }

        *hits += hit;
        *misses += miss;
        *evictions += eviction;

        if (flag_verbose) {
            print_trace(trace, hit, miss, eviction);
        }

        traces = traces->next;
        free(trace);
    }

    free_cache(cache, s, E, b);

    return;
}

int main(int argc, char *argv[]) {
    int flag_h = 0;
    int flag_v = 0;
    char *value_s = NULL;
    char *value_E = NULL;
    char *value_b = NULL;
    char *tracefile = NULL;

    parse_arguments(argc, argv, &flag_h, &flag_v, &value_s, &value_E, &value_b, &tracefile);

    if (flag_h) {
        print_help(argv[0]);
        exit(EXIT_SUCCESS);
    }

    if (!value_s || !value_E || !value_b || !tracefile) {
        printf("%s: Missing required command line argument\n", argv[0]);
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    int s = atoi(value_s);
    int E = atoi(value_E);
    int b = atoi(value_b);

    Trace *traces = parse_tracefile(tracefile);

    if (s <= 0 || s >= 16 || E <= 0 || E >= 16 || b <= 0 || b >= 16) {
        printf("%s: Arguments too big\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int hits = 0;
    int misses = 0;
    int evictions = 0;

    simulate(s, E, b, traces, &hits, &misses, &evictions, flag_v);

    printSummary(hits, misses, evictions);

    return 0;
}
