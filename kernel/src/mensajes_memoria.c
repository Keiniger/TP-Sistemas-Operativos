#include "mensajes_memoria.h"

void memalloc(t_nucleo_cpu* estructura_nucleo_cpu) {

	int32_t entero;
	uint32_t mallocSize;

	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &entero, sizeof(uint32_t), MSG_WAITALL);
	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &mallocSize, sizeof(uint32_t), MSG_WAITALL);

	int conexion_memoria = crear_conexion(CONFIG_KERNEL.ip_memoria , CONFIG_KERNEL.puerto_memoria);

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MATE_MEMALLOC;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(uint32_t)*2;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	int offset = 0;

	memcpy(paquete->buffer->stream, &estructura_nucleo_cpu->lugar_pcb_carpincho->pid, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + offset, &mallocSize, sizeof(unsigned int));

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	int32_t dir_logica;

	send(conexion_memoria, a_enviar, bytes, 0);
	recv(conexion_memoria, &dir_logica , sizeof(int32_t) , 0);

	send(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &dir_logica, sizeof(int32_t), 0);

	free(a_enviar);
	eliminar_paquete(paquete);

	close(conexion_memoria);

}

void memread(t_nucleo_cpu* estructura_nucleo_cpu) {

	int32_t entero;
	uint32_t origin;
	uint32_t size;

	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &entero, sizeof(uint32_t), MSG_WAITALL);
	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &origin, sizeof(uint32_t), MSG_WAITALL);
	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &size, sizeof(uint32_t), MSG_WAITALL);

	int conexion_memoria = crear_conexion(CONFIG_KERNEL.ip_memoria , CONFIG_KERNEL.puerto_memoria);

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MATE_MEMREAD;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int32_t) * 3;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &estructura_nucleo_cpu->lugar_pcb_carpincho->pid, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t), &origin, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + (sizeof(uint32_t)*2), &size, sizeof(uint32_t));

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	MATE_RETURNS retorno;
	void* dest = malloc(size);

	send(conexion_memoria, a_enviar, bytes, 0);
	recv(conexion_memoria, &retorno , sizeof(uint32_t) , 0);
	recv(conexion_memoria, dest, size, 0);

	send(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &retorno, sizeof(uint32_t), 0);
	send(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, dest, size, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
	free(dest);

	close(conexion_memoria);

}

void memfree(t_nucleo_cpu* estructura_nucleo_cpu) {

	int32_t entero;
	int32_t addr;
	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &entero, sizeof(uint32_t), MSG_WAITALL);
	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &addr, sizeof(uint32_t), MSG_WAITALL);

	int conexion_memoria = crear_conexion(CONFIG_KERNEL.ip_memoria , CONFIG_KERNEL.puerto_memoria);

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MATE_MEMFREE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int32_t)*2;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &estructura_nucleo_cpu->lugar_pcb_carpincho->pid, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t), &addr, sizeof(uint32_t));

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	MATE_RETURNS retorno;

	send(conexion_memoria, a_enviar, bytes, 0);
	recv(conexion_memoria, &retorno, sizeof(uint32_t), 0);

	send(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &retorno, sizeof(uint32_t), 0);

	free(a_enviar);
	eliminar_paquete(paquete);

	close(conexion_memoria);

}

void memwrite(t_nucleo_cpu* estructura_nucleo_cpu) {

	int32_t entero;
	void* origin;
	uint32_t size;
	uint32_t dest;

	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &entero, sizeof(uint32_t), MSG_WAITALL);
	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &dest, sizeof(uint32_t), MSG_WAITALL);
	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &size, sizeof(uint32_t), MSG_WAITALL);

	origin = malloc(size);

	recv(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, origin, size, MSG_WAITALL);

	int conexion_memoria = crear_conexion(CONFIG_KERNEL.ip_memoria , CONFIG_KERNEL.puerto_memoria);

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MATE_MEMWRITE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int32_t) * 3 + size;
	paquete->buffer->stream = malloc(paquete->buffer->size);


	memcpy(paquete->buffer->stream, &estructura_nucleo_cpu->lugar_pcb_carpincho->pid, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t), &dest, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t)*2, &size, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t)*3, origin, size);

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	MATE_RETURNS retorno;

	send(conexion_memoria, a_enviar, bytes, 0);
	recv(conexion_memoria, &retorno , sizeof(uint32_t) , 0);

	send(estructura_nucleo_cpu->lugar_pcb_carpincho->conexion, &retorno, sizeof(uint32_t), 0);

	free(a_enviar);
	free(origin);
	eliminar_paquete(paquete);

	close(conexion_memoria);

}

void avisar_suspension_a_memoria(pcb_carpincho* pcb) {

	int conexion_memoria = crear_conexion(CONFIG_KERNEL.ip_memoria , CONFIG_KERNEL.puerto_memoria);

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MATE_MEMSUSP;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(uint32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &pcb->pid, sizeof(uint32_t));

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	MATE_RETURNS retorno;

	send(conexion_memoria, a_enviar, bytes, 0);
	recv(conexion_memoria, &retorno, sizeof(uint32_t), 0);

	free(a_enviar);
	eliminar_paquete(paquete);

	close(conexion_memoria);
}

void avisar_dessuspension_a_memoria(pcb_carpincho* pcb) {

	int conexion_memoria = crear_conexion(CONFIG_KERNEL.ip_memoria , CONFIG_KERNEL.puerto_memoria);

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MATE_MEMDESSUSP;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &pcb->pid, sizeof(uint32_t));

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	MATE_RETURNS retorno;

	send(conexion_memoria, a_enviar, bytes, 0);
	recv(conexion_memoria, &retorno, sizeof(uint32_t), 0);

	free(a_enviar);
	eliminar_paquete(paquete);

	close(conexion_memoria);
}

void avisar_close_a_memoria(pcb_carpincho* pcb) {

	int conexion_memoria = crear_conexion(CONFIG_KERNEL.ip_memoria , CONFIG_KERNEL.puerto_memoria);

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MATE_CLOSE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &pcb->pid, sizeof(uint32_t));

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	MATE_RETURNS retorno;

	send(conexion_memoria, a_enviar, bytes, 0);
	recv(conexion_memoria, &retorno, sizeof(uint32_t), 0);

	free(a_enviar);
	eliminar_paquete(paquete);

	close(conexion_memoria);
}