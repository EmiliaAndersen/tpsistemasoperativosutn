#include "main.h"

int conexion_memoria_cpu;
int hay_interrupcion;
t_log* cpu_log;
int cant_entradas_tlb;
char* algoritmo; 
t_log* log_aux_cpu;


int main(void){

	t_config* cpu_config;   
    char* ip_cpu;
    
    int cpu_dispatch_server;
    int cpu_interrupt_server;
    char* puerto_cpu_dispatch;
    char* puerto_cpu_interrupt;

    char* ip_memoria;       
    char* puerto_memoria; 

    hay_interrupcion = 0;
     
    cpu_log = iniciar_logger("cpu.log","cpu");
    log_aux_cpu = iniciar_logger("cpu_aux.log","cpu_aux");
    cpu_config = iniciar_config("cpu.config");

    // Creo estructura para los hilos
    t_config_cpu *cpu_argumentos = malloc(sizeof(t_config_cpu));

	/* CPU - Cliente */

    // Extraer configs

	ip_memoria = config_get_string_value(cpu_config,"IP_MEMORIA");
	puerto_memoria = config_get_string_value(cpu_config, "PUERTO_MEMORIA");
    algoritmo = config_get_string_value(cpu_config,"ALGORITMO_TLB");
    cant_entradas_tlb = config_get_int_value(cpu_config, "CANTIDAD_ENTRADAS_TLB"); 
    if(cant_entradas_tlb != 0){
        iniciar_tlb();
    }
	
    // Establecer conexiones
    
	conexion_memoria_cpu = crear_conexion(ip_memoria, puerto_memoria);
    log_info(log_aux_cpu, "CPU conectado a MEMORIA");
    send_handshake(conexion_memoria_cpu, cpu_log, "CPU / MEMORIA");
    enviar_pedido_tam_mem(conexion_memoria_cpu);
    recibir_tamanio_pag(conexion_memoria_cpu, cpu_log, &tam_pag);

    // TODO: Hace falta un hilo para atender a memoria... Sino tenemos espera activa

    /* CPU - Server */
    
    //Extraer configs
    cpu_argumentos->config_leida.puerto_cpu_dispatch = config_get_string_value(cpu_config, "PUERTO_ESCUCHA_DISPATCH");
	cpu_argumentos->config_leida.puerto_cpu_interrupt = config_get_string_value(cpu_config, "PUERTO_ESCUCHA_INTERRUPT");

    // Mutex para la flag de interrupciones
    pthread_mutex_t lock_interrupt;
    pthread_mutex_init(&lock_interrupt, NULL);

    // Inicio CPU DISPATCH server  
    pthread_t hilo_conectar_dispatch;
    pthread_create(&hilo_conectar_dispatch, NULL, conectar_dispatch, (void *) cpu_argumentos);
    pthread_join(hilo_conectar_dispatch, NULL);

    // Inicio CPU INTERRUPT server
    pthread_t hilo_conectar_interrupt;
    pthread_create(&hilo_conectar_interrupt, NULL, conectar_interrupt, (void *) cpu_argumentos);
    pthread_join(hilo_conectar_interrupt, NULL);

    // Creo hilo para atender DISPATCH
    t_atender_cliente_args* args_dispatch = malloc(sizeof(t_atender_cliente_args));
    args_dispatch->log = cpu_log;
    args_dispatch->c_socket = cpu_argumentos->socket_dispatch;
    args_dispatch->server_name = "CPU DISPATCH";
    args_dispatch->lock_interrupt = lock_interrupt;
    pthread_t hilo_atender_cpu_dispatch;
    pthread_create(&hilo_atender_cpu_dispatch, NULL, (void *)atender_cliente, (void*) args_dispatch);
    pthread_detach(hilo_atender_cpu_dispatch);

    // Creo hilo para atender INTERUPT       
    t_atender_cliente_args* args_interrupt = malloc(sizeof(t_atender_cliente_args));
    args_interrupt->log = cpu_log;
    args_interrupt->c_socket = cpu_argumentos->socket_interrupt;
    args_interrupt->server_name = "CPU INTERRUPT";
    args_interrupt->lock_interrupt = lock_interrupt;
    pthread_t hilo_atender_cpu_interrupt;
    pthread_create(&hilo_atender_cpu_interrupt, NULL, (void *) atender_cliente, (void*) args_interrupt);
    pthread_join(hilo_atender_cpu_interrupt, NULL);

    //free(args_dispatch);
    //free(args_interrupt);
    free(cpu_argumentos);
    pthread_mutex_destroy(&lock_interrupt);
    log_destroy(log_aux_cpu);

    return 0;
}

void* conectar_dispatch(void* void_args)
{
    t_config_cpu *cpu_argumentos = (t_config_cpu*) void_args;

    int cpu_dispatch_server = iniciar_servidor(cpu_argumentos->config_leida.puerto_cpu_dispatch, cpu_log);
    log_info(log_aux_cpu, "CPU DISPATCH listo para recibir a KERNEL");

    cpu_argumentos->socket_dispatch = esperar_cliente(cpu_dispatch_server, cpu_log);
    log_debug(log_aux_cpu, "Kernel conectado a Dispatch en socket: %d", cpu_argumentos->socket_dispatch);
}

void* conectar_interrupt(void* args)
{
   t_config_cpu *cpu_argumentos = (t_config_cpu*) args;

    int cpu_interrupt_server = iniciar_servidor(cpu_argumentos->config_leida.puerto_cpu_interrupt, cpu_log);
    log_info(log_aux_cpu, "CPU INTERRUPT listo para recibir a KERNEL");

    cpu_argumentos->socket_interrupt = esperar_cliente(cpu_interrupt_server, cpu_log);
    log_debug(log_aux_cpu, "Kernel conectado a Interrupt en socket: %d", cpu_argumentos->socket_interrupt);

}