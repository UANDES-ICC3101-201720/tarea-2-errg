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
struct disk *disk;
struct node *head = NULL;
int nframes;
//Linked lists
struct node{
	int frame;
	int page;
	struct node *next;
};

void pop_primero(struct node **head){
	struct node * next_node = NULL;
	next_node = (*head)->next;
	free(*head);
	*head = next_node;	
}

void pop_indice(struct node ** head, int index){
	struct node * current = *head;
	struct node * temp = NULL;
	if (index ==0){
		pop_primero(head);
		return;
	}
	for(int i = 0; i < index-1; i++){
		current = current->next;
	}
	temp = current->next;
	current->next = temp->next;
	free(temp);
}


void push_lfr(struct node * head, int page, int frame){
	struct node * current = head;
	while(current->next != NULL){
		current = current->next;
	}
	current->next = malloc(sizeof(struct node));
	current->next->page = page;
	current->next->frame = frame;
	current->next->next = NULL;
}

void print_list(struct node * list){
    struct node * current = list;
    while (current != NULL) {
        printf("%d\n", current->page);
				printf("marco %i\n", current->frame);
        current = current->next;
    }
}

void handler_fifo( struct page_table *pt, int page )
{
	printf("page fault on page #%d\n",page);
	struct node *node = head;
	int using_frame = -1;
	while(node != NULL && using_frame == -1){
		if(node->page == -1){
			using_frame = node->frame;
			node->page = page;
		}
		node = node->next;
	}
	char * physical_pointer;
	physical_pointer = page_table_get_physmem(pt);

	if(using_frame == -1){
		node = head;
		int old_page = node->page;
		using_frame = node->frame;
		disk_write(disk, old_page, &physical_pointer[using_frame * PAGE_SIZE]);
		page_table_set_entry(pt, old_page, using_frame, 0);
		push_lfr(head, page, using_frame);
		pop_primero(&head);
	}
	if(using_frame != -1){
		page_table_set_entry(pt, page, using_frame, PROT_READ|PROT_WRITE);
		disk_read(disk, page, &physical_pointer[using_frame * PAGE_SIZE]);
	}
}
void handler_rand( struct page_table *pt, int page )
{
	printf("page fault on page #%d\n",page);
	struct node *node = head;
	int using_frame = -1;
	char * physical_pointer;
	physical_pointer = page_table_get_physmem(pt);

	if(using_frame == -1){
		int ran_num = lrand48()%nframes;
		int ran_num2 = ran_num;
		while(ran_num > 0){
			node = node->next;
			ran_num--;
		}
		using_frame = node->frame;
		int old_page = node->page;
		if(old_page != -1){
			disk_write(disk, old_page, &physical_pointer[using_frame * PAGE_SIZE]);
			page_table_set_entry(pt, old_page, using_frame, 0);
		}
		pop_indice(&head, ran_num2);
		push_lfr(head, page, using_frame);
	}
	if(using_frame != -1){
		page_table_set_entry(pt, page, using_frame, PROT_READ|PROT_WRITE);
		disk_read(disk, page, &physical_pointer[using_frame * PAGE_SIZE]);
	}
}

void handler_lru( struct page_table *pt, int page )
{
	printf("page fault on page #%d\n",page);
	exit(1);
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

	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}
	head = malloc(sizeof(struct node));
	head->frame = 0;
	head->page = -1;
	head->next = NULL;

	for(int i = 1; i<nframes; i++){
		push_lfr(head, -1, i);
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

	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
