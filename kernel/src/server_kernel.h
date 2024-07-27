#ifndef SERVER_H_
#define SERVER_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/string.h>
#include <utils/catedra/server.h>
#include <utils/catedra/inicio.h>
#include <inttypes.h>
#include <commons/log.h>
#include <pthread.h>
#include <utils/mensajesPropios.h>
#include <commons/collections/list.h>
#include <semaphore.h>
#include <commons/collections/queue.h>
#include <utils/funcionesUtiles.h>
#include <utils/planificador.h>

extern t_list* interfaces;
extern sem_t habilitacion_io;
extern sem_t sem_contador_int;
extern int logica_int;

typedef struct
{
    t_log *log;
    int c_socket;
    char *server_name;
    t_planificacion *planificador;

} t_atender_cliente_args;

void atender_cliente(void* void_args);
int server_escuchar(void *arg);
t_queue_block* buscar_interfaz_por_socket(t_planificacion* kernel_argumentos, int socket);
void eliminar_procesos_bloqueados_por_interfaz(t_queue_block* interfaz_desconectada, t_planificacion *kernel_argumentos);


#endif
