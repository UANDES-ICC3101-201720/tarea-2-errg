CFLAGS=-std=gnu11 -Wall -Werror

virtmem: main.o page_table.o disk.o program.o
	gcc main.o page_table.o disk.o program.o -o virtmem $(CFLAGS)

main.o: main.c
	gcc -Wall -g -c main.c -o main.o $(CFLAGS)

page_table.o: page_table.c
	gcc -Wall -g -c page_table.c -o page_table.o $(CFLAGS)

disk.o: disk.c
	gcc -Wall -g -c disk.c -o disk.o $(CFLAGS)

program.o: program.c
	gcc -Wall -g -c program.c -o program.o $(CFLAGS)


clean:
	rm -f *.o virtmem
