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
    // cpu cliente de kernel y de memoria
    socket_kernel = crear_conexion(ip_kernel, puerto_kernel);
    socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
    enviar_operacion(socket_memoria, CPU);

    return 0;
}

void iniciar_config(){
    config = config_create("cpu.config");
    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_int_value(config, "PUERTO_KERNEL");
    log_level = log_level_from_string(config_get_string_value(config, "LOG_LEVEL"));
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_int_value(config, "PUERTO_MEMORIA");

}