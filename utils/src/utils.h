#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <commons/log.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/collections/list.h>
#include <commons/config.h>


void saludar(char *quien);

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef enum{
    MENSAJE, 
    ENTERO
} utils_op_code;

typedef enum {
    KERNEL,
    CPU,
    LEER,
    ESCRIBIR,
    PAGINA_FISICA,
    CARGAR_PROCESO,
    DESCARGAR_PROCESO
} op_code_memoria;

int iniciar_servidor(int puerto);
int esperar_conexion(int fd_escucha, t_log *logger);
int crear_conexion(char *ip, int puerto);
void enviar_operacion(int socket_cliente, int op_code);
t_paquete *crear_paquete(void)
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio)
void eliminar_paquete(t_paquete *paquete)
void liberar_conexion(int socket_cliente)
int recibir_operacion(int socket_cliente)
void *recibir_buffer(int *size, int socket_cliente)
void recibir_mensaje(int socket_cliente)
t_list *recibir_paquete(int socket_cliente)
#endif /* KERNEL_H_ */