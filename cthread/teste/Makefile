#
# Makefile para programas exemplo
# 

CC=gcc
LIB_DIR=../lib

all: testeidentify testecsetprio testesemaforo

testeidentify: testeidentify.c $(LIB_DIR)/libcthread.a
	$(CC) -o testeidentify testeidentify.c -L$(LIB_DIR) -lcthread -lm -Wall
	
testecsetprio: testecsetprio.c $(LIB_DIR)/libcthread.a
	$(CC) -o testecsetprio testecsetprio.c -L$(LIB_DIR) -lcthread -lm -Wall
 
testesemaforo: testesemaforo.c $(LIB_DIR)/libcthread.a
	$(CC) -o testesemaforo testesemaforo.c -L$(LIB_DIR) -lcthread -lm -Wall
	
clean:
	rm -rf testeidentify testecsetprio testesemaforo *.o *~

