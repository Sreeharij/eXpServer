#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFF_SIZE 10000
#define MAX_ACCEPT_BACKLOG 2

void strrev(char *str) {
    int len = strlen(str);
    int end = len - 1;

    if (len > 0 && str[end] == '\n') {
        end--;
    }

    for (int start = 0; start < end; start++, end--) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
    }
}

int main() {
    int listen_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int enable = 1;
    setsockopt(listen_sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(listen_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_sock_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(listen_sock_fd, MAX_ACCEPT_BACKLOG) < 0) {
        perror("listen");
        close(listen_sock_fd);
        exit(EXIT_FAILURE);
    }

    printf("[INFORMATION] Server listening on port %d\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int conn_sock_fd = accept(listen_sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (conn_sock_fd < 0) {
            perror("accept");
            continue;
        }

        printf("[INFORMATION] Client connected to Server\n");

        while (1) {
            char buff[BUFF_SIZE];
            memset(buff, 0, BUFF_SIZE);

            ssize_t read_n = recv(conn_sock_fd, buff, sizeof(buff), 0);

            if (read_n < 0) {
                perror("[ERROR] recv");
                close(conn_sock_fd);
                break;  
            } else if (read_n == 0) {
                printf("[INFORMATION] Client disconnected.\n");
                close(conn_sock_fd);
                break;
            }

            printf("[CLIENT MESSAGE] %s", buff);
            strrev(buff);
            send(conn_sock_fd, buff, read_n, 0);
        }
    }

    close(listen_sock_fd);
    return 0;
}
