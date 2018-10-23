/*
Main program for the virtual memory project.
Make all of your modifications to this file.
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
void handler_rand( struct page_table *pt, int page)
{
	if (page<=page_table_get_nframes(pt)-1)
	{
		page_table_set_entry(pt,page,page,PROT_READ|PROT_WRITE|PROT_EXEC);
		printf("Rand: page fault on page #%d\n",page);
	}
	else
	{
		printf("a\n");
		exit(1);
	}
	
}
void handler_lru( struct page_table *pt, int page )
{
	page_table_set_entry(pt,page,page,PROT_READ|PROT_WRITE|PROT_EXEC);
	printf("LRU: page fault on page #%d\n",page);
}
void handler_fifo( struct page_table *pt, int page )
{
	page_table_set_entry(pt,page,page,PROT_READ|PROT_WRITE|PROT_EXEC);
	printf("FIFO: page fault on page #%d\n",page);
}
int main( int argc, char *argv[] )
{
	if(argc!=5) {
		/* Add 'random' replacement algorithm if the size of your group is 3 */
		printf("use: virtmem <npages> <nframes> <rand|lru|fifo> <sort|scan|focus>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	const char *handler = argv[3];
	const char *program = argv[4];

	struct disk *disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}
	struct page_table *pt;
	//handler -a
	if(!strcmp(handler,"rand"))
	{
		//random
		pt = page_table_create( npages, nframes, handler_rand );
	}
	else if(!strcmp(handler,"fifo"))
	{
		//fifo
		pt = page_table_create( npages, nframes, handler_fifo );
	}
	else if(!strcmp(handler,"lru"))
	{
		//custom
		pt = page_table_create( npages, nframes, handler_lru );
	}
	else 
	{
		printf("Unknown handler\n");
	}
	
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}
	
	char *virtmem = page_table_get_virtmem(pt);

	char *physmem = page_table_get_physmem(pt);

	//ejecucion de program
	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);

	}
	page_table_print(pt);
	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
