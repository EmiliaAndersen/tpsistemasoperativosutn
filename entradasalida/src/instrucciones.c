#include "instrucciones.h"


instruccion_params* deserializar_io_gen_sleep(t_buffer_ins* buffer) {
    instruccion_params* parametros = malloc(sizeof(instruccion_params));
    memcpy(&(parametros->params.io_gen_sleep.unidades_trabajo), buffer->stream, sizeof(int));
    return parametros;
}

instruccion_params* deserializar_registro_direccion_tamanio(t_buffer_ins* buffer) {
    instruccion_params* parametros = malloc(sizeof(instruccion_params));
    parametros->registro_direccion = malloc(sizeof(t_dir_fisica));
    void* stream = buffer->stream;
    memcpy(&(parametros->registro_direccion->nro_frame), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(parametros->registro_direccion->desplazamiento), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(parametros->registro_tamanio), stream, sizeof(uint32_t));
    return parametros;
}

instruccion_params* deserializar_io_fs_create_delete(t_buffer_ins* buffer) {
    instruccion_params* parametros = malloc(sizeof(instruccion_params));
    
    uint32_t offset = 0;
    uint32_t archivo_len;
    memcpy(&archivo_len, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    parametros->params.io_fs.nombre_archivo = malloc(archivo_len);
    memcpy(parametros->params.io_fs.nombre_archivo, buffer->stream + offset, archivo_len);
    
    return parametros;
}

instruccion_params* deserializar_io_fs_truncate(t_buffer_ins* buffer) {
    instruccion_params* parametros = malloc(sizeof(instruccion_params));
    
    uint32_t offset = 0;
    uint32_t archivo_len;
    memcpy(&archivo_len, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    parametros->params.io_fs.nombre_archivo = malloc(archivo_len);
    memcpy(parametros->params.io_fs.nombre_archivo, buffer->stream + offset, archivo_len);
    offset += archivo_len;
    memcpy(&(parametros->registro_tamanio), buffer->stream + offset, sizeof(uint32_t));
    
    return parametros;
}

instruccion_params* deserializar_io_fs_write_read(t_buffer_ins* buffer) {
    instruccion_params* parametros = malloc(sizeof(instruccion_params));
    parametros->registro_direccion = malloc(sizeof(t_dir_fisica));
    
    uint32_t offset = 0;
    uint32_t archivo_len;
    memcpy(&archivo_len, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    parametros->params.io_fs.nombre_archivo = malloc(archivo_len);
    memcpy(parametros->params.io_fs.nombre_archivo, buffer->stream + offset, archivo_len);
    offset += archivo_len;
    memcpy(&(parametros->registro_direccion->nro_frame), buffer->stream + offset, sizeof(int));
    offset += sizeof(int);
    memcpy(&(parametros->registro_direccion->desplazamiento), buffer->stream + offset, sizeof(int));
    offset += sizeof(int);
    memcpy(&(parametros->registro_tamanio), buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(&(parametros->params.io_fs.registro_puntero_archivo), buffer->stream + offset, sizeof(off_t));
    
    return parametros;
}

void recibir_instruccion(char* tipo_interfaz)
{
    while (1){
        t_paquete_instruccion* instruccion = malloc(sizeof(t_paquete_instruccion));
        instruccion->buffer = malloc(sizeof(t_buffer_ins));
        uint32_t pid = -1;
        recv(conexion_kernel, &(instruccion->codigo_operacion), sizeof(instrucciones), MSG_WAITALL);
        recv(conexion_kernel, &(pid), sizeof(uint32_t), MSG_WAITALL);
        recv(conexion_kernel, &(instruccion->buffer->size), sizeof(uint32_t), MSG_WAITALL);
        instruccion->buffer->stream = malloc(instruccion->buffer->size);
        recv(conexion_kernel, instruccion->buffer->stream, instruccion->buffer->size, MSG_WAITALL);
        instruccion_params* param;
        
        if(validar_operacion(tipo_interfaz, instruccion->codigo_operacion)){
            switch (instruccion->codigo_operacion)
            {
            case IO_GEN_SLEEP:{
            param = deserializar_io_gen_sleep(instruccion->buffer);
            break;
            }
            case IO_STDIN_READ:{
            param = deserializar_registro_direccion_tamanio(instruccion->buffer);
            break;
            }
            case IO_STDOUT_WRITE:{
            param = deserializar_registro_direccion_tamanio(instruccion->buffer);
            break;
            }
            case IO_FS_CREATE: {
                param = deserializar_io_fs_create_delete(instruccion->buffer);
                break;
            }
            case IO_FS_DELETE: {
                param = deserializar_io_fs_create_delete(instruccion->buffer);
                break;
            }
            case IO_FS_TRUNCATE: {
                param = deserializar_io_fs_truncate(instruccion->buffer);
                break;
            }
            case IO_FS_WRITE: {
                param = deserializar_io_fs_write_read(instruccion->buffer);
                break;
            }
            case IO_FS_READ: {
                param = deserializar_io_fs_write_read(instruccion->buffer);
                break;
            }
        // OTRAS FUNCIONES
            default:
            printf("Tipo de operación no válido.\n");
            free(instruccion->buffer);
            free(instruccion);
            continue;
            }
            /*char* logica = "1";
            aviso_segun_cod_op(logica, conexion_kernel, AVISO_OPERACION_VALIDADA);*/
            atender_cod_op(param, instruccion->codigo_operacion, pid);
            free(instruccion->buffer);
            free(instruccion);
            // if (param != NULL) {
            // free(param);
            // }
        }
        else{
            aviso_segun_cod_op(nombre_interfaz, conexion_kernel, AVISO_OPERACION_INVALIDA);
            param = NULL;
            free(instruccion->buffer);
            free(instruccion);
        }
        aviso_segun_cod_op(nombre_interfaz, conexion_kernel,AVISO_OPERACION_FINALIZADA);
    }

}

void atender_cod_op(instruccion_params* parametros, instrucciones op_code, uint32_t pid){
    log_info(entradasalida_log, "PID: <%i> - Operacion: <%s>", pid, op_code_a_string(op_code));
    switch (op_code)
    {
    case IO_GEN_SLEEP:{
        int result = 0;
        int unidades_trabajo_recibidas = parametros->params.io_gen_sleep.unidades_trabajo;
        result = unidades_trabajo_recibidas * tiempo_unidad_trabajo * 1000; 
        usleep(result);
        log_debug(entradasalida_log, "Termine de dormir %dms", result);
        break;
    }
    case IO_STDIN_READ:{
        uint32_t tamanio = parametros->registro_tamanio;
        char* prompt = (char*)malloc(256); // Para un mensaje de entrada personalizado
        if (prompt == NULL) {
            log_error(entradasalida_log, "Error al asignar memoria para el prompt");
            break;
        }   
        snprintf(prompt, 256, "Ingrese el texto de tamaño %u: ", tamanio);
        char* texto = readline(prompt);
        free(prompt); 
        if (texto == NULL) {
            log_error(entradasalida_log, "Error al leer el texto");
            break;
        }
    // Si el texto es más largo que el tamaño esperado, truncar
        if (strlen(texto) > tamanio) {
            texto[tamanio] = '\0'; // Truncar el texto al tamaño permitido
        }
        parametros->texto = texto;
        t_paquete_instruccion* instruccion_enviar = malloc(sizeof(t_paquete_instruccion));
        instruccion_enviar->codigo_operacion = READ_IO;
        enviar_instruccion_IO_Mem(instruccion_enviar,parametros,conexion_memoria, pid);
        free(texto);
        free(instruccion_enviar);
        break;
    }
    case IO_STDOUT_WRITE:{
        t_paquete_instruccion* instruccion_enviar = malloc(sizeof(t_paquete_instruccion));
        instruccion_enviar->codigo_operacion = WRITE_IO;
        enviar_instruccion_IO_Mem(instruccion_enviar,parametros,conexion_memoria, pid);
        free(instruccion_enviar);
        char* imprimir = recibir_mensaje(conexion_memoria, entradasalida_log);
        free(imprimir);
        break;
    }
    case IO_FS_CREATE:{
        log_info(entradasalida_log, "PID: <%i> - Crear Archivo: <%s>", pid,  parametros->params.io_fs.nombre_archivo);
        crear_archivo(parametros->params.io_fs.nombre_archivo);
        break;
    }
    case IO_FS_DELETE:{
        log_info(entradasalida_log, "PID: <%i> - Eliminar Archivo: <%s>", pid,  parametros->params.io_fs.nombre_archivo);
        borrar_archivo(parametros->params.io_fs.nombre_archivo);
        break;
    }
    case IO_FS_TRUNCATE:{
        log_info(entradasalida_log, "PID: <%i> - Truncar Archivo: <%s> - Tamaño: <%i>", pid,  parametros->params.io_fs.nombre_archivo, parametros->registro_tamanio);
        truncar_archivo(parametros->params.io_fs.nombre_archivo, parametros->registro_tamanio, pid);
        break;
    }
    case IO_FS_WRITE:{
        log_info(entradasalida_log, "PID: <%i> - Escribir Archivo: <%s> - Tamaño a Escribir: <%i> - Puntero Archivo: <%jd>", pid,  parametros->params.io_fs.nombre_archivo, parametros->registro_tamanio, (intmax_t)parametros->params.io_fs.registro_puntero_archivo);
        t_paquete_instruccion* instruccion_enviar = malloc(sizeof(t_paquete_instruccion));
        instruccion_enviar->codigo_operacion = WRITE_IO_FS;
        enviar_instruccion_IO_Mem(instruccion_enviar,parametros,conexion_memoria, pid);
        free(instruccion_enviar);
        char* a_escribir = recibir_mensaje(conexion_memoria, entradasalida_log);
        escribir_archivo(parametros->params.io_fs.nombre_archivo, parametros->params.io_fs.registro_puntero_archivo, a_escribir, parametros->registro_tamanio);
        free(a_escribir);
        break;
    }
    case IO_FS_READ:{
        log_info(entradasalida_log, "PID: <%i> - Leer Archivo: <%s> - Tamaño a Escribir: <%i> - Puntero Archivo: <%jd>", pid,  parametros->params.io_fs.nombre_archivo, parametros->registro_tamanio, (intmax_t)parametros->params.io_fs.registro_puntero_archivo);
        char* leido = leer_archivo(parametros->params.io_fs.nombre_archivo, parametros->params.io_fs.registro_puntero_archivo, parametros->registro_tamanio);
        t_paquete_instruccion* instruccion_enviar = malloc(sizeof(t_paquete_instruccion));
        instruccion_enviar->codigo_operacion = READ_IO_FS;
        parametros->texto = leido;
        enviar_instruccion_IO_Mem(instruccion_enviar,parametros,conexion_memoria, pid);
        free(leido);
        break;
    }
    default:
        break;
    }
    free(parametros);
}

int validar_operacion(char* tipo_interfaz, int codigo_operacion){
    int resultado = 0;
    if(strcmp(tipo_interfaz, "GENERICA") == 0 && codigo_operacion == 10){resultado = 1;}
    else if(strcmp(tipo_interfaz, "STDIN") == 0 && codigo_operacion == 11){resultado = 1;}
    else if(strcmp(tipo_interfaz, "STDOUT") == 0 && codigo_operacion == 12){resultado = 1;}
    else if(strcmp(tipo_interfaz, "DIALFS") == 0 && (codigo_operacion > 12 && codigo_operacion < 18)){resultado = 1;}
    return resultado;
}

const char* op_code_a_string(instrucciones op_code){
    switch (op_code) {
        case IO_GEN_SLEEP: return "IO_GEN_SLEEP";
        case IO_STDIN_READ: return "IO_STDIN_READ";
        case IO_STDOUT_WRITE: return "IO_STDOUT_WRITE";
        case IO_FS_CREATE: return "IO_FS_CREATE";
        case IO_FS_DELETE: return "IO_FS_DELETE";
        case IO_FS_TRUNCATE: return "IO_FS_TRUNCATE";
        case IO_FS_WRITE: return "IO_FS_WRITE";
        case IO_FS_READ: return "IO_FS_READ";
        default: return "UNKNOWN";
    }
}

