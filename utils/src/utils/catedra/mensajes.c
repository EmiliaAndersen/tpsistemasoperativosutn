#include <utils/catedra/client.h>
#include <utils/catedra/mensajes.h>
int recibir_operacion(int socket_cliente)
{
    int cod_op;
    if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
        return cod_op;
    else
    {
        close(socket_cliente);
        return -1;
    }
}

void* recibir_buffer(int* size, int socket_cliente)
{
    void * buffer;
    recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
    buffer = malloc(*size);
    recv(socket_cliente, buffer, *size, MSG_WAITALL);
    return buffer;
}

char* recibir_mensaje(int socket_cliente, t_log* logger)
{
    int size;
    char* buffer = recibir_buffer(&size, socket_cliente);
    //log_info(logger, "Me llego el mensaje %s", buffer);
    return buffer;
}

t_list* recibir_paquete(int socket_cliente) {
    int size;
    int desplazamiento = 0;
    void * buffer;
    t_list* valores = list_create();
    
    buffer = recibir_buffer(&size, socket_cliente);
    if (!buffer) {
        fprintf(stderr, "Error al recibir buffer\n");
        return NULL;
    }

    while (desplazamiento < size) {
        int tamanio;
        memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
        desplazamiento += sizeof(int);
        char* valor = malloc(tamanio);
        if (!valor) {
            fprintf(stderr, "Error al asignar memoria para valor\n");
            free(buffer); // Liberar el buffer antes de retornar NULL
            list_destroy(valores);
            return NULL;
        }

        memcpy(valor, buffer + desplazamiento, tamanio);
        desplazamiento += tamanio;
        list_add(valores, valor);
    }

    free(buffer); // Liberar el buffer después de procesarlo completamente
    return valores;
}


void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;
    
    memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
    return magic;
}

void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

    int resultado_send = send(socket_cliente, a_enviar, bytes, MSG_NOSIGNAL);  // Evita la generación de SIGPIPE

    if (resultado_send == -1) {
        fprintf(stderr, "Error al enviar el mensaje: socket cerrado.\n");
    }

    free(a_enviar);
    eliminar_paquete(paquete);
}

void enviar_a_mem(int socket_cliente,char* mensaje, op_code operacion){
    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion =  operacion;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje)+1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
    memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

    int resultado_send = send(socket_cliente, a_enviar, bytes, MSG_NOSIGNAL);  // Evita la generación de SIGPIPE

    if (resultado_send == -1) {
        fprintf(stderr, "Error al enviar el pedido de lectura: socket cerrado.\n");
    }

    free(a_enviar);
    eliminar_paquete(paquete);
}

void enviar_tamanio_pag_frame(int client_socket, int tamanio){
        // Convierte el entero a un formato de red (network byte order)
    int numero_network_order = htonl(tamanio);

    // Envía el entero
    int resultado_send = send(client_socket, &numero_network_order, sizeof(numero_network_order), 0);
    if (resultado_send == -1) {
        fprintf(stderr, "Error al enviar el Tam Pag: socket cerrado.\n");
    }
}

void  enviar_pedido_tam_mem(int socket_cliente){
    t_paquete* paquete = malloc(sizeof(t_paquete));
    char * mensaje = "pedi tam_pag";
	paquete->codigo_operacion = TAM_PAG;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

    int resultado_send = send(socket_cliente, a_enviar, bytes, MSG_NOSIGNAL);  // Evita la generación de SIGPIPE

    if (resultado_send == -1) {
        fprintf(stderr, "Error al enviar el mensaje: socket cerrado.\n");
    }

    free(a_enviar);
    eliminar_paquete(paquete);
}

char* recibir_pedido(int socket_cliente)
{
    int size;
    char* buffer = recibir_buffer(&size, socket_cliente);
    return buffer;
}


void recibir_ped_tamanio_pag(int socket_cliente, t_log* logger)
{
    int size;
    char* buffer = recibir_buffer(&size, socket_cliente);
    free(buffer);
}

void recibir_tamanio_pag(int socket_cliente, t_log* logger, int* numero)
{
    int numero_network_order;
    int resultado_recv = recv(socket_cliente, &numero_network_order, sizeof(numero_network_order), 0);
    if (resultado_recv == -1) {
        log_info(logger, "Error con el Pedido de TAM PAG");
        exit(EXIT_FAILURE);
    }

    // Convierte el entero del formato de red al formato de host
    *numero = ntohl(numero_network_order);
}

int recibir_frame(int socket_cliente){
    int frame;
    int numero_network_order;
    int resultado_recv = recv(socket_cliente, &numero_network_order, sizeof(numero_network_order), 0);
    if (resultado_recv == -1) {
        printf("ERROR AL RECIBIR EL FRAME");
        exit(EXIT_FAILURE);
    }
    frame = ntohl(numero_network_order);
    return frame;
}

void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(void)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = PAQUETE;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

bool rcv_handshake(int fd_conexion){
    size_t bytes;

    int32_t handshake;
    int32_t resultOk = 0;
    int32_t resultError = -1;

    if(fd_conexion != -1){

        bytes = recv(fd_conexion, &handshake, sizeof(int32_t), MSG_WAITALL);
        if (handshake == 1) {
        bytes = send(fd_conexion, &resultOk, sizeof(int32_t), 0);
        } else {
        bytes = send(fd_conexion, &resultError, sizeof(int32_t), 0);
        }
    }
    return true;
}

bool send_handshake(int conexion, t_log* logger, const char* conexion_name){
    size_t bytes;

    int32_t handshake = 1;
    int32_t result;

    bytes = send(conexion, &handshake, sizeof(int32_t), 0);
    bytes = recv(conexion, &result, sizeof(int32_t), MSG_WAITALL);

    if (result == 0) {
    log_info(logger, "Handshake OK de %s", conexion_name);
    // Handshake OK
    } 
    else {
    // Handshake ERROR
    log_info(logger,"Error handshake de %s", conexion_name); 
    }   
    return true;
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}
