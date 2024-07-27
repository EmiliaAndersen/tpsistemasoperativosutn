#include "mensajesPropios.h"

/////////////////////////////* INTERFACES *//////////////////////////////////////

char* recibir_interfaz(int socket_cliente, t_log* logger)
{
    int size;
    char* buffer = recibir_buffer(&size, socket_cliente);
    log_debug(logger, "Interfaz conectada: %s", buffer);
   return buffer;
}

void aviso_segun_cod_op(char* mensaje, int socket_cliente, int codigo_operacion){
     t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = codigo_operacion;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = strlen(mensaje) + 1;
    paquete->buffer->stream = malloc(paquete->buffer->size);
    memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);
    int bytes = paquete->buffer->size + 2*sizeof(int);
    void* a_enviar = serializar_paquete(paquete, bytes);
    send(socket_cliente, a_enviar, bytes, 0);
    free(a_enviar);
    eliminar_paquete(paquete);
}


char* recibir_error_oi(int socket){
    int size;
    char* buffer = recibir_buffer(&size, socket);
    return buffer;
}

int recibir_op_validada(int socket){
    int size;
    char* buffer = recibir_buffer(&size, socket);
    int logica = atoi(buffer);
    free(buffer);
    return logica;
}

char* recibir_desconexion(int socket_cliente, t_log* logger)
{
    int size;
    char* buffer = recibir_buffer(&size, socket_cliente);
    log_info(logger, "Interfaz desconectada: %s", buffer);
    return buffer;
}
char* recibir_op_finalizada(int socket_cliente)
{
    int size;
    char* buffer = recibir_buffer(&size, socket_cliente);
    return buffer;
}

//////////////////////////////* INSTRUCCIONES MEMORIA *//////////////////////////////////////

void enviar_instruccion_mem(int socket_cliente, t_instruccion* instruccion){
    
    t_buffer_ins* buffer = serializar_instruccion(instruccion);
    instruccion->codigo_operacion = INSTRUCCION;
    instruccion->buffer=buffer;
    int offset = 0;
    void* a_enviar = malloc(buffer->size + sizeof(op_code) + sizeof(uint32_t));
    memcpy(a_enviar + offset, &(instruccion->codigo_operacion), sizeof(op_code));
    offset += sizeof(op_code);
    memcpy(a_enviar + offset, &(instruccion->buffer->size), sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(a_enviar + offset, instruccion->buffer->stream, instruccion->buffer->size);
    offset += sizeof(buffer->size);
    int resultado_send = send(socket_cliente, a_enviar, buffer->size + sizeof(op_code) + sizeof(uint32_t), MSG_NOSIGNAL);
    if (resultado_send == -1)
        {
            printf("Error al enviar la instrucción: socket cerrado.\n");
        }
    free(buffer);
    free(a_enviar);
    }

t_buffer_ins* serializar_instruccion(t_instruccion* ins){
    
    t_buffer_ins* buffer = malloc(sizeof(t_buffer_ins));
    buffer->size = ins->buffer->size ;//strlen(ins->buffer->stream) + 1; // Longitud de la cadena + 1 para el carácter nulo
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);
    memcpy(buffer->stream, ins->buffer->stream, buffer->size); // Copiar la cadena directamente

    return buffer;
}

t_instruccion* recibir_instruccion_cpu(int socket_servidor){

    t_instruccion* instruccion = malloc(sizeof(t_instruccion));

    instruccion->buffer = malloc(sizeof(t_buffer_ins));

    recv(socket_servidor, &(instruccion->codigo_operacion), sizeof(op_code), MSG_WAITALL);

    recv(socket_servidor, &(instruccion->buffer->size), sizeof(uint32_t), MSG_WAITALL);

    instruccion->buffer->stream = malloc(instruccion->buffer->size);
    recv(socket_servidor, instruccion->buffer->stream, instruccion->buffer->size, MSG_WAITALL);
 
    return instruccion;  
}

//////////////////////////////* PCB *//////////////////////////////////////

void enviar_pc(char* pc, int socket_cliente){

    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = PC;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = strlen(pc)+1;
    paquete->buffer->stream = malloc(paquete->buffer->size);
    memcpy(paquete->buffer->stream, pc, paquete->buffer->size);

    int bytes = paquete->buffer->size + 2*sizeof(int);

    void* a_enviar = serializar_paquete(paquete, bytes);
    
    int resultado_send = send(socket_cliente, a_enviar, bytes, MSG_NOSIGNAL);  

    if (resultado_send == -1)
        {
            printf("Error al enviar la instrucción: socket cerrado.\n");
        }
    
    eliminar_paquete(paquete);
}

void enviar_pid(char* pc, int socket_cliente){

    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = PID;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = strlen(pc)+1;
    paquete->buffer->stream = malloc(paquete->buffer->size);
    memcpy(paquete->buffer->stream, pc, paquete->buffer->size);

    int bytes = paquete->buffer->size + 2*sizeof(int);

    void* a_enviar = serializar_paquete(paquete, bytes);
    
    int resultado_send = send(socket_cliente, a_enviar, bytes, MSG_NOSIGNAL);  

    if (resultado_send == -1)
        {
            printf("Error al enviar la instrucción: socket cerrado.\n");
        }
    
    eliminar_paquete(paquete);
}


char* recibir_pc(int socket_cliente){

    int size;
    char* buffer = recibir_buffer(&size, socket_cliente);

    if(buffer==NULL){
        printf("No se recibio nada\n");
    }
    return buffer;
}

void enviar_pcb(t_pcb* pcb, int socket_cliente){

    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion= PCB;
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(pcb->quantum), sizeof(int));
    agregar_a_paquete(paquete, pcb->registros, sizeof(cpu_registros));
    agregar_a_paquete(paquete, &(pcb->estado), sizeof(t_proceso_estado));
    agregar_a_paquete(paquete, &(pcb->motivo_desalojo), sizeof(int));
    // agregar_a_paquete(paquete, &strlen(pcb->nombre_interfaz_desalojo), sizeof(uint32_t));
    // agregar_a_paquete(paquete, &(pcb->nombre_interfaz_desalojo), strlen(pcb->nombre_interfaz_desalojo));
    // agregar_a_paquete(paquete, &strlen(pcb->recursos_waiteados), sizeof(uint32_t));
    // agregar_a_paquete(paquete, &(pcb->recursos_waiteados), strlen(pcb->recursos_waiteados));
    enviar_paquete(paquete, socket_cliente);
    eliminar_paquete(paquete);
    // printf("Paquete enviado a socket: %d\n", socket_cliente);
}

t_pcb* recibir_pcb(int socket_cliente) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    t_list* valores_paquete = recibir_paquete(socket_cliente);
    // pcb->registros = malloc(sizeof(cpu_registros));
    uint32_t* pid_ptr = (uint32_t*)list_remove(valores_paquete, 0);
    pcb->pid = *pid_ptr;
    free(pid_ptr);

    int* quantum_ptr = (int*)list_remove(valores_paquete, 0);
    pcb->quantum = *quantum_ptr;
    free(quantum_ptr);

    pcb->registros = (cpu_registros*)list_remove(valores_paquete, 0);
    
    t_proceso_estado *estado_ptr = (t_proceso_estado*)list_remove(valores_paquete, 0);
    pcb->estado = *estado_ptr;
    free(estado_ptr);

    int* desalojo_ptr = (int*)list_remove(valores_paquete, 0);
    pcb->motivo_desalojo = *desalojo_ptr;
    free(desalojo_ptr);

    list_destroy(valores_paquete);

    return pcb;
}

void enviar_nombre_recurso(char* nombre_recurso, int socket_cliente)
{
    // t_paquete *paquete = malloc(sizeof(t_paquete));
    // paquete->codigo_operacion = NOMBRE_RECURSO;
    // paquete->buffer = malloc(sizeof(t_buffer));

    // uint32_t tamanio = strlen(nombre_recurso) + 1;

    // paquete->buffer->size = sizeof(uint32_t)*2 + tamanio;
    // paquete->buffer->stream = malloc(paquete->buffer->size);

    // uint32_t offset = 0;
    // memcpy(paquete->buffer->stream + offset, &tamanio, sizeof(uint32_t));
    // offset += sizeof(uint32_t);

    // memcpy(paquete->buffer->stream + offset, nombre_recurso, tamanio);
    // offset += tamanio;

    // uint32_t bytes = paquete->buffer->size + (2*sizeof(int32_t)) + sizeof(op_code);
    // void* buffer_intermedio = malloc(bytes);
    // int desplazamiento = 0;

    // memcpy(buffer_intermedio + desplazamiento, &(paquete->codigo_operacion), sizeof(op_code));
    // desplazamiento += sizeof(op_code);
    // memcpy(buffer_intermedio + desplazamiento, &(paquete->buffer->size), sizeof(uint32_t));
    // desplazamiento += sizeof(uint32_t);
    // memcpy(buffer_intermedio + desplazamiento, &offset, sizeof(uint32_t));
    // desplazamiento += sizeof(uint32_t);
    // memcpy(buffer_intermedio + desplazamiento, &(paquete->buffer->stream), paquete->buffer->size);
    // desplazamiento += paquete->buffer->size;

    // send(socket_cliente, buffer_intermedio, bytes, 0);
    // printf("Paquete enviado al socket %d, con un tamanio de %d\n", socket_cliente, bytes);
    // free(buffer_intermedio);

    // free(paquete->buffer->stream);
    // free(paquete->buffer);
    // free(paquete);
}

char* recibir_nombre_recurso(int socket_cliente)
{
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    uint32_t offset;
    recv(socket_cliente, &(paquete->codigo_operacion), sizeof(op_code), MSG_WAITALL);
    printf("OPCODE: %d", paquete->codigo_operacion);
    recv(socket_cliente, &(paquete->buffer->size), sizeof(uint32_t), MSG_WAITALL);
    printf("Size: %d", paquete->buffer->size);
    recv(socket_cliente, &offset, sizeof(uint32_t), MSG_WAITALL);
    paquete->buffer->stream = malloc(sizeof(paquete->buffer->size));
    recv(socket_cliente, &(paquete->buffer->stream), paquete->buffer->size, MSG_WAITALL);
    
    void* stream = paquete->buffer->stream;

    uint32_t tamanio;
    memcpy(tamanio, stream, sizeof(uint32_t));
    printf("Tamanio de la palabra: %d", tamanio);
    stream += sizeof(uint32_t);
    char* ret = malloc(tamanio);
    memcpy(ret, stream, tamanio);
    stream += tamanio;
    
    return ret;
}


char *estado_a_string(t_proceso_estado estado)
{
    switch (estado)
    {
    case 0:
        return "NEW";
    case 1:
        return "READY";
    case 2:
        return "EXEC";
    case 3:
        return "BLOCKED";
    case 4:
        return "EXIT";
    default:
        return 0;
    }
}

void enviar_int_a_interrupt(int socket_cpu_interrupt, uint32_t pid)
{
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion= KERNEL_CPU_INTERRUPT;
    agregar_a_paquete(paquete, &(pid), sizeof(uint32_t));
    enviar_paquete(paquete, socket_cpu_interrupt);
    eliminar_paquete(paquete);
}

uint32_t recibir_int_a_interrupt(int socket_cpu_interrupt)
{
    uint32_t ret;

    t_list* valores_paquete = recibir_paquete(socket_cpu_interrupt);
    ret = (uint32_t)list_get(valores_paquete, 0);
    list_destroy(valores_paquete);

    return ret;
}
