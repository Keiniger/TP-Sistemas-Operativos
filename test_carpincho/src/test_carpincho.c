/*
 ============================================================================
 Name        : test_carpincho.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <matelib/matelib.h>

//TODO deberia liberar semaforos asignados

int main(void) {

	mate_instance mate_ref;
	mate_init(&mate_ref, "../../matelib/matelib.config");
	mate_sem_init(&mate_ref, "SEM", 2);
	mate_sem_wait(&mate_ref, "SEM");
	//mate_sem_wait(&mate_ref, "SEM");
	//int32_t dirLogica = mate_memalloc(&mate_ref, 5);
	//printf("La direccion logica es: %d\n", dirLogica);
	//mate_sem_wait(&mate_ref, "SEM");
	//mate_sem_post(&mate_ref, "SEM");
	//mate_sem_destroy(&mate_ref, "SEM");
	//mate_call_io(&mate_ref, "laguna", "Hola, estoy imprimiendo desde un dispositivo");
	//mate_call_io(&mate_ref, "hierbitas", "Hola, estoy imprimiendo desde un dispositivo");
	//mate_printf();
	mate_close(&mate_ref);

	return EXIT_SUCCESS;
}
