#include <utils.h>
#include "kernel.h"

int socket_servidor;
int puerto_escucha;
t_config* config;
int socket_cpu;
t_log_level log_level; // en el config se define como string, pero acá lo guardamos como t_log_level para usarlo con el logger
t_log* logger;

int main(int argc, char* argv[]) {
    saludar("kernel");
    iniciar_config();
    logger = log_create("kernel.log", "KERNEL", 1, log_level);
    socket_servidor = iniciar_servidor(puerto_escucha); // kernel actúa como servidor
    socket_cpu = esperar_conexion(socket_servidor, logger); // cpu se conecta como cliente
    log_info(logger, "CPU se conecto al kernel :D");
    return 0;
}

void iniciar_config(){
    config = config_create("kernel.config");
    if (config == NULL) {
        config = config_create("src/kernel.config");
    }

    if (config == NULL) {
        fprintf(stderr, "No se pudo cargar kernel.config\n");
        exit(EXIT_FAILURE);
    }

    puerto_escucha = config_get_int_value(config, "PUERTO_ESCUCHA");
    log_level = log_level_from_string(config_get_string_value(config, "LOG_LEVEL")); // obtengo el log_level como string
}