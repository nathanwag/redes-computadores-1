import socket
import threading
import subprocess
import time

def servidor_udp(portaServidor=8888):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as socketServidor:
        socketServidor.bind(('', portaServidor))
        print(f"Servidor UDP pronto, escutando na porta {portaServidor}")
        while True:
            data, enderecoCliente = socketServidor.recvfrom(1024)
            mensagemRecebida = data.decode()
            print(f"\n{enderecoCliente[0]}: {mensagemRecebida}")
            resposta = "ok".encode()
            socketServidor.sendto(resposta, enderecoCliente)

def cliente_udp(portaServidor=8888):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as socketCliente:
        while True:
            mensagem = input()
            sendData = mensagem.encode()
            enderecoServidor = input("Digite o IP do servidor: ")
            start = time.time()  # Tempo antes do envio
            socketCliente.sendto(sendData, (enderecoServidor, portaServidor))
            data, _ = socketCliente.recvfrom(1024)
            end = time.time()  # Tempo depois do recebimento
            print(f"Latência: {end - start} segundos")

def iniciar_monitoramento_ebpf():
    # Inicia o eBPF loader e o programa de análise
    print("Iniciando monitoramento com eBPF...")
    subprocess.Popen(['python3', 'ebpf_loader.py'])  # Executa o script ebpf_loader.py

# Inicia o servidor em uma thread separada
threading.Thread(target=servidor_udp).start()

# Inicia o monitoramento de eBPF em segundo plano
iniciar_monitoramento_ebpf()

# Inicia o cliente em uma thread separada
threading.Thread(target=cliente_udp).start()