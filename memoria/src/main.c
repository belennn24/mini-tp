#include "memoria.h"

t_log *logger;
t_log_level log_level;
t_config *config;
int puerto_escucha;
int socket_servidor;
int socket_cpu;
int socket_kernel;
int socket_memoria;
int tam_memoria;
int tam_pagina;
void *memoria_fisica;
t_bitarray *bitmap;
t_dictionary *de_pid_a_tabla;
int memoria_restante;

// uso paginación

int main(int argc, char *argv[])
{
    saludar("memoria");
    iniciar_config();
    logger = log_create("memoria.log", "MEMORIA", 0, LOG_LEVEL_INFO);
    socket_servidor = iniciar_servidor(puerto_escucha); // memoria actúa como servidor

    memoria_fisica = malloc(tam_memoria); // reservo memoria (dinámica)

    crear_bitmap();
    de_pid_a_tabla = dictionary_create();
    // escribir_memoria(500,"Holaa");
    // leer_memoria(500,4);
    identificar_cliente();
    return 0;
}

char *leer_memoria(int direccion, int tamanio)
{
    char *lectura = malloc(tamanio + 1);                  // reservo espacio para el string que voy a devolver
    memcpy(lectura, memoria_fisica + direccion, tamanio); // memcpy copia n bytes desde la dirección de memoria origen a la dirección de memoria destino
    // 1° arg: destino (donde quiero copiar)
    // 2° arg: origen (de donde quiero copiar) memoria_fisica + direccion significa empezar a leer desde el byte direccion dentro del bloque de memoria física.
    // 3° arg: cantidad de bytes a copiar
    lectura[tamanio] = '\0'; // agrego el carácter de fin de string
    return lectura;
}

void escribir_memoria(int direccion, char *contenido)
{
    memccpy(memoria_fisica + direccion, contenido, '\0', strlen(contenido));
    log_info(logger, "Escritura realizada. Direccion %d, contenido %s", direccion, contenido);
}
void iniciar_config()
{
    config = config_create("memoria.config");
    log_level = log_level_from_string(config_get_string_value(config, "LOG_LEVEL"));
    puerto_escucha = config_get_int_value(config, "PUERTO_ESCUCHA");
    tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    tam_pagina = config_get_int_value(config, "TAM_PAGINA");
}

void crear_bitmap()
{
    int cant_paginas = tam_memoria / tam_pagina;
    int tam_bitmap_en_bytes = (cant_paginas + 7) / 8;
    char *contenido_bitmap = malloc(tam_bitmap_en_bytes);
    memset(contenido_bitmap, 0, tam_bitmap_en_bytes);
    bitmap = bitarray_create_with_mode(contenido_bitmap, tam_bitmap_en_bytes, LSB_FIRST);
}

char *de_int_a_string(int ent)
{
    int tam = contar_digitos(ent);
    char *buffer = malloc(tam + 1);
    snprintf(buffer, tam + 1, "%d", ent);
    return buffer;
}

int contar_digitos(int num)
{
    if (num == 0)
        return 1;
    int count = 0;
    while (num > 0)
    {
        num /= 10;
        count++;
    }
    return count;
}

bool reservar_memoria_para_proceso(int pid, int tam)
{
    int paginas_necesarias = (tam + tam_pagina - 1) / tam_pagina; // redondea hacia arriba
    // contar marcos libres en el bitmap
    int marcos_libres = 0;
    size_t total_marcos = bitarray_get_max_bit(bitmap);
    for (int i = 0; i < total_marcos; i++)
    {
        if (!bitarray_test_bit(bitmap, i))
        {
            marcos_libres++;
        }
    }
    if (marcos_libres < paginas_necesarias)
    {
        log_error(logger, "No hay suficiente memoria libre");
        return false;
    }

    tabla_de_paginas *tabla = malloc(sizeof(tabla_de_paginas));
    tabla->cant_pags = paginas_necesarias;
    tabla->entradas = malloc(paginas_necesarias * sizeof(int));

    // asignar los marcos libres y marcarlos en el bitmap
    int paginas_asignadas = 0;
    for (size_t i = 0; i < total_marcos && paginas_asignadas < paginas_necesarias; i++)
    {
        if (!bitarray_test_bit(bitmap, i))
        {
            bitarray_set_bit(bitmap, i);            // esto marca como ocupado el marco
            tabla->entradas[paginas_asignadas] = i; // número de marco asignado
            paginas_asignadas++;
        }
    }
    // convertir el PID a string para usarlo como clave del diccionario
    char *pid_key = de_int_a_string(pid);
    dictionary_put(de_pid_a_tabla, pid_key, tabla);
    free(pid_key); // dictionary_put duplica la clave, por lo que debemos liberar la memoria solicitada en de_int_a_string para evitar memory leaks
    log_info(logger, "Memoria reservada para el proceso %d", pid);
    return true;
}

void liberar_memoria_de_proceso(int pid)
{
    // 1. Convertir el PID a string y guardarlo en una variable
    char *pid_key = de_int_a_string(pid);

    // 2. Remover la tabla del diccionario usando esa key
    tabla_de_paginas *tabla = dictionary_remove(de_pid_a_tabla, pid_key);

    // 3. Liberar la key temporal para evitar el memory leak
    free(pid_key);

    // 4. Validación por seguridad por si el proceso no tenía memoria asignada
    if (tabla == NULL)
    {
        log_error(logger, "No se pudo liberar memoria: el PID %d no existe en el diccionario", pid);
        return;
    }

    // 5. Limpiar los bits en el bitmap
    for (int i = 0; i < tabla->cant_pags; i++)
    {
        bitarray_clean_bit(bitmap, tabla->entradas[i]);
    }

    // 6. Actualizar la memoria restante y liberar la estructura interna
    memoria_restante += tabla->cant_pags * tam_pagina;
    free(tabla->entradas);
    free(tabla);

    log_info(logger, "Memoria liberada para el proceso %d", pid);
}

int paginaFisicaDeLogica(int pid, int pagina_logica)
{
    // 1. Convertir el PID a string para buscarlo en el diccionario
    char *pid_key = de_int_a_string(pid);

    // 2. Obtener la tabla de páginas del proceso
    tabla_de_paginas *tabla = dictionary_get(de_pid_a_tabla, pid_key);

    // Liberar la key temporal para evitar memory leaks
    free(pid_key);

    // Validación por seguridad
    if (tabla == NULL)
    {
        log_error(logger, "No se encontró la tabla de páginas para el PID %d", pid);
        return -1;
    }

    // Validar que la página lógica esté dentro del rango del proceso
    if (pagina_logica >= tabla->cant_pags)
    {
        log_error(logger, "La página lógica %d excede el tamaño del proceso PID %d", pagina_logica, pid);
        return -1;
    }

    // 3. Retornar el número de marco físico correspondiente a esa entrada
    return tabla->entradas[pagina_logica];
}

void identificar_cliente()
{
    socket_servidor = iniciar_servidor(puerto_escucha);
    for (int i = 0; i < 2; i++)
    {
        int socket_misterioso = esperar_conexion(socket_servidor);
        int op_code = recibir_operacion(socket_misterioso);
        if (op_code == KERNEL)
        {
            socket_kernel = socket_misterioso;
            log_info(logger, "Kernel se conecto a la memoria :D");
        }
        else if (op_code == CPU)
        {
            socket_cpu = socket_misterioso;
            log_info(logger, "CPU se conecto a la memoria :D");
        }
    }
}

void atender_cpu()
{
    while (1)
    {
        int op_code = recibir_operacion(socket_cpu);
        switch (op_code)
        {
        case LEER:
            break;
        case ESCRIBIR:
            break;
        case PAGINA_FISICA:
            break;
        }
    }
}

void atender_kernel()
{
    while (1)
    {
        int op_code = recibir_operacion(socket_kernel);
        switch (op_code)
        {
        case CARGAR_PROCESO:
            break;
        case DESCARGAR_PROCESO:
            break;
        }
    }
}