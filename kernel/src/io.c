#include "io.h"



instruccion_params* deserializar_io_gen_sleep_con_interfaz(t_buffer_ins* buffer) {
    instruccion_params* parametros = malloc(sizeof(instruccion_params));
    
    uint32_t offset = 0;
    uint32_t interfaz_len;
    memcpy(&interfaz_len, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    parametros->interfaz = malloc(interfaz_len);
    memcpy(parametros->interfaz, buffer->stream + offset, interfaz_len);
    offset += interfaz_len;
    memcpy(&(parametros->params.io_gen_sleep.unidades_trabajo), buffer->stream + offset, sizeof(int));
    
    return parametros;
}

instruccion_params* deserializar_io_stdin_stdout_con_interfaz(t_buffer_ins* buffer) {
    instruccion_params* parametros = malloc(sizeof(instruccion_params));
    uint32_t offset = 0;
    uint32_t interfaz_len;
    memcpy(&interfaz_len, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    parametros->interfaz = malloc(interfaz_len);
    memcpy(parametros->interfaz, buffer->stream + offset, interfaz_len);
    offset += interfaz_len;
    memcpy(&(parametros->registro_tamanio), buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(&(parametros->cant_direcciones), buffer->stream + offset, sizeof(int));
    offset += sizeof(int);
    parametros->registro_direccion = malloc(sizeof(t_dir_fisica) * parametros->cant_direcciones);
    memcpy(parametros->registro_direccion,buffer->stream + offset, sizeof(t_dir_fisica) * parametros->cant_direcciones);
    
    
    return parametros;
}

instruccion_params* deserializar_io_fs_create_delete_con_interfaz(t_buffer_ins* buffer) {
    instruccion_params* parametros = malloc(sizeof(instruccion_params));
    
    uint32_t offset = 0;
    uint32_t interfaz_len;
    memcpy(&interfaz_len, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    parametros->interfaz = malloc(interfaz_len);
    memcpy(parametros->interfaz, buffer->stream + offset, interfaz_len);
    offset += interfaz_len;
    
    uint32_t archivo_len;
    memcpy(&archivo_len, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    parametros->params.io_fs.nombre_archivo = malloc(archivo_len);
    memcpy(parametros->params.io_fs.nombre_archivo, buffer->stream + offset, archivo_len);
    
    return parametros;
}

instruccion_params* deserializar_io_fs_truncate_con_interfaz(t_buffer_ins* buffer) {
    instruccion_params* parametros = malloc(sizeof(instruccion_params));
    
    uint32_t offset = 0;
    uint32_t interfaz_len;
    memcpy(&interfaz_len, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    parametros->interfaz = malloc(interfaz_len);
    memcpy(parametros->interfaz, buffer->stream + offset, interfaz_len);
    offset += interfaz_len;
    
    uint32_t archivo_len;
    memcpy(&archivo_len, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    parametros->params.io_fs.nombre_archivo = malloc(archivo_len);
    memcpy(parametros->params.io_fs.nombre_archivo, buffer->stream + offset, archivo_len);
    offset += archivo_len;
    
    memcpy(&(parametros->registro_tamanio), buffer->stream + offset, sizeof(uint32_t));
    
    return parametros;
}

instruccion_params* deserializar_io_fs_write_read_con_interfaz(t_buffer_ins* buffer) {
    instruccion_params* parametros = malloc(sizeof(instruccion_params));
    // parametros->registro_direccion = malloc(sizeof(t_dir_fisica));
    uint32_t offset = 0;
    uint32_t interfaz_len;
    memcpy(&interfaz_len, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    parametros->interfaz = malloc(interfaz_len);
    memcpy(parametros->interfaz, buffer->stream + offset, interfaz_len);
    offset += interfaz_len;
    
    uint32_t archivo_len;
    memcpy(&archivo_len, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    parametros->params.io_fs.nombre_archivo = malloc(archivo_len);
    memcpy(parametros->params.io_fs.nombre_archivo, buffer->stream + offset, archivo_len);
    offset += archivo_len;
    
    memcpy(&(parametros->registro_tamanio), buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(&(parametros->cant_direcciones), buffer->stream + offset, sizeof(int));
    offset += sizeof(int);
    parametros->registro_direccion = malloc(sizeof(t_dir_fisica) * parametros->cant_direcciones);
    memcpy(parametros->registro_direccion,buffer->stream + offset, sizeof(t_dir_fisica) * parametros->cant_direcciones);
    offset += parametros->cant_direcciones * sizeof(t_dir_fisica);
    memcpy(&(parametros->params.io_fs.registro_puntero_archivo), buffer->stream + offset, sizeof(off_t));
    
    return parametros;
}

