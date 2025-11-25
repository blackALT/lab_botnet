// bot_safe.c - Cliente Bot Educacional (Mirai Safe-Mode)
// Compilar: gcc bot_safe.c -o bot_safe
// Uso: ./bot_safe <IP_C2> <PORTA_C2>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/utsname.h>
#include <time.h>

#define BUF_SIZE 2048

// Função para gerar um ID aleatório para o Bot
void generate_bot_id(char *buffer, size_t size) {
    srand(time(NULL) ^ getpid());
    snprintf(buffer, size, "BOT-%04d-IOT", rand() % 9999);
}

// Simula a execução de um comando shell e retorna a saída (Safe Mode)
void execute_command(char *cmd, char *response) {
    // Limpeza de buffer
    memset(response, 0, BUF_SIZE);

    if (strncmp(cmd, "PING", 4) == 0) {
        strcpy(response, "PONG: I am alive and listening.");
    } 
    else if (strncmp(cmd, "SYSINFO", 7) == 0) {
        struct utsname unameData;
        uname(&unameData);
        snprintf(response, BUF_SIZE - 1, 
            "OS: %s\nNode: %s\nRelease: %s\nVersion: %s\nMachine: %s",
            unameData.sysname, unameData.nodename, unameData.release, 
            unameData.version, unameData.machine);
    } 
    else if (strncmp(cmd, "NETINFO", 7) == 0) {
        // Em um malware real, isso buscaria interfaces reais. 
        // Aqui, simulamos para não complicar com headers de netlink.
        strcpy(response, "Interface: eth0 (Promiscuous Mode: OFF)\nIP: Hidden/Internal\nMAC: AA:BB:CC:DD:EE:FF");
    } 
    else if (strncmp(cmd, "PS", 2) == 0) {
        // Simula uma lista de processos fake para mostrar que o bot "esconde" sua presença
        strcpy(response, "PID  USER     CMD\n1    root     /sbin/init\n554  www-data apache2\n666  root     [kworker/u4:0] <--- (Malware Hidden)");
    } 
    else {
        strcpy(response, "UNKNOWN COMMAND");
    }
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE];
    char bot_id[64];
    char response[BUF_SIZE];

    if (argc != 3) {
        fprintf(stderr, "Uso: %s <IP_C2> <PORTA>\n", argv[0]);
        return 1;
    }

    generate_bot_id(bot_id, sizeof(bot_id));
    printf("[BOT] Iniciando... ID gerado: %s\n", bot_id);

    // Criação do Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return 1;
    }

    printf("[BOT] Tentando conectar ao C2 em %s:%s...\n", argv[1], argv[2]);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return 1;
    }

    printf("[BOT] Conectado! Enviando Handshake...\n");

    // Protocolo de Handshake esperado pelo C2: "HELLO <ID>"
    snprintf(buffer, sizeof(buffer), "HELLO %s\n", bot_id);
    send(sock, buffer, strlen(buffer), 0);

    // Loop principal de escuta
    while (1) {
        memset(buffer, 0, BUF_SIZE);
        int valread = recv(sock, buffer, BUF_SIZE - 1, 0);
        
        if (valread <= 0) {
            printf("[BOT] Conexão encerrada pelo servidor.\n");
            break;
        }

        // Remove quebra de linha do comando recebido
        buffer[strcspn(buffer, "\r\n")] = 0;
        printf("[BOT] Comando recebido: %s\n", buffer);

        // Executa lógica e prepara resposta
        execute_command(buffer, response);

        printf("[BOT] Enviando resposta...\n");
        send(sock, response, strlen(response), 0);
    }

    close(sock);
    return 0;
}