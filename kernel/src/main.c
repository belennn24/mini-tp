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
pthread_mutex_t mutex_ready;
pthread_mutex_t mutex_new;
pthread_mutex_t mutex_memoria;

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Error: Faltan argumentos. Uso: %s <ruta_script> <tamanio>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *primer_codigo = strdup(argv[1]);
    int primer_tamanio = atoi(argv[2]);
    saludar("kernel");
    iniciar_config();
    logger = log_create("kernel.log", "KERNEL", 1, log_level);
    socket_servidor = iniciar_servidor(puerto_escucha);     // kernel actúa como servidor
    socket_cpu = esperar_conexion(socket_servidor, logger); // cpu se conecta como cliente
    socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
    enviar_operacion(socket_memoria, KERNEL);
    log_info(logger, "CPU se conecto al kernel :D");
    iniciar_semaforos();

    cola_new = queue_create();
    cola_ready = queue_create();
    iniciar_proceso(primer_tamanio, primer_codigo);
    pthread_t hilo_largo_plazo;
    pthread_t hilo_corto_plazo;
    pthread_t hilo_cpu;
    pthread_create(&hilo_largo_plazo, NULL, planificador_largo_plazo, NULL);
    pthread_detach(hilo_largo_plazo);
    pthread_create(&hilo_corto_plazo, NULL, planificador_corto_plazo, NULL);
    pthread_detach(hilo_corto_plazo);
    pthread_create(&hilo_cpu, NULL, atender_cpu, NULL);
    pthread_join(hilo_cpu, NULL);

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
    sem_init(&sem_new, 0, 0);
    sem_init(&sem_ready, 0, 0);
    sem_init(&sem_cpu_disponible, 0, 1); // la cpu empieza libre
    pthread_mutex_init(&mutex_ready, NULL);
    pthread_mutex_init(&mutex_new, NULL);
    pthread_mutex_init(&mutex_memoria, NULL);
}

void *planificador_largo_plazo(void *args)
{
    while (1)
    {
        sem_wait(&sem_new);
        pthread_mutex_lock(&mutex_new);
        proceso *p = queue_peek(cola_new);
        pthread_mutex_unlock(&mutex_new);

        if (p == NULL)
        {
            // Si el semáforo se activó pero la cola está vacía, algo raro pasó.
            // Esperamos un poco y continuamos para no consumir CPU inútilmente.
            sleep(1);
            continue;
        }
        t_paquete *paquete = crear_paquete();
        paquete->codigo_operacion = CARGAR_PROCESO;
        agregar_a_paquete(paquete, &(p->pid), sizeof(int));
        agregar_a_paquete(paquete, &(p->tamanio), sizeof(int));

        pthread_mutex_lock(&mutex_memoria);
        enviar_paquete(socket_memoria, paquete);
        eliminar_paquete(paquete);

        int rta_memoria = recibir_operacion(socket_memoria);
        pthread_mutex_unlock(&mutex_memoria);
        if (rta_memoria == PROCESO_CARGADO)
        {
            log_info(logger, "Se agrega proceso %d a la cola ready", p->pid);

            // Ahora que sabemos que se cargó, lo sacamos de NEW y lo ponemos en READY
            pthread_mutex_lock(&mutex_new);
            proceso *proceso_a_ready = queue_pop(cola_new);
            pthread_mutex_unlock(&mutex_new);

            pthread_mutex_lock(&mutex_ready);
            queue_push(cola_ready, proceso_a_ready);
            pthread_mutex_unlock(&mutex_ready);
            sem_post(&sem_ready);
        }
        else if (rta_memoria == ESPACIO_INSUFICIENTE)
        {
            log_info(logger, "No hay espacio en Memoria para procedo %d de tamanio %d", p->pid, p->tamanio);
            log_info(logger, "Planificador de Largo Plazo pausado por falta de memoria. Reintentará en 5 segundos...");
            sleep(5); // Pausamos para no saturar a la memoria con peticiones.
            sem_post(&sem_new);
        }
    }
}

void *planificador_corto_plazo(void *args)
{
    while (1)
    {
        sem_wait(&sem_ready);
        sem_wait(&sem_cpu_disponible);
        pthread_mutex_lock(&mutex_ready);
        proceso *p = queue_pop(cola_ready);
        pthread_mutex_unlock(&mutex_ready);
        if (p == NULL)
            continue;
        t_paquete *paquete = crear_paquete();
        paquete->codigo_operacion = PROCESO;
        agregar_a_paquete(paquete, &(p->pid), sizeof(int));
        if (p->codigo != NULL)
        {
            agregar_a_paquete(paquete, p->codigo, strlen(p->codigo) + 1);
        }
        else
        {
            char *vacio = "";
            agregar_a_paquete(paquete, vacio, strlen(vacio) + 1);
        }
        enviar_paquete(socket_cpu, paquete);
        log_info(logger, "Proceso %d enviado a EXEC", p->pid);
        eliminar_paquete(paquete);
        if (p->codigo != NULL)
        {
            free(p->codigo);
        }
        free(p);
    }
}

void *atender_cpu(void *args)
{
    while (1)
    {
        int op_code = recibir_operacion(socket_cpu);
        if (op_code == -1)
            break;
        t_list *lista_paquete;
        int pid;
        int tam;
        char *codigo;
        switch (op_code)
        {
        case INIT_PROC:
            lista_paquete = recibir_paquete(socket_cpu);
            if (list_size(lista_paquete) != 2 ||
                list_get(lista_paquete, 0) == NULL ||
                list_get(lista_paquete, 1) == NULL)
            {
                log_error(logger, "Paquete INIT_PROC invalido");
                list_destroy_and_destroy_elements(lista_paquete, free);
                break;
            }
            tam = *(int *)list_get(lista_paquete, 0);
            codigo = strdup((char *)list_get(lista_paquete, 1));
            iniciar_proceso(tam, codigo);
            list_destroy_and_destroy_elements(lista_paquete, free);
            break;
        case FIN_PROC:
            pid = recibir_operacion(socket_cpu); // Asumiendo que el PID se envía como una operación simple
            finalizar_proceso(pid);
            break;
        }
    }
    return NULL;
}

void iniciar_proceso(int tam, char *codigo)
{
    static int pid = 0;
    proceso *p = malloc(sizeof(proceso));
    p->pid = pid;
    p->tamanio = tam;
    p->codigo = codigo;
    log_info(logger, "Se agrega proceso %d a la cola new", p->pid);
    pthread_mutex_lock(&mutex_new);
    queue_push(cola_new, p);
    pthread_mutex_unlock(&mutex_new);
    sem_post(&sem_new);
    pid++;
}

void finalizar_proceso(int pid)
{
    pthread_mutex_lock(&mutex_memoria);
    enviar_operacion(socket_memoria, DESCARGAR_PROCESO);
    enviar_operacion(socket_memoria, pid);
    int respuesta = recibir_operacion(socket_memoria);
    pthread_mutex_unlock(&mutex_memoria);
    if (respuesta == OK)
    {
        log_info(logger, "Memoria liberada para el proceso %d", pid);
        sem_post(&sem_cpu_disponible);
    }
}
