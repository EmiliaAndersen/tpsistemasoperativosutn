#include "server_kernel.h"


void atender_cliente(void *void_args)
{
    t_atender_cliente_args *args = (t_atender_cliente_args *)void_args;

    t_log *logger = args->log;
    int client_socket = args->c_socket;
    char *server_name = args->server_name;

    while (client_socket != -1)
    {   
        log_debug(args->planificador->log_aux, "Escuchando");
        op_code cop = recibir_operacion(client_socket);

        if (cop == -1)
        {

            t_queue_block* interfaz_desconectada = buscar_interfaz_por_socket(args->planificador, client_socket);
            if(interfaz_desconectada == NULL)
            {
                log_info(args->planificador->log_aux, "DISCONECT!");
                return;
            }
            cop = AVISO_DESCONEXION;

        }

        switch (cop) 
        {
        case PCB:
        {
            t_pcb *pcb_desalojado = recibir_pcb(client_socket);
            bool necesario_planificar = planificador_recepcion_pcb(pcb_desalojado, args->planificador);
            if(necesario_planificar)
            {
                planificador_planificar(args->planificador);
            }
            break;
        }
        case INTERFAZ:
        {   
            t_queue_block *new_client = malloc(sizeof(t_queue_block));
            new_client->identificador = recibir_interfaz(client_socket, args->planificador->log_aux);
            new_client->socket_interfaz = client_socket;
            new_client->block_queue = queue_create();
            new_client->block_dictionary = list_create();

            dictionary_put(args->planificador->colas.lista_block, new_client->identificador, new_client);
            log_debug(args->planificador->log_aux, "Se ha conectado la interfaz: %s", new_client->identificador);
            log_debug(args->planificador->log_aux, "Socket de la conexion: %d", new_client->socket_interfaz);

            break;
        }

        case AVISO_DESCONEXION:
        {

            t_queue_block* interfaz_desconectada = buscar_interfaz_por_socket(args->planificador, client_socket);

            if(interfaz_desconectada == NULL)
            {
                log_error(args->planificador->log_aux, "Se ha desconectado una interfaz desconocida");
            }

            log_debug(args->planificador->log_aux, "Se deconecta la interfaz: %s", interfaz_desconectada->identificador);
            eliminar_procesos_bloqueados_por_interfaz(interfaz_desconectada, args->planificador);
            queue_destroy(interfaz_desconectada->block_queue);
            list_destroy(interfaz_desconectada->block_dictionary);
            free(interfaz_desconectada->identificador);
            free(interfaz_desconectada);

            return;
         
        }

        case AVISO_OPERACION_INVALIDA:
        {
            char* nombre_interfaz = recibir_error_oi(client_socket); // TODO: Pedir a Zoe que esto devuelva el nombre de la interfaz
            log_debug(args->planificador->log_aux, "Se ha recibido una notificacion de operacion invalida");

            t_queue_block *interfaz = dictionary_get(args->planificador->colas.lista_block, nombre_interfaz);
            pcb_a_exit_por_sol_invalida(interfaz, args->planificador);

            break;
        }
        case AVISO_OPERACION_FINALIZADA:
        {
            char* interfaz_recibida = recibir_op_finalizada(client_socket);
            log_debug(args->planificador->log_aux, "Operacion finalizada por la interfaz: %s", interfaz_recibida);

            t_queue_block *interfaz = dictionary_get(args->planificador->colas.lista_block, interfaz_recibida);

            procesar_entradasalida_terminada(interfaz, args->planificador);
            free(interfaz_recibida);

            break;
        }
        default:
            log_error(args->planificador->log_aux, "Algo anduvo mal en el server de %s", server_name);
            log_info(args->planificador->log_aux, "Cop: %d", cop);
            break;
        }
    }

    free(args);
    //log_warning(logger, "El cliente se desconecto de %s server", server_name);  
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
            log_info(args->planificador->log_aux, "cree hilo");

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
/* PROTOCOLO */

t_queue_block* buscar_interfaz_por_socket(t_planificacion* kernel_argumentos, int socket)
{
    t_queue_block* ret = NULL;
    t_list* lista = dictionary_elements(kernel_argumentos->colas.lista_block);

    int i = 0, tamanio = list_size(lista);
    while(i < tamanio)
    {
        t_queue_block* candidato = list_remove(lista, 0);
        if(candidato->socket_interfaz == socket)
        {
            ret = candidato;
        }
        i++;
    }
    list_destroy(lista);
    
    return ret;
}

void eliminar_procesos_bloqueados_por_interfaz(t_queue_block* interfaz_desconectada, t_planificacion *kernel_argumentos)
{
    if(queue_is_empty(interfaz_desconectada->block_queue))
    {
        return;
    }
    int i = 0, tamanio = queue_size(interfaz_desconectada->block_queue);
    while(i<tamanio)
    {
        t_pcb* pcb_bloqueado = queue_pop(interfaz_desconectada->block_queue);
        mover_a_exit(pcb_bloqueado, kernel_argumentos);
        i++;
    }
}

