#ifndef SEMAFOROS_H_
#define SEMAFOROS_H_

#include "deadlock.h"
#include "dispositivos_io.h"
#include "kernel.h"
#include "procesar_carpinchos.h"
#include "mensajes_memoria.h"

void init_sem(t_nucleo_cpu*);
void wait_sem(t_nucleo_cpu*);
void post_sem(t_nucleo_cpu*);
void destroy_sem(t_nucleo_cpu*);

#endif