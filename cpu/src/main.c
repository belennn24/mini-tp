#include "cpu.h"

char *ip_kernel;
int puerto_kernel;
char *ip_memoria;
int puerto_memoria;
int socket_memoria;
int socket_kernel;
t_log_level log_level;
t_config *config;
t_log *logger;
char *ruta_scripts;
int tam_pagina;

// acá, en lugar de traducir las direcciones de memoria que sean necesarias,
//  se va a sobreescribir el espacio de memoria de otro proceso (cosa que no debería pasar, pero sirve a fines de aprendizaje)
int main(int argc, char *argv[])
{
    saludar("cpu");
    iniciar_config();
    logger = log_create("cpu.log", "CPU", 1, log_level);
    // cpu cliente de kernel y de memoria
    socket_kernel = crear_conexion(ip_kernel, puerto_kernel);
    socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
    enviar_operacion(socket_memoria, CPU);
    tam_pagina = recibir_operacion(socket_memoria);
    atender_kernel();

    return 0;
}

void iniciar_config()
{
    config = config_create("cpu.config");
    if (config == NULL)
    {
        config = config_create("src/cpu.config");
    }

    if (config == NULL)
    {
        fprintf(stderr, "No se pudo cargar cpu.config\n");
        exit(EXIT_FAILURE);
    }
    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_int_value(config, "PUERTO_KERNEL");
    log_level = log_level_from_string(config_get_string_value(config, "LOG_LEVEL"));
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_int_value(config, "PUERTO_MEMORIA");
    ruta_scripts = config_get_string_value(config, "RUTA_SCRIPTS");
}

void atender_kernel()
{
    while (1)
    {
        int op_code = recibir_operacion(socket_kernel);
        switch (op_code)
        {
        case PROCESO:
            t_list *lista_paquete = recibir_paquete(socket_kernel);
            if (list_size(lista_paquete) != 2 ||
                list_get(lista_paquete, 0) == NULL ||
                list_get(lista_paquete, 1) == NULL)
            {
                log_error(logger, "Paquete PROCESO invalido");
                list_destroy_and_destroy_elements(lista_paquete, free);
                break;
            }
            int pid = *(int *)list_get(lista_paquete, 0);
            char *codigo = strdup((char *)list_get(lista_paquete, 1));
            log_info(logger, "PID: %d - Script: %s - Inicio de ejecucion", pid, codigo);
            ejecutar_script(pid, codigo);
            log_info(logger, "PID: %d - Script: %s - Fin de ejecucion", pid, codigo);
            enviar_operacion(socket_kernel, FIN_PROC);
            enviar_operacion(socket_kernel, pid);
            free(codigo);
            list_destroy_and_destroy_elements(lista_paquete, free);
            break;
        }
    }
}

void ejecutar_script(int pid, char *codigo)
{
    char *ruta_script;
    if (codigo[0] == '/')
    {
        ruta_script = strdup(codigo);
    }
    else
    {
        ruta_script = malloc(strlen(ruta_scripts) + strlen(codigo) + 1);
        snprintf(ruta_script, strlen(ruta_scripts) + strlen(codigo) + 1, "%s%s", ruta_scripts, codigo);
    }
    FILE *archivo = fopen(ruta_script, "r");
    if (archivo == NULL)
    {
        log_error(logger, "No se pudo abrir el script %s", ruta_script);
        free(ruta_script);
        return;
    }
    char *linea = NULL;
    size_t len;
    int dir;
    int tam;
    while (getline(&linea, &len, archivo) != -1)
    {
        if (strncmp(linea, "WRITE", 5) == 0)
        {
            char *mensaje = NULL;
            sscanf(linea, "WRITE %d %ms", &dir, &mensaje);
            ejecutar_write(pid, dir, mensaje);
            free(mensaje);
        }
        else if (strncmp(linea, "READ", 4) == 0)
        {
            sscanf(linea, "READ %d %d", &dir, &tam);
            ejecutar_read(pid, dir, tam);
        }
        else if (strncmp(linea, "INIT_PROC", 9) == 0)
        {
            char *nombre = NULL;
            if (sscanf(linea, "INIT_PROC %ms %d", &nombre, &tam) == 2)
            {
                ejecutar_init_proc(tam, nombre);
            }
            else
            {
                log_error(logger, "Instruccion INIT_PROC invalida: %s", linea);
            }
            free(nombre);
        }
    }
    fclose(archivo);
    free(ruta_script);
    free(linea);
}

void ejecutar_write(int pid, int direccion, char *mensaje)
{
    t_paquete *paquete = crear_paquete();
    paquete->codigo_operacion = ESCRIBIR;

    agregar_a_paquete(paquete, &pid, sizeof(int));
    agregar_a_paquete(paquete, &direccion, sizeof(int));
    agregar_a_paquete(paquete, mensaje, strlen(mensaje) + 1);

    enviar_paquete(socket_memoria, paquete);
    eliminar_paquete(paquete);

    int respuesta = recibir_operacion(socket_memoria);
    if (respuesta == OK)
    {
        log_info(logger, "PID: %d - Acción: WRITE - Dirección Física: %d - Mensaje: %s", pid, direccion, mensaje);
    }
}

void ejecutar_read(int pid, int direccion, int tam)
{
    t_paquete *paquete = crear_paquete();
    paquete->codigo_operacion = LEER;

    agregar_a_paquete(paquete, &pid, sizeof(int));
    agregar_a_paquete(paquete, &direccion, sizeof(int));
    agregar_a_paquete(paquete, &tam, sizeof(int));

    enviar_paquete(socket_memoria, paquete);
    eliminar_paquete(paquete);

    int respuesta = recibir_operacion(socket_memoria);
    if (respuesta == RTA_LECTURA)
    {
        char *leido = recibir_mensaje(socket_memoria);
        log_info(logger, "PID: %d - Acción: READ - Dirección Física: %d - Valor Leído: %s", pid, direccion, leido);
        free(leido);
    }
}

void ejecutar_init_proc(int tam, char *nombre)
{

    t_paquete *paquete = crear_paquete();
    paquete->codigo_operacion = INIT_PROC;

    agregar_a_paquete(paquete, &tam, sizeof(int));
    agregar_a_paquete(paquete, nombre, strlen(nombre) + 1);

    enviar_paquete(socket_kernel, paquete);
    eliminar_paquete(paquete);

    log_info(logger, "Solicitud de INIT_PROC enviada al Kernel con tamaño %d y archivo %s", tam, nombre);
}
