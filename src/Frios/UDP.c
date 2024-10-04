#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024
#define SERVER_PORT 8888

void *servidorUDP(void *arg) {
    int socketServidor;
    struct sockaddr_in enderecoServidor, enderecoCliente;
    socklen_t clienteLen = sizeof(enderecoCliente);
    char buffer[BUFFER_SIZE];

    // Cria o socket do servidor
    socketServidor = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketServidor < 0) {
        perror("Erro ao criar o socket do servidor");
        exit(1);
    }

    memset(&enderecoServidor, 0, sizeof(enderecoServidor));
    enderecoServidor.sin_family = AF_INET;
    enderecoServidor.sin_addr.s_addr = INADDR_ANY;
    enderecoServidor.sin_port = htons(SERVER_PORT);

    // Associa o socket a uma porta
    if (bind(socketServidor, (struct sockaddr *) &enderecoServidor, sizeof(enderecoServidor)) < 0) {
        perror("Erro no bind");
        close(socketServidor);
        exit(1);
    }

    printf("Servidor UDP pronto, escutando na porta %d\n", SERVER_PORT);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        // Recebe dados do cliente
        if (recvfrom(socketServidor, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &enderecoCliente, &clienteLen) < 0) {
            perror("Erro ao receber dados");
            continue;
        }

        printf("\nMensagem recebida de %s: %s\n", inet_ntoa(enderecoCliente.sin_addr), buffer);

        // Envia uma resposta ao cliente (opcional)
        const char *resposta = "ok";
        if (sendto(socketServidor, resposta, strlen(resposta), 0, (struct sockaddr *) &enderecoCliente, clienteLen) < 0) {
            perror("Erro ao enviar resposta");
        }
    }

    close(socketServidor);
    return NULL;
}

void *clienteUDP(void *arg) {
    int socketCliente;
    struct sockaddr_in enderecoServidor;
    char buffer[BUFFER_SIZE];
    char ipServidor[INET_ADDRSTRLEN];

    // Cria o socket do cliente
    socketCliente = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketCliente < 0) {
        perror("Erro ao criar o socket do cliente");
        exit(1);
    }

    printf("Digite o IP do servidor: ");
    scanf("%s", ipServidor);

    memset(&enderecoServidor, 0, sizeof(enderecoServidor));
    enderecoServidor.sin_family = AF_INET;
    enderecoServidor.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, ipServidor, &enderecoServidor.sin_addr);

    while (1) {
        printf("Digite uma mensagem para enviar ao servidor: ");
        scanf(" %[^\n]", buffer); // Lê a mensagem com espaços

        // Envia a mensagem ao servidor
        if (sendto(socketCliente, buffer, strlen(buffer), 0, (struct sockaddr *) &enderecoServidor, sizeof(enderecoServidor)) < 0) {
            perror("Erro ao enviar dados");
            continue;
        }

        // Recebe resposta do servidor (opcional)
        memset(buffer, 0, BUFFER_SIZE);
        if (recvfrom(socketCliente, buffer, BUFFER_SIZE, 0, NULL, NULL) < 0) {
            perror("Erro ao receber resposta");
        } else {
            printf("Resposta do servidor: %s\n", buffer);
        }
    }

    close(socketCliente);
    return NULL;
}

int main() {
    pthread_t threadServidor, threadCliente;

    // Inicia a thread do servidor UDP
    if (pthread_create(&threadServidor, NULL, servidorUDP, NULL) != 0) {
        perror("Erro ao criar a thread do servidor");
        exit(1);
    }

    // Inicia a thread do cliente UDP
    if (pthread_create(&threadCliente, NULL, clienteUDP, NULL) != 0) {
        perror("Erro ao criar a thread do cliente");
        exit(1);
    }

    // Espera as threads finalizarem (nunca acontecerá, pois os loops são infinitos)
    pthread_join(threadServidor, NULL);
    pthread_join(threadCliente, NULL);

    return 0;
}
