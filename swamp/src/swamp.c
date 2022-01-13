#include "swamp.h"

void atender_peticiones(int* cliente){
	t_peticion_swap operacion = recibir_op_swap(*cliente);

	recibir_entero_swap(*cliente);

	uint32_t id_proceso, nro_pagina, rta, cant_paginas;
	t_frame* frame;
	t_metadata_archivo* archivo;
	void* addr;
	void* buffer_pag;

	switch (operacion) {
	case RESERVAR_ESPACIO:;
		id_proceso = recibir_entero(*cliente);
		cant_paginas = recibir_entero(*cliente);

		log_info(LOGGER, "%i paginas se reservan para carpincho %i.", cant_paginas, id_proceso);

		rta = reservar_espacio(id_proceso, cant_paginas);
		send(*cliente, &rta, sizeof(uint32_t), 0);

		break;

	case TIRAR_A_SWAP:;
		buffer_pag = malloc(CONFIG.tamanio_pagina);

		id_proceso  = recibir_entero(*cliente);
		nro_pagina = recibir_entero(*cliente);
		recv(*cliente, buffer_pag, CONFIG.tamanio_pagina, 0);

		log_info(LOGGER, "Se guarda pagina %i de carpincho %i en swamp.", nro_pagina, id_proceso);

		frame = frame_de_pagina(id_proceso, nro_pagina);

		if (frame == NULL) {
			log_error(LOGGER, "No se encontró el frame.");
			exit(1);
		}

		archivo = obtener_id_archivo(frame->id_archivo);

		if (archivo == NULL) {
			log_error(LOGGER, "No se encontró el archivo.");
			exit(1);
		}

		addr = mmap(NULL, CONFIG.tamanio_swamp, PROT_READ | PROT_WRITE, MAP_SHARED, archivo->fd, 0);

		if (addr == MAP_FAILED) {
			log_error(LOGGER, "Error al  mappear.");
			exit(1);
		}

		memcpy(addr + frame->offset, buffer_pag, CONFIG.tamanio_pagina);
		msync(addr, CONFIG.tamanio_swamp, 0);

		munmap(addr, CONFIG.tamanio_swamp);

		free(buffer_pag);

		break;

	case TRAER_DE_SWAP:;
		buffer_pag = malloc(CONFIG.tamanio_pagina);

		id_proceso = recibir_entero(*cliente);
		nro_pagina = recibir_entero(*cliente);

		log_info(LOGGER, "Se manda la pagina %i de carpincho %i a memoria principal.", nro_pagina, id_proceso);

		frame   = frame_de_pagina(id_proceso, nro_pagina);
		archivo = obtener_id_archivo(frame->id_archivo);

		addr = mmap(NULL, CONFIG.tamanio_swamp, PROT_READ | PROT_WRITE, MAP_SHARED, archivo->fd, 0);

		if (addr == MAP_FAILED) {
			log_error(LOGGER, "Error al mapear.");
			exit(1);
		}

		memcpy(buffer_pag, addr + frame->offset, CONFIG.tamanio_pagina);
		munmap(addr, CONFIG.tamanio_swamp);

		usleep(CONFIG.retardo_swap * 1000);
		send(*cliente, buffer_pag, CONFIG.tamanio_pagina, 0);

		free(buffer_pag);

		break;

	case LIBERAR_PAGINA:;
		id_proceso        = recibir_entero(*cliente);
		nro_pagina = recibir_entero(*cliente);
		log_info(LOGGER, "Se libera pagina %i de carpincho %i.", nro_pagina, id_proceso);

		frame   = frame_de_pagina(id_proceso, nro_pagina);
		archivo = obtener_id_archivo(frame->id_archivo);

		archivo->espacio_disponible += CONFIG.tamanio_pagina;

		if (CONFIG.marcos_max < 0) {
			frame->id_proceso = -1;
		}

		frame->id_pagina   = -1;
		frame->ocupado  = false;
		break;

	case SOLICITAR_MARCOS_MAX:;
		log_info(LOGGER, "Se mandan los marcos_max a memoria principal.");
		send(*cliente, &CONFIG.marcos_max, sizeof(uint32_t), 0);
		break;

	case KILL_PROCESO:;

		id_proceso = recibir_entero(*cliente);
		log_info(LOGGER, "Eliminando carpincho %i en swap.", id_proceso);
		eliminar_proceso_swap(id_proceso);
		break;

	case EXIT:;
		exit(1);
		finish_swamp();
		break;

	default:;
		sleep(20);
		break;
	}

	return;
}

int main(void) {
	init_swamp();
	int *socket_cliente = malloc(sizeof(int));
	(*socket_cliente) = esperar_cliente(SERVIDOR_SWAP);

	log_info(LOGGER, "Cliente %i conectado a swamp.", *socket_cliente);

	while(1) {
		atender_peticiones(socket_cliente);
	}

	return 1;
}

int32_t reservar_espacio(int32_t id_proceso, int cant_paginas) {

	int nro_archivo = archivo_proceso_existente(id_proceso);
	t_metadata_archivo* archivo;

	//CASO 1: Proceso existente
	if (nro_archivo != -1) {
		archivo = obtener_id_archivo(nro_archivo);

		if (archivo->espacio_disponible >= cant_paginas * CONFIG.tamanio_pagina) {
			t_list* frames;
			if (CONFIG.marcos_max > 0) {
				frames = frames_libres_proceso(nro_archivo, id_proceso);
			} else {
				frames = frames_libres_archivo(nro_archivo);
			}

			int id_ult_pag = ultima_pagina_proceso(id_proceso);

			for (int i = 1; i <= cant_paginas; i++) {
				t_frame* frame = list_get(frames, i-1);
				frame->ocupado = true;
				frame->id_proceso     = id_proceso;
				frame->id_pagina  = id_ult_pag + i;
			}

			archivo->espacio_disponible -= cant_paginas * CONFIG.tamanio_pagina;

			log_info(LOGGER, "id_proceso: %i.", id_proceso);
			log_info(LOGGER, "fd_archivo: %i.", archivo->fd);
			log_info(LOGGER, "id_archivo: %i.", archivo->id);
			log_info(LOGGER, "espacio_disponible: %i.", archivo->espacio_disponible);
			log_info(LOGGER, "Se reservaron %i paginas para carpincho %i.", cant_paginas, id_proceso);
			list_destroy(frames);
			return 1;
		}

		log_info(LOGGER, "Sin espacio disponible para carpincho %i.",id_proceso);
		return -1;

	} else {

		//CASO 2: Proceso nuevo
		archivo = obtener_archivo_mayor_espacio_libre();
		int cant_frames_requeridos = cant_paginas;

		//Si hay asignacion fija, reservo todos los marcos max para el proceso
		if (CONFIG.marcos_max > 0) {
			cant_frames_requeridos = CONFIG.marcos_max;
		}

		if (archivo->espacio_disponible >= cant_frames_requeridos * CONFIG.tamanio_pagina) {

			//Reserva de frames del proceso
			t_list* _frames_libres_archivo = frames_libres_archivo(archivo->id);
			for (int f = 0 ; f < cant_frames_requeridos ; f++) {

				t_frame* frame = list_get(_frames_libres_archivo, f);
				frame->id_proceso     = id_proceso;
				frame->ocupado = false;
			}

			//Ocupar frames con las paginas del proceso
			t_list* _frames_libres_proceso = frames_libres_proceso(archivo->id, id_proceso);
			for (int p = 0 ; p < cant_paginas ; p++) {

				t_frame* frame_a_ocupar = list_get(_frames_libres_proceso, p);
				frame_a_ocupar->id_pagina  = p;
				frame_a_ocupar->ocupado = true;

			}

			archivo->espacio_disponible -= cant_frames_requeridos * CONFIG.tamanio_pagina;

			log_info(LOGGER, "id_proceso: %i.", id_proceso);
			log_info(LOGGER, "fd_archivo: %i.", archivo->fd);
			log_info(LOGGER, "id_archivo: %i.", archivo->id);
			log_info(LOGGER, "espacio_disponible: %i.", archivo->espacio_disponible);
			log_info(LOGGER, "Se reservaron %i frames y %i paginas para carpincho %i.", cant_frames_requeridos, cant_paginas, id_proceso);
			list_destroy(_frames_libres_archivo);
			list_destroy(_frames_libres_proceso);
			return 1;
		}

		log_info(LOGGER, "No hay espacio disponible para carpincho %i.",id_proceso);
		return -1;
	}
}

void eliminar_proceso_swap(int32_t id_proceso) {
	int fd = archivo_proceso_existente(id_proceso);

	if (fd == -1) {
		return;
	}

	t_list* _frames_proceso = frames_proceso(id_proceso);
	t_frame* frame;

	for (int i = 0 ; i < list_size(_frames_proceso) ; i++) {
		frame = list_get(frames_proceso, i);
		frame->id_proceso = -1;
		frame->id_pagina = -1;
		frame->ocupado = false;
	}
}

