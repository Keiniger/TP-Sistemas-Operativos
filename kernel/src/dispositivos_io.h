#ifndef DISPOSITIVOS_IO_H_
#define DISPOSITIVOS_IO_H_

#include "deadlock.h"
#include "kernel.h"
#include "procesar_carpinchos.h"
#include "mensajes_memoria.h"
#include "semaforos.h"

void matar_dispositivo_io();
void call_IO(t_nucleo_cpu*);
void ejecutar_io(t_dispositivo*);

#endif
