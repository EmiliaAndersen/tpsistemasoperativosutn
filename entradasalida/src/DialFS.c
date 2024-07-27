#include "DialFS.h"

t_list* lista_archivos;
t_bitarray* bitmap;
size_t bitmap_size;
char bitmap_path[256];
char blocks_path[256];
char lista_salida_path[256];

void inicio_filesystem() {

    /* Archivo de Bloques */
    
    snprintf(blocks_path, sizeof(blocks_path), "%s/bloques.dat", path_base_dialfs);
    int blocks_file = open(blocks_path, O_CREAT | O_RDWR, 0666);
    if (blocks_file < 0) {
        log_error(log_aux, "Error al crear bloques.dat");
        exit(EXIT_FAILURE);
    }

    off_t tamanio_archivo = (off_t) block_size * block_size;
    if (ftruncate(blocks_file, tamanio_archivo) < 0) {
        log_error(log_aux, "No se pudo establecer el tamanio del archivo");
        close(blocks_file);
        exit(EXIT_FAILURE);
    }
    close(blocks_file);

    /* Archivo de Bitmap */
    snprintf(bitmap_path, sizeof(bitmap_path), "%s/bitmap.dat", path_base_dialfs);
    struct stat st;
    int bitmap_file;

    if (stat(bitmap_path, &st) == 0) { // Si el archivo existe
        bitmap_file = open(bitmap_path, O_RDWR);
        if (bitmap_file < 0) {
            log_error(log_aux, "Error al abrir bitmap.dat");
            exit(EXIT_FAILURE);
        }
        bitmap_size = st.st_size;
    } else { // Si el archivo no existe, crea el bitmap inicial, crea el archivo y lo guarda
        bitmap_file = open(bitmap_path, O_CREAT | O_RDWR, 0666);
        if (bitmap_file < 0) {
            log_error(log_aux, "Error al abrir bitmap.dat");
            exit(EXIT_FAILURE);
        }

        bitmap_size = (block_count + 7) / 8; // Redondea hacia arriba los bytes
        char* bitmap_data = (char *)calloc(bitmap_size, sizeof(char)); // sizeof de un char es 1 byte
        if (bitmap_data == NULL) {
            log_error(log_aux, "Error al asignar memoria para bitmap_data");
            close(bitmap_file);
            exit(EXIT_FAILURE);
        }
        bitmap = bitarray_create_with_mode(bitmap_data, bitmap_size, LSB_FIRST);
        if (bitmap == NULL) {
            log_error(log_aux, "Error al crear bitmap");
            free(bitmap_data);
            close(bitmap_file);
            exit(EXIT_FAILURE);
        }


        ftruncate(bitmap_file, bitmap_size);

        char* bitmap_mmap = mmap(NULL, bitmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, bitmap_file, 0);
        if (bitmap_mmap == MAP_FAILED) {
            log_error(log_aux, "Error al mapear bitmap.dat");
            free(bitmap_data);
            bitarray_destroy(bitmap);
            close(bitmap_file);
            exit(EXIT_FAILURE);
        }

        memcpy(bitmap_mmap, bitmap_data, bitmap_size);
        munmap(bitmap_mmap, bitmap_size);
        free(bitmap_data);
    }

    // Mapear el archivo bitmap a memoria
    char* bitmap_mmap = mmap(NULL, bitmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, bitmap_file, 0);
    if (bitmap_mmap == MAP_FAILED) {
        log_error(log_aux, "Error al mapear bitmap.dat");
        close(bitmap_file);
        exit(EXIT_FAILURE);
    }
    if (bitmap != NULL) {
        bitarray_destroy(bitmap);
    }
    bitmap = bitarray_create_with_mode(bitmap_mmap, bitmap_size, LSB_FIRST);
    
    if (bitmap == NULL) {
        log_error(log_aux, "Error al crear bitmap con mmap");
        munmap(bitmap_mmap, bitmap_size);
        close(bitmap_file);
        exit(EXIT_FAILURE);
    }
    close(bitmap_file); 

    snprintf(lista_salida_path, sizeof(lista_salida_path), "%s/lista_archivos.txt", path_base_dialfs);
    struct stat lista_st;
    if (stat(lista_salida_path, &lista_st) == 0) {
        cargar_lista_archivos();
    } else { 
        lista_archivos = list_create();
    }
}

void cargar_lista_archivos(){
    lista_archivos = list_create();
    FILE* archivo_lista = fopen(lista_salida_path, "r");
    char nombre[256];
    char ruta[256];
    int comienzo;
    int tamanio;
    while (fscanf(archivo_lista, "%255s %255s %d %d", ruta, nombre, &comienzo, &tamanio) == 4) {
        Archivo* archivo = malloc(sizeof(Archivo));
        archivo->ruta = strdup(ruta);
        archivo->nombre = strdup(nombre);
        archivo->comienzo = comienzo;
        archivo->tamanio = tamanio;
        list_add(lista_archivos, archivo);
    }
    fclose(archivo_lista);
}

void crear_archivo(char* nombre){
    usleep(tiempo_unidad_trabajo * 1000);

    char new_file_path[256];
    snprintf(new_file_path, sizeof(new_file_path), "%s/%s", path_base_dialfs, nombre);
    int fd = open(new_file_path, O_CREAT | O_RDWR, 0666);
    close(fd);
    
    t_config* new_file_config;
    new_file_config = config_create(new_file_path);
    config_set_value(new_file_config, "TAMANIO_ARCHIVO", "0");
    
    uint32_t bloque_libre = buscar_bloque_libre();
    if (bloque_libre == -1) {
        log_debug(log_aux, "No hay bloques libres disponibles.");
        munmap(bitmap->bitarray, bitmap_size);
        exit(EXIT_FAILURE);
    }
    bitarray_set_bit(bitmap, bloque_libre);
    msync(bitmap->bitarray, bitmap_size, MS_SYNC); // Sincroniza los cambios del bitmap
    char* bloque_inicial_config = int_to_char(bloque_libre);
    config_set_value(new_file_config, "BLOQUE_INICIAL", bloque_inicial_config);
    Archivo* file = malloc(sizeof(Archivo));
    file->nombre = strdup(nombre);
    file->ruta = strdup(new_file_path);
    file->comienzo = bloque_libre;
    file->tamanio = 0;
    list_add(lista_archivos, file);
    config_save(new_file_config);
    config_destroy(new_file_config);
    free(bloque_inicial_config);
    guardar_lista_archivos();
}

void borrar_archivo(char* nombre){
    usleep(tiempo_unidad_trabajo * 1000);

    Archivo* archivo = buscar_archivo_por_nombre(nombre);
    t_config* file_config = config_create(archivo->ruta);
    
    int bloque_inicial = config_get_int_value(file_config, "BLOQUE_INICIAL");
    int tamanio_archivo = config_get_int_value(file_config, "TAMANIO_ARCHIVO");
    int cantidad_bloques = (tamanio_archivo + block_size - 1) / block_size; //Es para redondear hacia arriba
    int bloque_final;
    if(cantidad_bloques == 1) {
        bloque_final = bloque_inicial; 
    }else{ bloque_final = bloque_inicial + cantidad_bloques - 1;}
    
    for (int contador_bloques = bloque_inicial; contador_bloques <= bloque_final; contador_bloques++) {
        bitarray_clean_bit(bitmap, contador_bloques);
    }

    config_destroy(file_config);
    if (unlink(archivo->ruta) == 0) {
        log_debug(log_aux, "Archivo %s eliminado exitosamente.", nombre);
    } else {
        log_error(log_aux, "Error al eliminar el archivo");
    }
    FILE* archivo_bloques = fopen(blocks_path, "rb+");
    char* buffer = calloc(block_size, 1); //lleno de 0
    fseek(archivo_bloques, bloque_inicial * block_size, SEEK_SET);
    for (int i = 0; i < cantidad_bloques; i++) {
        fwrite(buffer, block_size, 1, archivo_bloques);
    }
    fclose(archivo_bloques);
    free(buffer);
    msync(bitmap->bitarray, bitmap_size, MS_SYNC);
    eliminar_archivo_de_lista(bloque_inicial);
    guardar_lista_archivos();
}

void truncar_archivo(char* nombre, uint32_t tamanio, uint32_t pid){
    usleep(tiempo_unidad_trabajo * 1000);
    
    Archivo* archivo = buscar_archivo_por_nombre(nombre);
    t_config* file_config = config_create(archivo->ruta);
    int bloque_inicial = config_get_int_value(file_config, "BLOQUE_INICIAL");
    int cantidad_bloques = (tamanio + block_size - 1) / block_size; //Es para redondear hacia arriba
    int bloque_final = bloque_inicial + cantidad_bloques - 1;

    int tamanio_anterior = config_get_int_value(file_config, "TAMANIO_ARCHIVO");
    int bloque_final_anterior;
    if(tamanio_anterior == 0) {
        bloque_final_anterior = bloque_inicial;
    }
    else{
        int bloques_ocupados_anteriormente = (tamanio_anterior + block_size - 1) / block_size;
        bloque_final_anterior = bloque_inicial + bloques_ocupados_anteriormente - 1;
    }
    
    if (bloque_final_anterior > bloque_final) { //Achicar
        for (int i = bloque_final + 1; i <= bloque_final_anterior; i++) {
            bitarray_clean_bit(bitmap, i);
        }
    }
    else if (bloque_final > bloque_final_anterior) { //Agrandar
        if (!bloques_contiguos_libres(bloque_final_anterior, bloque_final)){
            log_info(entradasalida_log, "PID: %i - Inicio Compactación.", pid);
            compactar(&bloque_inicial, bloque_final_anterior, archivo);
            log_info(entradasalida_log, "PID: %i - Fin Compactación.", pid);
            archivo->comienzo = bloque_inicial;
            char * bloque_inicial_config = int_to_char(bloque_inicial);
            config_set_value(file_config, "BLOQUE_INICIAL", bloque_inicial_config);
            free(bloque_inicial_config);
            bloque_final = bloque_inicial + cantidad_bloques - 1;
        }
        for (int i = bloque_inicial; i <= bloque_final; i++) {
            bitarray_set_bit(bitmap, i);
        }
    }
    char * tamanio_config = int_to_char(tamanio);
    config_set_value(file_config, "TAMANIO_ARCHIVO", tamanio_config);
    config_save(file_config);
    archivo->tamanio = tamanio;
    config_destroy(file_config);
    msync(bitmap->bitarray, bitmap_size, MS_SYNC);
    guardar_lista_archivos();
    free(tamanio_config);
}

void escribir_archivo(char* nombre, off_t puntero, char* a_escribir, uint32_t tamanio){
    usleep(tiempo_unidad_trabajo * 1000);

    Archivo* archivo = buscar_archivo_por_nombre(nombre);
    if (puntero + tamanio > archivo->tamanio) {
        fprintf(stderr, "Error: Tamaño insuficiente para escribir todo. La escritura excede el tamaño del archivo.\n");
        return;
    }
    FILE* archivo_bloques = fopen(blocks_path, "rb+");
    fseeko(archivo_bloques, (archivo->comienzo * block_size) + puntero, SEEK_SET);
    size_t espacio_disponible = archivo->tamanio - puntero;
    size_t bytes_a_escribir;
    size_t longitud = strlen(a_escribir);
    if (tamanio > espacio_disponible) {
        bytes_a_escribir = espacio_disponible;
    } else if (longitud < tamanio){
        bytes_a_escribir = longitud;
    }
    else{bytes_a_escribir = tamanio;}
    if (bytes_a_escribir < tamanio) {
        fwrite(a_escribir, 1, bytes_a_escribir, archivo_bloques);

        size_t bytes_restantes = tamanio - bytes_a_escribir;
        char* buffer_cero = calloc(bytes_restantes, sizeof(char)); // Buffer lleno de ceros
        if (buffer_cero) {
            fwrite(buffer_cero, 1, bytes_restantes, archivo_bloques);
            free(buffer_cero);
        } else {
            log_error(log_aux, "Error al asignar memoria para buffer de ceros");
        }
    } else {
        fwrite(a_escribir, 1, bytes_a_escribir, archivo_bloques);
    }
    fclose(archivo_bloques);
}

char* leer_archivo(char* nombre, off_t puntero, uint32_t tamanio){
    usleep(tiempo_unidad_trabajo * 1000);
    Archivo* archivo = buscar_archivo_por_nombre(nombre);
    if (!archivo) {
        fprintf(stderr, "Error: Archivo no encontrado.\n");
        return NULL;
    }
    FILE* archivo_bloques = fopen(blocks_path, "rb");
    if (!archivo_bloques) {
        log_error(log_aux, "Error al abrir bloques.dat");
        return NULL;
    }

    off_t posicion_final = (archivo->comienzo * block_size) + puntero + tamanio;
    off_t tamanio_archivo = archivo->comienzo * block_size + archivo->tamanio;
    if (posicion_final > tamanio_archivo) {
        fprintf(stderr, "Error: Intento de leer fuera del rango del archivo.\n");
        fclose(archivo_bloques);
        return NULL;
    }
    fseeko(archivo_bloques, (archivo->comienzo * block_size) + puntero, SEEK_SET);
    char* buffer = malloc(tamanio + 1);
    if (buffer == NULL) {
        log_error(log_aux, "Error al asignar memoria para el buffer");
        fclose(archivo_bloques);
        return NULL;  // No se pudo asignar memoria
    }
    size_t bytes_leidos = fread(buffer, 1, tamanio, archivo_bloques);
    if (bytes_leidos < tamanio) {
        if (ferror(archivo_bloques)) {
            log_error(log_aux, "Error al leer el archivo");
            free(buffer);
            fclose(archivo_bloques);
            return NULL;
        }
    }
    
    buffer[bytes_leidos] = '\0';  // termina en NULL

    fclose(archivo_bloques);
    return buffer;
}


uint32_t buscar_bloque_libre(){
    for(int nroBloque = 0; nroBloque < block_count; nroBloque++){
        if (!leer_de_bitmap(nroBloque)) {
            return nroBloque;
        }
    }
    return -1;
}

uint8_t leer_de_bitmap(uint32_t nroBloque){
    return bitarray_test_bit(bitmap, nroBloque);
}

Archivo* buscar_archivo_por_nombre(char* nombre) {
    for (int i = 0; i < list_size(lista_archivos); i++) {
        Archivo* archivo = (Archivo*)list_get(lista_archivos, i);
        if (strcmp(archivo->nombre, nombre) == 0) {
            return archivo;
        }
    }
    return NULL;
}

int buscar_archivo_x_bloque_inicial(int bloque_inicial){
    for (int i = 0; i < list_size(lista_archivos); i++) {
        Archivo*file = (Archivo*)list_get(lista_archivos, i);
        if (file->comienzo == bloque_inicial) {
            return i; 
        }
    }
    return -1; 
}


void eliminar_archivo_de_lista(int bloque_inicial){
    int posicion_a_borrar=  buscar_archivo_x_bloque_inicial(bloque_inicial);

    if (posicion_a_borrar != -1) {
        list_remove_and_destroy_element(lista_archivos, posicion_a_borrar, destruir_file);
    } else {
        log_error(log_aux, "Archivo no encontrado en la lista.\n");
    }
}

void destruir_file(void* elemento){
    Archivo* file = (Archivo*)elemento;
    free(file->nombre);
    free(file->ruta);
    free(file);
}

bool bloques_contiguos_libres(int bloque_inicial, int bloque_final){
    for (int i = bloque_inicial + 1; i <= bloque_final; i++) {
        if (leer_de_bitmap(i) != 0) {
            return false;
        }
    }
    return true;
}
void ordenar_archivos_por_comienzo(){
    size_t num_archivos = list_size(lista_archivos);
    Archivo** archivos_array = malloc(num_archivos * sizeof(Archivo*));
    for (size_t i = 0; i < num_archivos; i++) {
        archivos_array[i] = (Archivo*)list_get(lista_archivos, i);
    }
    qsort(archivos_array, num_archivos, sizeof(Archivo*), comparar_por_comienzo);
    
    for (size_t i = 0; i < num_archivos; i++) {
        list_replace(lista_archivos, i, archivos_array[i]);
    }
    free(archivos_array);
}

int comparar_por_comienzo(const void* a, const void* b) {
    Archivo* archivo_a = *(Archivo**)a;
    Archivo* archivo_b = *(Archivo**)b;
    if (archivo_a->comienzo < archivo_b->comienzo) return -1;
    if (archivo_a->comienzo > archivo_b->comienzo) return 1;
    return 0;
}

int limpiar_bloques_y_buscar_libres(int bloque_inicial, int bloque_final) {
    for (int i = bloque_inicial; i <= bloque_final; i++) {
        bitarray_clean_bit(bitmap, i);
    }
    int siguiente_bloque = 0;
    while (leer_de_bitmap(siguiente_bloque) != 0) {
        siguiente_bloque++;
    }
    return siguiente_bloque;
}


void compactar(int* bloque_inicial, int bloque_final, Archivo* file) {
    ordenar_archivos_por_comienzo();
    int num_bloques = 0;
    Archivo** bloques_originales = malloc(sizeof(Archivo) * list_size(lista_archivos));
    char** buffers_datos = NULL;

    FILE* archivo_bloques = fopen(blocks_path, "rb+");
    char* buffer_de_archivo_a_mover = malloc(file->tamanio);
    fseek(archivo_bloques, file->comienzo * block_size, SEEK_SET);
    fread(buffer_de_archivo_a_mover, file->tamanio, 1, archivo_bloques);

    for (int i = 0; i < list_size(lista_archivos); i++) {
        Archivo* archivo = (Archivo*)list_get(lista_archivos, i);
        if (archivo->comienzo > bloque_final) { //Guardo los bloques que le siguen al del archivo a compactar
            bloques_originales[num_bloques] = archivo;
            num_bloques++;
        }
    }
    buffers_datos = guardar_contenido_bloques(bloques_originales, num_bloques, archivo_bloques);

    int bloque_libre_actual = limpiar_bloques_y_buscar_libres(*bloque_inicial, bloque_final);

    escribir_datos_en_nuevos_bloques(bloques_originales, buffers_datos, num_bloques, &bloque_libre_actual, archivo_bloques);

    fseek(archivo_bloques, bloque_libre_actual * block_size, SEEK_SET);
    fwrite(buffer_de_archivo_a_mover, file->tamanio, 1, archivo_bloques);

    *bloque_inicial = bloque_libre_actual;
    free(bloques_originales);
    for (int i = 0; i < num_bloques; i++) {
        free(buffers_datos[i]);
    }
    free(buffers_datos);
    free(buffer_de_archivo_a_mover);

    msync(bitmap->bitarray, bitmap_size, MS_SYNC);
    fclose(archivo_bloques);
    usleep(retraso_compactacion * 1000);
}

char** guardar_contenido_bloques(Archivo** bloques_originales, int num_bloques, FILE* archivo_bloques) {
    char** buffers_datos = malloc(sizeof(char*) * num_bloques);
    if (buffers_datos == NULL) {
        log_error(log_aux, "Error al asignar memoria para buffers_datos");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < num_bloques; i++) {
        int start =  bloques_originales[i]->comienzo * block_size;
        int tamanio = start + bloques_originales[i]->tamanio;
        buffers_datos[i] = malloc(tamanio); 
        if (buffers_datos[i] == NULL) {
            log_error(log_aux, "Error al asignar memoria para buffers_datos[i]");
            for (int j = 0; j < i; j++) {
                free(buffers_datos[j]);
            }
            free(buffers_datos);
            return NULL;
        }
        fseek(archivo_bloques, start, SEEK_SET);
        fread(buffers_datos[i], tamanio, 1, archivo_bloques);
    }

    return buffers_datos;
}

void escribir_datos_en_nuevos_bloques(Archivo** bloques_originales, char** buffers_datos, int num_bloques, int* bloque_libre_actual, FILE* archivo_bloques) {
    if (!archivo_bloques) {
        log_error(log_aux, "Error al abrir el archivo de bloques");
        return;
    }

    for (int i = 0; i < num_bloques; i++) {
        Archivo* archivo = (Archivo*)list_get(lista_archivos, buscar_archivo_x_bloque_inicial(bloques_originales[i]->comienzo));
        int bloque_final_archivo;
        if (archivo->tamanio == 0) {
            bloque_final_archivo = archivo->comienzo;
        } else {
            bloque_final_archivo = archivo->comienzo + ((archivo->tamanio + block_size - 1) / block_size) - 1;
        }

        int start =  (*bloque_libre_actual) * block_size;
        int tamanio = start + bloques_originales[i]->tamanio;
        fseek(archivo_bloques, start, SEEK_SET);
        fwrite(buffers_datos[i], tamanio, 1, archivo_bloques);
        for (int bloque = archivo->comienzo; bloque <= bloque_final_archivo; bloque++) {
            bitarray_clean_bit(bitmap, bloque); 
        }
        
        int contador_bloques = *bloque_libre_actual;
        for (int bloque = archivo->comienzo; bloque <= bloque_final_archivo; bloque++) {
            bitarray_set_bit(bitmap, contador_bloques); 
            contador_bloques++;
        }

        archivo->comienzo = *bloque_libre_actual; // Actualizar el comienzo del archivo
        *bloque_libre_actual = contador_bloques; // Avanzar al siguiente bloque libre

        t_config* new_file_config = config_create(archivo->ruta);
        char * bloque_inicial_config = int_to_char(archivo->comienzo);
        config_set_value(new_file_config, "BLOQUE_INICIAL", bloque_inicial_config);
        config_save(new_file_config);
        config_destroy(new_file_config);
        free(bloque_inicial_config);
    }

}


void guardar_lista_archivos(){
    snprintf(lista_salida_path, sizeof(lista_salida_path), "%s/lista_archivos.txt", path_base_dialfs);
    int archivo_salida = open(lista_salida_path, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    char buffer[512];
    for (int i = 0; i < list_size(lista_archivos); i++) {
        Archivo* archivo = (Archivo*)list_get(lista_archivos, i);
        int len = snprintf(buffer, sizeof(buffer), "%s %s %d %d\n", archivo->ruta, archivo->nombre, archivo->comienzo, archivo->tamanio);
        write(archivo_salida, buffer, len);
    }
    close(archivo_salida);
}
