#ifndef PLANIFICADOR_H
#define PLANIFICADOR_H

#include <commons/collections/dictionary.h>
#include <commons/collections/queue.h>
#include <commons/log.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>

typedef struct {
    char *identificador;
    t_queue *block_queue;
    int cantidad_instancias;
    int socket_interfaz;
    t_list *block_dictionary;
} t_queue_block;

typedef struct {
    t_queue *new;
    t_queue *ready;
    t_queue *exec;
    t_dictionary *lista_block;
    int cantidad_procesos_block;
    t_queue *exit;
    t_queue *prioridad;

    pthread_mutex_t mutex_new;
    pthread_mutex_t mutex_ready;
    pthread_mutex_t mutex_exec;
    pthread_mutex_t mutex_block;
    pthread_mutex_t mutex_exit;
    pthread_mutex_t mutex_prioridad;

} t_planificador_colas;

typedef struct{
    int puerto_escucha;
    char *ip_memoria;
    int puerto_memoria;
    char *ip_cpu;
    char *puerto_cpu_dispatch;
    char *puerto_cpu_interrupt;
} t_config_leida_kernel;

typedef struct{
    t_config_leida_kernel config_leida;
    int grado_multiprogramacion;
    char *algoritmo_planificador;
    int quantum;

} t_planificador_config;

typedef struct {
    timer_t timer;
    struct sigaction sa;
    struct sigevent sev;
    struct itimerspec its;
} t_timer_planificador;
typedef enum {
    FIFO,
    RR,
    VRR
}t_tipo_planificacion;
typedef struct
{
    t_log *logger;
    t_log *log_aux;
    t_planificador_colas colas;
    t_planificador_config config;
    sem_t planificar;
    int detener_planificacion;
    t_timer_planificador* timer_quantum;
    int socket_cpu_dispatch;
    int socket_cpu_interrupt;
    int socket_memoria;
    t_tipo_planificacion algo_planning;
    pthread_mutex_t planning_mutex;
    t_dictionary* recursos_tomados;
    t_dictionary* parametros_en_espera;
} t_planificacion;

#endif