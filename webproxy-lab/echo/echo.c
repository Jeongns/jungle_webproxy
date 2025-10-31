#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(void) {
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if(bind(socketfd, (const struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
        perror("bind");
        exit(1);
    }

    listen(socketfd, 1024);

    char buffer[1 << 13] = {0};

    while (1) {
        int fd = accept(socketfd, NULL, NULL);
        read(fd, &buffer, 1 << 13);
        write(fd, buffer, strlen(buffer));
        memset(&buffer, 0, sizeof(buffer));
        close(fd);
    }

    return 0;
}