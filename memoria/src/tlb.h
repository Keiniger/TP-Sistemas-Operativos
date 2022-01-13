#ifndef TLB_H_
#define TLB_H_

#include "memoria.h"

// ESTRUCTURAS

typedef struct {
    int pag;
    int frame;
    int pid;
    int ultimo_uso;
}t_entrada_tlb;

typedef struct {
	int pid;
	int contador;
}tlb_event;

// VARIABLES

int TIEMPO_TLB;
t_list* TLB;

t_list* TLB_HITS;
t_list* TLB_MISS;

//// --- semaforos
pthread_mutex_t mutexTiempoTLB;
pthread_mutex_t mutexTLB;

// FUNCIONES

//// ---- funciones de inicializacion
void init_tlb();
void crear_entrada(int pid, int pag, int frame);

//// ---- funciones de reemplazo
void reemplazar_FIFO(int pid, int pag, int frame);
void reemplazar_LRU(int pid, int pag, int frame);

//// ---- funciones principales
int  buscar_pag_tlb(int pid, int pag);
void actualizar_tlb(int pid, int pag, int frame);
void registrar_evento(int pid, int event);
void desreferenciar_pag_tlb(int pid , int nro_pagina , int frame);

//// ---- funciones secundarias
int  obtener_tiempo_TLB();
void printear_tlb();

//// ---- funciones signals
void generar_metricas_tlb();
void dumpear_tlb();
void limpiar_tlb();

void destruir_tlb();

//// ---- funciones DUMP
void  mostrar_entradas_tlb(FILE* file);
void  mostrar_entrada(int entrada, FILE* file);
char* nombrar_dump_file();


#endif /* TLB_H_ */
