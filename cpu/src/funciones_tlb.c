#include "funciones_tlb.h"

TLB* tlb;

void iniciar_tlb() { 
    tlb = malloc(sizeof(TLB));
    tlb->entradas = (Entrada*)malloc(cant_entradas_tlb * sizeof(Entrada));
    for (int i = 0; i < cant_entradas_tlb; i++) {
        tlb->entradas[i].validez = 0;  
    }
    tlb->siguiente_fifo = 0;
    tlb->contador_lru = 0;
}

int buscar_tlb(uint32_t pid, int num_pag){
  for (int i = 0; i < cant_entradas_tlb; i++) {
        if (tlb->entradas[i].validez && tlb->entradas[i].pid == pid && tlb->entradas[i].pagina == num_pag) {
            tlb->entradas[i].ultimo_acceso = tlb->contador_lru++; 
            return tlb->entradas[i].frame; //HIT
        }
    }
    return -1;  //MISS
}

t_dir_fisica* mmu(int dir_logica, uint32_t pid){

    int numero_pagina = floor(dir_logica / tam_pag);
    int desplazamiento = dir_logica - numero_pagina * tam_pag;
    int frame;
    if (cant_entradas_tlb == 0){
        frame = manejar_tlb_miss(pid, numero_pagina);
    }else {
        frame = buscar_tlb(pid, numero_pagina);
        if (frame != -1) {  // HIT
            manejar_tlb_hit(pid, numero_pagina);
        } else {  // MISS
            log_info(cpu_log, "PID: <%u> - TLB MISS - Pagina: <%d>", pid, numero_pagina);
            frame = manejar_tlb_miss(pid, numero_pagina);
            if (string_equals_ignore_case(algoritmo, "FIFO")) {
                remplazo_fifo(pid, numero_pagina, frame);
            } else if (string_equals_ignore_case(algoritmo, "LRU")) {
                remplazo_lru(pid, numero_pagina, frame);
            }
        }
    }
    
   t_dir_fisica* direccionFisica = malloc(sizeof(t_dir_fisica));
   direccionFisica->nro_frame = frame;
   direccionFisica->desplazamiento = desplazamiento;
   return direccionFisica;
}

int manejar_tlb_miss(uint32_t pid, int numero_pagina) {
    char* mensaje = malloc(sizeof(pid) + sizeof(numero_pagina));
    sprintf(mensaje, "%u/%d", pid, numero_pagina);
    enviar_a_mem(conexion_memoria_cpu, mensaje, FRAME);

    int frame = recibir_frame(conexion_memoria_cpu);
    log_info(cpu_log, "PID: <%u> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, numero_pagina, frame);
    free(mensaje);
    return frame;
}

void manejar_tlb_hit(uint32_t pid, int numero_pagina) {
    log_info(cpu_log, "PID: <%u> - TLB HIT - Pagina: <%d>", pid, numero_pagina);
}


void remplazo_fifo(uint32_t pid, int pagina, int frame){
    tlb->entradas[tlb->siguiente_fifo].pid = pid;
    tlb->entradas[tlb->siguiente_fifo].pagina = pagina;
    tlb->entradas[tlb->siguiente_fifo].frame = frame;
    tlb->entradas[tlb->siguiente_fifo].validez = 1; 
    tlb->siguiente_fifo = (tlb->siguiente_fifo + 1) % cant_entradas_tlb;
}

void remplazo_lru(uint32_t pid, int pagina, int frame){
    int posicion_tlb = -1;
    int contador = tlb->contador_lru;
    for(int i=0; i < cant_entradas_tlb; i++){
        if(tlb->entradas[i].validez == 0){
            posicion_tlb = i;
            break;
        }
        if(tlb->entradas[i].ultimo_acceso < contador){
            posicion_tlb = i;
            contador = tlb->entradas[i].ultimo_acceso;
        }
    }
    tlb->entradas[posicion_tlb].pid = pid;
    tlb->entradas[posicion_tlb].pagina = pagina;
    tlb->entradas[posicion_tlb].frame = frame;
    tlb->entradas[posicion_tlb].ultimo_acceso = tlb->contador_lru++;
    tlb->entradas[posicion_tlb].validez = 1;
}