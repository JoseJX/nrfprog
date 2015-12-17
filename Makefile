all:	nrfprog
CFLAGS=-ggdb -Wall
DEBUG := $(if $(DEBUG),-DDEBUG_PRINT)

nrfprog: nrfprog.c bp.h nrf24le1.h
	gcc ${CFLAGS} -o nrfprog nrfprog.c $(DEBUG)

clean:
	rm -f nrfprog
