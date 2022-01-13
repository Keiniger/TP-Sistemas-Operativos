#include "dispositivos_io.h"

void call_IO(t_nucleo_cpu* estructura_nucleo_cpu) {
	uint32_t size_nombre_dispositivo;

	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &size_nombre_dispositivo, sizeof(uint32_t), MSG_WAITALL);

	char* nombre = malloc(size_nombre_dispositivo);

	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, nombre, size_nombre_dispositivo, MSG_WAITALL);

	log_info(LOGGER, "El carpincho %d, pidió hacer I/O en %s\n", estructura_nucleo_cpu->lugar_pcb_carpincho->pid, nombre);

	bool _criterio_busqueda_dispositivo(void* elemento) {
		return (strcmp(((t_dispositivo*)elemento)->nombre, nombre) == 0);
	}

	pthread_mutex_lock(&mutex_lista_dispositivos_io);
	t_dispositivo* dispositivo = (t_dispositivo*) list_find(LISTA_DISPOSITIVOS_IO, _criterio_busqueda_dispositivo);
	list_add(dispositivo->cola_espera, estructura_nucleo_cpu->lugar_pcb_carpincho);
	sem_post(&dispositivo->sem);
	pthread_mutex_unlock(&mutex_lista_dispositivos_io);

	pthread_mutex_lock(&mutex_lista_blocked);
	gettimeofday(&estructura_nucleo_cpu->timeValAfter, NULL);
	estructura_nucleo_cpu->lugar_pcb_carpincho->real_anterior = ((estructura_nucleo_cpu->timeValAfter.tv_sec - estructura_nucleo_cpu->timeValBefore.tv_sec)*1000000L+estructura_nucleo_cpu->timeValAfter.tv_usec) - estructura_nucleo_cpu->timeValBefore.tv_usec;
	list_add(LISTA_BLOCKED, estructura_nucleo_cpu->lugar_pcb_carpincho);
	log_info(LOGGER, "Pasó a BLOCKED el carpincho %d\n", estructura_nucleo_cpu->lugar_pcb_carpincho->pid);
	pthread_mutex_unlock(&mutex_lista_blocked);
	pthread_mutex_lock(&mutex_lista_nucleos_cpu);
	sem_wait(&estructura_nucleo_cpu->sem_exec);
	estructura_nucleo_cpu->ocupado = false;
	pthread_mutex_unlock(&mutex_lista_nucleos_cpu);
	sem_post(&sem_grado_multiprocesamiento);
	algoritmo_planificador_mediano_plazo_blocked_suspended();
	free(nombre);
}

void ejecutar_io(t_dispositivo* dispositivo) {
	while(1) {
		sem_wait(&dispositivo->sem);
		pcb_carpincho* pcb = (pcb_carpincho*) list_remove(dispositivo->cola_espera, 0);
		usleep(dispositivo->rafaga*1000);

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
		signal(SIGINT,  &matar_dispositivo_io);
	}
}

void matar_dispositivo_io() {
	pthread_exit(NULL);
}
