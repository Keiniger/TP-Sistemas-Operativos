/*
 ============================================================================
 Name        : futiles.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "futiles.h"

void printeame_un_cuatro() {
	printf("4");
}

int crear_conexion(char *ip, char* puerto) {
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

/*int iniciar_servidor(char* IP, char* PUERTO)
{
   int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(IP, PUERTO, &hints, &servinfo);

    for (p=servinfo; p != NULL; p = p->ai_next) {
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            continue;
        }
        break;
    }

   listen(socket_servidor, SOMAXCONN);

   freeaddrinfo(servinfo);

   printf("Listo para escuchar a mi cliente\n");

   return socket_servidor;
}*/

int iniciar_servidor(char* IP, char* PUERTO)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(IP, PUERTO, &hints, &servinfo);

	socket_servidor = socket(servinfo->ai_family,
	                         servinfo->ai_socktype,
	                         servinfo->ai_protocol);

	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);

	printf("Listo para escuchar a mi cliente\n");

	return socket_servidor;

}


int esperar_cliente(int socket_servidor) {
  int socket_cliente = accept(socket_servidor, NULL, NULL);

  printf("Se conecto un cliente\n");
  return socket_cliente;
}

int esperar_cliente_kernel(int socket_servidor) {
	  int socket_cliente = accept(socket_servidor, NULL, NULL);

	  printf("Se conecto un cliente\n");
	  return socket_cliente;
}

int recibir_operacion(int socket_cliente) {
   int cod_op;
   if (recv(socket_cliente, &cod_op, sizeof(peticion_carpincho), MSG_WAITALL) != 0) {
      return cod_op;
   }
   else
   {
      close(socket_cliente);
      return -1;
   }
}

peticion_carpincho recibir_operacion_carpincho(int socket_cliente) {
	peticion_carpincho cod_op;
	recv(socket_cliente, &cod_op, sizeof(peticion_carpincho), MSG_WAITALL);
	//if (recv(socket_cliente, &cod_op, sizeof(peticion_carpincho), MSG_WAITALL) != 0) {
		return cod_op;
	//}
	//else
	//{
	//	close(socket_cliente);
	//	return -1;
	//}
}

int32_t recibir_entero(int cliente){
	int32_t entero;

	if (recv(cliente, &entero, sizeof(uint32_t), MSG_WAITALL) != 0) {
	      return entero;
	} else {
	    close(cliente);
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

char* recibir_mensaje(int socket_cliente) {
   uint32_t size;
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

int size_char_array(char** array) {

	int i = 0;

	while(array[i]!= NULL){
		i++;
	}

	return i;
}


// --------------------------------------------------- MEMORIA + SWAMP ------------------------------------------------------//

void solicitar_pagina(int socket, int pid, int nro_pagina) {
	t_paquete_swap* paquete = malloc(sizeof(t_paquete_swap));
	int offset = 0;

	paquete->cod_op         = TRAER_DE_SWAP;
	paquete->buffer         = malloc(sizeof(t_buffer));
	paquete->buffer->size   = sizeof(uint32_t) * 2;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &pid, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + offset, &nro_pagina, sizeof(uint32_t));

	int bytes;

	void* a_enviar = serializar_paquete_swap(paquete, &bytes);
	send(socket, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete_swap(paquete);
}

void* serializar_paquete_swap(t_paquete_swap* paquete, int* bytes) {

   int size_serializado = sizeof(t_peticion_swap) + sizeof(uint32_t) + paquete->buffer->size;

   void* buffer = malloc(size_serializado);
   buffer = realloc(buffer, size_serializado);

   int offset = 0;

   memcpy(buffer + offset, &(paquete->cod_op), sizeof(int));
   offset += sizeof(int);
   memcpy(buffer + offset, &(paquete->buffer->size), sizeof(uint32_t));
   offset += sizeof(uint32_t);
   memcpy(buffer + offset, paquete->buffer->stream, paquete->buffer->size);
   offset += paquete->buffer->size;

   (*bytes) = size_serializado;
   return buffer;
}

void eliminar_paquete_swap(t_paquete_swap* paquete) {
   free(paquete->buffer->stream);
   free(paquete->buffer);
   free(paquete);
}

