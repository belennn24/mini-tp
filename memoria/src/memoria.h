#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/config.h>
#include <commons/log.h>
#include <utils.h>
#include <commons/bitarray.h>
#include <commons/collections/dictionary.h>

typedef struct {
    int cant_pags;
    int* entradas;
} tabla_de_paginas;

void iniciar_config();
char* leer_memoria(int direccion, int tamanio);
void escribir_memoria(int direccion, char* contenido);
bool reservar_memoria_para_proceso(int pid, int tam);
int contar_digitos(int num);
void crear_bitmap();
char* de_int_a_string(int ent);
void liberar_memoria_de_proceso(int pid);
int paginaFisicaDeLogica(int pid, int pagina_logica);
void identificar_cliente();
void* atender_cpu(void* args);
void* atender_kernel(void* args);

#endif /* MEMORIA_H_ */