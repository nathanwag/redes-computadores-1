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
#define MAX_SERVERS 10

// Lista de IPs dos servidores
const char *enderecosIPs[] = {
    "172.17.0.2",
    "172.17.0.3",
    "172.17.0.4"
};
int numServidores;
int periodicidade = 10; // Tempo padrão de coleta em segundos
char ip_local[INET_ADDRSTRLEN]; // Variável global para armazenar o IP local

// Estrutura para armazenar as métricas de um servidor
typedef struct {
    char ip[INET_ADDRSTRLEN];
    int total_cpu;
    int total_memoria;
    int contagem;
} Metrics;

// Array para armazenar as métricas de cada servidor
Metrics metrics[MAX_SERVERS];

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

// Função para calcular e armazenar as métricas
void armazenar_metricas(const char *metricas) {
    char ip[INET_ADDRSTRLEN];
    int uso_cpu, uso_memoria;

    // Extrair IP, uso de CPU e memória das métricas recebidas
    sscanf(metricas, "IP: %s - CPU: %d%% - Memória: %dMB", ip, &uso_cpu, &uso_memoria);

    for (int i = 0; i < numServidores; i++) {
        if (strcmp(ip, enderecosIPs[i]) == 0) {
            // Atualizar métricas para este servidor
            metrics[i].total_cpu += uso_cpu;
            metrics[i].total_memoria += uso_memoria;
            metrics[i].contagem++;

            return;
        }
    }
}

// Função para exibir a tabela de métricas
void exibir_tabela() {
    printf("\n+-----------------+--------------------+-----------------------+----------+\n");
    printf("| IP              | Média CPU (%)      | Média Memória (MB)    | Contagem |\n");
    printf("+-----------------+--------------------+-----------------------+----------+\n");
    
    for (int i = 0; i < numServidores; i++) {
        if (metrics[i].contagem > 0) {
            printf("| %-15s | %-18.2f | %-21.2f | %-8d |\n", 
                metrics[i].ip,
                (float)metrics[i].total_cpu / metrics[i].contagem,
                (float)metrics[i].total_memoria / metrics[i].contagem,
                metrics[i].contagem);
        }
    }

    printf("+-----------------+--------------------+-----------------------+----------+\n");
    printf("IP local: %s\n", ip_local); // Mostrar IP local
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

    // Inicializar as métricas
    for (int i = 0; i < numServidores; i++) {
        strcpy(metrics[i].ip, enderecosIPs[i]);
        metrics[i].total_cpu = 0;
        metrics[i].total_memoria = 0;
        metrics[i].contagem = 0;
    }

    while (1) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clienteAddr, &addrLen);
        buffer[n] = '\0';

        // Apenas exibir as métricas recebidas
        printf("Métricas recebidas: %s\n", buffer);

        // Armazenar métricas
        armazenar_metricas(buffer);

        // Enviar confirmação
        const char *resposta = "ok";
        sendto(sockfd, resposta, strlen(resposta), 0, (struct sockaddr *)&clienteAddr, addrLen);

        // Exibir tabela após receber as métricas
        exibir_tabela();
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

int main() {
    pthread_t threadServidor, threadCliente;

    // Número de servidores
    numServidores = sizeof(enderecosIPs) / sizeof(enderecosIPs[0]);

    // Criar as threads para servidor e cliente
    pthread_create(&threadServidor, NULL, servidor_udp, NULL);
    pthread_create(&threadCliente, NULL, cliente_udp, NULL);

    // Esperar as threads terminarem
    pthread_join(threadServidor, NULL);
    pthread_join(threadCliente, NULL);

    return 0;
}
