#include "procesar_carpinchos.h"

void ejecutar(t_nucleo_cpu* estructura_nucleo_cpu) {

	while (1) {
		signal(SIGINT,  &matar_nucleo_cpu);
		sem_wait(&estructura_nucleo_cpu->sem_exec);
		sem_post(&estructura_nucleo_cpu->sem_exec);

		peticion_carpincho cod_op = recibir_operacion_carpincho(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion);

		switch (cod_op) {
		case MATE_INIT:
			init_sem(estructura_nucleo_cpu);
			break;
		case MATE_SEM_WAIT:
			wait_sem(estructura_nucleo_cpu);
			break;
		case MATE_SEM_POST:
			post_sem(estructura_nucleo_cpu);
			break;
		case MATE_SEM_DESTROY:
			destroy_sem(estructura_nucleo_cpu);
			break;
		case MATE_CALL_IO:
			call_IO(estructura_nucleo_cpu);
			break;
		case MATE_MEMALLOC:
			memalloc(estructura_nucleo_cpu);
			break;
		case MATE_MEMREAD:
			memread(estructura_nucleo_cpu);
			break;
		case MATE_MEMFREE:
			memfree(estructura_nucleo_cpu);
			break;
		case MATE_MEMWRITE:
			memwrite(estructura_nucleo_cpu);
			break;
		case MATE_CLOSE:
			mate_close(estructura_nucleo_cpu);
			break;
		default:
			log_warning(LOGGER,"Operacion desconocida. No quieras meter la pata");
			printf("Operacion desconocida. No quieras meter la pata\n");
			pthread_exit(NULL);
			break;
		}
	}
}

void algoritmo_planificador_largo_plazo() {

	while(1) {
		sem_wait(&sem_algoritmo_planificador_largo_plazo);
		sem_wait(&sem_grado_multiprogramacion);

		pthread_mutex_lock(&mutex_lista_ready_suspended);

		if(list_size(LISTA_READY_SUSPENDED) > 0){
			algoritmo_planificador_mediano_plazo_ready_suspended();
		} else {
			pthread_mutex_lock(&mutex_lista_new);
			pcb_carpincho* pcb = (pcb_carpincho*) list_remove(LISTA_NEW, 0);
			pthread_mutex_unlock(&mutex_lista_new);

			pthread_mutex_lock(&mutex_lista_ready);
			list_add(LISTA_READY, pcb);
			log_info(LOGGER, "Pasó a READY el carpincho %d\n", pcb->pid);
			pthread_mutex_unlock(&mutex_lista_ready);

		}
		pthread_mutex_unlock(&mutex_lista_ready_suspended);

		sem_post(&sem_cola_ready);
	}
}

void algoritmo_planificador_mediano_plazo_ready_suspended() {
	pcb_carpincho* pcb = (pcb_carpincho*) list_remove(LISTA_READY_SUSPENDED, 0);
	pthread_mutex_lock(&mutex_lista_ready);
	list_add(LISTA_READY, pcb);
	log_info(LOGGER, "Pasó a READY el carpincho %d\n", pcb->pid);
	pthread_mutex_unlock(&mutex_lista_ready);
	avisar_dessuspension_a_memoria(pcb);

}

void algoritmo_planificador_mediano_plazo_blocked_suspended() {

	pthread_mutex_lock(&mutex_lista_blocked);

	int cantidad_procesos_blocked = list_size(LISTA_BLOCKED);

	pthread_mutex_lock(&mutex_lista_new);
	int cantidad_procesos_new = list_size(LISTA_NEW);
	pthread_mutex_unlock(&mutex_lista_new);

	if(cantidad_procesos_blocked == CONFIG_KERNEL.grado_multiprogramacion && cantidad_procesos_new > 0) {
		pcb_carpincho* pcb = (pcb_carpincho*) list_remove(LISTA_BLOCKED, cantidad_procesos_blocked-1);

		pthread_mutex_lock(&mutex_lista_blocked_suspended);
		list_add(LISTA_BLOCKED_SUSPENDED, pcb);
		log_info(LOGGER, "Pasó a BLOCKED-SUSPENDED el carpincho %d\n", pcb->pid);
		pthread_mutex_unlock(&mutex_lista_blocked_suspended);
		avisar_suspension_a_memoria(pcb);

		sem_post(&sem_grado_multiprogramacion);
	}
	pthread_mutex_unlock(&mutex_lista_blocked);
}

void algoritmo_planificador_corto_plazo() {

	while(1) {
		sem_wait(&sem_cola_ready);
		sem_wait(&sem_grado_multiprocesamiento);
		pcb_carpincho* pcb;

		if(strcmp(CONFIG_KERNEL.alg_plani, "SJF") == 0) {
			pcb = algoritmo_SJF();
		} else if (strcmp(CONFIG_KERNEL.alg_plani, "HRRN") == 0){
			pcb = algoritmo_HRRN();
		} else {
			log_info(LOGGER, "El algoritmo de planificacion ingresado no existe\n");
		}
		correr_dispatcher(pcb);
	}
}

void correr_dispatcher(pcb_carpincho* pcb) {
	bool _criterio_nucleo_cpu_libre(void* elemento) {
		return (((t_nucleo_cpu*)elemento)->ocupado == false);
	}

	pthread_mutex_lock(&mutex_lista_nucleos_cpu);
	t_nucleo_cpu* nucleo_cpu_libre = list_find(LISTA_NUCLEOS_CPU, _criterio_nucleo_cpu_libre);
	nucleo_cpu_libre->lugar_pcb_carpincho = pcb;
	nucleo_cpu_libre->ocupado = 1;
	dar_permiso_para_continuar(pcb->conexion);
	gettimeofday(&nucleo_cpu_libre->timeValBefore, NULL);
	log_info(LOGGER, "Pasó a EXEC el carpincho %d\n", pcb->pid);
	sem_post(&nucleo_cpu_libre->sem_exec);
	pthread_mutex_unlock(&mutex_lista_nucleos_cpu);
}

pcb_carpincho* algoritmo_SJF() {
	void* _eleccion_SJF(void* elemento1, void* elemento2) {

		double estimacion_actual1;
		double estimacion_actual2;

		estimacion_actual1 = (CONFIG_KERNEL.alfa) * (((pcb_carpincho*)elemento1)->real_anterior) + (1-(CONFIG_KERNEL.alfa))*(((pcb_carpincho*)elemento1)->estimado_anterior);
		estimacion_actual2 = (CONFIG_KERNEL.alfa) * (((pcb_carpincho*)elemento2)->real_anterior) + (1-(CONFIG_KERNEL.alfa))*(((pcb_carpincho*)elemento2)->estimado_anterior);

		if(estimacion_actual1 <= estimacion_actual2) {
			return ((pcb_carpincho*)elemento1);
		} else {
			return ((pcb_carpincho*)elemento2);
		}
	}

	pthread_mutex_lock(&mutex_lista_ready);
	pcb_carpincho* pcb = (pcb_carpincho*) list_get_minimum(LISTA_READY, _eleccion_SJF);
	for(int i = 0; i < list_size(LISTA_READY); i++) {
		pcb_carpincho* pcbActualizacion = list_get(LISTA_READY, i);
		pcbActualizacion->estimado_anterior = (CONFIG_KERNEL.alfa) * (pcb->real_anterior) + (1-(CONFIG_KERNEL.alfa))*(pcb->estimado_anterior);
	}
	pthread_mutex_unlock(&mutex_lista_ready);

	bool _criterio_remocion_lista(void* elemento) {
		return (((pcb_carpincho*)elemento)->pid == pcb->pid);
	}

	pthread_mutex_lock(&mutex_lista_ready);
	list_remove_by_condition(LISTA_READY, _criterio_remocion_lista);
	pthread_mutex_unlock(&mutex_lista_ready);

	return pcb;
}

pcb_carpincho* algoritmo_HRRN() {
	void* _eleccion_HRRN(void* elemento1, void* elemento2) {

		double valor_actual1;
		double valor_actual2;

		valor_actual1 = (((CONFIG_KERNEL.alfa) * (((pcb_carpincho*)elemento1)->real_anterior) + (1-(CONFIG_KERNEL.alfa))*(((pcb_carpincho*)elemento1)->estimado_anterior))
						+(((pcb_carpincho*)elemento1)->tiempo_espera))
						/(((CONFIG_KERNEL.alfa) * (((pcb_carpincho*)elemento1)->real_anterior) + (1-(CONFIG_KERNEL.alfa))*(((pcb_carpincho*)elemento1)->estimado_anterior)));
		valor_actual2 = (((CONFIG_KERNEL.alfa) * (((pcb_carpincho*)elemento2)->real_anterior) + (1-(CONFIG_KERNEL.alfa))*(((pcb_carpincho*)elemento2)->estimado_anterior))
						+(((pcb_carpincho*)elemento2)->tiempo_espera))
						/(((CONFIG_KERNEL.alfa) * (((pcb_carpincho*)elemento2)->real_anterior) + (1-(CONFIG_KERNEL.alfa))*(((pcb_carpincho*)elemento2)->estimado_anterior)));

		if(valor_actual1 >= valor_actual2) {
			return ((pcb_carpincho*)elemento1);
		} else {
			return ((pcb_carpincho*)elemento2);
		}
	}

	pthread_mutex_lock(&mutex_lista_ready);
	pcb_carpincho* pcb = (pcb_carpincho*) list_get_maximum(LISTA_READY, _eleccion_HRRN);
	for(int i = 0; i < list_size(LISTA_READY); i++) {
		pcb_carpincho* pcbActualizacion = list_get(LISTA_READY, i);
		pcbActualizacion->estimado_anterior = (CONFIG_KERNEL.alfa) * (pcb->real_anterior) + (1-(CONFIG_KERNEL.alfa))*(pcb->estimado_anterior);
	}
	pthread_mutex_unlock(&mutex_lista_ready);


	bool _criterio_remocion_lista(void* elemento) {
		return (((pcb_carpincho*)elemento)->pid == pcb->pid);
	}

	pthread_mutex_lock(&mutex_lista_ready);
	list_remove_by_condition(LISTA_READY, _criterio_remocion_lista);
	pthread_mutex_unlock(&mutex_lista_ready);

	return pcb;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------- MENSAJES MATE LIB ------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------


void recibir_peticion_para_continuar(int conexion) {
	uint32_t result;
	//int bytes_recibidos =
	recv(conexion, &result, sizeof(uint32_t), MSG_WAITALL);
	//printf("Ya recibi peticion %d\n", bytes_recibidos);
}

void dar_permiso_para_continuar(int conexion){
	uint32_t handshake = 1;
	//int numero_de_bytes =
	send(conexion, &handshake, sizeof(uint32_t), 0);
	//printf("Ya di permiso %d\n", numero_de_bytes);
}

void matar_nucleo_cpu() {
	pthread_exit(NULL);
}

void mate_close(t_nucleo_cpu* estructura_nucleo_cpu) {

	log_info(LOGGER, "El carpincho %d, terminó\n", estructura_nucleo_cpu->lugar_pcb_carpincho->pid);

	dar_permiso_para_continuar(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion);

	close(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion);

	avisar_close_a_memoria(estructura_nucleo_cpu->lugar_pcb_carpincho);

	pthread_mutex_lock(&mutex_lista_nucleos_cpu); // TENER CUIDADO (si se bloquea todo ver aqui)
	sem_wait(&estructura_nucleo_cpu->sem_exec);
	estructura_nucleo_cpu->ocupado = false;
	pthread_mutex_unlock(&mutex_lista_nucleos_cpu);

	for(int k = 0; k < list_size(estructura_nucleo_cpu->lugar_pcb_carpincho->recursos_usados); k++) {

		t_registro_uso_recurso* recurso_usado = list_get(estructura_nucleo_cpu->lugar_pcb_carpincho->recursos_usados, k);

		bool _criterio_busqueda_semaforo(void* elemento) {
			return (strcmp(((t_semaforo*)elemento)->nombre, recurso_usado->nombre) == 0);
		}

		pthread_mutex_lock(&mutex_lista_semaforos_mate);

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

	bool _criterio_igual_pid(void* elemento) {
		return (((pcb_carpincho*)elemento)->pid == estructura_nucleo_cpu->lugar_pcb_carpincho->pid);
	}

	for(int f = 0; f < list_size(LISTA_SEMAFOROS); f++) {
		t_semaforo* sema = list_get(LISTA_SEMAFOROS, f);
		if(list_any_satisfy(sema->cola_bloqueados, _criterio_igual_pid)) {
			list_remove_by_condition(sema->cola_bloqueados, _criterio_igual_pid);
		}
	}

	pthread_mutex_unlock(&mutex_lista_semaforos_mate);

	void _element_destroyer(void* elemento) {
		free(((t_registro_uso_recurso*)elemento)->nombre);
		free(((t_registro_uso_recurso*)elemento));
	}

	list_destroy_and_destroy_elements(estructura_nucleo_cpu->lugar_pcb_carpincho->recursos_usados, _element_destroyer);
	free(estructura_nucleo_cpu->lugar_pcb_carpincho);

	sem_post(&sem_grado_multiprocesamiento);
	sem_post(&sem_grado_multiprogramacion);
}
