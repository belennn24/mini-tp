#ifndef CPU_H_
#define CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/config.h>
#include <commons/log.h>
#include <utils.h>

void iniciar_config();
void atender_kernel();
void ejecutar_script(int pid, char *codigo);
void ejecutar_write(int pid, int direccion, char *mensaje);
void ejecutar_read(int pid, int direccion, int tam);
void ejecutar_init_proc(int tam, char *nombre);

#endif /* CPU_H_ */