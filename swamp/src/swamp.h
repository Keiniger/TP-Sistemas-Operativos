#ifndef SWAMP_H_
#define SWAMP_H_

#include <futiles/futiles.h>

typedef struct {
	char* ip;
	char* puerto;
	int tamanio_swamp;
	int tamanio_pagina;
	int marcos_max;
	int retardo_swap;
	char** archivos_swamp;
}t_swamp_config;

typedef struct {
	int id;
	int espacio_disponible;
	int fd;
	void* mapeo;
}t_metadata_archivo;

typedef struct {
	int id_archivo;
	int id_pagina;
	int id_proceso;
	int offset; // Byte en donde arranca la pagina
	bool ocupado;
}t_frame;

int SERVIDOR_SWAP;
t_swamp_config CONFIG;
t_config* CFG;
t_log* LOGGER;

t_list* METADATA_ARCHIVOS;
t_list* FRAMES_SWAP;

#include "init_swamp.h"
#include "mensajes_swamp.h"
#include "utils.h"

void crear_frames();
void crear_archivos();
int  crear_archivo(char* path, int size);
void atender_peticiones(int* cliente);
void eliminar_proceso_swap(int id_proceso);
int reservar_espacio(int id_proceso , int cant_paginas);


#endif