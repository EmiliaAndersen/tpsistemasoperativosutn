#include "alg_planificacion.h"

void* hilo_planificador(void *args)
{
    t_planificacion *kernel_argumentos = (t_planificacion*) args;

    t_tipo_planificacion algoritmo_planificador = obtener_algoritmo_planificador(kernel_argumentos->config.algoritmo_planificador);
    log_debug(kernel_argumentos->log_aux, "Algoritmo planificador: %d", algoritmo_planificador);
    kernel_argumentos->algo_planning = algoritmo_planificador;
    while(1)
    {
        // Espero a que me soliciten planificar
        sem_wait(&kernel_argumentos->planificar);
        log_debug(kernel_argumentos->log_aux, "Planificando...");

        // Si se ejecuto DETENER_PLANIFICACION, no planifico por mas que me soliciten hacerlo
        if(kernel_argumentos->detener_planificacion)
        {
            log_debug(kernel_argumentos->log_aux, "Planificacion detenida.");
            pthread_mutex_unlock(&kernel_argumentos->planning_mutex);
            continue;
        }

        // Planifico a largo plazo
        planificador_largo_plazo(kernel_argumentos);

        // Planifico a corto plazo
        planificador_corto_plazo(algoritmo_planificador, kernel_argumentos);
        pthread_mutex_unlock(&kernel_argumentos->planning_mutex);
        //loguear_recursos(kernel_argumentos);
    }
    return NULL;
}

void loguear_recursos(t_planificacion* kernel_argumentos)
{
    t_list* lista_recursos = dictionary_elements(kernel_argumentos->colas.lista_block);
    int i = 0, tamanio = list_size(lista_recursos);
    while(i<tamanio)
    {
        t_queue_block* recurso = list_remove(lista_recursos, 0);
        log_debug(kernel_argumentos->log_aux, "El recurso %s tiene %d instancias disponibles", recurso->identificador, recurso->cantidad_instancias);
        list_add(lista_recursos, recurso);
        i++;
    }
}

void fifo(t_planificacion *kernel_argumentos)
{
    if(queue_size(kernel_argumentos->colas.exec) == 0 && queue_size(kernel_argumentos->colas.ready) >= 1)
    {
        t_pcb *proximo_pcb = planificador_ready_a_exec(kernel_argumentos);
        proximo_pcb->estado = EXEC;
        enviar_pcb(proximo_pcb, kernel_argumentos->socket_cpu_dispatch);
    }
    if(kernel_argumentos->colas.ready == 0)
    {
        log_debug(kernel_argumentos->log_aux, "No hay mas procesos para enviar a EXEC. Se detiene la planificacion");
        kernel_argumentos->detener_planificacion = 1;
    }
    return;
}

void round_robin(t_planificacion *kernel_argumentos)
{
    if(queue_size(kernel_argumentos->colas.exec) == 0 && queue_size(kernel_argumentos->colas.ready) >= 1)
    {
        t_pcb* proximo_pcb = planificador_ready_a_exec(kernel_argumentos);
        proximo_pcb->estado = EXEC;
        iniciar_timer(kernel_argumentos->timer_quantum, kernel_argumentos->config.quantum);
        enviar_pcb(proximo_pcb, kernel_argumentos->socket_cpu_dispatch);
    }
    if(kernel_argumentos->colas.ready == 0)
    {
        log_debug(kernel_argumentos->log_aux, "No hay mas procesos para enviar a EXEC. Se detiene la planificacion");
        kernel_argumentos->detener_planificacion = 1;
    }
    return;
}

void virtual_round_robin(t_planificacion *kernel_argumentos)
{
    if(queue_size(kernel_argumentos->colas.exec) == 0 && queue_size(kernel_argumentos->colas.prioridad) >= 1)
    {
        t_pcb *proximo_pcb = planificador_prioridad_a_exec(kernel_argumentos);
        // proximo_pcb->estado = EXEC;

        // int quantum_restante = kernel_argumentos->config.quantum - proximo_pcb->quantum;
        iniciar_timer(kernel_argumentos->timer_quantum, proximo_pcb->quantum);

        enviar_pcb(proximo_pcb, kernel_argumentos->socket_cpu_dispatch);
        return;
    }
    if(queue_size(kernel_argumentos->colas.exec) == 0 && queue_size(kernel_argumentos->colas.ready) >= 1)
    {
        t_pcb *proximo_pcb = planificador_ready_a_exec(kernel_argumentos);
        // proximo_pcb->estado = EXEC;

        iniciar_timer(kernel_argumentos->timer_quantum, kernel_argumentos->config.quantum);

        enviar_pcb(proximo_pcb, kernel_argumentos->socket_cpu_dispatch);
    }
    if(kernel_argumentos->colas.ready == 0)
    {
        log_debug(kernel_argumentos->log_aux, "No hay mas procesos para enviar a EXEC. Se detiene la planificacion");
        kernel_argumentos->detener_planificacion = 1;
    }
    return;
}

void iniciar_timer(t_timer_planificador* timer, int milisegundos)
{
    timer->its.it_value.tv_sec = milisegundos / 1000;
    timer->its.it_value.tv_nsec = (milisegundos % 1000) * 1000000;
    timer->its.it_interval.tv_nsec = 0;
    timer->its.it_interval.tv_sec = 0;

    timer_settime(timer->timer, 0, &timer->its, NULL);
}

int frenar_timer(t_timer_planificador* timer)
{
    struct itimerspec remaining;
    timer_gettime(timer->timer, &remaining);
    //timer_delete(timer->timer);
    return remaining.it_value.tv_nsec / 1000000;
}

void enviar_interrupcion(union sigval sv)
{
    t_planificacion *kernel_argumentos = (t_planificacion*)sv.sival_ptr;

    // Enviar interrupcion a CPU Interrupt
    if(!queue_is_empty(kernel_argumentos->colas.exec))
    {
        t_pcb* pcb_actual = queue_peek(kernel_argumentos->colas.exec);
        enviar_int_a_interrupt(kernel_argumentos->socket_cpu_interrupt, pcb_actual->pid);
        log_debug(kernel_argumentos->log_aux, "Interrupcion al PID: %d enviada.", pcb_actual->pid);
        return;
    }
    
    // log_debug(kernel_argumentos->logger, "No se envia interrupcion al no haber nada en EXEC");
}

void planificador_planificar(t_planificacion *kernel_argumentos)
{
    sem_post(&kernel_argumentos->planificar);
}

bool planificador_recepcion_pcb(t_pcb *pcb_desalojado, t_planificacion *kernel_argumentos)
{
    // Saco el pcb viejo de EXEC
    pthread_mutex_lock(&kernel_argumentos->planning_mutex);

    if(queue_is_empty(kernel_argumentos->colas.exec))
    {
        log_debug(kernel_argumentos->log_aux, "El proceso recibido fue terminado por el usuario");
        free(pcb_desalojado->registros);
        free(pcb_desalojado);
        return true;
    }

    pthread_mutex_lock(&kernel_argumentos->colas.mutex_exec);
    t_pcb* pcb_outdated = queue_pop(kernel_argumentos->colas.exec);
    pthread_mutex_unlock(&kernel_argumentos->colas.mutex_exec);

    log_debug(kernel_argumentos->log_aux, "PID recibido: %d", pcb_desalojado->pid);
    log_debug(kernel_argumentos->log_aux, "PID que tengo en EXEC: %d", pcb_outdated->pid);
    
    if(pcb_desalojado->pid != pcb_outdated->pid)
    {   
        log_error(kernel_argumentos->log_aux, "Discordancia entre el pcb en EXEC y el ejecutado por CPU.");
        return true;
    }

    free(pcb_outdated->registros);
    free(pcb_outdated);

    // Meto el pcb a la cola correspondiente
    if(pcb_desalojado->motivo_desalojo == 0) // Desalojado por haber ejecutado EXIT
    {
        // Mato al timer si corresponde
        if(kernel_argumentos->algo_planning != FIFO)
        {
            frenar_timer(kernel_argumentos->timer_quantum);
        }

        log_info(kernel_argumentos->logger, "Finaliza el proceso %d - Motivo: SUCCESS", pcb_desalojado->pid);
        mover_a_exit(pcb_desalojado, kernel_argumentos);

        return true;
        //pcb_desalojado->estado = EXIT;
        //queue_push(kernel_argumentos->colas.exit, pcb_desalojado);
        // Enviar solicitud a memoria para desalojar el proceso
        //log_info(kernel_argumentos->logger, "PID: %d - Estado anterior: EXEC - Estado actual: EXIT", pcb_desalojado->pid);
    }
    if(pcb_desalojado->motivo_desalojo == 1) // Desalojado por fin de quantum
    {
        log_info(kernel_argumentos->logger, "PID: %d - Desalojado por fin de Quantum", pcb_desalojado->pid);
        mover_a_ready(pcb_desalojado, kernel_argumentos);
    }
    if(pcb_desalojado->motivo_desalojo == 2) // Desalojado por IO_BLOCK
    {
        int milisegundos_restantes = frenar_timer(kernel_argumentos->timer_quantum);
        if(kernel_argumentos->algo_planning == VRR)
        {
            pcb_desalojado->quantum = milisegundos_restantes;
            log_debug(kernel_argumentos->log_aux, "Milisegundos restantes: %d", pcb_desalojado->quantum);
        }
        // Recibir los parametros del io_block
        t_instruccion_params_opcode parametros_solicitud;
        parametros_solicitud = recibir_solicitud_cpu(kernel_argumentos->socket_cpu_dispatch, pcb_desalojado, kernel_argumentos);
        // log_debug(kernel_argumentos->log_aux, "Solicitud recibida: %s, %d", parametros_solicitud.params->interfaz, parametros_solicitud.params->params.io_gen_sleep.unidades_trabajo);

        validar_peticion(parametros_solicitud.params, pcb_desalojado, parametros_solicitud.opcode, kernel_argumentos);
        // mover_a_block(kernel_argumentos, pcb_desalojado, nombre_interfaz);
        //log_info(kernel_argumentos->logger, "PID: %d - Estado anterior: EXEC - Estado actual: BLOCK", pcb_desalojado->pid);
        // Realizar la solicitud correspondiente a IO
    }
    if(pcb_desalojado->motivo_desalojo == 3) // Desalojado por WAIT
    {
        int milisegundos_restantes;
        if(kernel_argumentos->algo_planning != FIFO)
        {
            milisegundos_restantes = frenar_timer(kernel_argumentos->timer_quantum);
        }

        recibir_operacion(kernel_argumentos->socket_cpu_dispatch);
        char* recurso_solicitado = recibir_mensaje(kernel_argumentos->socket_cpu_dispatch, kernel_argumentos->logger);
        log_debug(kernel_argumentos->log_aux, "Se recibe una solicitud de WAIT al recurso: %s", recurso_solicitado);
        
        return administrador_recursos_wait(pcb_desalojado, recurso_solicitado, milisegundos_restantes, kernel_argumentos);
    }
    if(pcb_desalojado->motivo_desalojo == 4) // Desalojado por SIGNAL
    {
        int milisegundos_restantes;
        if(kernel_argumentos->algo_planning != FIFO)
        {
            milisegundos_restantes = frenar_timer(kernel_argumentos->timer_quantum);
        }
        
        recibir_operacion(kernel_argumentos->socket_cpu_dispatch);
        char* recurso_solicitado = recibir_mensaje(kernel_argumentos->socket_cpu_dispatch, kernel_argumentos->logger);
        log_debug(kernel_argumentos->log_aux, "Se recibe una solicitud de SIGNAL al recurso: %s", recurso_solicitado);
        
        return administrador_recursos_signal(pcb_desalojado, recurso_solicitado, milisegundos_restantes, kernel_argumentos);
    }
    if(pcb_desalojado->motivo_desalojo == 5)
    {
        if(kernel_argumentos->algo_planning != FIFO)
        {
            frenar_timer(kernel_argumentos->timer_quantum);
        }
        
        log_info(kernel_argumentos->logger, "Finaliza el proceso %d - Motivo: OUT_OF_MEMORY", pcb_desalojado->pid);
        mover_a_exit(pcb_desalojado, kernel_argumentos);
        return true;
    }
    return true;
}

void planificador_corto_plazo(t_tipo_planificacion algoritmo, t_planificacion *kernel_argumentos)
{
    switch(algoritmo)
    {
        case FIFO:
        {
            fifo(kernel_argumentos);
        }
        case RR:
        {
            round_robin(kernel_argumentos);
        }
        case VRR:
        {
            virtual_round_robin(kernel_argumentos);
        }
    }
}

void planificador_largo_plazo(t_planificacion *kernel_argumentos)
{
    int cantidad_procesos_actual = queue_size(kernel_argumentos->colas.exec);
    cantidad_procesos_actual += queue_size(kernel_argumentos->colas.ready);
    cantidad_procesos_actual += kernel_argumentos->colas.cantidad_procesos_block;
    cantidad_procesos_actual += queue_size(kernel_argumentos->colas.prioridad);

    if(cantidad_procesos_actual < kernel_argumentos->config.grado_multiprogramacion)
    {
        if(queue_is_empty(kernel_argumentos->colas.new))
        {
            return;
        }
        while(cantidad_procesos_actual < kernel_argumentos->config.grado_multiprogramacion && !queue_is_empty(kernel_argumentos->colas.new))
        {
            pthread_mutex_lock(&kernel_argumentos->colas.mutex_new);
            t_pcb* pcb = queue_pop(kernel_argumentos->colas.new);
            pthread_mutex_unlock(&kernel_argumentos->colas.mutex_new);

            mover_a_ready(pcb, kernel_argumentos);
            cantidad_procesos_actual++;
        }
    }
}

t_planificacion *inicializar_t_planificacion(t_config *kernel_config, t_log *kernel_log, t_log *log_aux)
{
   t_planificacion *planificador = malloc(sizeof(t_planificacion));
    
    planificador->logger = kernel_log;
    planificador->log_aux = log_aux;

    planificador->colas.new = queue_create();
    planificador->colas.ready = queue_create();
    planificador->colas.exec = queue_create();
    planificador->colas.exit = queue_create();
    planificador->colas.prioridad = queue_create();
    planificador->colas.lista_block = dictionary_create();

    planificador->detener_planificacion = 1;
    planificador->colas.cantidad_procesos_block = 0;

    sem_init(&planificador->planificar, 0, 0);

    inicializar_lista_recursos(planificador, kernel_config);

    inicializar_config_kernel(planificador, kernel_config);

    planificador->timer_quantum = malloc(sizeof(t_timer_planificador));
    planificador->timer_quantum->timer = inicializar_timer(planificador);

    pthread_mutex_init(&planificador->planning_mutex, NULL);
    pthread_mutex_init(&planificador->colas.mutex_block, NULL);
    pthread_mutex_init(&planificador->colas.mutex_exec, NULL);
    pthread_mutex_init(&planificador->colas.mutex_exit, NULL);
    pthread_mutex_init(&planificador->colas.mutex_new, NULL);
    pthread_mutex_init(&planificador->colas.mutex_prioridad, NULL);
    pthread_mutex_init(&planificador->colas.mutex_ready, NULL);

    planificador->parametros_en_espera = dictionary_create();
    planificador->recursos_tomados = dictionary_create(); 

    return planificador;
}

timer_t inicializar_timer(t_planificacion *kernel_argumentos)
{
    struct sigevent sev;
    timer_t ret;

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_value.sival_ptr = kernel_argumentos;
    sev.sigev_notify_function = enviar_interrupcion;
    sev.sigev_notify_attributes = NULL;

    timer_create(CLOCK_REALTIME, &sev, &ret);

    return ret;
}

t_tipo_planificacion obtener_algoritmo_planificador(char* algoritmo)
{
    if(strcmp(algoritmo, "FIFO") == 0)
    {
        return FIFO;
    }
    if(strcmp(algoritmo, "RR") == 0)
    {
        return RR;
    }
    if(strcmp(algoritmo, "VRR") == 0)
    {
        return VRR;
    }
    
    // Se elige fifo por default
    return FIFO;
}

void inicializar_lista_recursos(t_planificacion *planificador, t_config *kernel_config)
{
    char* string_arr_recursos = config_get_string_value(kernel_config, "RECURSOS");
    char* string_arr_instancias_recursos = config_get_string_value(kernel_config, "INSTANCIAS_RECURSOS");

    char** array_nombre_recursos = string_get_string_as_array(string_arr_recursos);
    char** array_instancias_recursos = string_get_string_as_array(string_arr_instancias_recursos);

    int i = 0;
    while(array_nombre_recursos[i] != NULL)
    {
        t_queue_block *block_queue = malloc(sizeof(t_queue_block));
        block_queue->block_dictionary = list_create();
        block_queue->block_queue = queue_create();
        block_queue->identificador = string_duplicate(array_nombre_recursos[i]);
        block_queue->cantidad_instancias = atoi(array_instancias_recursos[i]);
        block_queue->socket_interfaz = 0;
        log_debug(planificador->log_aux, "Recurso agregado. Identificador: %s, Cantidad de instancias: %d", block_queue->identificador, block_queue->cantidad_instancias);

        dictionary_put(planificador->colas.lista_block, block_queue->identificador, block_queue);
        //free(block_queue); // Este free capaz no tenga que estar aca, sino al final de la ejecucion
        i++;
    }

    string_array_destroy(array_nombre_recursos);
    string_array_destroy(array_instancias_recursos);
    //free(array_nombre_recursos);
    //free(array_instancias_recursos);
}

void inicializar_config_kernel(t_planificacion *planificador, t_config *kernel_config)
{
    planificador->config.algoritmo_planificador = config_get_string_value(kernel_config, "ALGORITMO_PLANIFICACION");
    planificador->config.grado_multiprogramacion = config_get_int_value(kernel_config, "GRADO_MULTIPROGRAMACION");
    planificador->config.quantum = config_get_int_value(kernel_config, "QUANTUM");
    planificador->config.config_leida.ip_cpu = config_get_string_value(kernel_config, "IP_CPU");
    planificador->config.config_leida.ip_memoria = config_get_string_value(kernel_config, "IP_MEMORIA");
    planificador->config.config_leida.puerto_cpu_dispatch = config_get_string_value(kernel_config, "PUERTO_CPU_DISPATCH");
    planificador->config.config_leida.puerto_cpu_interrupt = config_get_string_value(kernel_config, "PUERTO_CPU_INTERRUPT");
    planificador->config.config_leida.puerto_escucha = config_get_int_value(kernel_config, "PUERTO_ESCUCHA");
    planificador->config.config_leida.puerto_memoria = config_get_int_value(kernel_config, "PUERTO_MEMORIA");
}

// -------- ADMINISTRACION DE RECURSOS --------

bool administrador_recursos_wait(t_pcb *pcb_solicitante, char* nombre_recurso, int milisegundos_restantes, t_planificacion *kernel_argumentos)
{
    t_queue_block *recurso = dictionary_get(kernel_argumentos->colas.lista_block, nombre_recurso);
    // Verificar existencia y disponibilidad del recurso solicitado
    // Si no existe, a EXIT y return true.
    // Si existe, pero no hay instancias, a BLOCK correspondiente y return true
    // Si existe y hay instancias, se lo devuelve a EXEC, y se retorna un valor para que no se replanifique

    if(recurso == NULL)
    {
        // El recurso no existe. Mando el proceso a EXIT y habilito la replanificacion
        log_info(kernel_argumentos->logger, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", pcb_solicitante->pid);
        mover_a_exit(pcb_solicitante, kernel_argumentos);
        free(nombre_recurso);
        return true;
    }

    recurso->cantidad_instancias--;
    log_debug(kernel_argumentos->log_aux, "Cantidad de instancias restantes para el recurso %s: %d", nombre_recurso, recurso->cantidad_instancias);

    if(recurso->cantidad_instancias >= 0)
    {
        // Se habilita la instancia del recurso para el proceso
        pthread_mutex_lock(&kernel_argumentos->colas.mutex_exec);
        queue_push(kernel_argumentos->colas.exec, pcb_solicitante);
        pthread_mutex_unlock(&kernel_argumentos->colas.mutex_exec);

        log_debug(kernel_argumentos->log_aux, "Se devuelve el PID: %d a cpu por haber solicitado un recurso disponible.", pcb_solicitante->pid);

        if(kernel_argumentos->algo_planning != FIFO)
        {
            iniciar_timer(kernel_argumentos->timer_quantum, milisegundos_restantes);
        }
        enviar_pcb(pcb_solicitante, kernel_argumentos->socket_cpu_dispatch);
        pthread_mutex_unlock(&kernel_argumentos->planning_mutex);

        agregar_recurso_a_lista_global(pcb_solicitante->pid, nombre_recurso, kernel_argumentos);
        return false;
    }

    // Se bloque el proceso
    pcb_solicitante->estado = BLOCKED;
    pthread_mutex_lock(&kernel_argumentos->colas.mutex_block);
    list_add(recurso->block_dictionary, pcb_solicitante);
    kernel_argumentos->colas.cantidad_procesos_block++;
    pthread_mutex_unlock(&kernel_argumentos->colas.mutex_block);
    log_info(kernel_argumentos->logger, "PID: %d - Estado anterior: EXEC - Estado actual: BLOCK", pcb_solicitante->pid);
    log_info(kernel_argumentos->logger, "PID: %d - Bloqueado por: %s", pcb_solicitante->pid, nombre_recurso);
    
    agregar_recurso_a_lista_global(pcb_solicitante->pid, nombre_recurso, kernel_argumentos);

    return true;
}

bool administrador_recursos_signal(t_pcb *pcb_desalojado, char* recurso_solicitado, int milisegundos_restantes, t_planificacion *kernel_argumentos)
{
    // Verificar existencia del recurso solicitado
    // Si no existe, a EXIT y return true
    // Si existe, se le suma 1 al indice correspondiente y se lo devuelve a EXEC.
    // Si hay procesos esperando, se mueve uno de BLOCK a READY
    t_queue_block *recurso = dictionary_get(kernel_argumentos->colas.lista_block, recurso_solicitado);
    log_debug(kernel_argumentos->log_aux, "Recurso obtenido del diccionario: %s. Se procesa un SIGNAL", recurso->identificador);

    if(recurso == NULL)
    {
        // El recurso no existe. Mando el proceso a EXIT y habilito la replanificacion
        log_info(kernel_argumentos->logger, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", pcb_desalojado->pid);
        mover_a_exit(pcb_desalojado, kernel_argumentos);
        free(recurso_solicitado);
        return true;
    }

    recurso->cantidad_instancias++;
    eliminar_recurso_de_lista_global(pcb_desalojado->pid, recurso_solicitado, kernel_argumentos);

    procesar_desbloqueo_factible(recurso_solicitado, kernel_argumentos);
    log_debug(kernel_argumentos->log_aux, "Desbloqueo procesado.");

    pthread_mutex_lock(&kernel_argumentos->colas.mutex_exec);
    queue_push(kernel_argumentos->colas.exec, pcb_desalojado);
    pthread_mutex_unlock(&kernel_argumentos->colas.mutex_exec);

    // Devuelvo el pcb a CPU
    if(kernel_argumentos->algo_planning != FIFO)
    {
        iniciar_timer(kernel_argumentos->timer_quantum, milisegundos_restantes);
    }
    enviar_pcb(pcb_desalojado, kernel_argumentos->socket_cpu_dispatch);

    pthread_mutex_unlock(&kernel_argumentos->planning_mutex);

    return false;
}

void agregar_recurso_a_lista_global(uint32_t pid, char* nombre_recurso, t_planificacion* kernel_argumentos)
{
    char* pid_proceso = string_itoa(pid);
    t_list* lista_proceso = dictionary_get(kernel_argumentos->recursos_tomados, pid_proceso);
    if(lista_proceso == NULL)
    {
        lista_proceso = list_create();
        dictionary_put(kernel_argumentos->recursos_tomados, pid_proceso, lista_proceso);
    }
    list_add(lista_proceso, nombre_recurso);
    free(pid_proceso);
}

void eliminar_recurso_de_lista_global(uint32_t pid, char* recurso_afectado, t_planificacion* kernel_argumentos)
{
    char* pid_proceso = string_itoa(pid);
    t_list* lista_proceso = dictionary_get(kernel_argumentos->recursos_tomados, pid_proceso);
    if(lista_proceso != NULL)
    {
        int i = 0, tamanio = list_size(lista_proceso);
        char*  nombre_recurso;
        while(i<tamanio)
        {
            nombre_recurso = list_remove(lista_proceso, 0);
            
            if(string_equals_ignore_case(nombre_recurso, recurso_afectado))
            {
                log_debug(kernel_argumentos->log_aux, "%s liberado de la lista global para el proceso %d!", nombre_recurso, pid);
                free(nombre_recurso);
                free(pid_proceso);
                return;
            }
            else
            {
                list_add(lista_proceso, nombre_recurso);
            }
            
            i++;
        }
    }
    free(pid_proceso);
}

void procesar_desbloqueo_factible(char* recurso_solicitado, t_planificacion *kernel_argumentos)
{
    t_queue_block *recurso = dictionary_get(kernel_argumentos->colas.lista_block, recurso_solicitado);
    free(recurso_solicitado);
    log_debug(kernel_argumentos->log_aux, "Recurso obtenido del diccionario: %s", recurso->identificador);

    if(!list_is_empty(recurso->block_dictionary))
    {
        int tamanio = list_size(recurso->block_dictionary);
        // log_debug(kernel_argumentos->log_aux, "Tamanio de la lista: %d", tamanio);
        t_pcb *pcb_desbloqueado = list_remove(recurso->block_dictionary, 0);
        // log_debug(kernel_argumentos->log_aux, "Obtuve un PCB de PID: %d", pcb_desbloqueado->pid);
        
        mover_a_ready(pcb_desbloqueado, kernel_argumentos);
        
        kernel_argumentos->colas.cantidad_procesos_block--;
        // log_debug(kernel_argumentos->log_aux, "Cantidad de procesos en block: %d", kernel_argumentos->colas.cantidad_procesos_block);
        // recurso->cantidad_instancias--;
        // log_debug(kernel_argumentos->log_aux, "Cantidad de instancias disponibles para el recurso %s: %d", recurso->identificador, recurso->cantidad_instancias);
        return;
    }

    log_debug(kernel_argumentos->log_aux, "No hay procesos bloqueados por el recurso: %s", recurso->identificador);
}

// -------- ADMINISTRACION DE INTERFACES --------

void validar_peticion(instruccion_params* parametros, t_pcb* pcb, int codigo_op, t_planificacion* kernel_argumentos) {

    if(dictionary_is_empty(kernel_argumentos->colas.lista_block))
    {
        log_warning(kernel_argumentos->log_aux, "No hay interfaces concetadas.");
        pcb->estado = EXIT;
        queue_push(kernel_argumentos->colas.exit, pcb);
        log_info(kernel_argumentos->logger, "PID: %d - Estado anterior: EXEC - Estado actual: EXIT", pcb->pid);
        log_info(kernel_argumentos->logger, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", pcb->pid);
        return;
    }

    t_queue_block *interfaz_solicitada = dictionary_get(kernel_argumentos->colas.lista_block, parametros->interfaz);
    if(interfaz_solicitada == NULL)
    {
        pcb->estado = EXIT;
        queue_push(kernel_argumentos->colas.exit, pcb);
        log_info(kernel_argumentos->logger, "PID: %d - Estado anterior: EXEC - Estado actual: EXIT", pcb->pid);
        log_info(kernel_argumentos->logger, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", pcb->pid);
        return;
    }

    if(queue_is_empty(interfaz_solicitada->block_queue))
    {
        enviar_instruccion_a_interfaz(interfaz_solicitada, parametros, codigo_op, pcb->pid);
        log_debug(kernel_argumentos->log_aux, "Instruccion solicitada a la interfaz: %s", interfaz_solicitada->identificador);
        log_debug(kernel_argumentos->log_aux, "Enviada por el socket: %d", interfaz_solicitada->socket_interfaz);
    }
    else
    {
        agregar_a_cola_interfaz(kernel_argumentos, parametros, codigo_op, pcb);
        log_debug(kernel_argumentos->log_aux, "Se agrega el proceso a la cola correspondiente a la interfaz solicitada");
    }


    pcb->estado = BLOCKED;
    pthread_mutex_lock(&kernel_argumentos->colas.mutex_block);
    queue_push(interfaz_solicitada->block_queue, pcb);
    pthread_mutex_unlock(&kernel_argumentos->colas.mutex_block);
    kernel_argumentos->colas.cantidad_procesos_block++;
    log_info(kernel_argumentos->logger, "PID: %d - Bloqueado por: INTERFAZ", pcb->pid);
    log_info(kernel_argumentos->logger, "PID: %d - Estado anterior: EXEC - Estado actual: BLOCK", pcb->pid);
}

void agregar_a_cola_interfaz(t_planificacion* kernel_argumentos, instruccion_params* parametros, int op_code, t_pcb* pcb)
{
    char* pid = string_itoa(pcb->pid);
    t_instruccion_params_opcode* param = malloc(sizeof(t_instruccion_params_opcode));
    param->opcode = op_code;
    param->params = parametros;
    dictionary_put(kernel_argumentos->parametros_en_espera, pid, param);
    free(pid);
}

interfaz* buscar_interfaz_por_nombre(char* nombre_interfaz) {
    // int tamanio_lista = list_size(interfaces);
    // interfaz* interfaz_encontrada = NULL;

    // for (int i = 0; i < tamanio_lista; i++) {
    //     interfaz* posible_interfaz = (interfaz*)list_get(interfaces, i);
    //     if (string_equals_ignore_case(posible_interfaz->nombre_interfaz, nombre_interfaz)) {
    //         interfaz_encontrada = posible_interfaz;
    //         break;
    //     }
    // }
    // return interfaz_encontrada;
}

void enviar_instruccion_a_interfaz(t_queue_block* interfaz_destino, instruccion_params* parametros, int codigo_op, uint32_t pid) {
    t_paquete_instruccion* instruccion_enviar = malloc(sizeof(t_paquete_instruccion));

    instruccion_enviar->codigo_operacion = codigo_op;
    enviar_instruccion(instruccion_enviar, parametros, interfaz_destino->socket_interfaz, pid);

    if(codigo_op == IO_STDIN_READ || codigo_op == IO_STDOUT_WRITE)
    {
        free(parametros->registro_direccion);
    }
    if(codigo_op == IO_FS_READ || codigo_op == IO_FS_WRITE)
    {
        free(parametros->registro_direccion);
        free(parametros->params.io_fs.nombre_archivo);
    }
    if(codigo_op == IO_FS_TRUNCATE)
    {
        free(parametros->params.io_fs.nombre_archivo);
    }
    if(codigo_op == IO_FS_CREATE || codigo_op == IO_FS_DELETE)
    {
        free(parametros->params.io_fs.nombre_archivo);
    }
    
    free(parametros->interfaz);
    free(parametros);
    free(instruccion_enviar);
}


t_instruccion_params_opcode recibir_solicitud_cpu(int socket_servidor, t_pcb* pcb, t_planificacion* kernel_argumentos)
{
    t_paquete_instruccion* instruccion = malloc(sizeof(t_paquete_instruccion));
    instruccion->buffer = malloc(sizeof(t_buffer_ins));
    
 
    recv(socket_servidor, &(instruccion->codigo_operacion), sizeof(instrucciones), MSG_WAITALL);
    recv(socket_servidor, &(instruccion->buffer->size), sizeof(uint32_t), MSG_WAITALL);
 
    
    instruccion->buffer->stream = malloc(instruccion->buffer->size);
    
    recv(socket_servidor, instruccion->buffer->stream, instruccion->buffer->size, MSG_WAITALL);
    instruccion_params* param = NULL;
   
    switch (instruccion->codigo_operacion) {
        case IO_GEN_SLEEP:{
            param = deserializar_io_gen_sleep_con_interfaz(instruccion->buffer);
            break;
        }
        case IO_STDIN_READ:
        { param = deserializar_io_stdin_stdout_con_interfaz(instruccion->buffer);
            break;
            }
        case IO_STDOUT_WRITE:{
            param = deserializar_io_stdin_stdout_con_interfaz(instruccion->buffer);
            break;}
        case IO_FS_CREATE:{
            param = deserializar_io_fs_create_delete_con_interfaz(instruccion->buffer);
            break;
        }
        case IO_FS_DELETE:{
            param = deserializar_io_fs_create_delete_con_interfaz(instruccion->buffer);
            break;
        }
        case IO_FS_TRUNCATE:{
            param = deserializar_io_fs_truncate_con_interfaz(instruccion->buffer);
            break;
        }
        case IO_FS_WRITE:{
            param = deserializar_io_fs_write_read_con_interfaz(instruccion->buffer);
            break;
        }
        case IO_FS_READ:{
            param = deserializar_io_fs_write_read_con_interfaz(instruccion->buffer);
            break;
        }
        default:
            log_warning(kernel_argumentos->log_aux, "Tipo de operación no válido.\n");
            break;
        }

    t_instruccion_params_opcode ret;
    ret.opcode = instruccion->codigo_operacion;
    ret.params = param;

    free(instruccion->buffer->stream);
    free(instruccion->buffer);
    free(instruccion);

    return ret;
}

void pcb_a_exit_por_sol_invalida(t_queue_block* interfaz, t_planificacion* kernel_argumentos)
{
    pthread_mutex_lock(&kernel_argumentos->colas.mutex_block);
    t_pcb *pcb_desalojado = queue_pop(interfaz->block_queue);
    pthread_mutex_unlock(&kernel_argumentos->colas.mutex_block);

    log_info(kernel_argumentos->logger, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", pcb_desalojado->pid);
    mover_a_exit(pcb_desalojado, kernel_argumentos);

}

void procesar_entradasalida_terminada(t_queue_block *interfaz, t_planificacion *kernel_argumentos)
{
    pthread_mutex_lock(&kernel_argumentos->colas.mutex_block);
    t_pcb *pcb_desalojado = queue_pop(interfaz->block_queue);
    kernel_argumentos->colas.cantidad_procesos_block--;
    pthread_mutex_unlock(&kernel_argumentos->colas.mutex_block);

    log_debug(kernel_argumentos->log_aux, "PID: %d", pcb_desalojado->pid);

    if(pcb_desalojado->quantum != 0)
    {
        mover_a_prioridad(pcb_desalojado, kernel_argumentos);
        // pthread_mutex_lock(&kernel_argumentos->colas.mutex_prioridad);
        // queue_push(kernel_argumentos->colas.prioridad, pcb_desalojado);
        // pthread_mutex_unlock(&kernel_argumentos->colas.mutex_prioridad);
    }
    else
    {
        mover_a_ready(pcb_desalojado, kernel_argumentos);
        // pthread_mutex_lock(&kernel_argumentos->colas.mutex_ready);
        // queue_push(kernel_argumentos->colas.ready, pcb_desalojado);
        // pthread_mutex_unlock(&kernel_argumentos->colas.mutex_ready);
    }
    
    verificar_potencial_envio(kernel_argumentos, interfaz);

    if(queue_is_empty(kernel_argumentos->colas.exec))
    {
        if(pthread_mutex_trylock(&kernel_argumentos->planning_mutex) == 0)
        {
            planificador_planificar(kernel_argumentos);
        }
    }
}

void verificar_potencial_envio(t_planificacion* kernel_argumentos, t_queue_block* interfaz)
{
    if(queue_is_empty(interfaz->block_queue))
    {
        return;
    }

    t_pcb* pcb = queue_peek(interfaz->block_queue);
    char* pid = string_itoa(pcb->pid);

    t_instruccion_params_opcode* parametros = dictionary_get(kernel_argumentos->parametros_en_espera, pid);

    enviar_instruccion_a_interfaz(interfaz, parametros->params, parametros->opcode, pcb->pid);
    log_debug(kernel_argumentos->log_aux, "Se envia una solicitud de instruccion de un proceso que estaba en espera (%d), a la interfaz: %s", pcb->pid, interfaz->identificador);

    free(pid);
    free(parametros);
}

// -------- RECEPCION DE INTERFACES --------


// ------- TRANSICIONES DE ESTADOS --------
t_pcb *planificador_ready_a_exec(t_planificacion *kernel_argumentos)
{
    
    pthread_mutex_lock(&kernel_argumentos->colas.mutex_ready);
    t_pcb *proximo_proceso = queue_pop(kernel_argumentos->colas.ready);
    pthread_mutex_unlock(&kernel_argumentos->colas.mutex_ready);

    proximo_proceso->estado = EXEC;
    pthread_mutex_lock(&kernel_argumentos->colas.mutex_exec);
    queue_push(kernel_argumentos->colas.exec, proximo_proceso);
    pthread_mutex_unlock(&kernel_argumentos->colas.mutex_exec);

    log_info(kernel_argumentos->logger, "PID: %d - Estado anterior: READY - Estado actual: EXEC", proximo_proceso->pid);

    return proximo_proceso;
}

t_pcb *planificador_prioridad_a_exec(t_planificacion *kernel_argumentos)
{
    pthread_mutex_lock(&kernel_argumentos->colas.mutex_prioridad);
    t_pcb *proximo_pcb = queue_pop(kernel_argumentos->colas.prioridad);
    pthread_mutex_unlock(&kernel_argumentos->colas.mutex_prioridad);
    
    proximo_pcb->estado = EXEC;
    pthread_mutex_lock(&kernel_argumentos->colas.mutex_exec);
    queue_push(kernel_argumentos->colas.exec, proximo_pcb);
    pthread_mutex_unlock(&kernel_argumentos->colas.mutex_exec);
    
    log_info(kernel_argumentos->logger, "PID %d - Estado anterior: READY - Estado actual: EXEC", proximo_pcb->pid);
    
    return proximo_pcb;
}

void mover_a_exit(t_pcb* pcb_desalojado, t_planificacion *kernel_argumentos)
{
    t_proceso_estado aux = pcb_desalojado->estado;
    pcb_desalojado->estado = EXIT;

    pthread_mutex_lock(&kernel_argumentos->colas.mutex_exit);
    queue_push(kernel_argumentos->colas.exit, pcb_desalojado);
    pthread_mutex_unlock(&kernel_argumentos->colas.mutex_exit);

    // eliminar_recursos_afectados(pcb_desalojado, kernel_argumentos);
    solicitar_finalizacion_a_memoria(pcb_desalojado->pid, kernel_argumentos->socket_memoria);

    char* estado = proceso_estado_a_string(aux);
    log_info(kernel_argumentos->logger, "PID: %d - Estado anterior: %s - Estado actual: EXIT", pcb_desalojado->pid, estado);

    free(pcb_desalojado->registros);
    free(pcb_desalojado);
}

void solicitar_finalizacion_a_memoria(uint32_t pid, int socket_memoria)
{
    char* mensaje = string_itoa(pid);

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = FINALIZACION;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

    int resultado_send = send(socket_memoria, a_enviar, bytes, MSG_NOSIGNAL);  // Evita la generación de SIGPIPE

    if (resultado_send == -1) {
        fprintf(stderr, "Error al enviar el mensaje: socket cerrado.\n");
    }

    free(a_enviar);
    free(mensaje);
    eliminar_paquete(paquete);
}

char* proceso_estado_a_string(t_proceso_estado estado)
{
    if(estado == READY)
    {
        return "READY";
    }
    if(estado == NEW)
    {
        return "NEW";
    }
    if(estado == EXEC)
    {
        return "EXEC";
    }
    if(estado == BLOCKED)
    {
        return "BLOCK";
    }
    if(estado == EXIT)
    {
        return "EXIT";
    }
}

void mover_a_ready(t_pcb* pcb, t_planificacion* kernel_argumentos)
{
    t_proceso_estado aux = pcb->estado;
    pcb->estado = READY;
    pthread_mutex_lock(&kernel_argumentos->colas.mutex_ready);
    queue_push(kernel_argumentos->colas.ready, pcb);
    pthread_mutex_unlock(&kernel_argumentos->colas.mutex_ready);

    char* estado_anterior = proceso_estado_a_string(aux);
    log_info(kernel_argumentos->logger, "PID: %d - Estado anterior: %s - Estado actual: READY", pcb->pid, estado_anterior);

    logear_cola_ready(kernel_argumentos);
}

void mover_a_prioridad(t_pcb* pcb, t_planificacion* kernel_argumentos)
{
    t_proceso_estado aux = pcb->estado;
    pcb->estado = READY;
    pthread_mutex_lock(&kernel_argumentos->colas.mutex_prioridad);
    queue_push(kernel_argumentos->colas.prioridad, pcb);
    pthread_mutex_unlock(&kernel_argumentos->colas.mutex_prioridad);

    char* estado_anterior = proceso_estado_a_string(aux);
    log_info(kernel_argumentos->logger, "PID: %d - Estado anterior: %s - Estado actual: READY", pcb->pid, estado_anterior);

    logear_cola_prioridad(kernel_argumentos);
}

void logear_cola_ready(t_planificacion* kernel_argumentos)
{
    pthread_mutex_lock(&kernel_argumentos->colas.mutex_ready);
    int i = 0, tamanio = queue_size(kernel_argumentos->colas.ready);
    char* lista = string_new();
    while(i<tamanio)
    {
        t_pcb *pcb = queue_pop(kernel_argumentos->colas.ready);
        char* pid = string_itoa(pcb->pid);
        string_append(&lista, pid);
        if(i != tamanio - 1)
        {
            string_append(&lista, ", ");
        }
        queue_push(kernel_argumentos->colas.ready, pcb);
        free(pid);
        i++;
    }
    pthread_mutex_unlock(&kernel_argumentos->colas.mutex_ready);

    log_info(kernel_argumentos->logger, "Cola Ready: [%s]", lista);
    free(lista);
}

void logear_cola_prioridad(t_planificacion* kernel_argumentos)
{
    char* lista_ready = string_new();
    pthread_mutex_lock(&kernel_argumentos->colas.mutex_prioridad);
    int tamanio = queue_size(kernel_argumentos->colas.prioridad);
    int i = 0;
    while(i<tamanio)
    {            
        t_pcb* pcb = queue_pop(kernel_argumentos->colas.prioridad);
        char* pid = string_itoa(pcb->pid);
        string_append(&lista_ready, pid);
        if(i != tamanio - 1)
        {
            string_append(&lista_ready, ", ");
        }
        free(pid);
        queue_push(kernel_argumentos->colas.prioridad, pcb);
        i++;
    }
    pthread_mutex_unlock(&kernel_argumentos->colas.mutex_prioridad);

    log_info(kernel_argumentos->logger, "Cola Ready Prioridad: [%s]", lista_ready);
    free(lista_ready);
}
