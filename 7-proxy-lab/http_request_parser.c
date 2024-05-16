#include "http_request_parser.h"
#include "csapp.h"
#include "proxy_utils.h"
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static regex_t regex_method;
static const char *parrern_method = "^[!#$%&'*+-.0-9A-Z^_`a-z|~]+$";

static regex_t regex_url;
static const char *pattern_url = "^http://((\\w+\\.)*\\w+)(:([0-9]+))?(/([^?]*)?(\\?(.*))?)?$";
static const size_t regex_url_group_number = 9;
static const size_t regex_url_host_group_index = 1;
static const size_t regex_url_port_group_index = 4;
static const size_t regex_url_path_group_index = 6;
static const size_t regex_url_query_string_group_index = 8;

static regex_t regex_dotted_decimal_form_ip;
static const char *pattern_dotted_decimal_form_ip = "^([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)$";
static const size_t regex_dotted_decimal_form_ip_group_number = 5;
static const size_t regex_dotted_decimal_form_ip_part_1_group_index = 1;
static const size_t regex_dotted_decimal_form_ip_part_2_group_index = 2;
static const size_t regex_dotted_decimal_form_ip_part_3_group_index = 3;
static const size_t regex_dotted_decimal_form_ip_part_4_group_index = 4;

static regex_t regex_host_domain_name;
static const char *pattern_host_domain_name = "^[a-zA-Z0-9.-]{2,}$";

static regex_t regex_http_version;
static const char *pattern_http_version = "^HTTP/[0-9]+\\.[0-9]+$";

static regex_t regex_request_header;
// field-content = OCTET - CTL + LWS:
static const char *parrern_request_header = "^([!#$%&'*+-.0-9A-Z^_`a-z|~]+):([ -~]*)\r\n";
static const size_t regex_request_header_group_number = 3;
static const size_t regex_request_header_field_name_group_index = 1;
static const size_t regex_request_header_field_value_group_index = 2;

static void initialize_request_header_node(RequestHeaderNode *request_header_node) {
    request_header_node->field_name = NULL;
    request_header_node->field_name_length = -1;
    request_header_node->field_value = NULL;
    request_header_node->field_value_length = -1;
    request_header_node->next_node = NULL;
}

void initialize_http_parse_result(HTTPParseResult *http_parse_result) {
    http_parse_result->method = NULL;
    http_parse_result->method_length = -1;
    http_parse_result->host = NULL;
    http_parse_result->host_length = -1;
    http_parse_result->is_ip = FALSE;
    http_parse_result->port = NULL;
    http_parse_result->port_length = -1;
    http_parse_result->path = NULL;
    http_parse_result->path_length = -1;
    http_parse_result->query_string = NULL;
    http_parse_result->query_string_length = -1;
    http_parse_result->http_version = NULL;
    http_parse_result->http_version_length = -1;
    http_parse_result->request_headers = (RequestHeaderNode *)Malloc(sizeof(RequestHeaderNode));
    initialize_request_header_node(http_parse_result->request_headers);
    http_parse_result->last_request_header = http_parse_result->request_headers;
    http_parse_result->entity_body = NULL;
    http_parse_result->entity_body_length = -1;
}

static void free_request_header_linked_list(RequestHeaderNode *request_header_linked_list) {
    if (!request_header_linked_list) {
        return;
    }

    RequestHeaderNode *current_node = request_header_linked_list;
    RequestHeaderNode *next_node = current_node->next_node;
    while (1) {
        if (current_node->field_name) {
            Free(current_node->field_name);
        }

        if (current_node->field_value) {
            Free(current_node->field_value);
        }

        Free(current_node);

        if (!next_node) {
            return;
        }

        current_node = next_node;
        next_node = current_node->next_node;
    }
}

void free_http_parse_result(HTTPParseResult *http_parse_result) {
    if (!http_parse_result) {
        return;
    }

    if (http_parse_result->method) {
        Free(http_parse_result->method);
    }

    printf("已释放 http_parse_result->method 指针\n");

    if (http_parse_result->host) {
        Free(http_parse_result->host);
    }

    printf("已释放 http_parse_result->host 指针\n");

    if (http_parse_result->port) {
        Free(http_parse_result->port);
    }

    printf("已释放 http_parse_result->port 指针\n");

    if (http_parse_result->path) {
        Free(http_parse_result->path);
    }

    printf("已释放 http_parse_result->path 指针\n");

    if (http_parse_result->query_string) {
        Free(http_parse_result->query_string);
    }

    printf("已释放 http_parse_result->query_string 指针\n");

    if (http_parse_result->http_version) {
        Free(http_parse_result->http_version);
    }

    printf("已释放 http_parse_result->http_version 指针\n");

    if (http_parse_result->request_headers) {
        free_request_header_linked_list(http_parse_result->request_headers);
    }

    printf("已释放 http_parse_result->request_headers 链表\n");

    if (http_parse_result->entity_body) {
        Free(http_parse_result->entity_body);
    }

    printf("已释放 http_parse_result->entity_body 指针\n");
}

int initialize_http_request_parser() {
    if (regcomp(&regex_method, parrern_method, REG_EXTENDED)) {
        return FALSE;
    }

    if (regcomp(&regex_url, pattern_url, REG_EXTENDED)) {
        return FALSE;
    }

    if (regcomp(&regex_dotted_decimal_form_ip, pattern_dotted_decimal_form_ip, REG_EXTENDED)) {
        return FALSE;
    }

    if (regcomp(&regex_host_domain_name, pattern_host_domain_name, REG_EXTENDED)) {
        return FALSE;
    }

    if (regcomp(&regex_http_version, pattern_http_version, REG_EXTENDED)) {
        return FALSE;
    }

    if (regcomp(&regex_request_header, parrern_request_header, REG_EXTENDED)) {
        return FALSE;
    }

    return TRUE;
}

void free_http_request_parser() {
    regfree(&regex_method);
    regfree(&regex_url);
    regfree(&regex_dotted_decimal_form_ip);
    regfree(&regex_host_domain_name);
    regfree(&regex_http_version);
    regfree(&regex_request_header);
}

static int is_valid_method(const char *method, HTTPParseResult *http_parse_result) {
    if (regexec(&regex_method, method, 0, NULL, 0) != 0) {
        return FALSE;
    }

    http_parse_result->method_length = strlen(method);
    http_parse_result->method = (char *)Malloc(sizeof(char) * (http_parse_result->method_length + 1));
    strcpy(http_parse_result->method, method);

    return TRUE;
}

static int is_valid_dotted_decimal_form_ip_part(char *dotted_decimal_form_ip_part) {
    int num = atoi(dotted_decimal_form_ip_part);

    if (num >= 0 && num <= 255) {
        return TRUE;
    }

    return FALSE;
}

static int is_valid_dotted_decimal_form_ip(HTTPParseResult *http_parse_result) {
    regmatch_t matches[regex_dotted_decimal_form_ip_group_number];

    if (regexec(&regex_dotted_decimal_form_ip, http_parse_result->host, regex_dotted_decimal_form_ip_group_number, matches, 0) != 0) {
        return FALSE;
    }

    if (!is_valid_dotted_decimal_form_ip_part(http_parse_result->host + matches[regex_dotted_decimal_form_ip_part_1_group_index].rm_so)) {
        return FALSE;
    }
    if (!is_valid_dotted_decimal_form_ip_part(http_parse_result->host + matches[regex_dotted_decimal_form_ip_part_2_group_index].rm_so)) {
        return FALSE;
    }
    if (!is_valid_dotted_decimal_form_ip_part(http_parse_result->host + matches[regex_dotted_decimal_form_ip_part_3_group_index].rm_so)) {
        return FALSE;
    }
    if (!is_valid_dotted_decimal_form_ip_part(http_parse_result->host + matches[regex_dotted_decimal_form_ip_part_4_group_index].rm_so)) {
        return FALSE;
    }

    return TRUE;
}

static int is_valid_host_domain_name(HTTPParseResult *http_parse_result) {
    if (regexec(&regex_host_domain_name, http_parse_result->host, 0, NULL, 0) != 0) {
        return FALSE;
    }

    if (http_parse_result->host[0] == '.' || http_parse_result->host[0] == '-' || http_parse_result->host[http_parse_result->host_length - 1] == '.' || http_parse_result->host[http_parse_result->host_length - 1] == '-') {
        return FALSE;
    }

    return TRUE;
}

static int is_valid_host(HTTPParseResult *http_parse_result) {
    if (is_valid_dotted_decimal_form_ip(http_parse_result)) {
        http_parse_result->is_ip = TRUE;
        return TRUE;
    } else if (is_valid_host_domain_name(http_parse_result)) {
        http_parse_result->is_ip = FALSE;
        return TRUE;
    } else {
        return FALSE;
    }
}

static int is_valid_url(const char *url, HTTPParseResult *http_parse_result) {
    if (url[0] != 'h') { // [TODO] 这里简单将语法检查和不支持场景的过滤混杂在了一起
        return FALSE;
    }

    regmatch_t matches[regex_url_group_number];

    if (regexec(&regex_url, url, regex_url_group_number, matches, 0) != 0) {
        return FALSE;
    }

    // host:
    http_parse_result->host_length = matches[regex_url_host_group_index].rm_eo - matches[regex_url_host_group_index].rm_so;
    http_parse_result->host = (char *)Malloc(sizeof(char) * (http_parse_result->host_length + 1));
    strncpy(http_parse_result->host, url + matches[regex_url_host_group_index].rm_so, http_parse_result->host_length);
    http_parse_result->host[http_parse_result->host_length] = '\0';

    if (!is_valid_host(http_parse_result)) {
        return FALSE;
    }

    printf("host: %s\n", http_parse_result->host);

    // port:
    if (matches[regex_url_port_group_index].rm_so != -1) {
        http_parse_result->port_length = matches[regex_url_port_group_index].rm_eo - matches[regex_url_port_group_index].rm_so;
        http_parse_result->port = (char *)Malloc(sizeof(char) * (http_parse_result->port_length + 1));
        strncpy(http_parse_result->port, url + matches[regex_url_port_group_index].rm_so, http_parse_result->port_length);
        http_parse_result->port[http_parse_result->port_length] = '\0';

        printf("port: %s\n", http_parse_result->port);
    }

    // path:
    if (matches[regex_url_path_group_index].rm_so != -1) {
        size_t relative_path_length = matches[regex_url_path_group_index].rm_eo - matches[regex_url_path_group_index].rm_so;
        http_parse_result->path_length = relative_path_length + 1;
        http_parse_result->path = (char *)Malloc(sizeof(char) * (http_parse_result->path_length + 1));
        http_parse_result->path[0] = '/';
        strncpy(http_parse_result->path + 1, url + matches[regex_url_path_group_index].rm_so, relative_path_length);
        http_parse_result->path[http_parse_result->path_length] = '\0';
    } else {
        http_parse_result->path_length = 1;
        http_parse_result->path = (char *)Malloc(sizeof(char) * 2);
        http_parse_result->path[0] = '/';
        http_parse_result->path[1] = '\0';
    }

    printf("path: %s\n", http_parse_result->path);

    http_parse_result->is_dynamic = strstr(http_parse_result->path, "cgi-bin") ? TRUE : FALSE;

    printf("is_dynamic: %s\n", http_parse_result->is_dynamic ? "TRUE" : "FALSE");

    // query string:
    if (matches[regex_url_query_string_group_index].rm_so != -1) {
        http_parse_result->query_string_length = matches[regex_url_query_string_group_index].rm_eo - matches[regex_url_query_string_group_index].rm_so;
        http_parse_result->query_string = (char *)Malloc(sizeof(char) * (http_parse_result->query_string_length + 1));
        strncpy(http_parse_result->query_string, url + matches[regex_url_query_string_group_index].rm_so, http_parse_result->query_string_length);
        http_parse_result->query_string[http_parse_result->query_string_length] = '\0';

        printf("query_string: %s\n", http_parse_result->query_string);
    }

    return TRUE;
}

static int is_valid_http_version(const char *http_version, HTTPParseResult *http_parse_result) {
    if (regexec(&regex_http_version, http_version, 0, NULL, 0) != 0) {
        return FALSE;
    }

    http_parse_result->http_version_length = strlen(http_version);
    http_parse_result->http_version = (char *)Malloc(sizeof(char) * (http_parse_result->http_version_length + 1));
    strcpy(http_parse_result->http_version, http_version);

    return TRUE;
}

static int is_valid_request_line(const char *request_line, HTTPParseResult *http_parse_result) {
    printf("判断请求行的合法性\n");

    int status = TRUE;

    char *method = (char *)Malloc(sizeof(char) * MAXLINE);
    char *url = (char *)Malloc(sizeof(char) * MAXLINE);
    char *http_version = (char *)Malloc(sizeof(char) * MAXLINE);

    if (status && sscanf(request_line, "%s %s %s\r\n", method, url, http_version) != 3) {
        status = FALSE;
    }

    printf("成功分割为三个部分:\n%s\n%s\n%s\n", method, url, http_version);

    printf("字符串长度:\n%d\n%d\n%d\n", (int)strlen(method), (int)strlen(url), (int)strlen(http_version));

    if (status && !is_valid_method(method, http_parse_result)) {
        status = FALSE;
    }

    printf("第一个部分为合法\n");

    if (status && !is_valid_url(url, http_parse_result)) {
        status = FALSE;
    }

    printf("第二个部分为合法\n");

    if (status && !is_valid_http_version(http_version, http_parse_result)) {
        status = FALSE;
    }

    printf("第三个部分为合法\n");

    Free(http_version);
    Free(url);
    Free(method);

    return status;
}

static int request_line_parser(rio_t *rio, char *line_buffer, HTTPParseResult *http_parse_result) {
    // 请求行的三个部分, 两个空格, 以及结尾的 `\r\n`:
    if (Rio_readlineb(rio, line_buffer, MAXLINE) <= 7) {
        return FALSE;
    }

    printf("读取请求行\n");

    if (!is_valid_request_line(line_buffer, http_parse_result)) {
        return FALSE;
    }

    return TRUE;
}

static int is_valid_request_header(const char *request_line, HTTPParseResult *http_parse_result) {
    regmatch_t matches[regex_request_header_group_number];

    // 使用正则表达式解析请求头部:
    if (regexec(&regex_request_header, request_line, regex_request_header_group_number, matches, 0) != 0) {
        return FALSE;
    }

    RequestHeaderNode *new_request_header_node = (RequestHeaderNode *)Malloc(sizeof(RequestHeaderNode));

    // 字段名称:
    new_request_header_node->field_name_length = matches[regex_request_header_field_name_group_index].rm_eo - matches[regex_request_header_field_name_group_index].rm_so;
    new_request_header_node->field_name = (char *)Malloc(sizeof(char) * (new_request_header_node->field_name_length + 1));
    strncpy(new_request_header_node->field_name, request_line + matches[regex_request_header_field_name_group_index].rm_so, new_request_header_node->field_name_length);
    new_request_header_node->field_name[new_request_header_node->field_name_length] = '\0';

    // 字段值:
    new_request_header_node->field_value_length = matches[regex_request_header_field_value_group_index].rm_eo - matches[regex_request_header_field_value_group_index].rm_so;
    new_request_header_node->field_value = (char *)Malloc(sizeof(char) * (new_request_header_node->field_value_length + 1));
    strncpy(new_request_header_node->field_value, request_line + matches[regex_request_header_field_value_group_index].rm_so, new_request_header_node->field_value_length);
    new_request_header_node->field_value[new_request_header_node->field_value_length] = '\0';

    // 将解析到的头部插入链表至末尾:
    new_request_header_node->next_node = NULL;
    http_parse_result->last_request_header->next_node = new_request_header_node;
    http_parse_result->last_request_header = new_request_header_node;

    return TRUE;
}

static int request_headers_parser(rio_t *rio, char *line_buffer, HTTPParseResult *http_parse_result) {
    ssize_t line_length;
    while (1) {
        // 如果读取到少于 2 个字符说明不合法:
        if ((line_length = Rio_readlineb(rio, line_buffer, MAXLINE)) < 2) {
            return FALSE;
        }

        // 如果读取到恰好 2 个字符, 解析成功当且仅当该行等于 `\r\n`:
        if (line_length == 2) {
            return strcmp(line_buffer, "\r\n") == 0 ? TRUE : FALSE;
        }

        // 如果读到多于 2 个字符, 则送入解析器:
        if (!is_valid_request_header(line_buffer, http_parse_result)) {
            return FALSE;
        }
    }
}

static int request_body_parser(rio_t *rio, char *line_buffer, HTTPParseResult *http_parse_result) {
    // 如果是 GET 方法则直接返回:
    if (strcmp(http_parse_result->method, "GET") == 0) {
        http_parse_result->entity_body = NULL;
        http_parse_result->entity_body_length = -1;

        return TRUE;
    }

    printf("仅支持 GET 请求\n");

    return FALSE;
}

int http_request_parser(int client_socket_descriptor, HTTPParseResult *http_parse_result) {
    printf("进入解析器\n");

    rio_t *rio = (rio_t *)Malloc(sizeof(rio_t));
    Rio_readinitb(rio, client_socket_descriptor);

    printf("初始化缓冲区\n");

    char *line_buffer = (char *)Malloc(sizeof(char) * MAXLINE);

    int result = TRUE;

    if (result && !request_line_parser(rio, line_buffer, http_parse_result)) {
        result = FALSE;
    }

    printf("成功解析请求行, 状态: %d\n", result);

    if (result && !request_headers_parser(rio, line_buffer, http_parse_result)) {
        result = FALSE;
    }

    printf("成功解析所有请求头部, 状态: %d\n", result);

    if (result && !request_body_parser(rio, line_buffer, http_parse_result)) {
        result = FALSE;
    }

    printf("成功解析请求主体, 状态: %d\n", result);

    Free(line_buffer);
    Free(rio);

    return result;
}