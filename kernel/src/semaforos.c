#include "semaforos.h"

void init_sem(t_nucleo_cpu* estructura_nucleo_cpu) {

	t_semaforo* nuevo_semaforo = malloc(sizeof(t_semaforo));

	uint32_t size;
	uint32_t size_nombre_semaforo;

	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &size, sizeof(uint32_t), MSG_WAITALL);

	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &size_nombre_semaforo, sizeof(uint32_t), MSG_WAITALL);

	nuevo_semaforo->nombre = malloc(size_nombre_semaforo);

	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, nuevo_semaforo->nombre, size_nombre_semaforo, MSG_WAITALL);

	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &nuevo_semaforo->value, sizeof(unsigned int), MSG_WAITALL);

	nuevo_semaforo->cola_bloqueados = list_create();

	bool _criterio_busqueda_semaforo(void* elemento) {
		return (strcmp(((t_semaforo*)elemento)->nombre, nuevo_semaforo->nombre) == 0);
	}

	pthread_mutex_lock(&mutex_lista_semaforos_mate);

	if(list_any_satisfy(LISTA_SEMAFOROS, _criterio_busqueda_semaforo)) {
		free(nuevo_semaforo->nombre);
		list_destroy(nuevo_semaforo->cola_bloqueados);
		free(nuevo_semaforo);
	} else {
		list_add(LISTA_SEMAFOROS, nuevo_semaforo);
		log_info(LOGGER, "El carpincho %d, inicializó el semaforo %s con valor %d\n", estructura_nucleo_cpu->lugar_pcb_carpincho->pid, nuevo_semaforo->nombre, nuevo_semaforo->value);
	}

	pthread_mutex_unlock(&mutex_lista_semaforos_mate);

	dar_permiso_para_continuar(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion);
}


void wait_sem(t_nucleo_cpu* estructura_nucleo_cpu) {

	uint32_t size_nombre_semaforo;

	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &size_nombre_semaforo, sizeof(uint32_t), MSG_WAITALL);

	char* nombre = malloc(size_nombre_semaforo);

	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, nombre, size_nombre_semaforo, MSG_WAITALL);

	log_info(LOGGER, "El carpincho %d, hizo wait al semaforo %s\n", estructura_nucleo_cpu->lugar_pcb_carpincho->pid, nombre);

	bool _criterio_busqueda_semaforo(void* elemento) {
		return (strcmp(((t_semaforo*)elemento)->nombre, nombre) == 0);
	}

	pthread_mutex_lock(&mutex_lista_semaforos_mate);
	t_semaforo* semaforo = (t_semaforo*) list_find(LISTA_SEMAFOROS, _criterio_busqueda_semaforo);
	if(semaforo->value > 0) {
		semaforo->value -= 1;

		bool _criterio_busqueda_semaforo_en_carpincho(void* elemento) {
			return (strcmp(((t_registro_uso_recurso*)elemento)->nombre, nombre) == 0);
		}

		if(list_any_satisfy(estructura_nucleo_cpu->lugar_pcb_carpincho->recursos_usados, _criterio_busqueda_semaforo_en_carpincho)) {
			t_registro_uso_recurso* recurso = (t_registro_uso_recurso*) list_find(estructura_nucleo_cpu->lugar_pcb_carpincho->recursos_usados, _criterio_busqueda_semaforo_en_carpincho);
			recurso->cantidad += 1;
		} else {
			t_registro_uso_recurso* recurso = malloc(sizeof(t_registro_uso_recurso));
			recurso->nombre = malloc(size_nombre_semaforo);
			memcpy(recurso->nombre, nombre, size_nombre_semaforo);
			recurso->cantidad = 1;
			list_add(estructura_nucleo_cpu->lugar_pcb_carpincho->recursos_usados, recurso);
			//TODO hacerle free a esto
		}

		dar_permiso_para_continuar(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion);
	} else {
		list_add(semaforo->cola_bloqueados, estructura_nucleo_cpu->lugar_pcb_carpincho);
		pthread_mutex_lock(&mutex_lista_blocked);
		gettimeofday(&estructura_nucleo_cpu->timeValAfter, NULL);
		estructura_nucleo_cpu->lugar_pcb_carpincho->real_anterior = ((estructura_nucleo_cpu->timeValAfter.tv_sec - estructura_nucleo_cpu->timeValBefore.tv_sec)*1000000L+estructura_nucleo_cpu->timeValAfter.tv_usec) - estructura_nucleo_cpu->timeValBefore.tv_usec;
		list_add(LISTA_BLOCKED, estructura_nucleo_cpu->lugar_pcb_carpincho);
		log_info(LOGGER, "Pasó a BLOCKED el carpincho %d\n", estructura_nucleo_cpu->lugar_pcb_carpincho->pid);
		pthread_mutex_unlock(&mutex_lista_blocked);
		pthread_mutex_lock(&mutex_lista_nucleos_cpu); // TENER CUIDADO (si se bloquea todo ver aqui)
		sem_wait(&estructura_nucleo_cpu->sem_exec);
		estructura_nucleo_cpu->ocupado = false;
		pthread_mutex_unlock(&mutex_lista_nucleos_cpu);
		sem_post(&sem_grado_multiprocesamiento);
		algoritmo_planificador_mediano_plazo_blocked_suspended();
	}
	pthread_mutex_unlock(&mutex_lista_semaforos_mate);

	free(nombre);

}

void post_sem(t_nucleo_cpu* estructura_nucleo_cpu) {
	uint32_t size_nombre_semaforo;

	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &size_nombre_semaforo, sizeof(uint32_t), MSG_WAITALL);

	char* nombre = malloc(size_nombre_semaforo);

	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, nombre, size_nombre_semaforo, MSG_WAITALL);

	log_info(LOGGER, "El carpincho %d, hizo signal al semaforo %s\n", estructura_nucleo_cpu->lugar_pcb_carpincho->pid, nombre);

	bool _criterio_busqueda_semaforo(void* elemento) {
		return (strcmp(((t_semaforo*)elemento)->nombre, nombre) == 0);
	}

	pthread_mutex_lock(&mutex_lista_semaforos_mate);
	t_semaforo* semaforo = (t_semaforo*) list_find(LISTA_SEMAFOROS, _criterio_busqueda_semaforo);

	if(semaforo->value == 0 && list_size(semaforo->cola_bloqueados) > 0) {
		pcb_carpincho* pcb = (pcb_carpincho*) list_remove(semaforo->cola_bloqueados, 0);

		bool _criterio_remocion_lista(void* elemento) {
			return (((pcb_carpincho*)elemento)->pid == pcb->pid);
		}

		if(list_any_satisfy(LISTA_BLOCKED, _criterio_remocion_lista)) {

			pthread_mutex_lock(&mutex_lista_blocked);
			list_remove_by_condition(LISTA_BLOCKED, _criterio_remocion_lista);
			pthread_mutex_unlock(&mutex_lista_blocked);

			pthread_mutex_lock(&mutex_lista_ready);
			list_add(LISTA_READY, pcb);
			log_info(LOGGER, "Pasó a READY el carpincho %d\n", pcb->pid);
			pthread_mutex_unlock(&mutex_lista_ready);

			sem_post(&sem_cola_ready);
		} else {
			pthread_mutex_lock(&mutex_lista_blocked_suspended);
			list_remove_by_condition(LISTA_BLOCKED_SUSPENDED, _criterio_remocion_lista);
			pthread_mutex_unlock(&mutex_lista_blocked_suspended);

			pthread_mutex_lock(&mutex_lista_ready_suspended);
			list_add(LISTA_READY_SUSPENDED, pcb);
			log_info(LOGGER, "Pasó a READY-SUSPENDED el carpincho %d\n", pcb->pid);
			pthread_mutex_unlock(&mutex_lista_ready_suspended);

			sem_post(&sem_algoritmo_planificador_largo_plazo);
		}

	} else {
		semaforo->value += 1;
	}
	pthread_mutex_unlock(&mutex_lista_semaforos_mate);
	free(nombre);
	dar_permiso_para_continuar(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion);
}

void destroy_sem(t_nucleo_cpu* estructura_nucleo_cpu) {
	uint32_t size_nombre_semaforo;

	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &size_nombre_semaforo, sizeof(uint32_t), MSG_WAITALL);

	char* nombre = malloc(size_nombre_semaforo);

	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, nombre, size_nombre_semaforo, MSG_WAITALL);

	log_info(LOGGER, "El carpincho %d, hizo destroy al semaforo %s\n", estructura_nucleo_cpu->lugar_pcb_carpincho->pid, nombre);

	bool _criterio_busqueda_semaforo(void* elemento) {
		return (strcmp(((t_semaforo*)elemento)->nombre, nombre) == 0);
	}

	pthread_mutex_lock(&mutex_lista_semaforos_mate);
	t_semaforo* semaforo = (t_semaforo*) list_find(LISTA_SEMAFOROS, _criterio_busqueda_semaforo);

	if(list_size(semaforo->cola_bloqueados) > 0) {
		int tamanio_cola_blocked = list_size(semaforo->cola_bloqueados);
		for(int i = 0; i < tamanio_cola_blocked; i++) {

			pcb_carpincho* pcb = (pcb_carpincho*) list_remove(semaforo->cola_bloqueados, 0);

			bool _criterio_remocion_lista(void* elemento) {
				return (((pcb_carpincho*)elemento)->pid == pcb->pid);
			}

			if(list_any_satisfy(LISTA_BLOCKED, _criterio_remocion_lista)) {

				pthread_mutex_lock(&mutex_lista_blocked);
				list_remove_by_condition(LISTA_BLOCKED, _criterio_remocion_lista);
				pthread_mutex_unlock(&mutex_lista_blocked);

				pthread_mutex_lock(&mutex_lista_ready);
				list_add(LISTA_READY, pcb);
				log_info(LOGGER, "Pasó a READY el carpincho %d\n", pcb->pid);
				pthread_mutex_unlock(&mutex_lista_ready);

				sem_post(&sem_cola_ready);
			} else {
				pthread_mutex_lock(&mutex_lista_blocked_suspended);
				list_remove_by_condition(LISTA_BLOCKED_SUSPENDED, _criterio_remocion_lista);
				pthread_mutex_unlock(&mutex_lista_blocked_suspended);

				pthread_mutex_lock(&mutex_lista_ready_suspended);
				list_add(LISTA_READY_SUSPENDED, pcb);
				log_info(LOGGER, "Pasó a READY-SUSPENDED el carpincho %d\n", pcb->pid);
				pthread_mutex_unlock(&mutex_lista_ready_suspended);

				sem_post(&sem_algoritmo_planificador_largo_plazo);
			}
		}
	}

	list_remove_by_condition(LISTA_SEMAFOROS, _criterio_busqueda_semaforo);
	log_info(LOGGER, "Se eliminó el semaforo %s\n", nombre);

	pthread_mutex_unlock(&mutex_lista_semaforos_mate);

	//TODO aca tengo que eliminar cada elemento y destruir la lista
	free(semaforo->cola_bloqueados);
	free(semaforo->nombre);
	free(semaforo);
	free(nombre);

	dar_permiso_para_continuar(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion);
}