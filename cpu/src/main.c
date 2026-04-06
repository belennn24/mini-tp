
#include "cpu.h"

char* ip_kernel;
int puerto_kernel;
char* ip_memoria;
int puerto_memoria;
int socket_memoria;
int socket_kernel;
t_log_level log_level;
t_config* config;
t_log* logger;

int main(int argc, char* argv[]) {
    saludar("cpu");
    iniciar_config();
    logger = log_create("cpu.log", "CPU", 0, log_level);
    crear_conexion(ip_kernel, puerto_kernel);
    return 0;
}

void iniciar_config(){
    config = config_create("cpu.config");
    if (config == NULL) {
        config = config_create("src/cpu.config");
    }

    if (config == NULL) {
        fprintf(stderr, "No se pudo cargar cpu.config\n");
        exit(EXIT_FAILURE);
    }
    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_int_value(config, "PUERTO_KERNEL");
    log_level = log_level_from_string(config_get_string_value(config, "LOG_LEVEL"));
}