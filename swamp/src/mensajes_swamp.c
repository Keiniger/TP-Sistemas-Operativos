#include "mensajes_swamp.h"

int32_t recibir_op_swap(int socket_cliente) {
	int32_t cod_op;

	if (recv(socket_cliente, &cod_op, sizeof(t_peticion_swap), MSG_WAITALL) != 0){
		return cod_op;
	}

	return -1;
}

int32_t recibir_entero_swap(int cliente){
	uint32_t entero;

	if (recv(cliente, &entero, sizeof(uint32_t), MSG_WAITALL) != 0) {
		return entero;
	} else {
		close(cliente);
		return -1;
	}
}