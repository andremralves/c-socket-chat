#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/select.h>
#include <string.h>

#include "utils.h"

#define BUF_SIZE 256
#define STDIN 0

int server_fd;
int reuseaddr = 1;
unsigned short port;
char server_addr[20];
int ready, nfds = 0;
fd_set all_sockets, readfds;

int client_rooms[1024] = {0};

//Função que envia uma mensagem para todos os clientes na mesma sala, exceto o remetente.
void send_msg_to_room(int sender_id, char *msg) {
    int sender_room = client_rooms[sender_id];
    for(int j=1; j<=nfds; j++) {
        if(FD_ISSET(j, &all_sockets)) {
            if(j != server_fd && j != sender_id && client_rooms[j] == sender_room) {
                if(send(j, msg, strlen(msg), 0) < 0) {
                    handle_error("send");
                }
            }
        }
    }
}

void handle_request(int client_fd, char *request) {
    // Verificar se a requisição é um comando
    if(request[0] == '/') {

        char *token = strtok(request, " ");

        if(strcmp(token, "/join") == 0) {
            token = strtok(NULL, token);
            int room_id = atoi(token);

            // Verificar se o ID da sala é válid
            if(room_id == 0) {
                //handle invalid room_id
            } 
            else {
                client_rooms[client_fd] = room_id;
            }
        } 

        else if(strcmp(token, "/exit") == 0) {
            // Comando "/exit", o cliente sai da sala atual
            client_rooms[client_fd] = 0;
        }
    } 
    else {
        // A requisição é uma mensagem, enviar para a sala
        send_msg_to_room(client_fd, request);
    }
}

int main(int argc, char *argv[]) {

    if(argc != 3) {
        fprintf(stderr, "Usage: %s addr port\n", argv[0]);
        exit(1);
    }

    port = (unsigned short) atoi(argv[2]);
    strcpy(server_addr, argv[1]);

    struct sockaddr_in server = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = inet_addr(server_addr)
    };

    // Criar o socket do servidor
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        handle_error("socket");
    }

    // Configurar a opção SO_REUSEADDR para permitir a reutilização do endereço
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) < 0) {
        handle_error("setsockopt");
    }
    
    // Associar o socket ao endereço do servidor
    if((bind(server_fd, (struct sockaddr *)&server, sizeof(server))) < 0) {
        handle_error("bind");
    }

    printf("Listening on %s:%d\n", server_addr, port);

    // Iniciar a escuta por conexões de clientes
    if(listen(server_fd, 10) < 0) {
        handle_error("listen");
    }

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
                    // Socket do servidor: nova conexão de cliente
                    int client_fd;
                    struct sockaddr_in client;
                    socklen_t client_len = sizeof(client);

                    if((client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len)) < 0) {
                        handle_error("accept");
                    }
                    printf("[%s] %s:%d connected\n", get_current_time(), inet_ntoa(client.sin_addr), ntohs(client.sin_port));

                    // Enviar uma mensagem de boas-vindas para o cliente
                    char welcome_msg[] = "Bem-vindo ao Chat_Fundamento de Redes! Você pode usar os seguintes comandos:\n/join <sala>: entra em uma sala\n/exit: sai da sala\n";
                    if(send(client_fd, welcome_msg, strlen(welcome_msg), 0) < 0) {
                        handle_error("send");
                    }
                    
                    FD_SET(client_fd, &all_sockets);
                    nfds = max(client_fd, nfds);
                } 
                else 
                {
                    // Mensagem de cliente
                    char request[BUF_SIZE] = {0};
                    if(recv(i, request, sizeof(request), 0) < 0) {
                        handle_error("recv");
                    }
                    // Tratar a requisição recebida do cliente
                    handle_request(i, request);
                }
            }
        }
    }

    return EXIT_SUCCESS;
}