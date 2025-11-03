#include "csapp.h"
#include "utils/http_status.h"
#include "utils/uri_parser.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void proxy(int connfd);

/* You won't lose style points for including this long line in your code */
static const char* user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

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
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char host[MAXLINE], port[MAXLINE], path[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    if (gethost(uri, host) || getport(uri, port) || getpath(uri, path)) {
        proxyerror(connfd, HTTP_BAD_REQUEST, "잘못된 URI");
        return;
    }
    printf("%s %s %s\n", host, port, path);
}

void proxyerror(int fd, HttpStatus errnum, char* msg) {
    char buf[MAXLINE];
    sprintf(buf,
            "%d\n"
            "%s\n"
            "%s\n",
            errnum, http_reason(errnum), msg);

    Rio_writen(fd, buf, strlen(buf));
}