#ifndef PROCESAR_CARPINCHOS_H_
#define PROCESAR_CARPINCHOS_H_

#include "deadlock.h"
#include "dispositivos_io.h"
#include "kernel.h"
#include "mensajes_memoria.h"
#include "semaforos.h"


void algoritmo_planificador_largo_plazo();
void algoritmo_planificador_mediano_plazo_ready_suspended();
void algoritmo_planificador_mediano_plazo_blocked_suspended();
void algoritmo_planificador_corto_plazo();
void correr_dispatcher(pcb_carpincho* pcb);
pcb_carpincho* algoritmo_SJF();
pcb_carpincho* algoritmo_HRRN();
void* minimum(pcb_carpincho*, pcb_carpincho*);
bool criterio_remocion_lista(void* pcb);
void ejecutar(t_nucleo_cpu*);
void recibir_peticion_para_continuar(int conexion);
void dar_permiso_para_continuar(int conexion);
void matar_nucleo_cpu();
void mate_close(t_nucleo_cpu*);

#endif
