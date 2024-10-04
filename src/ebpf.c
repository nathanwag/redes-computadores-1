#include <uapi/linux/ptrace.h>
#include <uapi/linux/bpf.h>
#include <linux/sched.h>

// Definimos um hash map para contar o número de chamadas de open por PID
BPF_HASH(counts, u32);

int count_open(struct pt_regs *ctx) {
    u32 pid = bpf_get_current_pid_tgid() >> 32; // Obtém o PID do processo atual
    u64 *count, zero = 0;
    
    // Verifica se o PID já existe no mapa, se não, inicializa com zero
    count = counts.lookup_or_init(&pid, &zero);
    (*count)++; // Incrementa a contagem de chamadas open para esse PID
    
    return 0; // Sempre retorna 0 no eBPF
}
