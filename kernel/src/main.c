#include "kernel.h"

int socket_servidor;
int puerto_escucha;
t_config *config;
int socket_cpu;
t_log_level log_level; // en el config se define como string, pero acá lo guardamos como t_log_level para usarlo con el logger
t_log *logger;
char *ip_memoria;
int puerto_memoria;
int socket_memoria;
t_queue *cola_new;
t_queue *cola_ready;
sem_t sem_new;
sem_t sem_ready;
sem_t sem_cpu_disponible;
pthread_mutex_t mutex_cpu;


int main(int argc, char *argv[])
{
    saludar("kernel");
    iniciar_config();
    logger = log_create("kernel.log", "KERNEL", 1, log_level);
    socket_servidor = iniciar_servidor(puerto_escucha);     // kernel actúa como servidor
    socket_cpu = esperar_conexion(socket_servidor, logger); // cpu se conecta como cliente
    socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
    enviar_operacion(socket_memoria, KERNEL);
    log_info(logger, "CPU se conecto al kernel :D");
    cola_new = queue_create();
    cola_ready = queue_create();

    return 0;
}

void iniciar_config()
{
    config = config_create("kernel.config");
    if (config == NULL)
    {
        config = config_create("src/kernel.config");
    }

    if (config == NULL)
    {
        fprintf(stderr, "No se pudo cargar kernel.config\n");
        exit(EXIT_FAILURE);
    }

    puerto_escucha = config_get_int_value(config, "PUERTO_ESCUCHA");
    log_level = log_level_from_string(config_get_string_value(config, "LOG_LEVEL")); // obtengo el log_level como string
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_int_value(config, "PUERTO_MEMORIA");
}

void iniciar_semaforos()
{
    sem_init(&sem_new, NULL, 1);
    sem_init(&sem_ready, NULL, 1);
}

void *planificador_largo_plazo(void *args)
{
    while (1)
    {
        sem_wait(&sem_new);
        proceso *p = queue_peek(cola_new);
        if (p == NULL)
        {
            sleep(1);
            continue;
        }
        t_paquete *paquete = crear_paquete();
        paquete->codigo_operacion = CARGAR_PROCESO;
        agregar_a_paquete(paquete, &(p->pid), sizeof(int));
        agregar_a_paquete(paquete, &(p->tamanio), sizeof(int));
        enviar_paquete(socket_memoria, paquete);
        eliminar_paquete(paquete);

        int rta_memoria = recibir_operacion(socket_memoria);
        if (rta_memoria == PROCESO_CARGADO)
        {
            log_info(logger, "Se agrega proceso %d a la cola ready", p->pid);
            queue_push(cola_ready, queue_pop(cola_new));
            sem_post(&sem_ready);
        }
        else if (rta_memoria == ESPACIO_INSUFICIENTE)
        {
            log_info(logger, "No hay espacio en Memoria para procedo %d de tamanio %d", p->pid, p->tamanio);
        }
    }
}

void *planificador_corto_plazo(void *args)
{
    while (1)
    {
        sem_wait(&sem_ready);
        sem_wait(&sem_cpu_disponible);
        pthread_mutex_lock(&mutex_cpu);
        proceso *p = queue_pop(cola_ready);
        pthread_mutex_unlock(&mutex_cpu);
        t_paquete *paquete = crear_paquete();
        agregar_a_paquete(paquete, &(p->pid), sizeof(int));
        agregar_a_paquete(paquete, &(p->tamanio), sizeof(int));
        enviar_paquete(socket_cpu, paquete);
        eliminar_paquete(paquete);
    }
}
