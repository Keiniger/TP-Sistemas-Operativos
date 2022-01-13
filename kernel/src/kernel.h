#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <futiles.h>
#include <sys/time.h>

int SERVIDOR_KERNEL;
int SERVIDOR_MEMORIA;
t_config* CONFIG;

//ESTRUCTURAS
typedef struct {
    char* ip_memoria;
    char* puerto_memoria;
    char* ip_kernel;
    char* puerto_kernel;
    char* alg_plani;
    int estimacion_inicial;
    double alfa;
    char** dispositivos_IO;
    char** duraciones_IO;
    int tiempo_deadlock;
    int grado_multiprogramacion;
	int grado_multiprocesamiento;
}t_kernel_config;

typedef enum{
	MATE_SUCCESS = 1,
	MATE_FREE_FAULT = -5,
	MATE_READ_FAULT = -6,
	MATE_WRITE_FAULT = -7
}MATE_RETURNS;

typedef struct {
	char* nombre;
	uint32_t cantidad;
}t_registro_uso_recurso;

typedef struct {
	uint32_t pid;
	unsigned long real_anterior;
	double estimado_anterior;
	uint32_t tiempo_espera;
	int conexion;
	t_list* recursos_usados;
}pcb_carpincho;

typedef struct {
	pcb_carpincho* lugar_pcb_carpincho;
	sem_t sem_exec;
	bool ocupado;
	struct timeval timeValBefore;
	struct timeval timeValAfter;
}t_nucleo_cpu;

typedef struct {
	char* nombre;
	int value;
	t_list* cola_bloqueados;
}t_semaforo;

typedef struct {
	char* nombre;
	uint32_t rafaga;
	sem_t sem;
	t_list* cola_espera;
}t_dispositivo;

//VARIABLES GLOBALES
t_list_iterator* iterador_lista_ready;
t_kernel_config CONFIG_KERNEL;
t_log* LOGGER;

//SEMAFOROS
sem_t sem_algoritmo_planificador_largo_plazo;
sem_t sem_cola_ready;
sem_t sem_grado_multiprogramacion;
sem_t sem_grado_multiprocesamiento;

//LISTAS
t_list* LISTA_NEW;
t_list* LISTA_READY;
t_list* LISTA_EXEC;
t_list* LISTA_BLOCKED;
t_list* LISTA_BLOCKED_SUSPENDED;
t_list* LISTA_READY_SUSPENDED;
t_list* LISTA_NUCLEOS_CPU;
t_list* LISTA_SEMAFOROS;
t_list* LISTA_DISPOSITIVOS_IO;

//MUTEXES
pthread_mutex_t mutex_id_unico;
pthread_mutex_t mutex_lista_new;
pthread_mutex_t mutex_lista_ready;
pthread_mutex_t mutex_lista_exec;
pthread_mutex_t mutex_lista_blocked;
pthread_mutex_t mutex_lista_blocked_suspended;
pthread_mutex_t mutex_lista_ready_suspended;
pthread_mutex_t mutex_lista_nucleos_cpu;
pthread_mutex_t mutex_lista_semaforos_mate;
pthread_mutex_t mutex_lista_dispositivos_io;

//HILOS
pthread_t planificador_largo_plazo;
pthread_t planificador_corto_plazo;
pthread_t administrador_deadlock;

#include "deadlock.h"
#include "dispositivos_io.h"
#include "procesar_carpinchos.h"
#include "mensajes_memoria.h"
#include "semaforos.h"

int crear_id_unico();
t_kernel_config crear_archivo_config_kernel(char* ruta);
void init_kernel(char* pathConfig);
void atender_carpinchos(int* cliente);
void coordinador_multihilo();
void pasar_a_new(pcb_carpincho* pcb_carpincho);
void iniciar_planificador_largo_plazo();
void iniciar_planificador_corto_plazo();
void iniciar_algoritmo_deadlock();
void crear_nucleos_cpu();
void crear_dispositivos_io();
void cerrar_kernel();

#endif


