#include "tlb.h"

void init_tlb() {

	TLB      = list_create();
	TLB_HITS = list_create();
	TLB_MISS = list_create();

	TIEMPO_TLB = 0;

	log_info(LOGGER , "TLB inicializada correctamente");
}

int buscar_pag_tlb(int pid, int pag) {

	if (CONFIG.cant_entradas_tlb == 0) {
			return -1;
	}

	int _mismo_pid_y_pag(t_entrada_tlb* entrada) {
		return (entrada->pid == pid && entrada->pag == pag);
	}

	pthread_mutex_lock(&mutexTLB);
	t_entrada_tlb* entrada = (t_entrada_tlb*)list_find(TLB, (void*)_mismo_pid_y_pag);
	pthread_mutex_unlock(&mutexTLB);

	if (entrada != NULL) {
		//TLB HIT
		registrar_evento(pid, 1);
		log_info(LOGGER , "[TLB HIT] PID: %i  NRO_PAG: %i  FRAME: %i \n", entrada->pid, entrada->pag, entrada->frame);
		entrada->ultimo_uso = obtener_tiempo_TLB();
		return entrada->frame;
	}

	//TLB MISS
	registrar_evento(pid, 0);
	log_info(LOGGER, "[TLB MISS] PID: %i NRO_PAG: %i", pid, pag);
	return -1;
}

int obtener_tiempo_TLB(){
	pthread_mutex_lock(&mutexTiempoTLB);
	int t = TIEMPO_TLB;
	TIEMPO_TLB++;
	pthread_mutex_unlock(&mutexTiempoTLB);
	return t;
}

void actualizar_tlb(int pid, int pag, int frame){

	if (CONFIG.cant_entradas_tlb == 0) {
		return;
	}

	pthread_mutex_lock(&mutexTLB);
	if (list_size(TLB) < CONFIG.cant_entradas_tlb) {
		crear_entrada(pid, pag, frame);
		pthread_mutex_unlock(&mutexTLB);
		return;
	}
	pthread_mutex_unlock(&mutexTLB);

	if (string_equals_ignore_case(CONFIG.alg_reemplazo_tlb,"LRU"))
		reemplazar_LRU(pid, pag, frame);

	if (string_equals_ignore_case(CONFIG.alg_reemplazo_tlb, "FIFO"))
		reemplazar_FIFO(pid, pag, frame);

}

void crear_entrada(int pid, int pag, int frame) {

	t_entrada_tlb* entrada = malloc(sizeof(t_entrada_tlb));

	entrada->pag        = pag;
	entrada->pid        = pid;
	entrada->frame      = frame;
	entrada->ultimo_uso = obtener_tiempo_TLB();

	list_add(TLB, entrada);

}

void reemplazar_FIFO(int pid, int pag, int frame){

	t_entrada_tlb* entrada_nueva = malloc(sizeof(t_entrada_tlb));
	entrada_nueva->pag           = pag;
	entrada_nueva->pid           = pid;
	entrada_nueva->frame         = frame;
	entrada_nueva->ultimo_uso    = -1;

	pthread_mutex_lock(&mutexTLB);
	t_entrada_tlb* victima = (t_entrada_tlb*)list_get(TLB, 0);
	log_info(LOGGER, "|| TLB FIFO || [VICTIMA] PID: %i | NRO_PAG: %i | FRAME: %i ||| [NUEVA ENTRADA] PID: %i | NRO_PAG: %i | FRAME: %i | \n", victima->pid, victima->pag, victima->frame, entrada_nueva->pid, entrada_nueva->pag, entrada_nueva->frame);
	list_remove_and_destroy_element(TLB, 0, free);
	list_add(TLB, entrada_nueva);
	pthread_mutex_unlock(&mutexTLB);

}

void reemplazar_LRU(int pid, int pag, int frame){

	t_entrada_tlb* entrada_nueva = malloc(sizeof(t_entrada_tlb));
	entrada_nueva->pid 	      = pid;
	entrada_nueva->pag        = pag;
	entrada_nueva->frame      = frame;
	entrada_nueva->ultimo_uso = obtener_tiempo_TLB();

	t_entrada_tlb* masVieja(t_entrada_tlb* una_entrada, t_entrada_tlb* otra_entrada){

		if (otra_entrada->ultimo_uso > una_entrada->ultimo_uso)
			return una_entrada;

		if (otra_entrada->ultimo_uso < una_entrada->ultimo_uso)
			return otra_entrada;

		return una_entrada;
    }

	pthread_mutex_lock(&mutexTLB);
	t_entrada_tlb* victima = list_get_minimum(TLB, (void*) masVieja);
	log_info(LOGGER, "|| TLB LRU || [VICTIMA] PID: %i | NRO_PAG: %i | FRAME: %i ||| [NUEVA ENTRADA] PID: %i | NRO_PAG: %i | FRAME: %i | \n", victima->pid, victima->pag, victima->frame, entrada_nueva->pid, entrada_nueva->pag, entrada_nueva->frame);
	//free(victima);
	list_add(TLB , entrada_nueva);
	pthread_mutex_unlock(&mutexTLB);

}

void desreferenciar_pag_tlb(int pid , int nro_pag , int frame){

	bool mismo_pid_id_frame(void* elemento){
		t_entrada_tlb* entrada = (t_entrada_tlb*) elemento;
		return entrada->frame == frame && entrada->pag == nro_pag && entrada->pid == pid;
	}

	pthread_mutex_lock(&mutexTLB);
	list_remove_and_destroy_by_condition(TLB , mismo_pid_id_frame , free);
	pthread_mutex_unlock(&mutexTLB);
}


//1 --> HIT
//0 --> MISS
void registrar_evento(int pid, int event){

	int _proceso_con_id(tlb_event* evento) {
		return (evento->pid == pid);
	}

	t_list* event_list = event ? TLB_HITS : TLB_MISS;

	tlb_event* nodo_proceso = (tlb_event*)list_find(event_list, (void*)_proceso_con_id);

	if (nodo_proceso != NULL) {
		nodo_proceso->contador++;
	} else {
		tlb_event* nodo_nuevo = malloc(sizeof(tlb_event));
		nodo_nuevo->pid       = pid;
		nodo_nuevo->contador  = 1;
		list_add(event_list, nodo_nuevo);
	}
}

void generar_metricas_tlb(){
	printf("-----------------------------------------\n");
	printf("METRICAS TLB:\n");

	printf("[HITS TOTALES]: %i \n", list_size(TLB_HITS));
	printf("[HITS POR PROCESO] \n");

	for (int i = 0 ; i < list_size(TLB_HITS) ; i++) {
		tlb_event* nodo = (tlb_event*)list_get(TLB_HITS, i);
		printf("Proceso %i ---> %i HITS \n", nodo->pid, nodo->contador);
	}

	printf("[MISS TOTALES]: %i \n", list_size(TLB_MISS));
	printf("[MISS POR PROCESO] \n");

	for (int i=0 ; i < list_size(TLB_MISS) ; i++) {
		tlb_event* nodo = (tlb_event*)list_get(TLB_MISS, i);
		printf("Proceso %i ---> %i MISS \n", nodo->pid, nodo->contador);
	}
}

void dumpear_tlb(){

	FILE* file;

    char* path_name = string_new();
    string_append(&path_name, CONFIG.path_dump_tlb);
    string_append(&path_name, "/");
    char* nombre = nombrar_dump_file();
    string_append(&path_name, nombre);

    log_info(LOGGER,"Creo un archivo de dump con el nombre %s\n", path_name);

    char* time = temporal_get_string_time("%d/%m/%y %H:%M:%S");

    file = fopen(path_name, "w+");

    fprintf(file, "\n\n");
    fprintf(file, "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    fprintf(file, "Dump: %s\n\n",time);
    fprintf(file, "Algoritmo utilizado: %s \n", CONFIG.alg_reemplazo_tlb);

    mostrar_entradas_tlb(file);

    fprintf(file, "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n");

    fclose(file);

    free(path_name);
    free(nombre);
    free(time);
}

void mostrar_entradas_tlb(FILE* file){

    for(int i = 0; i < list_size(TLB); i++){
        mostrar_entrada(i,file);
    }
}

//Entrada:0	Estado:Ocupado	Carpincho: 1	Pagina: 0	Marco: 2
void mostrar_entrada(int nro_entrada, FILE* file) {

	t_entrada_tlb* entrada = list_get(TLB, nro_entrada);
	t_frame* frame = list_get(FRAMES_MEMORIA, entrada->frame);

	fprintf(file, "Entrada: %d        Esta ocupado: %d        Carpincho: %d        Pag: %d        Frame: %d \n",nro_entrada, frame->ocupado, entrada->pid, entrada->pag, entrada->frame);
}

char* nombrar_dump_file(){
	//nombre: Dump_<Timestamp>.dmp

    char* nombre = "Dump_";
	char* time = temporal_get_string_time("%d-%m-%y_%H-%M-%S");//"MiArchivo_210614235915.txt" # 14/06/2021 a las 23:59:15
	char* extension = ".txt";

	char nombre_archivo[strlen(time) + strlen(extension) + strlen(nombre) + 1];

	sprintf(nombre_archivo,"%s%s%s",nombre,time,extension);

	char* nombre_final = malloc(sizeof(nombre_archivo));
	strcpy(nombre_final, nombre_archivo);

	free(time);

	return nombre_final;
}

void printear_tlb() {
	for(int i = 0; i < list_size(TLB); i++){
		t_entrada_tlb* entrada = list_get(TLB, i);
		t_frame* frame = list_get(FRAMES_MEMORIA, entrada->frame);
		printf("Entrada: %d        Esta ocupado: %d        Carpincho: %d        Pag: %d        Frame: %d \n",i, frame->ocupado, entrada->pid, entrada->pag, entrada->frame);
	}
}

void limpiar_tlb() {
	list_clean_and_destroy_elements(TLB, free);
}

void destruir_tlb(){
	list_destroy_and_destroy_elements(TLB, free);
}
