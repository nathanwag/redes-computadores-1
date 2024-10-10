#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ifaddrs.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>

#define PORT 8888
#define BUFFER_SIZE 1024
#define MAX_SERVERS 10
#define MAX_IPS 100 
char ipAddresses[MAX_IPS][INET_ADDRSTRLEN];
int numServers = 0; 
int periodicity = 10;
char local_ip[INET_ADDRSTRLEN];
float cpu_usage;
long memory_usage;
float latency;

typedef struct {
    char ip[INET_ADDRSTRLEN];
    float total_cpu;
    int total_memory;
    float total_latency;
    int count;
} Metrics;

Metrics metrics[MAX_SERVERS];

// Função para obter o IP local da máquina
void get_local_ip(char *local_ip) {
    struct ifaddrs *ifaddr, *ifa;
    void *tmpAddrPtr = NULL;

    if (getifaddrs(&ifaddr) == -1) {
        perror("Erro ao obter endereços de rede");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, local_ip, INET_ADDRSTRLEN);

            // Ignorar interfaces de loopback (127.0.0.1)
            if (strcmp(local_ip, "127.0.0.1") != 0) {
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
}

void receive_ip_addresses() {
    char input[BUFFER_SIZE];
    printf("Digite os endereços IPs, separados por espaço: ");
    fgets(input, sizeof(input), stdin);

    char *token = strtok(input, " ");
    numServers = 0;

    while (token != NULL && numServers < MAX_IPS) {
        strcpy(ipAddresses[numServers], token);
        ipAddresses[numServers][strcspn(ipAddresses[numServers], "\n")] = '\0';
        numServers++;
        token = strtok(NULL, " ");
    }
}

float calculate_cpu_usage() {
    FILE *fp;
    char buffer[1024];
    unsigned long long int user, nice, system, idle, iowait, irq, softirq, steal;
    static unsigned long long int prev_idle = 0, prev_total = 0;
    unsigned long long int total_idle, total;

    fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        perror("Erro ao abrir /proc/stat");
        return -1.0f;
    }

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

float calculate_latency(const char *machine_ip) {
    char command[BUFFER_SIZE];
    FILE *fp;
    float latency = 0.0;

    // Executa o comando ping e coleta a latência média
    snprintf(command, sizeof(command), "ping -c 4 %s | tail -1 | awk '{print $4}' | cut -d '/' -f 2", machine_ip);
    fp = popen(command, "r");
    if (fp == NULL) {
        perror("Erro ao executar comando ping");
        return -1.0f;
    }

    if (fscanf(fp, "%f", &latency) != 1) {
        perror("Erro ao tentar coletar latência");
        latency = -1.0f;
    }
    pclose(fp);

    return latency;
}

long calculate_memory_usage() {
    struct sysinfo sys_info;

    if (sysinfo(&sys_info) != 0) {
        perror("Erro ao obter métricas de memória");
        return -1;
    }

    long total_memory = sys_info.totalram / (1024 * 1024);
    long free_memory = sys_info.freeram / (1024 * 1024);
    long memory_usage = total_memory - free_memory;

    return memory_usage;
}

void collect_metrics(char *metrics, const char *machine_ip) {
    cpu_usage = calculate_cpu_usage();
    memory_usage = calculate_memory_usage();
    float latency = calculate_latency(machine_ip);

    snprintf(metrics, BUFFER_SIZE, "IP: %s - CPU: %.2f%% - Memória: %ldMB - Latência: %.2fms\n", machine_ip, cpu_usage, memory_usage, latency);
}

void store_metrics(const char *buffer) {
    char ip[INET_ADDRSTRLEN];
    float cpu = 0.0;
    long memory = 0;
    float latency = 0.0;

    // Extrair IP, uso de CPU, memória e latência do buffer recebido
    sscanf(buffer, "IP: %s - CPU: %f%% - Memória: %ldMB - Latência: %fms", ip, &cpu, &memory, &latency);

    // Percorre os servidores armazenados para encontrar o IP correspondente
    for (int i = 0; i < numServers; i++) {
        if (strcmp(ip, ipAddresses[i]) == 0) {
            // Atualiza os valores nas métricas globais
            metrics[i].total_cpu += cpu;
            metrics[i].total_latency += latency;
            metrics[i].total_memory += memory;
            metrics[i].count++; // Incrementa o número de registros

            return;
        }
    }
}

void display_table() {
    printf("\n+-----------------+--------------------+-----------------------+--------------------+----------+\n");
    printf("| IP              | Média CPU (%)      | Média Memória (MB)    | Média Latência (ms)| Contagem |\n");
    printf("+-----------------+--------------------+-----------------------+-------------------------------+\n");
    
    for (int i = 0; i < numServers; i++) {
        if (metrics[i].count > 0) {
            printf("| %-15s | %-18.2f | %-21.2f | %-17.2f  | %-8d |\n",
                metrics[i].ip,
                (metrics[i].count > 0) ? (float)metrics[i].total_cpu / metrics[i].count : 0,
                (metrics[i].count > 0) ? (float)metrics[i].total_memory / metrics[i].count : 0,
                (metrics[i].count > 0) ? (float)metrics[i].total_latency / metrics[i].count : 0,
                metrics[i].count);
        }
    }

    printf("+-----------------+--------------------+-----------------------+-----------------+-------------+\n");
    printf("IP local: %s\n", local_ip);
}

void *adjust_periodicity(void *arg) {
    while (1) {
        printf("Digite o novo valor de periodicidade em segundos (0 para sair): ");
        int new_period;
        scanf("%d", &new_period);
        if (new_period == 0) {
            printf("Saindo do ajuste de periodicidade.\n");
            break;
        }
        periodicity = new_period;
        printf("Periodicidade atualizada para %d segundos.\n", periodicity);
    }
    return NULL;
}

// Função para o servidor UDP (para receber mensagens e métricas)
void *udp_server(void *arg) {
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[BUFFER_SIZE];
    socklen_t addrLen = sizeof(clientAddr);

    // Criação do socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configuração do servidor
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind
    if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    printf("Servidor UDP pronto, escutando na porta %d\n", PORT);

    // Inicializar as métricas
    for (int i = 0; i < numServers; i++) {
        strcpy(metrics[i].ip, ipAddresses[i]);
        metrics[i].total_cpu = 0;
        metrics[i].total_memory = 0;
        metrics[i].total_latency = 0.0;
        metrics[i].count = 0;
    }

    // Receber dados
    while (1) {
        int bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &addrLen);
        buffer[bytes_received] = '\0';

        printf("Métricas recebidas: %s\n", buffer);

        store_metrics(buffer);

        const char *response = "ok";
        sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)&clientAddr, addrLen);

        display_table();
    }

    close(sockfd);
    return NULL;
}

// Função para o cliente UDP (para enviar métricas periodicamente)
void *udp_client(void *arg) {
    int sockfd;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];
    socklen_t addrLen = sizeof(serverAddr);
    // Obter o IP local da máquina
    get_local_ip(local_ip);
    printf("IP local: %s\n", local_ip);

    // Criação do socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configuração do cliente
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    while (1) {
        char metrics[BUFFER_SIZE] = "";

        // Coletar métricas simuladas
        collect_metrics(metrics, local_ip);
        printf("Enviando métricas: %s\n", metrics);

        // Enviar métricas para cada servidor
        for (int i = 0; i < numServers; i++) {
            inet_pton(AF_INET, ipAddresses[i], &serverAddr.sin_addr);
            sendto(sockfd, metrics, strlen(metrics), 0, (const struct sockaddr *)&serverAddr, sizeof(serverAddr));
        }

        sleep(periodicity);
    }

    close(sockfd);
    return NULL;
}

int main() {
    //numServidores = sizeof(enderecosIPs) / sizeof(enderecosIPs[0]);
    receive_ip_addresses();
    // Obter o IP local da máquina
    //obter_ip_local(ip_local);
    //printf("IP local: %s\n", ip_local);

    // Criar thread para o servidor UDP
    pthread_t server_thread;
    if (pthread_create(&server_thread, NULL, udp_server, NULL) != 0) {
        perror("Erro ao criar thread do servidor");
        return EXIT_FAILURE;
    }

    // Criar thread para o cliente UDP
    pthread_t client_thread;
    if (pthread_create(&client_thread, NULL, udp_client, NULL) != 0) {
        perror("Erro ao criar thread do cliente");
        return EXIT_FAILURE;
    }
    // Criar thread para ajustar a periodicidade
    pthread_t periodicity_thread;
    if (pthread_create(&periodicity_thread, NULL, adjust_periodicity, NULL) != 0) {
        perror("Erro ao criar thread de ajuste de periodicidade");
        return EXIT_FAILURE;
    }
    
    // Aguardar as threads de cliente e servidor
    pthread_join(client_thread, NULL);
    pthread_join(server_thread, NULL);
    pthread_join(periodicity_thread, NULL);

    return 0;
}