/*
 * futiles.h
 *
 *  Created on: 15 sep. 2021
 *      Author: utnso
 */

#ifndef FUTILES_H_
#define FUTILES_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <dirent.h>
#include <netdb.h>
#include <math.h>
#include <fcntl.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef enum {
	MATE_INIT,
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
	MENSAJE,
	MATE_CLOSE
} peticion_carpincho;

typedef struct{
	uint32_t prev_alloc;
	uint32_t next_alloc;
	uint8_t  is_free;
}__attribute__((packed))heap_metadata;

typedef struct {
	uint32_t size;
	void* stream;
} t_buffer;

typedef struct {
	peticion_carpincho codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef enum{
	RESERVAR_ESPACIO,     // MP -> SWAP (pid, size)
	TIRAR_A_SWAP,         // MP -> SWAP (pid, id, pag)
	TRAER_DE_SWAP,        // MP -> SWAP (pid, id)
	SOLICITAR_MARCOS_MAX, // MP -> SWAP (nada)
	LIBERAR_PAGINA,		  // MP -> SWAP (pid, id)
	KILL_PROCESO,
	EXIT
} t_peticion_swap;

typedef struct {
	t_peticion_swap cod_op;
	t_buffer* buffer;
} t_paquete_swap;


void printeame_un_cuatro();
int crear_conexion(char *ip, char* puerto);
int iniciar_servidor(char* IP, char* PUERTO);
void* serializar_paquete(t_paquete* paquete, int* bytes);
void eliminar_paquete(t_paquete*);
char* recibir_mensaje(int socket_cliente);
void* recibir_buffer(int* size, int socket_cliente);
int esperar_cliente(int socket);
int recibir_operacion(int socket);
peticion_carpincho recibir_operacion_carpincho(int socket);
void enviar_mensaje(char* mensaje, int servidor);
int size_char_array(char** array);


// Envios MEMORIA - SWAP
void* serializar_paquete_swap(t_paquete_swap* paquete, int* bytes);
void eliminar_paquete_swap(t_paquete_swap*);
int32_t recibir_entero(int cliente);

#endif /* FUTILES_H_ */
