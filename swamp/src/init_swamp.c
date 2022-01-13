#include "init_swamp.h"

void init_swamp(){

	//Incializamos logger
	LOGGER = log_create("/home/utnso/workspace/tp-2021-2c-DesacatadOS/swamp/SWAMP.log", "SWAMP", 1, LOG_LEVEL_INFO);

	//Inicializamos archivo de configuracion
	crear_config_swamp("/home/utnso/workspace/tp-2021-2c-DesacatadOS/swamp/src/swamp.config");

	//Iniciamos servidor
	SERVIDOR_SWAP = iniciar_servidor(CONFIG.ip, CONFIG.puerto);

	//Creamos estructuras correspondientes
	METADATA_ARCHIVOS = list_create();
	FRAMES_SWAP = list_create();

	crear_archivos();
	crear_frames();
}

void crear_config_swamp(char* ruta) {
	t_swamp_config swamp_config;
	t_config* config = config_create(ruta);

    if (config == NULL) {
        printf("No se pudo leer el archivo de configuracion de Swamp\n");
        exit(-1);
    }

    swamp_config.ip = config_get_string_value(config, "IP");
    swamp_config.puerto = config_get_string_value(config, "PUERTO");
    swamp_config.tamanio_swamp = config_get_int_value(config, "TAMANIO_SWAP");
    swamp_config.tamanio_pagina = config_get_int_value(config, "TAMANIO_PAGINA");
    swamp_config.archivos_swamp = config_get_array_value(config, "ARCHIVOS_SWAP");
    swamp_config.marcos_max = config_get_int_value(config, "MARCOS_POR_CARPINCHO");
    swamp_config.retardo_swap = config_get_int_value(config, "RETARDO_SWAP");

    CONFIG = swamp_config;
    CFG = config;

}

void crear_frames() {
	int frames_x_archivo = CONFIG.tamanio_swamp/CONFIG.tamanio_pagina;

	for (int i = 0 ; i < size_char_array(CONFIG.archivos_swamp) ; i++) {
		for (int j= 0 ; j < frames_x_archivo ; j++) {
			t_frame* frame = malloc(sizeof(t_frame));

			frame->id_archivo = i;
			frame->offset = j * CONFIG.tamanio_pagina;
			frame->ocupado = false;
			frame->id_pagina = -1;
			frame->id_proceso = -1;

			list_add(FRAMES_SWAP, frame);
		}
	}
}

void crear_archivos() {
	for (int i = 0 ; i < size_char_array(CONFIG.archivos_swamp) ; i++) {
		t_metadata_archivo* md_archivo = malloc(sizeof(t_metadata_archivo));

		md_archivo->id = i;
		md_archivo->espacio_disponible = CONFIG.tamanio_swamp;
		md_archivo->fd = crear_archivo(CONFIG.archivos_swamp[i], CONFIG.tamanio_swamp);

		list_add(METADATA_ARCHIVOS, md_archivo);
	}

}

int crear_archivo(char* path, int size) {
	int fd;
	void* addr;

	fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0777);
	if (fd < 0) {
	  exit(EXIT_FAILURE);
	}

	ftruncate(fd, size);
	addr = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);

	if (addr == MAP_FAILED) {
		log_error(LOGGER, "Error al mapear.");
		exit(1);
	}

	memset(addr, '\0', CONFIG.tamanio_swamp);
	munmap(addr, CONFIG.tamanio_swamp);

	return fd;
}

void finish_swamp(){
	log_destroy(LOGGER);
	config_destroy(CFG);

	list_destroy_and_destroy_elements(FRAMES_SWAP, free);
	list_destroy_and_destroy_elements(METADATA_ARCHIVOS, free);
}