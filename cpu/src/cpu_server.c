#include "cpu_server.h"

int kernel_socket;
int tam_pag;

void atender_cliente(void *void_args)
{
    t_atender_cliente_args *args = (t_atender_cliente_args *)void_args;

    t_log *logger = args->log;
    kernel_socket = args->c_socket;
    char *server_name = args->server_name;
    
    //free(args);
    uint32_t pid_actual;
    //MEMORIA a CPU
    while (kernel_socket != -1)
    {   
        op_code cop = recibir_operacion(args->c_socket);
        log_debug(log_aux_cpu, "[%s] Paquete recibido con opcode %d en: %d", server_name, cop, args->c_socket);

        if (cop == -1)
        {
            log_warning(log_aux_cpu, "DISCONNECT!");
            return;
        }

        switch (cop) 
        {
        case MENSAJE:{}
        case PAQUETE:{}
        case PCB:
        {
           t_pcb* pcb;
        
            pcb = recibir_pcb(args->c_socket);  
            pid_actual = pcb->pid;                   
            realizar_ciclo_inst(conexion_memoria_cpu, pcb, logger, args->c_socket, args->lock_interrupt);
          
			break;
        }
        case KERNEL_CPU_INTERRUPT:
        {
            uint32_t pid = recibir_int_a_interrupt(args->c_socket);

            pthread_mutex_lock(&args->lock_interrupt);
            log_debug(log_aux_cpu, "Se recibe un pedido de interrucpcion para el PID: %d", pid);
            hay_interrupcion = 1;
            pthread_mutex_unlock(&args->lock_interrupt);

            break;
        }
        default:
        {
            log_error(log_aux_cpu, "Algo anduvo mal en el server de %s", server_name);
            
            break;
        }
        }
    }

    log_warning(log_aux_cpu, "El cliente se desconecto de %s server", server_name);
    free(args);
    return;
}

int server_escuchar(void* arg)
{   
    t_atender_cliente_args*args = (t_atender_cliente_args*)arg;
    t_log *logger = args->log;
    char *server_name = args->server_name;
    int server_socket = args->c_socket;
    while (1){
        
        int client_socket = esperar_cliente(server_socket, logger);

        if (client_socket != -1) // != error
        {
            pthread_t hilo;
            args->log = logger;
            args->c_socket = client_socket;
            args->server_name = server_name;
 
            pthread_create(&hilo, NULL, (void *)atender_cliente,  (void*) args); //castear, lo convierto arg a tipo void*
            pthread_detach(hilo); //  hilo se ejecuta de manera independiente, el recurso del hilo se libera automáticamente 
        }
    }
    return 0;
}

void recibir_interrupcion_finq(int socket_cliente){
    /*int size;
    void* buffer;
    recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
    buffer = malloc(*size);
    recv(socket_cliente, buffer, *size, MSG_WAITALL);*/
    int size;
    uint32_t* buffer = (uint32_t*)recibir_buffer(&size, socket_cliente);
    //log_info(logger, "Me llego la Interrupcion %u", buffer);

}

void enviar_motivo(op_code motivo, int kernel_socket){

    t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = motivo;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(op_code);
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, &(motivo), paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

    int resultado_send = send(kernel_socket, a_enviar, bytes, MSG_NOSIGNAL);

    if (resultado_send == -1) {
        fprintf(stderr, "Error al enviar el mensaje: socket cerrado.\n");
    }
    free(a_enviar);
    eliminar_paquete(paquete);
}