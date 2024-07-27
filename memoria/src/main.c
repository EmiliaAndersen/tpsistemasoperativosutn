#include "main.h"

t_log* memoria_log;
t_config* memoria_config;
sem_t semaforo_mem;
t_log* log_aux_mem;

int main(void) {
    
    char* puerto;
    int memoria_server;
 
    log_aux_mem = iniciar_logger("memoria_aux.log","memoria_aux");
    memoria_log = iniciar_logger("memoria.log","memoria");
    memoria_config = iniciar_config("memoria.config");


    /* MEMORIA - Server */

    // Extraer configs

    tam_memoria=config_get_int_value(memoria_config, "TAM_MEMORIA");
    tam_pagina=config_get_int_value(memoria_config, "TAM_PAGINA");
    puerto = config_get_string_value(memoria_config ,"PUERTO_ESCUCHA");

    // INICIALIZO MEMORIA //
    memoria = malloc(tam_memoria);
    //INICIALIZO BITARRAY//
    size_t num_frames = tam_memoria / tam_pagina; 
    char* bitarray_data = malloc(num_frames); 
    bitarray = bitarray_create_with_mode(bitarray_data, num_frames, LSB_FIRST);
    
    for(int i=0; i<num_frames; i++){
        bitarray_clean_bit(bitarray, i); 
    }
    //INICIALIZO BITARRAY ESCRITO//
    /*char* escrito_data = malloc(num_frames); 
    escrito = bitarray_create_with_mode(escrito_data, num_frames, LSB_FIRST);
    
    for(int i=0; i<num_frames; i++){
        bitarray_clean_bit(escrito, i); 
    }*/

    //TABLA//
    tabla_pags = list_create();
    pthread_mutex_init(&mutex_tabla_pags, NULL);

    // Inicio server 

    memoria_server = iniciar_servidor(puerto, memoria_log);
    log_info(log_aux_mem, "MEMORIA lista para recibir clientes");
    sem_init(&semaforo_mem, 0, 0);

    t_atender_cliente_args* args = malloc(sizeof(t_atender_cliente_args));
    args->log = memoria_log;
    args->c_socket = memoria_server;
    args->server_name = "memoria"; 

    pthread_t hilo;
    pthread_create(&hilo, NULL, (void *)server_escuchar, args);


    pthread_join(hilo,NULL);
     
    log_destroy(log_aux_mem);
    return 0;
}