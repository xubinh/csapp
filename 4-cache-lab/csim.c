#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct Trace {
    char type;
    unsigned long long address;
    int size;
    struct Trace *next;
} Trace;

Trace *parse_traces(const char *const tracefile) {
    FILE *file = fopen(tracefile, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    Trace *head = NULL, *tail = NULL;

    while (fgets(line, sizeof(line), file)) {
        Trace *new_trace = (Trace *)malloc(sizeof(Trace));

        if (new_trace == NULL) {
            perror("Failed to allocate memory");
            while (head != NULL) {
                Trace *temp = head;
                head = head->next;
                free(temp);
            }
            fclose(file);
            exit(EXIT_FAILURE);
        }

        if (line[0] == 'I') {
            continue;
        }

        if (sscanf(line, " %c %llx,%d", &new_trace->type, &new_trace->address, &new_trace->size) == 3) {
            new_trace->next = NULL;
            if (head == NULL) {
                head = new_trace;
                tail = new_trace;
            } else {
                tail->next = new_trace;
                tail = new_trace;
            }
        } else {
            free(new_trace);
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
            printf("%s: Missing required command line argument\n", argv[0]);
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

typedef struct Line {
    unsigned long long tag;
    struct Line *next;
    struct Line *pre;
} Line;

typedef Line *Cache;

Cache get_cache(const int s, const int E, const int b) {
    int i;
    int total_set_number = (1 << s);
    Cache cache = (Line *)malloc(total_set_number * sizeof(Line));
    for (i = 0; i < total_set_number; i++) {
        cache[i].next = NULL;
    }
    return cache;
}

void free_set(Line *head) {
    Line *line = head;
    Line *temp;
    while (line) {
        temp = line->next;
        free(line);
        line = temp;
    }
    return;
}

void free_cache(Cache cache, const int s, const int E, const int b) {
    int total_set_number = (1 << s);
    int i;
    for (i = 0; i < total_set_number; i++) {
        free_set(cache[i].next);
    }
    free(cache);
    return;
}

void get_tag_and_set_number(const unsigned long long address, const int s, const int E, const int b,
                            unsigned long long *const tag, int *const set_number) {
    *tag = (address >> (s + b));
    *set_number = (address >> b) & ((1 << s) - 1);
}

void simulate_load(Cache cache, const int s, const int E, const int b, const unsigned long long address, int *const hit,
                   int *const miss, int *const eviction) {
    unsigned long long tag;
    int set_number;

    Line set;

    Line *line = NULL;
    Line *line_pre = NULL;
    int line_count = 0;

    *hit = 0;
    *miss = 0;
    *eviction = 0;

    get_tag_and_set_number(address, s, E, b, &tag, &set_number);
    printf("tag: %llu, set_number: %d\n", tag, set_number);

    set = cache[set_number];

    line = set.next;
    line_pre = &set;
    line_count = 0;

    while (line) {
        if (line->tag == tag) {
            line->pre->next = line->next;
            line->next->pre = line->pre;

            line->next = set.next;
            set.next->pre = line;

            line->pre = &set;
            set.next = line;

            *hit = 1;
            *miss = 0;
            *eviction = 0;

            break;
        }

        line_pre = line;
        line = line->next;
        line_count++;
    }

    if (!line) {
        Line *new_line = (Line *)malloc(sizeof(Line));

        new_line->tag = tag;

        new_line->next = set.next;
        if (set.next) {
            set.next->pre = new_line;
        }

        new_line->pre = &set;
        set.next = new_line;

        if (line_count == E) {
            free(line_pre->next);
            line_pre->next = NULL;
        }

        *hit = 0;
        *miss = 1;
        *eviction = (line_count == E);
    }

    return;
}

void simulate_store(Cache cache, const int s, const int E, const int b, const unsigned long long address,
                    int *const hit, int *const miss, int *const eviction) {
    *hit = 0;
    *miss = 0;
    *eviction = 0;

    simulate_load(cache, s, E, b, address, hit, miss, eviction);
}

void simulate_modify(Cache cache, const int s, const int E, const int b, const unsigned long long address,
                     int *const hit, int *const miss, int *const eviction) {
    *hit = 0;
    *miss = 0;
    *eviction = 0;

    int hit_t = 0;
    int miss_t = 0;
    int eviction_t = 0;

    simulate_load(cache, s, E, b, address, &hit_t, &miss_t, &eviction_t);

    *hit += hit_t;
    *miss += miss_t;
    *eviction += eviction_t;

    hit_t = 0;
    miss_t = 0;
    eviction_t = 0;

    simulate_store(cache, s, E, b, address, &hit_t, &miss_t, &eviction_t);

    *hit += hit_t;
    *miss += miss_t;
    *eviction += eviction_t;
}

void print_trace(Trace *trace, const int hit, const int miss, const int eviction) {
    char *prompt = hit ? "hit" : "miss";
    printf("%c %llx,%d %s\n", trace->type, trace->address, trace->size, prompt);
    return;
}

void simulate(const int s, const int E, const int b, Trace *traces, int *const hits, int *const misses,
              int *const evictions, const int flag_verbose) {
    Trace *trace = NULL;
    Cache cache = NULL;
    int hit = 0;
    int miss = 0;
    int eviction = 0;

    *hits = 0;
    *misses = 0;
    *evictions = 0;

    cache = get_cache(s, E, b);

    while (traces) {
        trace = traces;
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

        hit = 0;
        miss = 0;
        eviction = 0;
        traces = traces->next;
    }

    free_cache(cache, s, E, b);

    return;
}

int main(int argc, char *argv[]) {
    int flag_h = 0;
    int flag_v = 0;
    char *value_s = NULL;
    int s = 0;
    char *value_E = NULL;
    int E = 0;
    char *value_b = NULL;
    int b = 0;
    char *tracefile = NULL;

    int hits = 0;
    int misses = 0;
    int evictions = 0;

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

    s = atoi(value_s);
    E = atoi(value_E);
    b = atoi(value_b);

    Trace *traces = parse_traces(tracefile);

    if (s <= 0 || s >= 16 || E <= 0 || E >= 16 || b <= 0 || b >= 16) {
        printf("%s: Arguments too big\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    simulate(s, E, b, traces, &hits, &misses, &evictions, flag_v);

    printSummary(hits, misses, evictions);

    return 0;
}
