#include "memoria.h"

t_log *logger;
t_log_level log_level;
t_config *config;
int puerto_escucha;
int socket_servidor;
int socket_cpu;
int socket_memoria;

int main(int argc, char *argv[])
{
    saludar("memoria");
    iniciar_config();
    logger = log_create("memoria.log", "MEMORIA", 0, LOG_LEVEL_INFO);
    socket_servidor = iniciar_servidor(puerto_escucha);     // memoria actúa como servidor
    socket_cpu = esperar_conexion(socket_servidor, logger); // cpu se conecta como cliente
    socket_memoria = crear_conexion(ip_memoria, puerto_memoria); 
    return 0;
}

void iniciar_config()
{
    config = config_create("memoria.config");
    log_level = log_level_from_string(config_get_string_value(config, "LOG_LEVEL"));
    puerto_escucha = config_get_int_value(config, "PUERTO_ESCUCHA");
}