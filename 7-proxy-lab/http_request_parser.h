#include <stddef.h>

/**
 * @brief 表示请求头部的类
 *
 */
typedef struct RequestHeaderNode {
    char *field_name; ///< 请求头部的字段名称
    size_t field_name_length;
    char *field_value; ///< 请求头部的字段值
    size_t field_value_length;
    struct RequestHeaderNode *next_node;
} RequestHeaderNode;

/**
 * @brief 表示 HTTP 请求的类
 *
 */
typedef struct HTTPParseResult {
    char *method;
    size_t method_length;
    char *host;
    size_t host_length;
    int is_ip;
    char *port;
    size_t port_length;
    char *path;
    int is_dynamic;
    size_t path_length;
    char *query_string;
    size_t query_string_length;
    char *http_version;
    size_t http_version_length;
    RequestHeaderNode *request_headers;
    RequestHeaderNode *last_request_header;
    char *entity_body;
    size_t entity_body_length;
} HTTPParseResult;

void initialize_http_parse_result(HTTPParseResult *http_parse_result);

void free_http_parse_result(HTTPParseResult *http_parse_result);

/**
 * @brief 编译解析器所需的各个正则表达式
 *
 * @return int 是否成功初始化
 */
int initialize_http_request_parser();

void free_http_request_parser();

/**
 * @brief 从已连接套接字中读取并解析 HTTP 请求
 *
 * @param client_socket_descriptor 已连接套接字
 * @param http_parse_result 存储 HTTP 请求的对象
 * @return int 是否成功读取并解析
 */
int http_request_parser(int client_socket_descriptor, HTTPParseResult *http_parse_result);