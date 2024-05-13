#include <stddef.h>

typedef struct RequestHeaderNode {
    char *field_name;
    size_t field_name_length;
    char *field_value;
    size_t field_value_length;
    RequestHeaderNode *next_node;
} RequestHeaderNode;

typedef struct HTTPParseResult {
    char *method;
    size_t method_length;
    char *host;
    size_t host_length;
    int is_ip;
    char *port;
    size_t port_length;
    char *path;
    size_t path_length;
    char *query_string;
    size_t query_string_length;
    char *http_version;
    size_t http_version_length;
    RequestHeaderNode *request_headers;
    RequestHeaderNode *last_request_header;
    size_t request_header_number;
    char *entity_body;
    size_t entity_body_length;
} HTTPParseResult;

void initialize_http_parse_result(HTTPParseResult *http_parse_result);

void free_http_parse_result(HTTPParseResult *http_parse_result);

int initialize_http_request_parser();

void free_http_request_parser();

int http_request_parser(int client_socket_descriptor, HTTPParseResult *http_parse_result);