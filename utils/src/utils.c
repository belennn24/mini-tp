#include "utils.h"

#include <errno.h>

void crear_buffer(t_paquete *paquete);
extern t_log *logger;

static int enviar_todo(int socket_cliente, const void *datos, size_t tamanio)
{
    const char *cursor = datos;
    size_t enviados = 0;

    while (enviados < tamanio)
    {
        ssize_t resultado = send(socket_cliente, cursor + enviados, tamanio - enviados, MSG_NOSIGNAL);
        if (resultado < 0 && errno == EINTR)
            continue;
        if (resultado <= 0)
            return -1;
        enviados += resultado;
    }
    return 0;
}

static int recibir_todo(int socket_cliente, void *datos, size_t tamanio)
{
    char *cursor = datos;
    size_t recibidos = 0;

    while (recibidos < tamanio)
    {
        ssize_t resultado = recv(socket_cliente, cursor + recibidos, tamanio - recibidos, 0);
        if (resultado < 0 && errno == EINTR)
            continue;
        if (resultado <= 0)
            return resultado == 0 ? 0 : -1;
        recibidos += resultado;
    }
    return 1;
}

void saludar(char *quien)
{
    printf("Hola desde %s!!\n", quien);
}

int iniciar_servidor(int puerto)
{
    struct addrinfo hints, *servinfo;
    char puerto_str[16];
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    snprintf(puerto_str, sizeof(puerto_str), "%d", puerto);

    if (getaddrinfo(NULL, puerto_str, &hints, &servinfo) != 0)
        return -1;

    int fd_escucha = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (fd_escucha == -1)
    {
        freeaddrinfo(servinfo);
        return -1;
    }

    setsockopt(fd_escucha, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    if (bind(fd_escucha, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    {
        freeaddrinfo(servinfo);
        close(fd_escucha);
        return -1;
    }

    if (listen(fd_escucha, SOMAXCONN) == -1)
    {
        freeaddrinfo(servinfo);
        close(fd_escucha);
        return -1;
    }

    freeaddrinfo(servinfo);
    return fd_escucha;
}

int esperar_conexion(int fd_escucha, t_log *logger)
{
    int fd_conexion = accept(fd_escucha, NULL, NULL);
    if (fd_conexion == -1)
        return -1;
    log_info(logger, "Se conecto un cliente!");
    return fd_conexion;
}

int crear_conexion(char *ip, int puerto)
// Conecta al servidor y devuelve el fd del socket cliente.
{
    struct addrinfo hints;
    struct addrinfo *server_info;
    char puerto_str[16];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;

    snprintf(puerto_str, sizeof(puerto_str), "%d", puerto);

    if (getaddrinfo(ip, puerto_str, &hints, &server_info) != 0)
    {
        printf("Error al obtener la información de la dirección\n");
        exit(1);
    }

    // Ahora vamos a crear el socket.
    int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (socket_cliente == -1)
    {
        freeaddrinfo(server_info);
        printf("Error al crear el socket\n");
        exit(1);
    }
    // Ahora que tenemos el socket, vamos a conectarlo
    if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
    {
        freeaddrinfo(server_info);
        printf("Error al conectar el socket\n");
        close(socket_cliente);
        exit(1);
    }

    freeaddrinfo(server_info);

    return socket_cliente;
}

void enviar_operacion(int socket_cliente, int op_code)
{
    if (enviar_todo(socket_cliente, &op_code, sizeof(op_code)) == -1)
    {
        // Opcional: podrías agregar un log de error aquí
        perror("Error al enviar el código de operación");
    }
}

void crear_buffer(t_paquete *paquete)
{
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = 0;
    paquete->buffer->stream = NULL;
}
t_paquete *crear_paquete(void)
{
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = PAQUETE;
    crear_buffer(paquete);
    return paquete;
}

void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio)
{
    paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

    memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
    memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

    paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(int socket_cliente, t_paquete *paquete)
{
    int bytes = paquete->buffer->size + 2 * sizeof(int);
    void *a_enviar = malloc(bytes);
    int desplazamiento = 0;

    memcpy((char *)a_enviar + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy((char *)a_enviar + desplazamiento, &(paquete->buffer->size), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy((char *)a_enviar + desplazamiento, paquete->buffer->stream, paquete->buffer->size);

    if (enviar_todo(socket_cliente, a_enviar, bytes) == -1)
        perror("Error al enviar el paquete");

    free(a_enviar);
}

void enviar_mensaje(int socket_cliente, const char *mensaje)
{
    int tamanio = strlen(mensaje) + 1;
    if (enviar_todo(socket_cliente, &tamanio, sizeof(tamanio)) == -1 ||
        enviar_todo(socket_cliente, mensaje, tamanio) == -1)
        perror("Error al enviar el mensaje");
}

void eliminar_paquete(t_paquete *paquete)
{
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

void liberar_conexion(int socket_cliente)
{
    close(socket_cliente);
}

// servidor

int recibir_operacion(int socket_cliente)
{
    int cod_op;
    int resultado = recibir_todo(socket_cliente, &cod_op, sizeof(cod_op));
    if (resultado == 1)
        return cod_op;

    if (resultado == -1)
        perror("recv");
    close(socket_cliente);
    return -1;
}

void *recibir_buffer(int *size, int socket_cliente)
{
    void *buffer;

    if (recibir_todo(socket_cliente, size, sizeof(*size)) != 1 || *size <= 0)
        return NULL;
    buffer = malloc(*size);
    if (buffer == NULL)
        return NULL;
    if (recibir_todo(socket_cliente, buffer, *size) != 1)
    {
        free(buffer);
        return NULL;
    }

    return buffer;
}

char *recibir_mensaje(int socket_cliente)
{
    int size;
    char *buffer = recibir_buffer(&size, socket_cliente);
    // log_info(logger, "Me llego el mensaje %s", buffer);
    return buffer;
}

t_list *recibir_paquete(int socket_cliente)
{
    int size;
    int desplazamiento = 0;
    void *buffer;
    t_list *valores = list_create();
    int tamanio;

    buffer = recibir_buffer(&size, socket_cliente);
    if (buffer == NULL)
        return valores;
    while (desplazamiento < size)
    {
        if (size - desplazamiento < (int)sizeof(int))
            break;
        memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
        desplazamiento += sizeof(int);
        if (tamanio < 0 || tamanio > size - desplazamiento)
            break;
        char *valor = malloc(tamanio);
        if (valor == NULL)
            break;
        memcpy(valor, buffer + desplazamiento, tamanio);
        desplazamiento += tamanio;
        list_add(valores, valor);
    }
    free(buffer);
    return valores;
}
