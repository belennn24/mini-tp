#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/queue.h>
#include "utils.h"
typedef struct
{
    int pid;
    int tamanio;
    char *codigo;
} proceso;

void iniciar_config();
void iniciar_semaforos();
void *planificador_largo_plazo(void *args);
void *planificador_corto_plazo(void *args);
void* atender_cpu(void* args);
void iniciar_proceso(int tam, char* codigo);

#endif /* KERNEL_H_ */