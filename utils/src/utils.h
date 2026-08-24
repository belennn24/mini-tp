#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <commons/log.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <pthread.h>
#include <commons/collections/queue.h>
#include <semaphore.h>

void saludar(char *quien);
typedef enum
{
    // Generales
    PAQUETE,
    OK,
    // Memoria
    LEER,
    ESCRIBIR,
    PAGINA_FISICA,
    CARGAR_PROCESO,
    DESCARGAR_PROCESO,
    RTA_LECTURA,
    CPU,
    KERNEL,
    // Kernel
    PROCESO_CARGADO,
    ESPACIO_INSUFICIENTE,
    INIT_PROC,
    FIN_PROC,
    // CPU
    PROCESO
} op_code;

typedef struct
{
    int size;
    void *stream;
} t_buffer;

typedef struct
{
    op_code codigo_operacion;
    t_buffer *buffer;
} t_paquete;

int iniciar_servidor(int puerto);
int esperar_conexion(int fd_escucha, t_log *logger);
int crear_conexion(char *ip, int puerto);
void enviar_operacion(int socket_cliente, int op_code);
t_paquete *crear_paquete(void);
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);
void enviar_paquete(int socket_cliente, t_paquete *paquete);
void enviar_mensaje(int socket_cliente, const char *mensaje);
void eliminar_paquete(t_paquete *paquete);
void liberar_conexion(int socket_cliente);
int recibir_operacion(int socket_cliente);
void *recibir_buffer(int *size, int socket_cliente);
char* recibir_mensaje(int socket_cliente);
t_list *recibir_paquete(int socket_cliente);

#endif /* UTILS_H_ */
