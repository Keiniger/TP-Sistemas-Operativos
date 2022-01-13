#include "kernel.h"


t_kernel_config crear_archivo_config_kernel(char* path) {
    t_config* kernel_config;
    kernel_config = config_create(path);
    t_kernel_config config;

    if (kernel_config == NULL) {
        printf("Error en kernel.config\n");
        exit(-1);
    }

    config.ip_memoria = config_get_string_value(kernel_config, "IP_MEMORIA");
    config.puerto_memoria = config_get_string_value(kernel_config, "PUERTO_MEMORIA");
    config.ip_kernel = config_get_string_value(kernel_config, "IP_KERNEL");
    config.puerto_kernel = config_get_string_value(kernel_config, "PUERTO_KERNEL");
    config.alg_plani = config_get_string_value(kernel_config, "ALGORITMO_PLANIFICACION");
	config.estimacion_inicial = config_get_int_value(kernel_config, "ESTIMACION_INICIAL");
    config.alfa = config_get_double_value(kernel_config, "ALFA");
    config.dispositivos_IO = config_get_array_value(kernel_config,"DISPOSITIVOS_IO");
    config.duraciones_IO = config_get_array_value(kernel_config,"DURACIONES_IO");
    config.tiempo_deadlock = config_get_int_value(kernel_config, "TIEMPO_DEADLOCK");
    config.grado_multiprogramacion = config_get_int_value(kernel_config, "GRADO_MULTIPROGRAMACION");
    config.grado_multiprocesamiento = config_get_int_value(kernel_config, "GRADO_MULTIPROCESAMIENTO");

    CONFIG = kernel_config;

    return config;
}

void init_kernel(char* pathConfig){
	LOGGER = log_create("/kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
	CONFIG_KERNEL = crear_archivo_config_kernel(pathConfig);
	log_info(LOGGER, "\n\n===================================================\n\n");

	SERVIDOR_KERNEL = iniciar_servidor(CONFIG_KERNEL.ip_kernel,CONFIG_KERNEL.puerto_kernel);

	LISTA_NEW = list_create();
	LISTA_READY = list_create();
	LISTA_EXEC = list_create();
	LISTA_BLOCKED = list_create();
	LISTA_BLOCKED_SUSPENDED = list_create();
	LISTA_READY_SUSPENDED = list_create();
	LISTA_NUCLEOS_CPU = list_create();
	LISTA_SEMAFOROS = list_create();
	LISTA_DISPOSITIVOS_IO = list_create();

	pthread_mutex_init(&mutex_id_unico, NULL);
	pthread_mutex_init(&mutex_lista_new, NULL);
	pthread_mutex_init(&mutex_lista_ready, NULL);
	pthread_mutex_init(&mutex_lista_nucleos_cpu, NULL);
	pthread_mutex_init(&mutex_lista_semaforos_mate, NULL);
	pthread_mutex_init(&mutex_lista_dispositivos_io, NULL);

	sem_init(&sem_algoritmo_planificador_largo_plazo, 0, 0);
	sem_init(&sem_cola_ready, 0, 0);
	sem_init(&sem_grado_multiprogramacion, 0, CONFIG_KERNEL.grado_multiprogramacion);
	sem_init(&sem_grado_multiprocesamiento, 0, CONFIG_KERNEL.grado_multiprocesamiento);

	crear_dispositivos_io();
	crear_nucleos_cpu();
	iniciar_planificador_largo_plazo();
	iniciar_planificador_corto_plazo();
	iniciar_algoritmo_deadlock();

	signal(SIGINT,  &cerrar_kernel);

}

int main(int argc, char*argv[]) {
	init_kernel(argv[1]);
	coordinador_multihilo();

	return EXIT_SUCCESS;
}

void cerrar_kernel() {

	void _recurso_usado_destroyer(void* elemento) {
		free(((t_registro_uso_recurso*)elemento)->nombre);
		free(((t_registro_uso_recurso*)elemento));
	}

	void _pcb_carpincho_destroyer(void* elemento) {
		list_destroy_and_destroy_elements(((pcb_carpincho*)elemento)->recursos_usados, _recurso_usado_destroyer);
		free(((pcb_carpincho*)elemento));
	}

	list_destroy_and_destroy_elements(LISTA_NEW, _pcb_carpincho_destroyer);
	list_destroy_and_destroy_elements(LISTA_READY, _pcb_carpincho_destroyer);
	list_destroy_and_destroy_elements(LISTA_EXEC, _pcb_carpincho_destroyer);
	list_destroy_and_destroy_elements(LISTA_BLOCKED, _pcb_carpincho_destroyer);
	list_destroy_and_destroy_elements(LISTA_BLOCKED_SUSPENDED, _pcb_carpincho_destroyer);
	list_destroy_and_destroy_elements(LISTA_READY_SUSPENDED, _pcb_carpincho_destroyer);
	
	void _nucleo_cpu_destroyer(void* elemento) {
		sem_destroy(&((t_nucleo_cpu*)elemento)->sem_exec);
		free(((t_nucleo_cpu*)elemento));
	}

	list_destroy_and_destroy_elements(LISTA_NUCLEOS_CPU, _nucleo_cpu_destroyer);

	void _SEM_destroyer(void* elemento) {
		list_destroy_and_destroy_elements(((t_semaforo*)elemento)->cola_bloqueados, _pcb_carpincho_destroyer);
		free(((t_semaforo*)elemento)->nombre);
		free(((t_semaforo*)elemento));
	}

	list_destroy_and_destroy_elements(LISTA_SEMAFOROS, _SEM_destroyer);

	void _IO_destroyer(void* elemento) {
		list_destroy_and_destroy_elements(((t_dispositivo*)elemento)->cola_espera, _pcb_carpincho_destroyer);
		sem_destroy(&((t_dispositivo*)elemento)->sem);
		free(((t_dispositivo*)elemento)->nombre);
		free(((t_dispositivo*)elemento));
	}

	list_destroy_and_destroy_elements(LISTA_DISPOSITIVOS_IO, _IO_destroyer);

	log_destroy(LOGGER);
	config_destroy(CONFIG);

	exit(1);
}


void crear_dispositivos_io() {
	for(int i = 0; i < size_char_array(CONFIG_KERNEL.dispositivos_IO); i++) {
		t_dispositivo* dispositivo_io = malloc(sizeof(t_dispositivo));

		dispositivo_io->nombre = CONFIG_KERNEL.dispositivos_IO[i];
		dispositivo_io->rafaga = strtol(CONFIG_KERNEL.duraciones_IO[i], NULL, 10);
		dispositivo_io->cola_espera = list_create();
		sem_init(&dispositivo_io->sem, 0, 0);

		list_add(LISTA_DISPOSITIVOS_IO, dispositivo_io);

		pthread_t hilo_dispositivo;

		pthread_create(&hilo_dispositivo, NULL, (void*)ejecutar_io, dispositivo_io);
		pthread_detach(hilo_dispositivo);

		log_info(LOGGER, "Dispositivo '%s' creado con rafaga %d.\n", dispositivo_io->nombre, dispositivo_io->rafaga);
	}
}

void coordinador_multihilo(){

	while(1) {
		pthread_t hilo_atender_carpincho;
		int *socket_cliente = malloc(sizeof(int));
		(*socket_cliente) = accept(SERVIDOR_KERNEL, NULL, NULL);
		pthread_create(&hilo_atender_carpincho, NULL , (void*)atender_carpinchos, socket_cliente);
		pthread_detach(hilo_atender_carpincho);
	}
}

void iniciar_planificador_largo_plazo() {
	pthread_create(&planificador_largo_plazo, NULL, (void*)algoritmo_planificador_largo_plazo, NULL);
	log_info(LOGGER, "Planificador de largo plazo creado correctamente.\n");
	pthread_detach(planificador_largo_plazo);
}

void iniciar_planificador_corto_plazo() {
	pthread_create(&planificador_corto_plazo, NULL, (void*)algoritmo_planificador_corto_plazo, NULL);
	log_info(LOGGER, "Planificador de corto plazo creado correctamente utilizando el algoritmo '%s'.\n", CONFIG_KERNEL.alg_plani);
	pthread_detach(planificador_corto_plazo);
	//pthread_join(planificador_corto_plazo, NULL);
}

void iniciar_algoritmo_deadlock() {
	pthread_create(&administrador_deadlock, NULL, (void*)correr_algoritmo_deadlock, NULL);
	log_info(LOGGER, "Detector de deadlock iniciado.\n");
	pthread_detach(administrador_deadlock);
}

int crear_id_unico(){
	int id_unico=0;
	pthread_mutex_lock(&mutex_id_unico);
	usleep(1);
	id_unico = (int)time(NULL);
	usleep(1);
	pthread_mutex_unlock(&mutex_id_unico);
	return id_unico;
}

void atender_carpinchos(int* cliente) {

	pcb_carpincho* pcb_carpincho = malloc(sizeof(pcb_carpincho));

	pcb_carpincho->pid = crear_id_unico();

	pcb_carpincho->real_anterior = 0;
	pcb_carpincho->estimado_anterior = CONFIG_KERNEL.estimacion_inicial;
	pcb_carpincho->tiempo_espera = 0;
	pcb_carpincho->conexion = (*cliente);
	pcb_carpincho->recursos_usados = list_create();
	//TODO hacerle free a esto

	free(cliente);

	log_info(LOGGER, "Carpincho %d en conexion %d\n.", pcb_carpincho->pid, (*cliente));

	recibir_peticion_para_continuar(pcb_carpincho->conexion);

	pasar_a_new(pcb_carpincho);
}

void pasar_a_new(pcb_carpincho* pcb_carpincho) {

	pthread_mutex_lock(&mutex_lista_new);
	list_add(LISTA_NEW, pcb_carpincho);
	log_info(LOGGER, "Carpincho %d a lista NEW.\n", pcb_carpincho->pid);
	pthread_mutex_unlock(&mutex_lista_new);

	sem_post(&sem_algoritmo_planificador_largo_plazo);

	algoritmo_planificador_mediano_plazo_blocked_suspended();
}

void crear_nucleos_cpu() {

	for(int i = 0; i < CONFIG_KERNEL.grado_multiprocesamiento; i++) {

		t_nucleo_cpu* estructura_nucleo_cpu = malloc(sizeof(t_nucleo_cpu));
		sem_init(&estructura_nucleo_cpu->sem_exec, 0, 0);
		estructura_nucleo_cpu->ocupado = false;

		list_add(LISTA_NUCLEOS_CPU, estructura_nucleo_cpu);

		pthread_t hilo_nucleo_cpu;

		pthread_create(&hilo_nucleo_cpu, NULL, (void*)ejecutar, estructura_nucleo_cpu);
		log_info(LOGGER, "Nucleo CPU %i creado.\n", i);
		pthread_detach(hilo_nucleo_cpu);
	}
}

