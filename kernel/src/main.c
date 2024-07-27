#include "main.h"

t_log* kernel_log;
t_log* log_aux;
t_config* kernel_config;

t_queue* colaNew;
t_queue* colaReady;
t_queue* colaExec;
t_queue* colaBlocked;
t_queue* colaExit;

t_list* interfaces;
sem_t sem_contador_int;
sem_t grado_planificiacion;
sem_t cola_ready; //ESTE ES PARA VER SI HAY COSAS EN READY PARA EJECUTAR
sem_t habilitacion_io;
int quantum;


int nivel_multiprog;

int conexion_memoria;
int conexion_cpu_dispatch;

int main(void)
{

    int kernel_server;
    char *puerto_escucha;

    char *ip_memoria;
    char *puerto_memoria;

    char *ip_cpu;
    char *puerto_cpu_dispatch;
    char *puerto_cpu_interrupt;
    int conexion_cpu_interrupt;

    char *algoritmo;
    int grado_multiprog;

    interfaces = list_create();

    kernel_log = iniciar_logger("kernel.log","kernel");
    log_aux = iniciar_logger("kernel_aux.log", "kernel_aux");

    kernel_config = iniciar_config("kernel.config");

    // Extraer configs
    ip_memoria = config_get_string_value(kernel_config,"IP_MEMORIA");
    puerto_memoria = config_get_string_value(kernel_config, "PUERTO_MEMORIA");

    ip_cpu = config_get_string_value(kernel_config, "IP_CPU");
    puerto_cpu_dispatch = config_get_string_value(kernel_config, "PUERTO_CPU_DISPATCH");
    puerto_cpu_interrupt = config_get_string_value(kernel_config, "PUERTO_CPU_INTERRUPT");

    quantum = config_get_int_value(kernel_config, "QUANTUM");
    algoritmo = config_get_string_value(kernel_config, "ALGORITMO_PLANIFICACION");
    grado_multiprog = config_get_int_value(kernel_config, "GRADO_MULTIPROGRAMACION");

    puerto_escucha = config_get_string_value(kernel_config, "PUERTO_ESCUCHA");

    // Inicializo la estructura del planificador
    t_planificacion *planificador = inicializar_t_planificacion(kernel_config, kernel_log, log_aux);


    // ------- Establecer conexiones --------
    // ---- Memoria ----
    conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
    log_info(log_aux, "KERNEL se conectó a MEMORIA");
    send_handshake(conexion_memoria, log_aux, "KERNEL / MEMORIA");

    planificador->socket_memoria = conexion_memoria;

    // ---- CPU dispatch ----
    pthread_t hilo_conexion_dispatch;
    pthread_create(&hilo_conexion_dispatch, NULL, conectar_dispatch, (void*)planificador);
    pthread_join(hilo_conexion_dispatch, NULL);

    // ---- Hilo escucha dispatch-kernel ----
    t_atender_cliente_args *args_dispatch = malloc(sizeof(t_atender_cliente_args));
    args_dispatch->c_socket = planificador->socket_cpu_dispatch;
    args_dispatch->log = kernel_log;
    args_dispatch->server_name = "Dispatch - Kernel";
    args_dispatch->planificador = planificador;

    pthread_t hilo_escucha_dispatch_kernel;
    pthread_create(&hilo_escucha_dispatch_kernel, NULL, (void*) atender_cliente, (void*) args_dispatch);
    pthread_detach(hilo_escucha_dispatch_kernel);

    usleep(30);

    // ---- CPU interrupt ----
    pthread_t hilo_conexion_interrupt;
    pthread_create(&hilo_conexion_interrupt, NULL, conectar_interrupt, (void*)planificador);
    pthread_join(hilo_conexion_dispatch, NULL);

    /* KERNEL - Servidor */  
    // Inicio server
    kernel_server = iniciar_servidor(puerto_escucha, kernel_log);
    log_info(log_aux, "KERNEL listo para recibir clientes");

    t_atender_cliente_args* args = malloc(sizeof(t_atender_cliente_args));
    args->log = kernel_log;
    args->c_socket = kernel_server;
    args->server_name = "Kernel"; 
    args->planificador = planificador;
    
    pthread_t hilo;
    pthread_create(&hilo, NULL, (void *)server_escuchar, args);
    sem_init(&sem_contador_int, 0, 0); //semaforo para lista de interfaces
    sem_init(&habilitacion_io, 0, 0);

    // Consola
    consola_interactiva(planificador);

    // Planificador
    // inicializar_colas_estados();

    pthread_t hilo_planificacion;
    pthread_create(&hilo_planificacion, NULL, hilo_planificador, (void *)planificador);
    pthread_detach(hilo_planificacion);

    sem_init(&grado_planificiacion, 0, grado_multiprog);
    sem_init(&cola_ready, 0, 0);

    

    pthread_join(hilo,NULL);

    list_destroy(interfaces);
    free(args);
    log_destroy(kernel_log);
    log_destroy(log_aux);
    config_destroy(kernel_config);
    // free(colaNew);
    // free(colaExit);
    // free(colaReady);
    // free(colaBlocked);
    // free(colaExec);
    sem_destroy(&planificador->planificar);
    sem_destroy(&sem_contador_int);
    free(planificador);    
    return 0;
}

void* conectar_dispatch(void* args)
{
    t_planificacion *planificacion = (t_planificacion*) args;

    planificacion->socket_cpu_dispatch = crear_conexion(planificacion->config.config_leida.ip_cpu, planificacion->config.config_leida.puerto_cpu_dispatch);
    log_info(log_aux, "KERNEL se conectó a CPU DISPATCH");
    send_handshake(planificacion->socket_cpu_dispatch, log_aux, "KERNEL / CPU DISPATCH");

    log_debug(log_aux, "Conectado a dispatch en: %d", planificacion->socket_cpu_dispatch);
}

void* conectar_interrupt(void* args)
{
    t_planificacion *planificacion = (t_planificacion*) args;

    planificacion->socket_cpu_interrupt = crear_conexion(planificacion->config.config_leida.ip_cpu, planificacion->config.config_leida.puerto_cpu_interrupt);
    log_info(log_aux, "KERNEL se conectó a CPU INTERRUPT");
    send_handshake(planificacion->socket_cpu_interrupt, log_aux, "KERNEL / CPU INTERRUPT");

    log_debug(log_aux, "Conectado a interrupt en: %d", planificacion->socket_cpu_interrupt);
}
