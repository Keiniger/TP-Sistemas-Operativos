#ifndef DEADLOCK_H_
#define DEADLOCK_H_

#include "dispositivos_io.h"
#include "kernel.h"
#include "procesar_carpinchos.h"
#include "mensajes_memoria.h"
#include "semaforos.h"

void correr_algoritmo_deadlock();
void avisar_finalizacion_por_deadlock(int conexion);
void matar_algoritmo_deadlock();
void algoritmo_deteccion_deadlock();

#endif