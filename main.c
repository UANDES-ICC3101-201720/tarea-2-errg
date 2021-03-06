/*
Main program for the virtual memory project.
Maque all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//variables globales
struct disk * disk;
int * tabla_de_frames;
int nframes;
char *physmem;
int *cola;
int head = -1;
int fin_cola = -1;
int frame = 0;
int contador_marcos_victima = 0;
int contador_ciclos = 0;
int faltas_de_pagina = 0;
int cantidad_lecturas = 0;
int cantidad_escrituras_disco = 0;

//funcion para poner en cola
int poner_en_cola(int valor){
	if (fin_cola - head == nframes - 1){ // en el caso de que esté lleno
		return -1;
	}
	else{
		if (head == -1){
			head = 0;
		}
		fin_cola++;
		cola[fin_cola] = valor;
		return valor;
	}
}
//funcion para sacar de cola
int sacar_de_cola(){
	int ret = -1;
	if (head == -1){ // en el caso de que esté vacio
	}
	else{
		ret = cola[head];
		head++;
		if (head > fin_cola){
			head = -1;
			fin_cola = -1;
		}
	}
	return ret;
}
//funcion para imprimir la cola
void imprimir_cola(){ //funcion para imprimir la cola
	if (fin_cola == -1){
	}
	else{
		for (int i = head; i <= fin_cola; i++){
			printf("%d ", cola[i]);
		}
		printf("\n");
	}
}

//Funcion del handler para Random
void handler_rand(struct page_table *pt, int page){
	//printf("page fault on page #%d\n",page);
	faltas_de_pagina++;
	if (frame == nframes){
		int marco_victima = lrand48()%nframes;
		//printf("Marco victima: %d\n",marco_victima);		
		disk_write(disk, tabla_de_frames[marco_victima], &physmem[marco_victima*PAGE_SIZE]);
		cantidad_escrituras_disco++;
		disk_read(disk, page, &physmem[marco_victima*PAGE_SIZE]);
		cantidad_lecturas++;
		page_table_set_entry(pt, page, marco_victima, PROT_READ|PROT_WRITE|PROT_EXEC);
		page_table_set_entry(pt, tabla_de_frames[marco_victima], marco_victima, 0);
		tabla_de_frames[marco_victima] = page;
	}
	else{
		page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE|PROT_EXEC);
		disk_read(disk, page, &physmem[frame*PAGE_SIZE]);
		cantidad_lecturas++;
		tabla_de_frames[frame] = page;
		frame++;
	}
	
}

//Funcion del handler para FIFO
void handler_fifo(struct page_table *pt, int page){
	//printf("page fault on page #%d\n",page);
	faltas_de_pagina++;
	if (poner_en_cola(frame) != -1){ // si se pudo meter al cola
		page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE|PROT_EXEC);
		disk_read(disk, page, &physmem[frame*PAGE_SIZE]);
		cantidad_lecturas++;
		tabla_de_frames[frame] = page;
		frame++;
	}
	else{
		int marco_victima = sacar_de_cola();
		disk_write(disk, tabla_de_frames[marco_victima], &physmem[marco_victima*PAGE_SIZE]);
		cantidad_escrituras_disco++;
		disk_read(disk, page, &physmem[marco_victima*PAGE_SIZE]);
		cantidad_lecturas++;
		page_table_set_entry(pt, page, marco_victima, PROT_READ|PROT_WRITE|PROT_EXEC);
		page_table_set_entry(pt, tabla_de_frames[marco_victima], marco_victima, 0);
		tabla_de_frames[marco_victima] = page;
		poner_en_cola(marco_victima);
	}
}

//Funcion del handler para LRU/Custom

void handler_lru(struct page_table *pt, int page){
	//printf("page fault on page #%d\n",page);
	faltas_de_pagina++;
	if ( frame < nframes){ //para el primer ciclo
		page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE|PROT_EXEC);
		disk_read(disk, page, &physmem[frame*PAGE_SIZE]);
		cantidad_lecturas++;
		tabla_de_frames[frame] = page;
		frame++;
		if (frame == nframes){
			contador_ciclos++;
		}
	}
	if (contador_ciclos != 0){ //para a partir del 2ndo ciclo en adelante
		int marco_victima = contador_marcos_victima;
		//printf("Marco victima: %d\n",marco_victima);
		disk_write(disk, tabla_de_frames[marco_victima], &physmem[marco_victima*PAGE_SIZE]);
		cantidad_escrituras_disco++;
		disk_read(disk, page, &physmem[marco_victima*PAGE_SIZE]);
		cantidad_lecturas++;
		page_table_set_entry(pt, page, marco_victima, PROT_READ|PROT_WRITE|PROT_EXEC);
		page_table_set_entry(pt, tabla_de_frames[marco_victima], marco_victima, 0);
		tabla_de_frames[marco_victima] = page;
		contador_marcos_victima++;
		if (contador_marcos_victima == nframes){
			contador_marcos_victima = 0;
		}
	}
}


int main( int argc, char *argv[] )
{
	if(argc!=5) {
		/* Add 'random' replacement algorithm if the size of your group is 3 */
		printf("use: virtmem <npages> <nframes> <rand|lru|fifo> <sort|scan|focus>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	nframes = atoi(argv[2]);
	const char *handler = argv[3];
	const char *program = argv[4];

	tabla_de_frames = malloc(sizeof(int) * nframes);

	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}
	struct page_table *pt;

	//creacion de tablas segun su handler
	if (strcmp(handler, "rand") == 0)
	{
		//crear tabla de pagina random
		pt = page_table_create( npages, nframes, handler_rand );
	}
	else if (strcmp(handler, "fifo") == 0)
	{
		//crear tabla de pagina fifo
		pt = page_table_create( npages, nframes, handler_fifo );
	}
	else if (strcmp(handler, "lru") == 0)
	{
		//crear tabla de pagina lru
		pt = page_table_create( npages, nframes, handler_lru );
	}
	else 
	{
		//en el caso de que inserte un handler no valido
		printf("Unknown handler\n");
	}
	
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}
	
	//asignacion de la memoria virtual
	char *virtmem = page_table_get_virtmem(pt);
	//asignacion de la memoria fisica
	physmem = page_table_get_physmem(pt);

	cola = malloc(sizeof(int)*10000);

	//ejecucion de program -p
	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);

	}
	//impresion de datos finales
	printf("Faltas de pagina: %d\nCantidad de lecturas a disco: %d\nCantidad de escrituras a disco: %d\n", faltas_de_pagina, cantidad_lecturas, cantidad_escrituras_disco);
	
	//liberacion del espacio ocupado por la cola
	free(cola);

	//liberacion del espacio ocupado por la tabla de marcos
	free(tabla_de_frames);

	//eliminacion de la pagina de tabla
	page_table_delete(pt);

	//cierre de disco
	disk_close(disk);

	return 0;
}
