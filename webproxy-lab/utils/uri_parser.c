#include "uri_parser.h"

#include <string.h>

char* skip_scheme(const char* uri) {
    char* p = strstr(uri, "://");
    return p ? (p + 3) : uri;
}

int gethost(const char* uri, char* host) {
    char *start_pointer, *port_pointer;
    start_pointer = skip_scheme(uri);

    if (!(port_pointer = strchr(start_pointer, ':'))) return 1;
    int len = port_pointer - start_pointer;
    strncpy(host, start_pointer, len);
    host[len] = '\0';
    return 0;
}

int getport(const char* uri, char* port) {
    char *start_pointer, *port_pointer, *slash_pointer;
    start_pointer = skip_scheme(uri);

    if (!(port_pointer = strchr(start_pointer, ':')) ||
        !(slash_pointer = strchr(start_pointer, '/')))
        return 1;

    int len = slash_pointer - port_pointer - 1;
    strncpy(port, port_pointer + 1, len);
    port[len] = '\0';
    return 0;
}

int getpath(const char* uri, char* path) {
    char *start_pointer, *slash_pointer, *slash_end_pointer;
    start_pointer = skip_scheme(uri);

    if (!(slash_pointer = strchr(start_pointer, '/')) ||
        !(slash_end_pointer = strrchr(start_pointer, '\0')))
        return 1;

    int len = slash_end_pointer - slash_pointer + 1;
    strncpy(path, slash_pointer, len);
    path[len] = '\0';
    return 0;
}