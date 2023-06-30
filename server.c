#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>

#include <sys/select.h>
#include <time.h>
#include <string.h>

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#undef max
#define max(x,y) ((x) > (y) ? (x) : (y))

#define BUF_SIZE 256
#define STDIN 0
                       //
char *get_current_time() {
    time_t curr_time = time(NULL);
    return strtok(ctime(&curr_time), "\n");
}

int main(int argc, char *argv[]) {

    if(argc != 3) {
        fprintf(stderr, "Usage: %s addr port\n", argv[0]);
        exit(1);
    }

    int server_fd;
    int reuseaddr = 1;
    unsigned short port;
    char server_addr[20];

    port = (unsigned short) atoi(argv[2]);
    strcpy(server_addr, argv[1]);

    struct sockaddr_in server = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = inet_addr(server_addr)
    };

    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        handle_error("socket");
    }

    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) < 0) {
        handle_error("setsockopt");
    }

    if((bind(server_fd, (struct sockaddr *)&server, sizeof(server))) < 0) {
        handle_error("bind");
    }

    printf("Listening on %s:%d\n", server_addr, port);
    if(listen(server_fd, 10) < 0) {
        handle_error("listen");
    }

    int ready, nfds = 0;
    fd_set all_sockets, readfds;

    // Init SET
    FD_ZERO(&all_sockets);
    FD_SET(server_fd, &all_sockets);
    FD_SET(STDIN, &all_sockets);
    nfds = max(server_fd, nfds);

    for(;;) {

        readfds = all_sockets;

        if((ready = select(nfds+1, &readfds, NULL, NULL, NULL)) < 0) {
            handle_error("select");
        }

        for(int i=0; i<=nfds; i++) {
            if(FD_ISSET(i, &readfds)) {
                if(i == server_fd) {
                    // Server socket: new client connection
                    int client_fd;
                    struct sockaddr_in client;
                    socklen_t client_len = sizeof(client);
                    if((client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len)) < 0) {
                        handle_error("accept");
                    }
                    printf("[%s] %s:%d connected\n", get_current_time(), inet_ntoa(client.sin_addr), ntohs(client.sin_port));
                    FD_SET(client_fd, &all_sockets);
                    nfds = max(client_fd, nfds);
                } else {
                    // Client message
                    char request[BUF_SIZE] = {0};
                    printf("%d\n", i);
                    if(recv(i, request, sizeof(request), 0) < 0) {
                        handle_error("recv");
                    }
                    // send data to other clients
                    for(int j=1; j<=nfds; j++) {
                        if(FD_ISSET(j, &all_sockets)) {
                            if(j != server_fd && j != i) {
                                if(send(j, request, sizeof(request), 0) < 0) {
                                    handle_error("send");
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return EXIT_SUCCESS;
}
