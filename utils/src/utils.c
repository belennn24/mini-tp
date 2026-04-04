#include "utils.h"


void saludar(char* quien) {
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

	if (getaddrinfo(NULL, puerto_str, &hints, &servinfo) != 0) return -1;

    int fd_escucha = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (fd_escucha == -1) { freeaddrinfo(servinfo); return -1; }

    setsockopt(fd_escucha, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    if (bind(fd_escucha, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        freeaddrinfo(servinfo);
		close(fd_escucha);
        return -1;
    }

    if (listen(fd_escucha, SOMAXCONN) == -1) {
        freeaddrinfo(servinfo);
		close(fd_escucha);
        return -1;
    }

    freeaddrinfo(servinfo);
    return fd_escucha;
}

int esperar_conexion(int fd_escucha, t_log* logger)
{
    int fd_conexion = accept(fd_escucha, NULL, NULL);
    if (fd_conexion == -1) return -1;
    log_info(logger, "Se conecto un cliente!");
    return fd_conexion;
}

int crear_conexion(char *ip, int puerto)
//Conecta al servidor y devuelve el fd del socket cliente.
{
	struct addrinfo hints;
	struct addrinfo *server_info;
	char puerto_str[16];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

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
	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
	{
		freeaddrinfo(server_info);
		printf("Error al conectar el socket\n");
		close(socket_cliente);
		exit(1);
	}

	freeaddrinfo(server_info);

	return socket_cliente;
}