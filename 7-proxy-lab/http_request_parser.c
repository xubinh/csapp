#include "http_request_parser.h"
#include "csapp.h"
#include "proxy_utils.h"
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static regex_t regex_method;
static const char *parrern_method = "^[^\x00-\x1F\x7F()<>@,;:\\\"/\\[\\]?={} \\t]+$";

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
static const char *parrern_request_header = "^([^\x00-\x1F\x7F()<>@,;:\\\"/\\[\\]?={} \\t]+):([^\x00-\x1F\x7F]*)\r\n";
static const size_t regex_request_header_group_number = 3;
static const size_t regex_request_header_field_name_group_index = 1;
static const size_t regex_request_header_field_value_group_index = 2;

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
    http_parse_result->last_request_header = http_parse_result->request_headers;
    http_parse_result->request_header_number = 0;
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
    if (http_parse_result->host) {
        Free(http_parse_result->method);
    }
    if (http_parse_result->port) {
        Free(http_parse_result->method);
    }
    if (http_parse_result->path) {
        Free(http_parse_result->method);
    }
    if (http_parse_result->query_string) {
        Free(http_parse_result->method);
    }
    if (http_parse_result->http_version) {
        Free(http_parse_result->method);
    }
    if (http_parse_result->request_headers) {
        free_request_header_linked_list(http_parse_result->request_headers);
    }
    if (http_parse_result->entity_body) {
        Free(http_parse_result->method);
    }

    Free(http_parse_result);
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
    if (url[0] != 'h') {
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

    // port:
    if (matches[regex_url_port_group_index].rm_so != -1) {
        http_parse_result->port_length = matches[regex_url_port_group_index].rm_eo - matches[regex_url_port_group_index].rm_so;
        http_parse_result->port = (char *)Malloc(sizeof(char) * (http_parse_result->port_length + 1));
        strncpy(http_parse_result->port, url + matches[regex_url_port_group_index].rm_so, http_parse_result->port_length);
        http_parse_result->port[http_parse_result->port_length] = '\0';
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
        http_parse_result->path = (char *)Malloc(sizeof(char) * (http_parse_result->path_length + 1));
        http_parse_result->path[0] = '/';
        http_parse_result->path[http_parse_result->path_length] = '\0';
    }

    // query string:
    if (matches[regex_url_query_string_group_index].rm_so != -1) {
        http_parse_result->query_string_length = matches[regex_url_query_string_group_index].rm_eo - matches[regex_url_query_string_group_index].rm_so;
        http_parse_result->query_string = (char *)Malloc(sizeof(char) * (http_parse_result->query_string_length + 1));
        strncpy(http_parse_result->query_string, url + matches[regex_url_query_string_group_index].rm_so, http_parse_result->query_string_length);
        http_parse_result->query_string[http_parse_result->query_string_length] = '\0';
    }

    return TRUE;
}

static int is_valid_http_version(const char *http_version, HTTPParseResult *http_parse_result) {
    if (regexec(&regex_http_version, http_parse_result->http_version, 0, NULL, 0) != 0) {
        return FALSE;
    }

    http_parse_result->http_version_length = strlen(http_version);
    http_parse_result->http_version = (char *)Malloc(sizeof(char) * (http_parse_result->http_version_length + 1));
    strcpy(http_parse_result->http_version, http_version);

    return TRUE;
}

static int is_valid_request_line(const char *request_line, HTTPParseResult *http_parse_result) {
    char *method = (char *)Malloc(sizeof(char) * MAXLINE);
    char *url = (char *)Malloc(sizeof(char) * MAXLINE);
    char *http_version = (char *)Malloc(sizeof(char) * MAXLINE);
    if (sscanf(request_line, "%s %s %s\r\n", method, url, http_version) != 3) {
        Free(method);
        Free(url);
        Free(http_version);

        return FALSE;
    }

    if (!is_valid_method(method, http_parse_result)) {
        return FALSE;
    }

    if (!is_valid_url(url, http_parse_result)) {
        return FALSE;
    }

    if (!is_valid_http_version(http_version, http_parse_result)) {
        return FALSE;
    }

    return TRUE;
}

static int request_line_parser(rio_t *rio, char *line_buffer, HTTPParseResult *http_parse_result) {
    // 请求行的三个部分, 两个空格, 以及结尾的 `\r\n`:
    if (Rio_readlineb(rio, line_buffer, MAXLINE) <= 7) {
        return FALSE;
    }

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
    // 不断读取并丢弃, 直至遇到 EOF:
    while (rio_readnb(rio, line_buffer, 1))
        ;

    return TRUE;
}

int http_request_parser(int client_socket_descriptor, HTTPParseResult *http_parse_result) {
    rio_t *rio = (rio_t *)Malloc(sizeof(rio_t));
    char *line_buffer = (char *)Malloc(sizeof(char) * MAXLINE);

    Rio_readinitb(rio, client_socket_descriptor);

    int result = TRUE;

    if (result && !request_line_parser(rio, line_buffer, http_parse_result)) {
        result = FALSE;
    }

    if (result && !request_headers_parser(rio, line_buffer, http_parse_result)) {
        result = FALSE;
    }

    if (result && !request_body_parser(rio, line_buffer, http_parse_result)) {
        result = FALSE;
    }

    Free(line_buffer);
    Free(rio);

    return result;
}