#
# Makefile para programas exemplo
# 

CC=gcc
LIB_DIR=../lib

all: exemplo mandel teste_vetor series testeidentify testecsetprio testesemaforo

exemplo: exemplo.c $(LIB_DIR)/libcthread.a
	$(CC) -m32 -o exemplo exemplo.c -L$(LIB_DIR) -lcthread -Wall

teste_vetor: teste_vetor.c $(LIB_DIR)/libcthread.a
	$(CC) -o teste_vetor teste_vetor.c -L$(LIB_DIR) -lcthread -Wall

mandel: mandel.c $(LIB_DIR)/libcthread.a
	$(CC) -o mandel mandel.c -L$(LIB_DIR) -lcthread -lm -Wall

series: series.c $(LIB_DIR)/libcthread.a
	$(CC) -o series series.c -L$(LIB_DIR) -lcthread -lm -Wall
	
 testeidentify: testeidentify.c $(LIB_DIR)/libcthread.a
	$(CC) -o testeidentify testeidentify.c -L$(LIB_DIR) -lcthread -lm -Wall
	
 testecsetprio: testecsetprio.c $(LIB_DIR)/libcthread.a
	$(CC) -o testecsetprio testecsetprio.c -L$(LIB_DIR) -lcthread -lm -Wall
 
 testesemaforo: testesemaforo.c $(LIB_DIR)/libcthread.a
	$(CC) -o testesemaforo testesemaforo.c -L$(LIB_DIR) -lcthread -lm -Wall
	
clean:
	rm -rf exemplo mandel teste_vetor series testeidentify testecsetprio testesemaforo *.o *~

