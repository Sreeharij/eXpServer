#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>

#define PORT 8080
#define BUFF_SIZE 10000
#define MAX_ACCEPT_BACKLOG 2
#define MAX_EPOLL_EVENTS 10

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

    // EPOLL Setup BElow
    int epoll_fd = epoll_create1(0);
    if(epoll_fd < 0){
        perror("epoll_create1");
        close(listen_sock_fd);
        exit(EXIT_FAILURE);
    }
    struct epoll_event event, events[MAX_EPOLL_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = listen_sock_fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock_fd, &event) < 0){
        perror("epoll_ctl: listen_sock_fd");
        close(listen_sock_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        printf("[DEBUG]Epoll wait\n");

        int n_ready_fds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);
        if(n_ready_fds < 0){
            perror("epoll_wait");
            break;
        }

        for(int i=0;i<n_ready_fds;i++){
            int curr_fd = events[i].data.fd;
            if(curr_fd == listen_sock_fd){ //EVENT ON LISTEN SOCKET
                struct sockaddr_in client_addr; 
                socklen_t client_addr_len = sizeof(client_addr);

                int conn_sock_fd = accept(listen_sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);
                if (conn_sock_fd < 0) {
                    perror("accept");
                    continue;
                }

                printf("[INFO] Client connected to server\n");

                memset(&event, 0, sizeof(event));
                event.events = EPOLLIN;
                event.data.fd = conn_sock_fd;
                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock_fd, &event) < 0){
                    perror("epoll_ctl: conn_sock_fd");
                    close(conn_sock_fd);
                    continue;
                }
            }
            else{ //EVENT ON CONNECTION SOCKET
                char buff[BUFF_SIZE];
                memset(buff, 0, BUFF_SIZE);

                ssize_t read_n = recv(curr_fd, buff, sizeof(buff), 0);
                if (read_n < 0) {
                    perror("[ERROR] recv");
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, curr_fd, NULL);
                    close(curr_fd);
                } else if (read_n == 0) {
                    printf("[INFO] Client disconnected.\n");
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, curr_fd, NULL);
                    close(curr_fd);
                } else {
                    printf("[CLIENT MESSAGE] %s", buff);
                    strrev(buff);
                    send(curr_fd, buff, read_n, 0);
                }

            }
        }
    }

    close(listen_sock_fd);
    close(epoll_fd);
    return 0;
}
