#include "proxy_utils.h"

// 对字符串进行哈希, 参考自 <https://stackoverflow.com/questions/2535284/how-can-i-hash-a-string-to-an-int-using-c>:
unsigned long hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;

    while (c = *str++) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}
