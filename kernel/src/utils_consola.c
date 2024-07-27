#include "utils_consola.h"

t_pcb *crear_nuevo_pcb(uint32_t *pid_contador, t_planificacion *kernel_argumentos)
{

    t_pcb *nuevo_pcb = malloc(sizeof(t_pcb));

    if (nuevo_pcb == NULL)
    {
        log_warning(kernel_argumentos->log_aux, "Se creo un PCB NULL\n");
        return NULL;
    }

    inicializar_registro(nuevo_pcb);
    nuevo_pcb->pid = *pid_contador;
    nuevo_pcb->quantum = 0;
    nuevo_pcb->estado = NEW;
    nuevo_pcb->motivo_desalojo = 0;

    // log_info(kernel_argumentos->log_aux, "Se crea el proceso con %d en NEW", nuevo_pcb->pid);
    (*pid_contador)++;
    
    return nuevo_pcb;
}
/*
void inicializar_colas_estados()
{
    colaNew = queue_create();
    colaReady = queue_create();
    colaExec = queue_create();
    colaBlocked = queue_create();
    colaExit = queue_create();
}

void liberar_pcb(t_pcb *pcb)
{
    if (pcb != NULL)
    {
        //free(pcb->registros);
        free(pcb);
    }
}

void mostrar_pids_en_estado(t_proceso_estado estado)
{

    // t_queue *cola = cola_de_estado(estado);
    // int tamanio_cola = queue_size(cola);
    // int contador = 0;

    // if (tamanio_cola != 0)
    // {
    //     //log_info(kernel_log, "Los PID en la COLA %s son:", estado_a_string(estado));

    //     while (contador < tamanio_cola)
    //     {

    //         t_pcb *pcb_apunta = queue_pop(cola);
    //         uint32_t pid = pcb_apunta->pid;
    //         queue_push(cola, pcb_apunta);
    //         //log_info(kernel_log, "PID: %u", pid);

    //         contador++;
    //     }
    // }
    // else
    // {
    //     //log_warning(kernel_log, "La COLA %s esta VACIA", estado_a_string(estado));
    // }
}
/*
t_queue* cola_pcb(uint32_t num_pid){ //buscar cola en cada estado

    // t_queue* buscado = NULL;
    // for (t_proceso_estado estado = NEW; estado <= EXIT; estado++)
    // {
    //     if(!queue_is_empty(cola_de_estado(estado))){
    //         if (find_queue(num_pid, cola) == 1  )
    //         {
    //         buscado = cola;
    //         }
    //     }
        
    // }
    // if(buscado == NULL){
    //     //log_error(kernel_log, "No se pudo encontrar el PCB de PID: %u", num_pid);
    // }
    // return buscado;
}

void borrar_pcb(uint32_t num_pid)
{
    // t_queue* cola = cola_pcb(num_pid);
    // int tamanio_cola = queue_size(cola);
    // int contador = 0;

    // while (contador < tamanio_cola)
    //     {

    //         t_pcb *pcb_apunta = queue_pop(cola);

    //         if (pcb_apunta->pid == num_pid)
    //         {
    //             // No lo vuelve a encolar 
    //             // Ver liberar memoria, recursos, y archivos
    //         }
    //         else
    //         {
    //             queue_push(cola, pcb_apunta);
    //         }
    //         contador++;
    //     }

    // //log_info(kernel_log, "Borre PCB");
}

t_pcb* buscar_pcb(uint32_t num_pid){

    // t_queue* cola = cola_pcb(num_pid);
    // int tamanio_cola = queue_size(cola);
    // int contador = 0;

    // t_pcb* buscado;

    // while (contador < tamanio_cola)
    //     {
    //         t_pcb *pcb_apunta = queue_pop(cola);

    //         if (pcb_apunta->pid == num_pid)
    //         {
    //             buscado = pcb_apunta;
    //         }
    //         queue_push(cola, pcb_apunta);
    //         contador++;
    //     }
    // return buscado;
}


void cambiar_a_cola(t_pcb* pcb, t_proceso_estado estado ){
    // t_proceso_estado ant = pcb->estado;
    // //t_queue* colaAnt = cola_de_estado(pcb->estado);
    // //queue_pop(colaAnt);
    // pcb->estado = estado;
    // t_queue* colaNueva = cola_de_estado(estado);
    // queue_push(colaNueva, pcb);
    // // log_info(kernel_log, "PID: <%u> - Estado Anterior: <%s> - Estado Actual: <%s>", pcb->pid, estado_a_string(ant), estado_a_string(pcb->estado));
}


// t_queue *cola_de_estado(t_planificacion *kernel_argumentos, t_proceso_estado estado)
// {

//     // switch (estado)
//     // {
//     // case 0:
//     //     return colaNew;
//     // case 1:
//     //     return colaReady;
//     // case 2:
//     //     return colaExec;
//     // case 3:
//     //     return colaBlocked;
//     // case 4:
//     //     return colaExit;
//     // default:
//     //     return 0;
// }


t_queue *cola_de_estado(t_proceso_estado estado)
{

    switch (estado)
    {
   case 0:
      return colaNew;
     case 1:
   return colaReady;
    case 2:
       return colaExec;
     case 3:
        return colaBlocked;
     case 4:
         return colaExit;
     default:
         return 0;
}
*/

