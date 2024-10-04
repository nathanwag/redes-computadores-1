# ebpf_loader.py
from bcc import BPF
from time import sleep

# Carrega o programa eBPF em C
bpf = BPF(src_file="ebpf.c")
bpf.attach_kprobe(event="sys_open", fn_name="count_open")

print("Contando chamadas open()... Pressione Ctrl+C para parar.")

try:
    while True:
        sleep(1)
        print("\nPID    |   Chamadas open()")
        print("--------------------------")
        
        counts = bpf["counts"]
        for key, value in counts.items():
            print(f"{key.value}    |   {value.value}")
except KeyboardInterrupt:
    print("Encerrando...")

# Limpa o tracepoints e finaliza
bpf.remove_kprobe(event="sys_open")
