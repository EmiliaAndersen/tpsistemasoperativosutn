#include "instrucciones_memoria.h"


t_list* abrir_pseudocodigo(char* path){

    char* path_instrucciones = config_get_string_value(memoria_config ,"PATH_INSTRUCCIONES");
    size_t total_length = strlen(path_instrucciones) + strlen(path) + 2; // 1 para el '/' y 1 para el '\0'
    char* path_completo = malloc(total_length);
    snprintf(path_completo, total_length, "%s%s", path_instrucciones, path);

    log_info(memoria_log, path_completo);
    FILE* arch_pseudocodigo = fopen(path_completo, "r");
        if(arch_pseudocodigo == NULL){
            log_error(memoria_log, "No se pudo abrir el archivo.\n");
            return NULL;
        }
    free(path_completo);

    t_list* lista_inst= list_create();
    char instruccionlinea[50]; //Ver 

    int cont = 0;
        while (fgets(instruccionlinea, sizeof(instruccionlinea), arch_pseudocodigo) != NULL) {
            
            size_t longitud = strlen(instruccionlinea);
            char* instruccion = strdup(instruccionlinea);
            list_add_in_index(lista_inst,cont,instruccion);
            cont++;
            log_info(log_aux_mem,"Contenido: %s", instruccion);
            if(strcmp(strdup(instruccionlinea),"EXIT") == 0){
                 break;  
            }
        }   
    

    log_info(log_aux_mem,"Cerrando archivo pseudocodigo...");
    fclose(arch_pseudocodigo);
    return lista_inst;
}
