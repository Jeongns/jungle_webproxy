#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

// 인원 파악하기

int main(void) {
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socketfd, (const struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
        perror("bind");
        exit(1);
    }

    listen(socketfd, 1024);

    fd_set allset;
    FD_ZERO(&allset);
    FD_SET(socketfd, &allset);
    int fdmax = socketfd;
    char buf[1 << 13];

    while (1) {
        fd_set rset = allset;
        select(fdmax + 1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(socketfd, &rset)) {
            int connfd = accept(socketfd, NULL, NULL);
            FD_SET(connfd, &allset);
            fdmax = connfd > fdmax ? connfd : fdmax;
        }

        for (int fd = 3; fd <= fdmax; fd++) {
            if (fd == socketfd) continue;
            if (FD_ISSET(fd, &rset)) {
                int n = read(fd, buf, sizeof(buf));
                if (n <= 0) {
                    close(fd);
                    FD_CLR(fd, &allset);
                    continue;
                }

                for (int other = 3; other <= fdmax; other++) {
                    if (other != socketfd && other != fd && FD_ISSET(other, &allset))
                        write(other, buf, n);
                }
            }
        }
    }

    return 0;
}