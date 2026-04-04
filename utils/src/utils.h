
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <commons/log.h>
#include <netdb.h>
#include <unistd.h>

void saludar(char* quien);

int iniciar_servidor(int puerto);
int esperar_conexion(int fd_escucha, t_log* logger);
int crear_conexion(char* ip, int puerto);