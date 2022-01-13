#include "deadlock.h"

void algoritmo_deteccion_deadlock() {

	int i;
	int j;
	t_list* lista_bloqueados_por_semaforo = list_create();
	t_list* lista_procesos_en_deadlock = list_create();

	pthread_mutex_lock(&mutex_lista_semaforos_mate);

	for(i = 0; i < list_size(LISTA_SEMAFOROS); i++) {
		t_semaforo* sem = list_get(LISTA_SEMAFOROS, i);
		for (j = 0; j < list_size(sem->cola_bloqueados); j++) {
			pcb_carpincho* proceso = list_get(sem->cola_bloqueados, j);
			list_add(lista_bloqueados_por_semaforo, proceso);
		}
	}

	for(int h = 0; h < list_size(lista_bloqueados_por_semaforo); h++) {
		pcb_carpincho* pcb = list_get(lista_bloqueados_por_semaforo, h);

		bool _criterio_igual_pid_en_bloqueados(void* elemento) {
			return (((pcb_carpincho*)elemento)->pid == pcb->pid);
		}

		bool _criterio_busqueda_semaforo_en_que_esta_bloqueado(void* elemento) {
			return (list_any_satisfy(((t_semaforo*)elemento)->cola_bloqueados, _criterio_igual_pid_en_bloqueados));
		}

		t_semaforo* semaforo_en_que_esta_bloqueado = list_find(LISTA_SEMAFOROS, _criterio_busqueda_semaforo_en_que_esta_bloqueado);

		if(list_size(pcb->recursos_usados) > 0) {
			for(int g = 0; g < list_size(pcb->recursos_usados); g++) {
				t_registro_uso_recurso* recurso_usado = list_get(pcb->recursos_usados, g);

				bool _criterio_busqueda_semaforo(void* elemento) {
					return (strcmp(((t_semaforo*)elemento)->nombre, recurso_usado->nombre) == 0);
				}

				t_semaforo* semaforo = (t_semaforo*) list_find(LISTA_SEMAFOROS, _criterio_busqueda_semaforo);

				bool _criterio_procesos_distintos(void* elemento) {
					return (((pcb_carpincho*)elemento)->pid != pcb->pid);
				}

				bool _criterio_remocion_lista(void* elemento) {
					return (((pcb_carpincho*)elemento)->pid == pcb->pid);
				}

				int procesos_que_lo_piden = list_count_satisfying(semaforo->cola_bloqueados, _criterio_procesos_distintos);

				if(procesos_que_lo_piden > 0 && !list_any_satisfy(lista_procesos_en_deadlock, _criterio_remocion_lista)) {
					t_list* lista_bloqueados_por_semaforo_menos_este = list_filter(lista_bloqueados_por_semaforo, _criterio_procesos_distintos);

					for(int k = 0; k < list_size(lista_bloqueados_por_semaforo_menos_este); k++) {
						pcb_carpincho* carpincho = list_get(lista_bloqueados_por_semaforo_menos_este, k);
						for(int o = 0; o < list_size(carpincho->recursos_usados); o++) {
							t_registro_uso_recurso* recurso_usado_carpincho = list_get(carpincho->recursos_usados, o);
							if(strcmp(recurso_usado_carpincho->nombre, semaforo_en_que_esta_bloqueado->nombre) == 0) {
								list_add(lista_procesos_en_deadlock, pcb);
							}
						}
					}

					list_destroy(lista_bloqueados_por_semaforo_menos_este);
				}
			}
		}
	}

	if(list_size(lista_procesos_en_deadlock) > 0) {

		void* _eleccion_mayor_pid(void* elemento1, void* elemento2) {
			if(((pcb_carpincho*)elemento1)->pid > ((pcb_carpincho*)elemento2)->pid) {
				return ((pcb_carpincho*)elemento1);
			} else {
				return ((pcb_carpincho*)elemento2);
			}
		}

		pcb_carpincho* proceso_de_mayor_pid = (pcb_carpincho*) list_get_maximum(lista_procesos_en_deadlock, _eleccion_mayor_pid);
		log_info(LOGGER, "Finalizo al carpincho %d por deadlock\n", proceso_de_mayor_pid->pid);
		avisar_close_a_memoria(proceso_de_mayor_pid);

		bool _criterio_igual_pid(void* elemento) {
			return (((pcb_carpincho*)elemento)->pid == proceso_de_mayor_pid->pid);
		}


		if(list_any_satisfy(LISTA_BLOCKED, _criterio_igual_pid)) {
			pthread_mutex_lock(&mutex_lista_blocked);
			list_remove_by_condition(LISTA_BLOCKED, _criterio_igual_pid);
			pthread_mutex_unlock(&mutex_lista_blocked);
		} else {
			pthread_mutex_lock(&mutex_lista_blocked_suspended);
			list_remove_by_condition(LISTA_BLOCKED_SUSPENDED, _criterio_igual_pid);
			pthread_mutex_unlock(&mutex_lista_blocked_suspended);
		}

		for(int k = 0; k < list_size(proceso_de_mayor_pid->recursos_usados); k++) {

			t_registro_uso_recurso* recurso_usado = list_get(proceso_de_mayor_pid->recursos_usados, k);

			bool _criterio_busqueda_semaforo(void* elemento) {
				return (strcmp(((t_semaforo*)elemento)->nombre, recurso_usado->nombre) == 0);
			}

			t_semaforo* recurso = (t_semaforo*) list_find(LISTA_SEMAFOROS, _criterio_busqueda_semaforo);

			for (int y = 0; y < recurso_usado->cantidad; y++) {

				if(recurso->value == 0 && list_size(recurso->cola_bloqueados) > 0) {
					pcb_carpincho* pcb = (pcb_carpincho*) list_remove(recurso->cola_bloqueados, 0);

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
					recurso->value += 1;
				}

			}

		}

		//TODO aca aumentar el grado de multiprogramacion, cerrar conexiones, y liquidar al carpincho
		avisar_finalizacion_por_deadlock(proceso_de_mayor_pid->conexion);
		close(proceso_de_mayor_pid->conexion);

		list_clean(lista_procesos_en_deadlock);
		list_clean(lista_bloqueados_por_semaforo);

		for(int f = 0; f < list_size(LISTA_SEMAFOROS); f++) {
			t_semaforo* sema = list_get(LISTA_SEMAFOROS, f);
			if(list_any_satisfy(sema->cola_bloqueados, _criterio_igual_pid)) {
				list_remove_by_condition(sema->cola_bloqueados, _criterio_igual_pid);
			}
		}

		void _element_destroyer(void* elemento) {
			free(((t_registro_uso_recurso*)elemento)->nombre);
			free(((t_registro_uso_recurso*)elemento));
		}

		list_destroy_and_destroy_elements(proceso_de_mayor_pid->recursos_usados, _element_destroyer);
		free(proceso_de_mayor_pid);

		sem_post(&sem_grado_multiprogramacion);

		pthread_mutex_unlock(&mutex_lista_semaforos_mate);
		list_destroy(lista_bloqueados_por_semaforo);
		list_destroy(lista_procesos_en_deadlock);
		algoritmo_deteccion_deadlock();

	} else {
		list_destroy(lista_bloqueados_por_semaforo);
		list_destroy(lista_procesos_en_deadlock);
		pthread_mutex_unlock(&mutex_lista_semaforos_mate);

	}

}

void matar_algoritmo_deadlock() {
	pthread_exit(NULL);
}

void correr_algoritmo_deadlock() {
	while(1) {
		signal(SIGINT,  &matar_algoritmo_deadlock);
		usleep(CONFIG_KERNEL.tiempo_deadlock*1000);
		algoritmo_deteccion_deadlock();
	}
}

void avisar_finalizacion_por_deadlock(int conexion) {
	uint32_t handshake = 2;
	//int numero_de_bytes =
	send(conexion, &handshake, sizeof(uint32_t), 0);
	//printf("Finalice un carpincho por deadlock %d\n", numero_de_bytes);
}