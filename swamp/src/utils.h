#ifndef UTILS_H_
#define UTILS_H_

#include "init_swamp.h"
#include "mensajes_swamp.h"
#include "swamp.h"

int archivo_proceso_existente(int id_proceso);
t_frame* frame_de_pagina(int id_proceso, int nro_pagina);
t_metadata_archivo* obtener_id_archivo(int id_archivo);
t_metadata_archivo* obtener_archivo_mayor_espacio_libre();
t_list* frames_libres_archivo(int id_archivo);
t_list* frames_libres_proceso(int id_archivo, int id_proceso);
int ultima_pagina_proceso(int id_proceso);
t_list* frames_proceso(int id_proceso);

#endif