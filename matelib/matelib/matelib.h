#ifndef MATELIB_H_
#define MATELIB_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/config.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>

//-------------------Type Definitions----------------------/
t_log* LOGGER;

typedef struct mate_instance {
    void *group_info;
} mate_instance;

typedef struct mate_inner_structure {
	void *memory;
	sem_t* sem_instance;
	int socket_conexion;
} mate_inner_structure;

typedef struct {
    char* ip_kernel;
    char* puerto_kernel;
    char* ip_memoria;
    char* puerto_memoria;
}t_lib_config;

typedef enum {
	MATE_INIT,
	MATE_CLOSE,
	MATE_SEM_WAIT,
	MATE_SEM_POST,
	MATE_SEM_DESTROY,
	MATE_CALL_IO,
	MATE_MEMALLOC,
	MATE_MEMREAD,
	MATE_MEMFREE,
	MATE_MEMWRITE,
	MATE_MEMSUSP,
	MATE_MEMDESSUSP,
	MENSAJE
} peticion_carpincho;

typedef struct {
	uint32_t size;
	void* stream;
} t_buffer;

typedef struct {
	peticion_carpincho codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef enum {
	MATE_SUCCESS = 1,
	MATE_FREE_FAULT = -5,
	MATE_READ_FAULT = -6,
	MATE_WRITE_FAULT = -7
} MATE_RETURNS;

typedef char *mate_io_resource;

typedef char *mate_sem_name;

typedef int32_t mate_pointer;


//------------------General Functions---------------------/

int mate_init(mate_instance *lib_ref, char *config);

int mate_close(mate_instance *lib_ref);

//-----------------Semaphore Functions---------------------/

int mate_sem_init(mate_instance *lib_ref, mate_sem_name sem, unsigned int value);

int mate_sem_wait(mate_instance *lib_ref, mate_sem_name sem);

int mate_sem_post(mate_instance *lib_ref, mate_sem_name sem);

int mate_sem_destroy(mate_instance *lib_ref, mate_sem_name sem);


//--------------------IO Functions------------------------/

int mate_call_io(mate_instance *lib_ref, mate_io_resource io, void *msg);


//--------------Memory Module Functions-------------------/

mate_pointer mate_memalloc(mate_instance *lib_ref, int size);

int mate_memfree(mate_instance *lib_ref, mate_pointer addr);

int mate_memread(mate_instance *lib_ref, mate_pointer origin, void *dest, int size);

int mate_memwrite(mate_instance *lib_ref, void *origin, mate_pointer dest, int size);


//-------------------Variables Globales----------------------/

 char* CONFIG_PATH = "/home/utnso/workspace/tp-2021-2c-DesacatadOS/matelib/matelib.config";


//--------------Extra Functions-------------------/

t_lib_config crear_archivo_config_lib(char* ruta);

int crear_conexion(char *ip, char* puerto);

int crear_conexion_kernel(char *ip, char* puerto);

void enviar_mensaje(char* mensaje, int socket_cliente);

void* serializar_paquete(t_paquete* paquete, int *bytes);

void eliminar_paquete(t_paquete* paquete);

void pedir_permiso_para_continuar(int conexion);

void recibir_permiso_para_continuar(int conexion);

int recibir_operacion(int socket_cliente);

peticion_carpincho recibir_operacion_carpincho(int socket_cliente);

char* recibir_mensaje(int socket_cliente);

void* recibir_buffer(int* size, int socket_cliente);

#endif
