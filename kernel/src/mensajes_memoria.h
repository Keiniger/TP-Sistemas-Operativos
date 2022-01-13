#ifndef MEMORIA_H_
#define MEMORIA_H_

#include "deadlock.h"
#include "dispositivos_io.h"
#include "kernel.h"
#include "procesar_carpinchos.h"
#include "semaforos.h"

void memalloc(t_nucleo_cpu*);
void memread(t_nucleo_cpu*);
void memfree(t_nucleo_cpu*);
void memwrite(t_nucleo_cpu*);
void avisar_suspension_a_memoria(pcb_carpincho* pcb);
void avisar_dessuspension_a_memoria(pcb_carpincho* pcb);
void avisar_close_a_memoria(pcb_carpincho* pcb);

#endif