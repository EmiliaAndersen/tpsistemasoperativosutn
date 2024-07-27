#ifndef PLANIFICACION_H_
#define PLANIFICACION_H_

//#include "main.h"
#include <utils/mensajesPropios.h>
#include <commons/temporal.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <commons/string.h>
#include "io.h"
#include <utils/planificador.h>

extern sem_t grado_planificiacion;
extern sem_t cola_ready;

typedef struct
{
    instruccion_params *params;
    instrucciones opcode;
} t_instruccion_params_opcode;
typedef struct {
    char* nombre_interfaz;
    int socket_interfaz;
    sem_t semaforo_interfaz;  
    t_queue* cola_block;
} interfaz;

void* hilo_planificador(void *args);

void fifo(t_planificacion *kernel_argumentos);
void round_robin(t_planificacion *kernel_argumentos);
void virtual_round_robin(t_planificacion *kernel_argumentos);

void enviar_interrupcion(union sigval sv);
void iniciar_timer(t_timer_planificador* timer, int milisegundos);
int frenar_timer(t_timer_planificador* timer);
timer_t inicializar_timer(t_planificacion *kernel_argumentos);

void planificador_planificar(t_planificacion *kernel_argumentos);
bool planificador_recepcion_pcb(t_pcb *pcb_desalojado, t_planificacion *kernel_argumentos);

void planificador_corto_plazo(t_tipo_planificacion algoritmo, t_planificacion *kernel_argumentos);
void planificador_largo_plazo(t_planificacion *kernel_argumentos);


t_planificacion *inicializar_t_planificacion(t_config *kernel_config, t_log *kernel_log, t_log *log_aux);
t_tipo_planificacion obtener_algoritmo_planificador(char* algoritmo);
void inicializar_lista_recursos(t_planificacion *planificador, t_config *kernel_config);
void inicializar_config_kernel(t_planificacion *planificador, t_config *kernel_config);


t_pcb *planificador_ready_a_exec(t_planificacion *kernel_argumentos);
t_pcb *planificador_prioridad_a_exec(t_planificacion *kernel_argumentos);

bool administrador_recursos_wait(t_pcb *pcb_solicitante, char* nombre_recurso, int milisegundos_restantes, t_planificacion *kernel_argumentos);
bool administrador_recursos_signal(t_pcb *pcb_desalojado, char* recurso_solicitado, int milisegundos_restantes, t_planificacion *kernel_argumentos);
void procesar_desbloqueo_factible(char* recurso_solicitado, t_planificacion *kernel_argumentos);
void agregar_recurso_a_lista_global(uint32_t pid, char* nombre_recurso, t_planificacion* kernel_argumentos);
void eliminar_recurso_de_lista_global(uint32_t pid, char* recurso_afectado, t_planificacion* kernel_argumentos);

void validar_peticion(instruccion_params* parametros, t_pcb* pcb, int codigo_op, t_planificacion* kernel_argumentos);
void enviar_instruccion_a_interfaz(t_queue_block* interfaz_destino, instruccion_params* parametros, int codigo_op, uint32_t pid);
interfaz* buscar_interfaz_por_nombre(char* nombre_interfaz);
void pcb_a_exit_por_sol_invalida(t_queue_block* interfaz, t_planificacion *kernel_argumentos);
void agregar_a_cola_interfaz(t_planificacion* kernel_argumentos, instruccion_params* parametros, int op_code, t_pcb* pcb);
void verificar_potencial_envio(t_planificacion* kernel_argumentos, t_queue_block* interfaz);

t_instruccion_params_opcode recibir_solicitud_cpu(int socket_servidor, t_pcb* pcb, t_planificacion* kernel_argumentos);
/*
instruccion_params* deserializar_io_gen_sleep_con_interfaz(t_buffer_ins* buffer);
instruccion_params* deserializar_io_fs_create_delete_con_interfaz(t_buffer_ins* buffer);
instruccion_params* deserializar_io_fs_truncate_con_interfaz(t_buffer_ins* buffer);
instruccion_params* deserializar_io_fs_write_read_con_interfaz(t_buffer_ins* buffer);*/


void mover_a_exit(t_pcb* pcb_desalojado, t_planificacion *kernel_argumentos);
char* proceso_estado_a_string(t_proceso_estado estado);
void solicitar_finalizacion_a_memoria(uint32_t pid, int socket_memoria);
void mover_a_ready(t_pcb* pcb, t_planificacion* kernel_argumentos);
void mover_a_prioridad(t_pcb* pcb, t_planificacion* kernel_argumentos);
void logear_cola_ready(t_planificacion* kernel_argumentos);
void logear_cola_prioridad(t_planificacion* kernel_argumentos);

void loguear_recursos(t_planificacion* kernel_argumentos);

#endif
