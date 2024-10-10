#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ifaddrs.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>
#include <stdio.h>

#define PORT 8888
#define BUFFER_SIZE 1024
#define MAX_SERVERS 10
#define MAX_IPS 100 
char enderecosIPs[MAX_IPS][INET_ADDRSTRLEN];
int numServidores = 0; 
int periodicidade = 10;
char ip_local[INET_ADDRSTRLEN];
float uso_cpu;
long uso_memoria;
float latencia;

typedef struct {
    char ip[INET_ADDRSTRLEN];
    float total_cpu;
    int total_memoria;
    float total_latencia;
    int contagem;
} Metrics;

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

void receber_enderecos_ips() {
    char input[BUFFER_SIZE * 10];  // Buffer grande para suportar muitos IPs
    printf("Digite os endereços IPs, separados por espaço: ");
    fgets(input, sizeof(input), stdin);  // Recebe os IPs como uma string

    // Usar strtok para separar os IPs
    char *token = strtok(input, " ");
    numServidores = 0;

    // Processar cada IP fornecido
    while (token != NULL && numServidores < MAX_IPS) {
        strcpy(enderecosIPs[numServidores], token);  // Copia o IP para o array
        enderecosIPs[numServidores][strcspn(enderecosIPs[numServidores], "\n")] = '\0'; // Remove o newline, se houver
        numServidores++;
        token = strtok(NULL, " ");
    }
}

float calcular_uso_cpu() {
    FILE *fp;
    char buffer[1024];
    unsigned long long int user, nice, system, idle, iowait, irq, softirq, steal;
    static unsigned long long int prev_idle = 0, prev_total = 0;
    unsigned long long int total_idle, total;

    // Abrir o arquivo /proc/stat para obter dados da CPU
    fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        perror("Erro ao abrir /proc/stat");
        return -1.0f;
    }

    // Ler os valores da linha "cpu" no /proc/stat
    fgets(buffer, sizeof(buffer), fp);
    sscanf(buffer, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", 
           &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    fclose(fp);

    // Calculando o tempo total e idle
    total_idle = idle + iowait;
    total = user + nice + system + idle + iowait + irq + softirq + steal;

    // Evitar inconsistências na primeira leitura
    if (prev_total == 0) {
        prev_total = total;
        prev_idle = total_idle;
        return 0.0f;
    }

    // Calcular a diferença em relação à leitura anterior
    unsigned long long int delta_total = total - prev_total;
    unsigned long long int delta_idle = total_idle - prev_idle;

    // Atualizar valores anteriores
    prev_total = total;
    prev_idle = total_idle;

    // Calcular o uso de CPU (percentual de tempo gasto processando)
    return (delta_total - delta_idle) / (float)delta_total * 100.0f;
}

float calcular_latencia(const char *ip_maquina) {
    char comando[BUFFER_SIZE];
    FILE *fp;
    float latencia = 0.0;

    // Executa o comando ping e coleta a latência média
    snprintf(comando, sizeof(comando), "ping -c 4 %s | tail -1 | awk '{print $4}' | cut -d '/' -f 2", ip_maquina);
    fp = popen(comando, "r");
    if (fp == NULL) {
        perror("Erro ao executar comando ping");
        return -1.0f;
    }

    if (fscanf(fp, "%f", &latencia) != 1) {
        perror("Erro ao ler latência");
        latencia = -1.0f;
    }
    pclose(fp);

    return latencia;
}

long calcular_uso_memoria() {
    struct sysinfo sys_info;

    if (sysinfo(&sys_info) != 0) {
        perror("Erro ao obter informações de memória");
        return -1;
    }

    long total_memoria = sys_info.totalram / (1024 * 1024);
    long memoria_livre = sys_info.freeram / (1024 * 1024);
    long uso_memoria = total_memoria - memoria_livre;

    return uso_memoria;
}

void coletar_metricas(char *metricas, const char *ip_maquina) {
    uso_cpu = calcular_uso_cpu();
    uso_memoria = calcular_uso_memoria();
    float latencia = calcular_latencia(ip_maquina);

    snprintf(metricas, BUFFER_SIZE, "IP: %s - CPU: %.2f%% - Memória: %ldMB - Latência: %.2fms\n", ip_maquina, uso_cpu, uso_memoria, latencia);
}


// Função para calcular e armazenar as métricas
void armazenar_metricas(const char *metricas) {
    char ip[INET_ADDRSTRLEN];
    float cpu = 0.0;
    long memoria = 0;
    float latencia = 0.0;

    // Extrair IP, uso de CPU e memória das métricas recebidas
    sscanf(metricas, "IP: %s - CPU: %f%% - Memória: %ldMB - Latência: %fms", ip, &cpu, &memoria, &latencia);

    for (int i = 0; i < numServidores; i++) {
        if (strcmp(ip, enderecosIPs[i]) == 0) {
            metrics[i].total_cpu += cpu;
            metrics[i].total_latencia += latencia;
            metrics[i].total_memoria += memoria;
            metrics[i].contagem++;

            return;
        }
    }
}


void exibir_tabela() {
    printf("\n+-----------------+--------------------+-----------------------+--------------------+----------+\n");
    printf("| IP              | Média CPU (%)      | Média Memória (MB)    | Média Latência (ms)| Contagem |\n");
    printf("+-----------------+--------------------+-----------------------+-------------------------------+\n");
    
    for (int i = 0; i < numServidores; i++) {
        if (metrics[i].contagem > 0) {
            printf("| %-15s | %-18.2f | %-21.2f | %-17.2f  | %-8d |\n",
                metrics[i].ip,
                (metrics[i].contagem > 0) ? (float)metrics[i].total_cpu / metrics[i].contagem : 0,
                (metrics[i].contagem > 0) ? (float)metrics[i].total_memoria / metrics[i].contagem : 0,
                (metrics[i].contagem > 0) ? (float)metrics[i].total_latencia / metrics[i].contagem : 0,
                metrics[i].contagem);
        }
    }

    printf("+-----------------+--------------------+-----------------------+-----------------+-------------+\n");
    printf("IP local: %s\n", ip_local);
}

void *ajustar_periodicidade(void *arg) {
    while (1) {
        printf("Digite o novo valor de periodicidade em segundos (0 para sair): ");
        int novo_periodo;
        scanf("%d", &novo_periodo);
        if (novo_periodo == 0) {
            printf("Saindo do ajuste de periodicidade.\n");
            break;
        }
        periodicidade = novo_periodo;
        printf("Periodicidade atualizada para %d segundos.\n", periodicidade);
    }
    return NULL;
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
        metrics[i].total_latencia = 0.0;
        metrics[i].contagem = 0;
    }

    while (1) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clienteAddr, &addrLen);
        buffer[n] = '\0';

        printf("Métricas recebidas: %s\n", buffer);

        armazenar_metricas(buffer);

        const char *resposta = "ok";
        sendto(sockfd, resposta, strlen(resposta), 0, (struct sockaddr *)&clienteAddr, addrLen);

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

        coletar_metricas(metricas, ip_local);
        printf("Enviando métricas: %s\n", metricas);

        // Enviar métricas para cada servidor
        for (int i = 0; i < numServidores; i++) {
            inet_pton(AF_INET, enderecosIPs[i], &servidorAddr.sin_addr);
            sendto(sockfd, metricas, strlen(metricas), 0, (const struct sockaddr *)&servidorAddr, sizeof(servidorAddr));
        }

        sleep(periodicidade);
    }

    close(sockfd);
    return NULL;
}

int main() {
    receber_enderecos_ips();

    // Criar thread para o servidor UDP
    pthread_t thread_servidor;
    if (pthread_create(&thread_servidor, NULL, servidor_udp, NULL) != 0) {
        perror("Erro ao criar thread do servidor");
        return EXIT_FAILURE;
    }

    // Criar thread para o cliente UDP
    pthread_t thread_cliente;
    if (pthread_create(&thread_cliente, NULL, cliente_udp, NULL) != 0) {
        perror("Erro ao criar thread do cliente");
        return EXIT_FAILURE;
    }
    // Criar thread para ajustar a periodicidade
    pthread_t thread_periodicidade;
    if (pthread_create(&thread_periodicidade, NULL, ajustar_periodicidade, NULL) != 0) {
        perror("Erro ao criar thread de ajuste de periodicidade");
        return EXIT_FAILURE;
    }
    
    // Aguardar as threads de cliente e servidor
    pthread_join(thread_cliente, NULL);
    pthread_join(thread_servidor, NULL);
    pthread_join(thread_periodicidade, NULL);

    return 0;
}