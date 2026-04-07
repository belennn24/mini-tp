#include "memoria.h"

t_log *logger;
t_log_level log_level;
t_config *config;
int puerto_escucha;
int socket_servidor;
int socket_cpu;
int socket_memoria;
int tam_memoria;
void* memoria_fisica;

int main(int argc, char *argv[])
{
    saludar("memoria");
    iniciar_config();
    logger = log_create("memoria.log", "MEMORIA", 0, LOG_LEVEL_INFO);
    socket_servidor = iniciar_servidor(puerto_escucha);     // memoria actúa como servidor
    socket_cpu = esperar_conexion(socket_servidor, logger); // cpu se conecta como cliente
    socket_memoria = crear_conexion(ip_memoria, puerto_memoria); 
    memoria_fisica = malloc(tam_memoria); //reservo memoria (dinámica)
    return 0;
}

char* leer_memoria(int direccion, int tamanio) {
    char* lectura = malloc(tamanio + 1); //reservo espacio para el string que voy a devolver
    memcpy(lectura, memoria_fisica + direccion, tamanio); // memcpy copia n bytes desde la dirección de memoria origen a la dirección de memoria destino
    // 1° arg: destino (donde quiero copiar) 
    // 2° arg: origen (de donde quiero copiar) memoria_fisica + direccion significa “empezar a leer desde el byte direccion dentro del bloque de memoria física”.
    // 3° arg: cantidad de bytes a copiar
    lectura[tamanio] = '\0'; // agrego el carácter de fin de string
    return lectura;
}
void iniciar_config()
{
    config = config_create("memoria.config");
    log_level = log_level_from_string(config_get_string_value(config, "LOG_LEVEL"));
    puerto_escucha = config_get_int_value(config, "PUERTO_ESCUCHA");
    tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
}