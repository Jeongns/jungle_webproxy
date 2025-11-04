#include "csapp.h"
#include "utils/http_status.h"
#include "utils/proxy_cache.h"
#include "utils/uri_parser.h"

void proxy(int connfd);
void read_host(rio_t* rio, char* method, char* uri, char* version);
void read_header(rio_t* rio, char* request_header);
void make_request(char* method, char* path, char* client_headers, char* request_headers);
void proxyerror(int fd, HttpStatus errnum, const char* msg);

int main(int argc, char** argv) {
    char* port = argv[1];
    int proxyfd = Open_listenfd(port);

    while (1) {
        int connfd = Accept(proxyfd, NULL, NULL);
        proxy(connfd);
        Close(connfd);
    }
    return 0;
}

void proxy(int connfd) {
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char host[MAXLINE], port[MAXLINE], path[MAXLINE];
    char client_headers[MAXLINE], request_headers[MAXLINE];
    rio_t client_rio, server_rio;
    int serverfd;

    Rio_readinitb(&client_rio, connfd);

    read_host(&client_rio, method, uri, version);
    read_header(&client_rio, client_headers);

    if (gethost(uri, host) || getport(uri, port) || getpath(uri, path)) {
        proxyerror(connfd, HTTP_BAD_REQUEST, "잘못된 URI");
        return;
    }

    char cache_key[MAXLINE];
    sprintf(cache_key, "%s %s", method, uri);

    cache_block* cache;
    if (cache = get_cache(cache_key)) {
        Rio_writen(connfd, cache->data, cache->datasize);
        return;
    }

    if ((serverfd = open_clientfd(host, port)) < 0) {
        proxyerror(connfd, HTTP_BAD_GATEWAY, "서버와 연동을 실패했습니다.");
        return -1;
    }

    make_request(method, path, client_headers, request_headers);

    Rio_readinitb(&server_rio, serverfd);
    Rio_writen(serverfd, request_headers, strlen(request_headers));

    char buf[MAXLINE] = {0};
    char* cache_buf;
    size_t total = 0;
    ssize_t n;
    while ((n = Rio_readnb(&server_rio, buf, MAXLINE)) > 0) {
        Rio_writen(connfd, buf, n);
        cache_buf = realloc(cache_buf, total + n + 1);
        memcpy(cache_buf + total, buf, n);
        total += n;
        cache_buf[total] = '\0';
    }

    Close(serverfd);
    insert_cache(cache_key, cache_buf);
    if (total) free(cache_buf);
}

//

void read_host(rio_t* rio, char* method, char* uri, char* version) {
    char* buf[MAXLINE];
    Rio_readlineb(rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
}

void read_header(rio_t* rio, char* request_header) {
    char* buf[MAXLINE];
    while (Rio_readlineb(rio, buf, MAXLINE) > 0) {
        if (!strcmp(buf, "\r\n")) break;
        if (!strncasecmp(buf, "User-Agent", 10) || !strncasecmp(buf, "Connection", 10) ||
            !strncasecmp(buf, "Proxy-Connection", 16))
            continue;
        strcat(request_header, buf);
    }
}

void make_request(char* method, char* path, char* client_headers, char* request_headers) {
    sprintf(request_headers,
            "%s %s HTTP/1.0\r\n"
            "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\n"
            "Connection: close\r\n"
            "Proxy-Connection: close\r\n"
            "%s\r\n",
            method, path, client_headers);
}

void proxyerror(int fd, HttpStatus errnum, const char* msg) {
    char header[MAXBUF], body[MAXBUF];

    sprintf(body,
            "<html><title>Proxy Error</title>"
            "<body>\r\n"
            "%d %s\r\n"
            "<p>%s</p>\r\n"
            "</body></html>\r\n",
            errnum, http_reason(errnum), msg);

    sprintf(header,
            "HTTP/1.0 %d %s\r\n"
            "Content-type: text/html\r\n"
            "Content-length: %d\r\n"
            "Connection: close\r\n"
            "\r\n",
            errnum, http_reason(errnum), strlen(body));

    Rio_writen(fd, header, strlen(header));
    Rio_writen(fd, body, strlen(body));
}