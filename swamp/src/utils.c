#include "utils.h"

int archivo_proceso_existente(int32_t id_proceso) {
	bool existe_proceso_por_id_proceso(void* elemento) {
		t_frame* frame_aux = (t_frame*) elemento;
		return frame_aux->id_proceso == id_proceso;
	}

	t_frame* frame = (t_frame*)list_find(FRAMES_SWAP, existe_proceso_por_id_proceso);

	if (frame == NULL)
		return -1;

    return frame->id_archivo;
}

t_frame* frame_de_pagina(int32_t id_proceso, int nro_pagina) {
	bool _existe_pag_id_proceso_nro(void * elemento) {
		t_frame* frame = (t_frame*) elemento;
		return frame->id_proceso == id_proceso && frame->id_pagina == nro_pagina;
	}

	return (t_frame*)list_find(FRAMES_SWAP,_existe_pag_id_proceso_nro);
}

t_metadata_archivo* obtener_id_archivo(int id_archivo) {
	bool existe_archivo_con_id(void* elemento) {
		t_metadata_archivo* archivo_aux = (t_metadata_archivo*) elemento;
		return archivo_aux->id == id_archivo;
	}

	return (t_metadata_archivo*) list_find(METADATA_ARCHIVOS, existe_archivo_con_id);
}

t_metadata_archivo* obtener_archivo_mayor_espacio_libre() {
	bool _de_mayor_espacio(t_metadata_archivo* un_archivo, t_metadata_archivo* otro_archivo) {
		return un_archivo->espacio_disponible > otro_archivo->espacio_disponible;
	}

	list_sort(METADATA_ARCHIVOS, (void*)_de_mayor_espacio);
	return (t_metadata_archivo*)list_get(METADATA_ARCHIVOS, 0);
}

t_list* frames_libres_archivo(int id_archivo) {
	bool frame_libre_del_archivo(void* elemento) {
		t_frame* frame_aux = (t_frame*) elemento;
		return !frame_aux->ocupado && frame_aux->id_archivo == id_archivo && frame_aux->id_proceso == -1;
	}

	return list_filter(FRAMES_SWAP, frame_libre_del_archivo);
}

t_list* frames_libres_proceso(int id_archivo, int32_t id_proceso) {
	bool frame_libre_del_proceso(void * elemento) {
		t_frame* frame_aux = (t_frame*) elemento;
		return frame_aux->id_proceso == id_proceso && !frame_aux->ocupado && frame_aux->id_archivo == id_archivo;
	}

	return list_filter(FRAMES_SWAP, frame_libre_del_proceso);
}

int ultima_pagina_proceso(int32_t id_proceso) {
	t_list* _frames_proceso = frames_proceso(id_proceso);

	bool frame_ocupado(void* elemento) {
		t_frame* frame_aux = (t_frame*) elemento;
		return frame_aux->ocupado && frame_aux->id_pagina != -1;
	}

	t_list* frames_asignados = list_filter(_frames_proceso, (void*)frame_ocupado);

	int id_pagina_asc(t_frame* unFrame, t_frame* otroFrame) {
		return (otroFrame->id_pagina < unFrame->id_pagina);
	}

	list_sort(frames_asignados, (void*)id_pagina_asc);
	int frame_ult_pag = ((t_frame*)list_get(frames_asignados, 0))->id_pagina;

	list_destroy(frames_asignados);
	list_destroy(_frames_proceso);

	return frame_ult_pag;
}

t_list* frames_proceso(int32_t id_proceso) {
	bool _frame_del_id_proceso(void * elemento) {
		t_frame* frame = (t_frame*) elemento;
		return frame->id_proceso == id_proceso;
	}

	return list_filter(FRAMES_SWAP, _frame_del_id_proceso);
}
