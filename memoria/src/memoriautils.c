#include "memoriautils.h"

int tam_pagina;
int tam_memoria;
t_list* tabla_pags; //tabla general
void* memoria;
t_bitarray* bitarray;
//t_bitarray* escrito;
pthread_mutex_t mutex_tabla_pags;


void escribir_en_mem_io(char* aescribir, t_dir_fisica* dir_fisica, int cant_direcciones,int tamanio, uint32_t pid ){
int desplazamiento = dir_fisica[0].desplazamiento;
    int tam_primera = tam_pagina - desplazamiento;

    int bytes_escritos = 0;
    int offset = 0;

    if (puede_escribir(pid, dir_fisica[0].nro_frame, cant_direcciones)) {
        for (int i = 0; i < cant_direcciones; i++) {
            int nro_frame = dir_fisica[i].nro_frame;
            int bytes_a_escribir = tam_pagina;

            if (i == 0) {
                bytes_a_escribir = tam_primera;
            } else if (i == cant_direcciones - 1) {
                bytes_a_escribir = tamanio - bytes_escritos;
            }

            if (bytes_a_escribir > tamanio - bytes_escritos) {
                bytes_a_escribir = tamanio - bytes_escritos;
            }

            if (bytes_a_escribir > 0) {
                memcpy((char*)memoria + (nro_frame * tam_pagina) + (i == 0 ? desplazamiento : 0), aescribir + offset, bytes_a_escribir);
                //bitarray_set_bit(escrito, nro_frame);

                bytes_escritos += bytes_a_escribir;
                offset += bytes_a_escribir;

                if (bytes_escritos >= tamanio) {
                    break;
                }
            }
        }

        log_info(memoria_log, "PID: <%u> - Accion: <ESCRIBIR> - Direccion fisica: <%d> - Tamaño: <%d>", pid, dir_fisica[0].nro_frame * tam_pagina + desplazamiento, tamanio);
    } else {
        log_error(memoria_log, "No puede escribir");
    }
}


void escribir_en_mem_cpu(char* aescribir, t_dir_fisica* dir_fisica, int tamanio , uint32_t pid){
    
    int tam_bytes = tamanio;
    int nro_frame = dir_fisica->nro_frame;
    int desplazamiento = dir_fisica->desplazamiento;

    if(puede_escribir(pid, nro_frame, 1)){
    if (tamanio == 1){ //uint8
        uint8_t escribir = (uint8_t) atoi(aescribir);
        memcpy((char*)memoria + (nro_frame * tam_pagina) + desplazamiento , &escribir, tam_bytes); 
        //bitarray_set_bit(escrito, nro_frame);
        log_info(memoria_log, "PID: <%u> - Accion: <ESCRIBIR> - Direccion fisica: <%d> - Tamaño <%d>",pid ,nro_frame+desplazamiento,tam_bytes);

    }
    if(tamanio ==4){//uint32
        uint32_t escribir = (uint32_t)atoi(aescribir);
        memcpy((char*)memoria + (nro_frame * tam_pagina) + desplazamiento , &escribir, tam_bytes);
        log_info(memoria_log, "PID: <%u> - Accion: <ESCRIBIR> - Direccion fisica: <%d> - Tamaño: <%d>",pid ,nro_frame+desplazamiento,tam_bytes);
       // bitarray_set_bit(escrito, nro_frame);

    }
    if(tamanio ==2){//uint32
        uint16_t escribir = (uint16_t)atoi(aescribir);
        memcpy((char*)memoria + (nro_frame * tam_pagina) + desplazamiento , &escribir, tam_bytes);
        log_info(memoria_log, "PID: <%u> - Accion: <ESCRIBIR> - Direccion fisica: <%d> - Tamaño: <%d>",pid ,nro_frame+desplazamiento,tam_bytes);
        //bitarray_set_bit(escrito, nro_frame);

    }
    }
    else
    {
        log_error(memoria_log, "No puede escribir"); 
    }
}

char* leer_en_mem_cpu(int tamanio, t_dir_fisica* dir_fisica, uint32_t pid){

    int nro_frame = dir_fisica->nro_frame;
    int desplazamiento = dir_fisica->desplazamiento;

    void* espacio_de_mem = (char*)memoria + (nro_frame * tam_pagina) + desplazamiento;

    int tam_primera=tam_pagina - desplazamiento;
    char* leido;
    if (tamanio == 1){
        uint8_t leo;
        memcpy(&leo, espacio_de_mem, tamanio);
        leido = int_to_char(leo);
         log_info(memoria_log, "PID: <%u> - Accion: <LEER> - Direccion fisica: <%d> - Tamaño: <%d>",pid ,nro_frame+desplazamiento,tamanio);
    }
    if(tamanio ==4){
       
        uint32_t leo;
        memcpy(&leo, espacio_de_mem, tamanio);
        leido = int_to_char(leo);
         log_info(memoria_log, "PID: <%u> - Accion: <LEER> - Direccion fisica: <%d> - Tamaño: <%d>",pid ,nro_frame+desplazamiento,tamanio);
    }
    if(tamanio ==2){

        uint16_t leo;
        memcpy(&leo, espacio_de_mem, tamanio);
        leido = int_to_char(leo);
        log_info(memoria_log, "PID: <%u> - Accion: <LEER> - Direccion fisica: <%d> - Tamaño: <%d>",pid ,nro_frame+desplazamiento,tamanio);
    }
    
    return leido;
}


char* leer_en_mem_io(int tamanio, t_dir_fisica* dir_fisica, int cant_direcciones, uint32_t pid) {
    int desplazamiento = dir_fisica[0].desplazamiento;
    int tam_primera = tam_pagina - desplazamiento;

    char* leido = malloc(tamanio);
    leido[0] = '\0'; // Inicializar la cadena

    int bytes_leidos = 0;
    int offset = 0;

    for (int i = 0; i < cant_direcciones; i++) {
        int nro_frame = dir_fisica[i].nro_frame;
        int bytes_a_leer = tam_pagina;

        if (i == 0) {
            bytes_a_leer = tam_primera;
        } else if (i == cant_direcciones - 1) {
            bytes_a_leer = tamanio - bytes_leidos;
        }

        if (bytes_a_leer > tamanio - bytes_leidos) {
            bytes_a_leer = tamanio - bytes_leidos;
        }

        if (bytes_a_leer > 0) {
            char* temp = malloc(bytes_a_leer + 1);
            memcpy(temp, (char*)memoria + (nro_frame * tam_pagina) + (i == 0 ? desplazamiento : 0), bytes_a_leer);
            temp[bytes_a_leer] = '\0';
            string_append(&leido, temp);
            free(temp);
    
            bytes_leidos += bytes_a_leer;
            offset += bytes_a_leer;


            if (bytes_leidos >= tamanio) {
                break;
            }
        }

        log_info(memoria_log, "PID: <%u> - Accion: <LEER> - Direccion fisica: <%d> - Tamaño: <%d>",pid ,nro_frame  + (i == 0 ? desplazamiento : 0),bytes_a_leer);
    }

    return leido;
}



int frame_sig_disp( uint32_t pid, int frame){
    int frame_siguiente_disp=-1;
    t_tabla* tabla_pid = buscar_por_pid_return(pid);
    bool encontrado = false;

    for (int i = frame+1; i < bitarray->size; i++) {
        if (bitarray_test_bit(bitarray, i) == 1 /*&& bitarray_test_bit(escrito, i) == 0*/) {
           

            for(int x = 0; x< list_size(tabla_pid->tabla); x++){
               
                if(list_get(tabla_pid->tabla,x) == i){
                    frame_siguiente_disp = i;
                    encontrado = true;
                    break;
                }  
            }
        }
        if (encontrado) {
            break;
            }
    }
    if(frame_siguiente_disp==-1){ // si hay libres pero por arriba del frame
    for (int i = 0; i < frame; i++) {
        if (bitarray_test_bit(bitarray, i) == 1 /*&& bitarray_test_bit(escrito, i) == 0*/) {
            for(int x = 0; x< list_size(tabla_pid->tabla); x++){
                if(list_get(tabla_pid->tabla,x) == i){
                    frame_siguiente_disp = i;
                    encontrado = true;
                    break;
                }  
                    }
                }
            if (encontrado) {
                break;
            }
    }
    }
    return frame_siguiente_disp;
}

int frame_sig_leer( uint32_t pid, int frame){
    int frame_siguiente=-1;
    t_tabla* tabla_pid = buscar_por_pid_return(pid);
    bool encontrado = false;
    for (int i = frame+1; i < bitarray->size; i++) {
        if (bitarray_test_bit(bitarray, i) == 1 /*&& bitarray_test_bit(escrito, i)==1*/) {
            for(int x = 0; x< list_size(tabla_pid->tabla); x++){
                if(list_get(tabla_pid->tabla,x) == i){
                    frame_siguiente = i;
                    encontrado = true;
                    break;
                }  
            }
        }
        if (encontrado) {
                break;
        }
    }
    if(frame_siguiente==-1){ // si hay libres pero por arriba del frame
    for (int i = 0; i < frame; i++) {
        if (bitarray_test_bit(bitarray, i) == 1 /*&& bitarray_test_bit(escrito, i)==1*/) {
            for(int x = 0; x< list_size(tabla_pid->tabla); x++){
                if(list_get(tabla_pid->tabla,x) == i){
                    frame_siguiente = i;
                    encontrado = true;
                    break;
                }  
                    }
                }
            if (encontrado) {
                break;
            }
    }
    }

    return frame_siguiente;
}

bool puede_escribir(uint32_t pid, int frame,int cant_pags ){
    
    t_tabla* tabla_pid = buscar_por_pid_return(pid);
    t_list* tabla_paginas_pid = tabla_pid->tabla;
    
    bool puede = false; 

    bool encontrado = false;

    //FRAME PERTENCE AL PID
    for(int i=0; i< list_size(tabla_paginas_pid); i++){
        int frame_encontrado = list_get(tabla_paginas_pid, i);
        if(frame_encontrado == frame){
            encontrado=true;
        }
    }


    //FRAME ESTA DISPONIBLE EN BIT ARRAY
    bool disponible = false;
    if( bitarray_test_bit(bitarray, frame)==1){//bitarray_test_bit(escrito, frame)==0 &&
        disponible=true;
    }
    //VER SI TIENE CANT PAGS NECESARIAS
    int cont=0;
    if(cant_pags != 1){
    
    for (int i = 0; i < bitarray->size; i++) {
        if (bitarray_test_bit(bitarray, i) == 1 ) {
            for(int x = 0; x< list_size(tabla_pid->tabla); x++){
                if(list_get(tabla_pid->tabla,x) == i){
                   cont++;
                }  
                    }
                }
    }
    }
    if(cant_pags == 1){
        cont = cant_pags;
    }
    if(disponible && encontrado && (cant_pags <= cont)){
        puede = true;
    }

    return puede;
}


t_tabla* buscar_por_pid_return(uint32_t pid) {

    pthread_mutex_lock(&mutex_tabla_pags);
    if(tabla_pags != NULL){
    t_link_element* current = tabla_pags->head;
    while (current != NULL) {
        t_tabla* current_tabla = (t_tabla*)current->data;
        if (current_tabla->pid == pid) {
            pthread_mutex_unlock(&mutex_tabla_pags);
            return current_tabla; 
        }
        current = current->next;
    }
    }
    pthread_mutex_unlock(&mutex_tabla_pags);
    return NULL; 
}
bool buscar_por_pid_bool(uint32_t pid) {
    pthread_mutex_lock(&mutex_tabla_pags);
    t_link_element* current = tabla_pags->head;
    while (current != NULL) {
        t_tabla* current_tabla = (t_tabla*)current->data;
        if (current_tabla->pid == pid) {
            pthread_mutex_unlock(&mutex_tabla_pags);
            return true;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&mutex_tabla_pags);
    return false; 
}

t_tabla* buscar_por_pid(uint32_t pid)
{
    pthread_mutex_lock(&mutex_tabla_pags);
    int i = 0, tamanio = list_size(tabla_pags);
    t_tabla* ret = NULL;
    while(i<tamanio)
    {
        t_tabla* candidato = list_remove(tabla_pags, 0);
        if(candidato != NULL && candidato->pid == pid)
        {
            ret = candidato;
        }
        list_add(tabla_pags, candidato);
        i++;
    }
    pthread_mutex_unlock(&mutex_tabla_pags);
    return ret;
}

t_tabla* eliminar_tabla_pid(uint32_t pid)
{
    pthread_mutex_lock(&mutex_tabla_pags);
    int i = 0, tamanio = list_size(tabla_pags);
    t_tabla* ret = NULL;
    while(i<tamanio)
    {
        t_tabla* candidato = list_remove(tabla_pags, 0);
        if(candidato->pid == pid)
        {
            ret = candidato;
        }
        else
        {
            list_add(tabla_pags, candidato);
        }
        i++;
    }
    pthread_mutex_unlock(&mutex_tabla_pags);
    return ret;
}

int buscar_frame_disp(t_bitarray* bit, int tam ){

    bool encontre = false;
    int i = 0;
    
    while((i<tam) && (!encontre)){
        if(bitarray_test_bit(bit,i)==0){
            encontre=true;
        }
        else{
            i++;
        }
        
    }
    return i; 
}



