all:	nrfprog
CFLAGS=-ggdb -Wall

nrfprog: nrfprog.c bp.h nrf24le1.h
	gcc ${CFLAGS} -o nrfprog nrfprog.c

clean:
	rm -f nrfprog
