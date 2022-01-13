#include "matelib.h"

//------------------General Functions---------------------/

int mate_init(mate_instance *lib_ref, char *config){
	LOGGER = log_create("/kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
	t_lib_config lib_config = crear_archivo_config_lib(config);
	int conexion;
	log_debug(LOGGER, "Intento conexion con kernel");
	conexion = crear_conexion_kernel(lib_config.ip_kernel, lib_config.puerto_kernel);

	if(conexion == (-1)) {
		log_debug(LOGGER, "Intento conexion con memoria");

		conexion = crear_conexion_kernel(lib_config.ip_memoria, lib_config.puerto_memoria);

		if(conexion == (-1)) {
			log_error(LOGGER, "Error al intentar conectar.");
			return 1;
		}
	}

	//Reserva memoria del tamanio de estructura administrativa
	lib_ref->group_info = malloc(sizeof(mate_inner_structure));

	if(lib_ref->group_info == NULL){
		return 1;
	}

	((mate_inner_structure *)lib_ref->group_info)->socket_conexion = conexion;

	log_info(LOGGER, "Conexion: %d\n", conexion);
	pedir_permiso_para_continuar(((mate_inner_structure *)lib_ref->group_info)->socket_conexion);
	recibir_permiso_para_continuar(((mate_inner_structure *)lib_ref->group_info)->socket_conexion);

	return 0;
}

int mate_close(mate_instance *lib_ref){
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MATE_CLOSE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	int offset = 0;

	int bytes;
	void* a_enviar = serializar_paquete(paquete, &bytes);

	send(((mate_inner_structure *)lib_ref->group_info)->socket_conexion, a_enviar, bytes, 0);
	recibir_permiso_para_continuar(((mate_inner_structure *)lib_ref->group_info)->socket_conexion);

	free(a_enviar);
	eliminar_paquete(paquete);
	close(((mate_inner_structure *)lib_ref->group_info)->socket_conexion);
	free(lib_ref->group_info);

	log_info(LOGGER, "Mate_instance cerrado.");

	return 0;
}

t_lib_config crear_archivo_config_lib(char* ruta) {
    t_config* lib_config;
    lib_config = config_create(ruta);
    t_lib_config config;

    if (lib_config == NULL) {
        log_error(LOGGER, "No se pudo leer el archivo de configuracion de matelib.\n");
        exit(-1);
    }

    config.ip_kernel = config_get_string_value(lib_config, "IP_KERNEL");
    config.puerto_kernel = config_get_string_value(lib_config, "PUERTO_KERNEL");
    config.ip_memoria = config_get_string_value(lib_config, "IP_MEMORIA");
    config.puerto_memoria = config_get_string_value(lib_config, "PUERTO_MEMORIA");

    return config;
}

int crear_conexion(char *ip, char* puerto){
   struct addrinfo hints;
   struct addrinfo *server_info;

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;

   getaddrinfo(ip, puerto, &hints, &server_info);

   int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

   if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
      printf("No se pudo conectar\n");

   freeaddrinfo(server_info);

   return socket_cliente;
}

int crear_conexion_kernel(char *ip, char* puerto){
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1) {
		return -1;
	}

	freeaddrinfo(server_info);

	return socket_cliente;
}

//-----------------Semaphore Functions---------------------/

int mate_sem_init(mate_instance *lib_ref, mate_sem_name sem, unsigned int value) {

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MATE_INIT;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(uint32_t) + strlen(sem) + 1 + sizeof(unsigned int);
	paquete->buffer->stream = malloc(paquete->buffer->size);
	int offset = 0;

	uint32_t tamanio_nombre_semaforo = strlen(sem) + 1;

	memcpy(paquete->buffer->stream + offset, &tamanio_nombre_semaforo, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + offset, sem, strlen(sem) + 1);
	offset += (strlen(sem) + 1);
	memcpy(paquete->buffer->stream + offset, &value, sizeof(unsigned int));
	offset += sizeof(unsigned int);

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	send(((mate_inner_structure *)lib_ref->group_info)->socket_conexion, a_enviar, bytes, 0);

	recibir_permiso_para_continuar(((mate_inner_structure *)lib_ref->group_info)->socket_conexion);


	free(a_enviar);
	eliminar_paquete(paquete);
	return 0;
}

int mate_sem_wait(mate_instance *lib_ref, mate_sem_name sem) {
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MATE_SEM_WAIT;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(sem) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	int offset = 0;
	memcpy(paquete->buffer->stream + offset, sem, strlen(sem) + 1);
	offset += strlen(sem) + 1;

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	send(((mate_inner_structure *)lib_ref->group_info)->socket_conexion, a_enviar, bytes, 0);

	//recibir_permiso_para_continuar(((mate_inner_structure *)lib_ref->group_info)->socket_conexion);
	uint32_t resultado = recibir_finalizacion_por_deadlock(((mate_inner_structure *)lib_ref->group_info)->socket_conexion);

	if(resultado == 1) {
		return 0;
		free(a_enviar);
		eliminar_paquete(paquete);
	} else {
		close(((mate_inner_structure *)lib_ref->group_info)->socket_conexion);
		free(lib_ref->group_info);
		//printf("Fui finalizado por deadlock, adiosss\n");
		free(a_enviar);
		eliminar_paquete(paquete);
		pthread_exit(NULL);
	}

	return 0;
}

int mate_sem_post(mate_instance *lib_ref, mate_sem_name sem){
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MATE_SEM_POST;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(sem) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	int offset = 0;
	memcpy(paquete->buffer->stream + offset, sem, strlen(sem) + 1);
	offset += strlen(sem) + 1;

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	send(((mate_inner_structure *)lib_ref->group_info)->socket_conexion, a_enviar, bytes, 0);

	recibir_permiso_para_continuar(((mate_inner_structure *)lib_ref->group_info)->socket_conexion);

	free(a_enviar);
	eliminar_paquete(paquete);
	return 0;
}

int mate_sem_destroy(mate_instance *lib_ref, mate_sem_name sem) {
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MATE_SEM_DESTROY;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(sem) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	int offset = 0;
	memcpy(paquete->buffer->stream + offset, sem, strlen(sem) + 1);
	offset += strlen(sem) + 1;

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	send(((mate_inner_structure *)lib_ref->group_info)->socket_conexion, a_enviar, bytes, 0);

	recibir_permiso_para_continuar(((mate_inner_structure *)lib_ref->group_info)->socket_conexion);

	free(a_enviar);
	eliminar_paquete(paquete);
	return 0;
}

//--------------------IO Functions------------------------/

int mate_call_io(mate_instance *lib_ref, mate_io_resource io, void *msg) {
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MATE_CALL_IO;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(io) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	int offset = 0;
	memcpy(paquete->buffer->stream + offset, io, strlen(io) + 1);
	offset += strlen(io) + 1;

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	send(((mate_inner_structure *)lib_ref->group_info)->socket_conexion, a_enviar, bytes, 0);

	printf("Printing content: %s\n", (char *)msg);

	recibir_permiso_para_continuar(((mate_inner_structure *)lib_ref->group_info)->socket_conexion);

	printf("Done with IO %s\n", io);

	free(a_enviar);
	eliminar_paquete(paquete);
	return 0;
}

//--------------Memory Module Functions-------------------/

mate_pointer mate_memalloc(mate_instance *lib_ref, int size) {
  	t_paquete* paquete = malloc(sizeof(t_paquete));

  	paquete->codigo_operacion = MATE_MEMALLOC;
  	paquete->buffer = malloc(sizeof(t_buffer));
  	paquete->buffer->size = sizeof(uint32_t);
  	paquete->buffer->stream = malloc(paquete->buffer->size);

  	memcpy(paquete->buffer->stream, &size, sizeof(unsigned int));

  	int bytes;

  	void* a_enviar = serializar_paquete(paquete, &bytes);

  	int dir_logica;

  	send(((mate_inner_structure *)lib_ref->group_info)->socket_conexion, a_enviar, bytes, 0);
  	recv(((mate_inner_structure *)lib_ref->group_info)->socket_conexion , &dir_logica , sizeof(int32_t) , 0);

  	free(a_enviar);
  	eliminar_paquete(paquete);

  	if(dir_logica < 0){
  		return NULL;
  	}

  	return dir_logica;
}

int mate_memfree(mate_instance *lib_ref, mate_pointer addr) {
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MATE_MEMFREE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &addr, sizeof(uint32_t));

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	MATE_RETURNS retorno;

	send(((mate_inner_structure *)lib_ref->group_info)->socket_conexion, a_enviar, bytes, 0);
	recv(((mate_inner_structure *)lib_ref->group_info)->socket_conexion , &retorno , sizeof(uint32_t) , 0);

	free(a_enviar);
	eliminar_paquete(paquete);

	return retorno;
}

int mate_memread(mate_instance *lib_ref, mate_pointer origin, void *dest, int size) {
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MATE_MEMREAD;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int32_t) * 2;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &origin, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t), &size, sizeof(uint32_t));

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	MATE_RETURNS retorno;

	send(((mate_inner_structure *)lib_ref->group_info)->socket_conexion, a_enviar, bytes, 0);
	recv(((mate_inner_structure *)lib_ref->group_info)->socket_conexion , &retorno , sizeof(uint32_t) , 0);
	recv(((mate_inner_structure *)lib_ref->group_info)->socket_conexion , dest , size , 0);

	free(a_enviar);
	eliminar_paquete(paquete);

  	return retorno;
}

int mate_memwrite(mate_instance *lib_ref, void *origin, mate_pointer dest, int size)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MATE_MEMWRITE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int32_t) * 2 + size;
	paquete->buffer->stream = malloc(paquete->buffer->size);


	memcpy(paquete->buffer->stream, &dest, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t), &size, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t)*2, origin, size);

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	MATE_RETURNS retorno;

	send(((mate_inner_structure *)lib_ref->group_info)->socket_conexion, a_enviar, bytes, 0);
	recv(((mate_inner_structure *)lib_ref->group_info)->socket_conexion , &retorno , sizeof(uint32_t) , 0);

	free(a_enviar);
	eliminar_paquete(paquete);

	return retorno;
}

void pedir_permiso_para_continuar(int conexion) {
	uint32_t handshake = 1;
	send(conexion, &handshake, sizeof(uint32_t), 0);
}

void recibir_permiso_para_continuar(int conexion) {
	uint32_t result;
	recv(conexion, &result, sizeof(uint32_t), MSG_WAITALL);
}

int recibir_finalizacion_por_deadlock(int conexion) {
	uint32_t result;
	recv(conexion, &result, sizeof(uint32_t), MSG_WAITALL);
	return result;
}

int recibir_operacion(int socket_cliente) {
   peticion_carpincho cod_op;
   if (recv(socket_cliente, &cod_op, sizeof(peticion_carpincho), MSG_WAITALL) != 0) {
      	return cod_op;
   }
   else {
		close(socket_cliente);
		return -1;
   }
}

peticion_carpincho recibir_operacion_carpincho(int socket_cliente) {
	peticion_carpincho cod_op;
	if (recv(socket_cliente, &cod_op, sizeof(peticion_carpincho), MSG_WAITALL) != 0) {
		return cod_op;
	}
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void enviar_mensaje(char* mensaje, int socket_cliente) {
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

void* serializar_paquete(t_paquete* paquete, int* bytes) {
   int size_serializado = sizeof(peticion_carpincho) + sizeof(uint32_t) + paquete->buffer->size;
   void *buffer = malloc(size_serializado);

   int offset = 0;

   memcpy(buffer + offset, &(paquete->codigo_operacion), sizeof(peticion_carpincho));
   offset += sizeof(peticion_carpincho);
   memcpy(buffer + offset, &(paquete->buffer->size), sizeof(uint32_t));
   offset += sizeof(uint32_t);
   memcpy(buffer + offset, paquete->buffer->stream, paquete->buffer->size);
   offset += paquete->buffer->size;

   (*bytes) = size_serializado;
   return buffer;
}

void eliminar_paquete(t_paquete* paquete) {
   free(paquete->buffer->stream);
   free(paquete->buffer);
   free(paquete);
}


char* recibir_mensaje(int socket_cliente) {
   int size;
   char* buffer = recibir_buffer(&size, socket_cliente);
   return buffer;
}

void* recibir_buffer(int* size, int socket_cliente) {
   void* buffer;
   recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
   buffer = malloc(*size);
   recv(socket_cliente, buffer, *size, MSG_WAITALL);

   return buffer;
}