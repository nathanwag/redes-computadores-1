#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ifaddrs.h>

// Definições
#define PORT 8888
#define BUFFER_SIZE 1024

// Lista de IPs dos servidores
const char *enderecosIPs[] = {
    "172.17.0.2",
    "172.17.0.3",
    "172.17.0.4"
};
int numServidores;
int periodicidade = 10; // Tempo padrão de coleta em segundos

// Função para obter o IP local da máquina
void obter_ip_local(char *ip_local) {
    struct ifaddrs *ifaddr, *ifa;
    void *tmpAddrPtr = NULL;

    // Obter lista de interfaces de rede
    if (getifaddrs(&ifaddr) == -1) {
        perror("Erro ao obter endereços de rede");
        exit(EXIT_FAILURE);
    }

    // Percorrer interfaces e pegar o endereço da primeira interface ativa
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        // Apenas considerar interfaces de família AF_INET (IPv4)
        if (ifa->ifa_addr->sa_family == AF_INET) {
            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, ip_local, INET_ADDRSTRLEN);

            // Ignorar interfaces de loopback (127.0.0.1)
            if (strcmp(ip_local, "127.0.0.1") != 0) {
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
}

// Função para coletar métricas (simulada, sem eBPF)
void coletar_metricas(char *metricas, const char *ip_maquina) {
    // Simulação de métricas de uso de CPU e memória
    int uso_cpu = rand() % 100;
    int uso_memoria = rand() % 16000;

    snprintf(metricas, BUFFER_SIZE, "IP: %s - CPU: %d%% - Memória: %dMB\n", ip_maquina, uso_cpu, uso_memoria);
}

// Função para o servidor UDP (para receber mensagens e métricas)
void *servidor_udp(void *arg) {
    int sockfd;
    struct sockaddr_in servidorAddr, clienteAddr;
    char buffer[BUFFER_SIZE];
    socklen_t addrLen = sizeof(clienteAddr);

    // Criação do socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configuração do servidor
    memset(&servidorAddr, 0, sizeof(servidorAddr));
    servidorAddr.sin_family = AF_INET;
    servidorAddr.sin_addr.s_addr = INADDR_ANY;
    servidorAddr.sin_port = htons(PORT);

    // Vincular o socket
    if (bind(sockfd, (const struct sockaddr *)&servidorAddr, sizeof(servidorAddr)) < 0) {
        perror("Erro ao vincular socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor UDP pronto, escutando na porta %d\n", PORT);

    while (1) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clienteAddr, &addrLen);
        buffer[n] = '\0';

        // Apenas exibir as métricas recebidas
        printf("Métricas recebidas: %s\n", buffer);

        // Verificar se é um comando para alterar periodicidade
        if (strncmp(buffer, "periodicidade:", 14) == 0) {
            int nova_periodicidade = atoi(buffer + 14);
            periodicidade = nova_periodicidade > 0 ? nova_periodicidade : periodicidade;
            printf("Periodicidade alterada para %d segundos\n", periodicidade);
        }

        // Enviar confirmação
        const char *resposta = "ok";
        sendto(sockfd, resposta, strlen(resposta), 0, (struct sockaddr *)&clienteAddr, addrLen);
    }

    close(sockfd);
    return NULL;
}

// Função para o cliente UDP (para enviar métricas periodicamente)
void *cliente_udp(void *arg) {
    int sockfd;
    struct sockaddr_in servidorAddr;
    char buffer[BUFFER_SIZE];
    socklen_t addrLen = sizeof(servidorAddr);
    char ip_local[INET_ADDRSTRLEN];

    // Obter o IP local da máquina
    obter_ip_local(ip_local);
    printf("IP local: %s\n", ip_local);

    // Criação do socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configuração do cliente
    memset(&servidorAddr, 0, sizeof(servidorAddr));
    servidorAddr.sin_family = AF_INET;
    servidorAddr.sin_port = htons(PORT);

    while (1) {
        char metricas[BUFFER_SIZE] = "";

        // Coletar métricas simuladas
        coletar_metricas(metricas, ip_local);
        printf("Enviando métricas: %s\n", metricas);

        // Enviar métricas para cada servidor, exceto para o próprio IP
        for (int i = 0; i < numServidores; i++) {
            if (strcmp(ip_local, enderecosIPs[i]) != 0) {  // Validação para evitar loopback
                inet_pton(AF_INET, enderecosIPs[i], &servidorAddr.sin_addr);
                sendto(sockfd, metricas, strlen(metricas), 0, (const struct sockaddr *)&servidorAddr, sizeof(servidorAddr));
            }
        }

        sleep(periodicidade);
    }

    close(sockfd);
    return NULL;
}

// Função para exibir o menu
void mostrar_menu() {
    printf("Menu de Configuração\n");
    printf("1. Definir Periodicidade das Coletas de Métricas\n");
    printf("2. Selecionar Máquinas Envolvidas\n");
    printf("3. Iniciar Coleta de Métricas\n");
    printf("4. Sair\n");
}

int main() {
    pthread_t threadServidor, threadCliente;
    int opcao;

    // Número de servidores
    numServidores = sizeof(enderecosIPs) / sizeof(enderecosIPs[0]);

    while (1) {
        mostrar_menu();
        printf("Escolha uma opção: ");
        scanf("%d", &opcao);

        switch (opcao) {
            case 1:
                printf("Digite a nova periodicidade (segundos): ");
                scanf("%d", &periodicidade);
                break;
            case 2:
                printf("Máquinas disponíveis:\n");
                for (int i = 0; i < numServidores; i++) {
                    printf("%d. %s\n", i + 1, enderecosIPs[i]);
                }
                // Implementar lógica para selecionar máquinas
                break;
            case 3:
                // Criar as threads do servidor e do cliente
                pthread_create(&threadServidor, NULL, servidor_udp, NULL);
                pthread_create(&threadCliente, NULL, cliente_udp, NULL);
                pthread_join(threadServidor, NULL);
                pthread_join(threadCliente, NULL);
                break;
            case 4:
                printf("Saindo...\n");
                exit(EXIT_SUCCESS);
            default:
                printf("Opção inválida.\n");
        }
    }

    return 0;
}
