#include "consola.h"

void consola_interactiva(t_planificacion *kernel_argumentos){
   pthread_t consola;
   
    int resultado_creacion = pthread_create(&consola, NULL, leer_consola, (void*)kernel_argumentos);
    if (resultado_creacion != 0) {
        fprintf(stderr, "Error: No se pudo crear el hilo. Código de error: %d\n", resultado_creacion);
    }
    pthread_detach(consola);
}

void* leer_consola(void* args) 
{
    t_planificacion *kernel_arguementos = (t_planificacion*) args;
    char* leido;
    while (1) {  
        leido = readline("> ");
    if (string_is_empty(leido)) {
        break; 
        }
    funciones(leido, kernel_arguementos);
    free(leido);
    }
    pthread_exit(NULL);
}

void funciones(char* leido, t_planificacion *kernel_argumentos) {
    char** funcion;
    funcion = string_split(leido, " ");
    if (string_equals_ignore_case(funcion[0], "EJECUTAR_SCRIPT") && string_array_size(funcion) == 2){
        ejecutar_script(funcion[1], kernel_argumentos);
    } else if (string_equals_ignore_case(funcion[0], "INICIAR_PROCESO") && string_array_size(funcion) == 2) {
        iniciar_proceso(funcion[1], kernel_argumentos);
    } else if (string_equals_ignore_case(funcion[0], "FINALIZAR_PROCESO") && string_array_size(funcion) == 2) {
        finalizar_proceso(atoi(funcion[1]), kernel_argumentos);
    } else if (string_equals_ignore_case(funcion[0], "INICIAR_PLANIFICACION")) {
        iniciar_planificacion(kernel_argumentos);
    } else if (string_equals_ignore_case(funcion[0], "DETENER_PLANIFICACION")) {
        detener_planificacion(kernel_argumentos);
    } else if (string_equals_ignore_case(funcion[0], "MULTIPROGRAMACION") && string_array_size(funcion) == 2) {
        multiprogramacion(funcion[1], kernel_argumentos);
    } else if (string_equals_ignore_case(funcion[0], "PROCESO_ESTADO")) {
        proceso_estado(kernel_argumentos);
    } else if (!string_is_empty(leido)){
        log_error(kernel_argumentos->log_aux, ">> COMANDO ERRONEO!");
    }
    string_array_destroy(funcion);
}

void ejecutar_script(char* path, t_planificacion *kernel_argumentos){
    char* complemento = "/home/utnso/c-comenta-pruebas"; 
    size_t len_path = strlen(path);
    size_t len_complemento = strlen(complemento);
    size_t len_total = len_path + len_complemento + 2;
    char* ruta_completa = malloc(len_total);
    if (ruta_completa == NULL) {
        log_error(kernel_argumentos->log_aux, "Error: No se pudo asignar memoria para la ruta completa.");
        free(ruta_completa);
        return;
    }
    strcpy(ruta_completa, complemento);
    strcat(ruta_completa, path);

    // log_debug(kernel_argumentos->log_aux, ">> Se ejecuta el script %s", path);
    log_debug(kernel_argumentos->log_aux, ">> Se ejecuta el script %s", ruta_completa);

    FILE* file = fopen(ruta_completa, "r");
    if (file == NULL) {
        log_error(kernel_argumentos->log_aux, "No se pudo abrir el archivo: %s", path);
        free(ruta_completa);
        return;
    }

    char* linea = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&linea, &len, file)) != -1) {
        linea[strcspn(linea, "\n")] = 0;
     /*   // Ignorar líneas vacías
        if (strlen(linea) == 0) {
            continue;
        }*/

        log_debug(kernel_argumentos->log_aux, ">> Ejecutando comando: %s", linea);
        funciones(linea, kernel_argumentos);
    }

    free(linea);
    fclose(file);
    free(ruta_completa);
    log_debug(kernel_argumentos->log_aux, ">> Finalizó la ejecución del script %s", path);
}

void iniciar_proceso(char* path, t_planificacion *kernel_argumentos){

    t_pcb *pcb;

    static uint32_t pid_contador = 0;

    pcb = crear_nuevo_pcb(&pid_contador, kernel_argumentos);
    queue_push(kernel_argumentos->colas.new, pcb);

    log_info(kernel_argumentos->logger, "Se crea el proceso %s en NEW", path);

    char* mensaje_enviar = malloc(sizeof(uint32_t)+strlen(path)+2);
    uint32_t pid = pcb->pid;
    // sprintf(mensaje_enviar, "%u/%s", pid,path);  

    char* pid_str = int_to_char(pid);

    strcpy(mensaje_enviar, path);
    strcat(mensaje_enviar, "$");
    strcat(mensaje_enviar, pid_str);
    enviar_mensaje(mensaje_enviar, kernel_argumentos->socket_memoria);

    free(pid_str);
    free(mensaje_enviar);
    /*char *pid_str = int_to_char(pcb->pid);
    size_t len = strlen(path) + strlen(pid_str) + 2;
    char *pathpid = malloc(len);
    strcpy(pathpid, path);
    strcat(pathpid, "$");
    strcat(pathpid, pid_str);
    enviar_mensaje(pathpid, kernel_argumentos->socket_memoria);
    free(pid_str);
    free(pathpid);

    */

}

void finalizar_proceso(uint32_t pid, t_planificacion *kernel_argumentos){

    t_pcb* pcb_candidato;
    t_pcb* pcb_a_eliminar = NULL;
    pthread_mutex_lock(&kernel_argumentos->planning_mutex);
    // log_warning(kernel_argumentos->log_aux, "Se bloquea la planificacion");

    log_debug(kernel_argumentos->log_aux, "Busco en EXEC");
    if(!queue_is_empty(kernel_argumentos->colas.exec))
    {
        pcb_candidato = queue_peek(kernel_argumentos->colas.exec);
        if(pcb_candidato->pid == pid)
        {
            log_debug(kernel_argumentos->log_aux, "Encontre el PID: %d en EXEC", pcb_candidato->pid);
            pcb_a_eliminar = queue_pop(kernel_argumentos->colas.exec);
            eliminar_proceso(pcb_a_eliminar, kernel_argumentos);
            pthread_mutex_unlock(&kernel_argumentos->planning_mutex);
            // log_debug(kernel_argumentos->logger, "Sali del mutex");
            enviar_int_a_interrupt(kernel_argumentos->socket_cpu_interrupt, pid);
            log_debug(kernel_argumentos->log_aux, "Interrupcion enviada al PID: %d", pid);
            return;
        }
    }

    log_debug(kernel_argumentos->log_aux, "Busco en READY");
    pcb_a_eliminar = buscar_pcb_en_cola(kernel_argumentos->colas.ready, pid);
    if(pcb_a_eliminar != NULL)
    {
        log_debug(kernel_argumentos->log_aux, "Encontre el PID: %d en READY", pcb_a_eliminar->pid);
        eliminar_proceso(pcb_a_eliminar, kernel_argumentos);
        pthread_mutex_unlock(&kernel_argumentos->planning_mutex);
        return;
    }

    log_debug(kernel_argumentos->log_aux, "Busco en PRIORIDAD");
    pcb_a_eliminar = buscar_pcb_en_cola(kernel_argumentos->colas.prioridad, pid);
    if(pcb_a_eliminar != NULL)
    {
        log_debug(kernel_argumentos->log_aux, "Encontre el PID: %d en PRIORIDAD", pcb_a_eliminar->pid);
        eliminar_proceso(pcb_a_eliminar, kernel_argumentos);
        pthread_mutex_unlock(&kernel_argumentos->planning_mutex);
        return;
    }

    log_debug(kernel_argumentos->log_aux, "Busco en NEW");
    pcb_a_eliminar = buscar_pcb_en_cola(kernel_argumentos->colas.new, pid);
    if(pcb_a_eliminar != NULL)
    {
        log_debug(kernel_argumentos->log_aux, "Encontre el PID: %d en NEW", pcb_a_eliminar->pid);
        eliminar_proceso(pcb_a_eliminar, kernel_argumentos);
        pthread_mutex_unlock(&kernel_argumentos->planning_mutex);
        return;
    }

    log_debug(kernel_argumentos->log_aux, "Busco en las colas de bloqueo");

    t_list* colas_bloqueo = dictionary_elements(kernel_argumentos->colas.lista_block);
    int tamanio_lista = list_size(colas_bloqueo);
    log_debug(kernel_argumentos->log_aux, "Tamanio de la lista: %d", tamanio_lista);
    int i = 0;
    int cola;
    char* nombre_recurso;
    while(i<tamanio_lista)
    {
        t_queue_block* cola_bloqueo = list_remove(colas_bloqueo, 0);
        log_debug(kernel_argumentos->log_aux, "Busco en la lista de: %s", cola_bloqueo->identificador);
        if(!queue_is_empty(cola_bloqueo->block_queue))
        {
            pcb_candidato = buscar_pcb_en_cola(cola_bloqueo->block_queue, pid);
            cola = 1;
        }
        if(!list_is_empty(cola_bloqueo->block_dictionary))
        {
            pcb_candidato = buscar_pcb_en_lista(cola_bloqueo->block_dictionary, pid);
            if(pcb_candidato != NULL)
            {
                nombre_recurso = string_duplicate(cola_bloqueo->identificador);
                pcb_a_eliminar = pcb_candidato;
            }
            cola = 0;
        }
        // list_add(colas_bloqueo, cola_bloqueo);
        i++;
    }
    list_destroy(colas_bloqueo);

    if(pcb_a_eliminar != NULL)
    {
        if(cola == 1)
        {
            eliminar_proceso(pcb_a_eliminar, kernel_argumentos);
        }
        if(cola == 0)
        {
            eliminar_proceso_recurso(pcb_a_eliminar, nombre_recurso, kernel_argumentos);
        }
        pthread_mutex_unlock(&kernel_argumentos->planning_mutex);
        // log_warning(kernel_argumentos->logger, "Se desbloquea la planificacion");
        return;
    }

    log_debug(kernel_argumentos->log_aux, "No se encontro el proceso de PID: %d", pid);
    pthread_mutex_unlock(&kernel_argumentos->planning_mutex);
    // log_warning(kernel_argumentos->logger, "Se desbloquea la planificacion");
}

t_pcb *buscar_pcb_en_cola(t_queue* cola, uint32_t pid)
{
    t_pcb* pcb_candidato;
    t_pcb* ret = NULL;
    int tamanio = queue_size(cola);
    int i = 0;
    while(i < tamanio)
    {
        pcb_candidato = queue_pop(cola);
        if(pcb_candidato->pid == pid)
        {
            ret = pcb_candidato;
        }
        else
        {
            queue_push(cola, pcb_candidato);
        }
        i++;
    }

    return ret;
}

t_pcb* buscar_pcb_en_lista(t_list* lista, uint32_t pid)
{
    t_pcb* ret = NULL;
    t_pcb* pcb_candidato;

    int i = 0;
    int tamanio = list_size(lista);
    while(i<tamanio)
    {
        pcb_candidato = list_remove(lista, 0);
        if(pcb_candidato->pid == pid)
        {
            ret = pcb_candidato;
        }
        else
        {
            list_add(lista, pcb_candidato);
        }
        i++;
    }

    return ret;
}

void eliminar_proceso(t_pcb* pcb, t_planificacion* kernel_argumentos)
{
    log_info(kernel_argumentos->logger, "Finaliza el proceso %d - Motivo: INTERRUPTED_BY_USER", pcb->pid);
    mover_a_exit(pcb, kernel_argumentos);
}

void eliminar_proceso_recurso(t_pcb* pcb, char* nombre_recurso, t_planificacion* kernel_argumentos)
{
    char* pid = string_itoa(pcb->pid);
    t_list* lista_recursos_tomados = dictionary_get(kernel_argumentos->recursos_tomados, pid);
    
    int i = 0, tamanio = list_size(lista_recursos_tomados);
    log_debug(kernel_argumentos->log_aux, "El proceso %s tenia tomados %d recursos", pid, tamanio);
    while(i<tamanio)
    {
        char* recurso = list_remove(lista_recursos_tomados, 0);

        t_queue_block* cola_recurso = dictionary_get(kernel_argumentos->colas.lista_block, recurso);
        if(cola_recurso != NULL)
        {
            cola_recurso->cantidad_instancias++;
            log_debug(kernel_argumentos->log_aux, "Instancias del recurso %s: %d", recurso, cola_recurso->cantidad_instancias);
            procesar_desbloqueo_factible(recurso, kernel_argumentos);
        }
        else
        {
            log_warning(kernel_argumentos->log_aux, "Obtuve un elemento t_queue_block nulo");
        }

        i++;
    }

    log_info(kernel_argumentos->logger, "Finaliza el proceso %d - Motivo: INTERRUPTED_BY_USER", pcb->pid);
    mover_a_exit(pcb, kernel_argumentos);
    free(nombre_recurso);
    free(pid);
}

// void borrar_recurso_eliminado_de_cola(t_queue_block* cola, char* pid)
// {
//     int i = 0, tamanio = list_size(cola->block_dictionary);

//     while(i < tamanio)
//     {
//         t_pcb* pcb
//     }

// }

void iniciar_planificacion(t_planificacion *kernel_argumentos){
    if(kernel_argumentos->detener_planificacion != 0)
    {
        kernel_argumentos->detener_planificacion = 0;
        sem_post(&kernel_argumentos->planificar);
        log_debug(kernel_argumentos->log_aux, ">> Se inicia la planificacion...");
        return;
    }
    log_debug(kernel_argumentos->log_aux, ">> La planificacion ya estaba iniciada.");
}

void detener_planificacion(t_planificacion *kernel_argumentos){
    kernel_argumentos->detener_planificacion = 1;
    log_debug(kernel_argumentos->logger, ">> Se detiene la planificacion");
}

void multiprogramacion(char* valor, t_planificacion *kernel_argumentos){
    
    int valor_decimal = atoi(valor);
    kernel_argumentos->config.grado_multiprogramacion = valor_decimal;
    log_debug(kernel_argumentos->log_aux, "Se cambia el grado de multiprogramacion a %d", valor_decimal);
}

void proceso_estado(t_planificacion *kernel_argumentos){

    pthread_mutex_lock(&kernel_argumentos->planning_mutex);
    log_info(kernel_argumentos->logger, "Listado de procesos por estado:");

    logear_cola_estado(kernel_argumentos->colas.new, kernel_argumentos->colas.mutex_new, "NEW", kernel_argumentos->logger);
    logear_cola_estado(kernel_argumentos->colas.ready, kernel_argumentos->colas.mutex_ready, "READY", kernel_argumentos->logger);
    logear_cola_estado(kernel_argumentos->colas.prioridad, kernel_argumentos->colas.mutex_prioridad, "READY (Prioridad)", kernel_argumentos->logger);

    pthread_mutex_lock(&kernel_argumentos->colas.mutex_exec);
    if(!queue_is_empty(kernel_argumentos->colas.exec))
    {
        t_pcb* pcb = queue_peek(kernel_argumentos->colas.exec);
        log_info(kernel_argumentos->logger, "   EXEC: [ %d ]", pcb->pid);
    }
    pthread_mutex_unlock(&kernel_argumentos->colas.mutex_exec);

    logear_colas_block(kernel_argumentos);

    pthread_mutex_unlock(&kernel_argumentos->planning_mutex);
}

void logear_cola_estado(t_queue* cola, pthread_mutex_t mutex, char* nombre_cola, t_log* logger)
{
    pthread_mutex_lock(&mutex);
    if(!queue_is_empty(cola))
    {
        int i = 0, tamanio = queue_size(cola);
        char* lista = string_new();
        while(i < tamanio)
        {
            t_pcb* pcb = queue_pop(cola);
            char* pid = string_itoa(pcb->pid);
            string_append(&lista, pid);
            if(i != tamanio - 1)
            {
                string_append(&lista, ", ");
            }
            queue_push(cola, pcb);
            i++;
        }
        log_info(logger, "   %s: [%s]", nombre_cola, lista);
        free(lista);
    }
    pthread_mutex_unlock(&mutex);
}

void logear_colas_block(t_planificacion* kernel_argumentos)
{
    pthread_mutex_lock(&kernel_argumentos->colas.mutex_block);
    
    if(kernel_argumentos->colas.cantidad_procesos_block == 0)
    {
        pthread_mutex_unlock(&kernel_argumentos->colas.mutex_block);
        return;
    }

    char* cadena_general = string_new();
    t_list* lista_de_listas = dictionary_elements(kernel_argumentos->colas.lista_block);

    int i = 0, tamanio = list_size(lista_de_listas);
    bool ultimo = false;
    while(i < tamanio)
    {
        t_queue_block* elemento = list_get(lista_de_listas, i);
        if(i == tamanio - 1)
        {
            ultimo = true;
        }

        if(!queue_is_empty(elemento->block_queue))
        {
            int j = 0, k = queue_size(elemento->block_queue);
            while(j < k)
            {
                t_pcb* pcb = queue_pop(elemento->block_queue);
                char* pid = string_itoa(pcb->pid);
                string_append(&cadena_general, pid);
                if(!ultimo && (j != k - 1))
                {
                    string_append(&cadena_general, ", ");
                }
                queue_push(elemento->block_queue, pcb);
                j++;
            }
        }
        if(!list_is_empty(elemento->block_dictionary))
        {
            int j = 0, k = list_size(elemento->block_dictionary);
            while(j < k)
            {
                t_pcb* pcb = list_get(elemento->block_dictionary, j);
                char* pid = string_itoa(pcb->pid);
                string_append(&cadena_general, pid);
                if(!ultimo && (j != k - 1))
                {
                    string_append(&cadena_general, ", ");
                }
                j++;
            }
        }

        i++;
    }
    log_info(kernel_argumentos->logger, "   BLOCK: [%s]", cadena_general);
    free(cadena_general);
    pthread_mutex_unlock(&kernel_argumentos->colas.mutex_block);
}
