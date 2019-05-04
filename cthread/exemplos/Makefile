#
# Makefile para programas exemplo
# 

CC=gcc
LIB_DIR=../lib

all: exemplo

exemplo: exemplo.c $(LIB_DIR)/libcthread.a
	$(CC) -o exemplo exemplo.c -L$(LIB_DIR)  -Wall

	
clean:
	rm -rf exemplo mandel teste_vetor barbeiro filosofos prodcons series *.o *~




