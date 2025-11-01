/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t* rp);
int parse_uri(char* uri, char* filename, char* cgiargs);
void serve_static(int fd, char* filename, int filesize);
void get_filetype(char* filename, char* filetype);
void serve_dynamic(int fd, char* filename, char* cgiargs);
void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg);

int main(int argc, char** argv) {
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA*)&clientaddr,
                        &clientlen);  // line:netp:tiny:accept
        Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);
        Close(connfd);  // line:netp:tiny:close
    }
}

// 7.	doit()
// •	위 전체를 묶어서 한 요청 처리

/// @brief 하나의 HTTP 요청을 처리한다.
/// @param fd 클라이언트와 연결된 소켓의 파일 디스크립터.
void doit(int fd) {
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char cgiargs[MAXLINE], filename[MAXLINE];
    struct stat sbuf;
    rio_t rio;

    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);

    sscanf(buf, "%s %s %s", method, uri, version);

    if (strcmp(method, "GET")) {
        clienterror(fd, "GET만됨", "501", "GET으로 오시오", "GET말고 구현 안되있음 ㅋㅋ");
        return;
    }

    if (parse_uri(uri, filename, cgiargs)) {
        serve_dynamic(fd, filename, cgiargs);
        return;
    }

    if (stat(filename, &sbuf) != 0) {
        clienterror(fd, "파일 못읽음", "500", "html 못찾음", "ㅠㅠㅠㅠ");
        return;
    }
    serve_static(fd, filename, sbuf.st_size);
    return;
}

/// @brief 클라이언트가 보낸 HTTP 요청 헤더를 읽고 무시한다.
/// @param rp RIO 버퍼 구조체 포인터 (버퍼링된 읽기용).
void read_requesthdrs(rio_t* rp) {
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
    }
}

/// @brief 요청 URI를 분석하여 정적/동적 요청을 구분하고, 파일 이름과 인자를 추출한다.
/// @param uri 요청된 URI 문자열.
/// @param filename 요청된 파일 경로를 저장할 버퍼.
/// @param cgiargs CGI 프로그램 인자를 저장할 버퍼.
/// @return 정적 콘텐츠면 1, 동적 콘텐츠면 0을 반환.
int parse_uri(char* uri, char* filename, char* cgiargs) {
    strcpy(filename, ".");
    if (strstr(uri, "cgi-bin")) {
        char* ptr = strchr(uri, '?');
        if (ptr == NULL) {
            strcpy(cgiargs, "");
            return;
        }
        strcpy(cgiargs, ptr + 1);
        *ptr = '\0';
        strcat(filename, uri);
        return 1;
    }
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/') strcat(filename, "home.html");
    return 0;
}

/// @brief 파일 이름의 확장자에 따라 MIME 타입을 결정한다. (참고:
/// https://www.iana.org/assignments/media-types/media-types.xhtml)
/// @param filename 파일 이름.
/// @param filetype MIME 타입 문자열을 저장할 버퍼.
void get_filetype(char* filename, char* filetype) {
    char* extension = strrchr(filename, '.') + 1;
    if (strstr(extension, "html"))
        strcpy(filetype, "text/html");
    else if (strstr(extension, "gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(extension, "png"))
        strcpy(filetype, "image/png");
    else
        strcpy(filetype, "text/plain");
}

/// @brief 정적 콘텐츠(HTML, 이미지 등)를 클라이언트에게 전송한다.
/// @param fd 클라이언트 소켓 디스크립터.
/// @param filename 전송할 파일 이름.
/// @param filesize 파일의 크기(바이트 단위).
void serve_static(int fd, char* filename, int filesize) {
    char fileType[MAXLINE], header[MAXLINE], *body;
    get_filetype(filename, fileType);

    sprintf(header, "HTTP/1.0 200 OK\r\n");
    sprintf(header, "%sContent-Length: %d\r\n", header, filesize);
    sprintf(header, "%sContent-Type: %s\r\n", header, fileType);
    sprintf(header, "%sConnection: close\r\n\r\n", header);

    int filefd = Open(filename, O_RDONLY, 0);
    body = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, filefd, 0);
    Close(filefd);

    Rio_writen(fd, header, strlen(header));
    Rio_writen(fd, body, filesize);

    Munmap(body, filesize);
}

/// @brief CGI 프로그램을 실행하여 동적 콘텐츠를 클라이언트에게 전송한다.
/// @param fd 클라이언트 소켓 디스크립터.
/// @param filename 실행할 CGI 프로그램 파일 이름.
/// @param cgiargs CGI 프로그램 인자 문자열.
void serve_dynamic(int fd, char* filename, char* cgiargs) {
    char header[MAXLINE], *emptylist[] = {NULL};
    sprintf(header, "HTTP/1.0 200 OK\r\n");
    sprintf(header, "%sServer: tiny\r\n", header);
    Rio_writen(fd, header, strlen(header));

    if (Fork() == 0) {
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);
        Execve(filename, emptylist, environ);
    }
    Wait(NULL);
}

/// @brief HTTP 에러 응답을 클라이언트에게 전송한다.
/// @param fd 클라이언트 소켓 디스크립터.
/// @param cause 에러 원인(예: 잘못된 파일 경로).
/// @param errnum HTTP 상태 코드 문자열(예: "404").
/// @param shortmsg 간단한 상태 메시지(예: "Not Found").
/// @param longmsg 상세 설명 메시지.
void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg) {
    char buf[MAXLINE];
    sprintf(buf, "%s\n", cause);
    sprintf(buf, "%s\n", errnum);
    sprintf(buf, "%s\n", shortmsg);
    sprintf(buf, "%s\n", longmsg);

    Rio_writen(fd, buf, strlen(buf));
}