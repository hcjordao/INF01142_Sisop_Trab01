/*
 *	Programa de exemplo de uso da biblioteca cthread
 *
 * 	Sistemas Operacionais I - www.inf.ufrgs.br
 * 
 * Teste do semaforo funções cwait e csignal
 *
 */

#include "../include/support.h"
#include "../include/cthread.h"
#include <stdio.h>
#include <stdlib.h>

#define TAM_SEM 2

csem_t semaforo;

void* func0(void *arg) {
    printf("Thread 1 -> Requisitando Recurso\n");
    cwait(&semaforo);
    printf("Thread 1 -> Utilizando o recurso na funcao %d\n", *((int *)arg));
    printf("Thread 1 -> Liberando Recurso\n");
    csignal(&semaforo);
    printf("Thread 1 -> Recurso Liberado\n");
    printf("Thread 1 -> Thread 1 finalizada...\n");
	return (NULL);
}

void* func1(void *arg) {
	printf("Thread 2 -> Requisitando Recurso\n");
    cwait(&semaforo);
    printf("Thread 2 -> Utilizando o recurso na funcao %d\n", *((int *)arg));
    printf("Thread 2 -> Liberando Recurso\n");
    csignal(&semaforo);
    printf("Thread 2 -> Recurso Liberado\n");
    printf("Thread 2 -> Thread 2 finalizada...\n");

	return (NULL);
}

int main(int argc, char *argv[]) 
{
    int id0, id1, i;
    
    printf("Thread Main -> Executando...\n");
    
    //Inicializando o semaforo.
    csem_init(&semaforo,TAM_SEM);

    //Utiliza um recurso
    printf("Thread Main -> Requisitando Recurso\n");
    cwait(&semaforo);
    printf("Thread Main -> Recurso Alocado\n");

    //Cria as threads
    id0 = ccreate(func0,(void *)&i, 0);
    if(id0 != -1) printf("Thread 1 criada\n");

    id1 = ccreate(func1,(void *)&i, 0);
    if(id1 != -1) printf("Thread 2 criada\n");

    cyield();

    printf("Thread Main -> Liberando Recurso\n");
    csignal(&semaforo);
    printf("Thread Main -> Recurso Liberado\n");

    cyield();

    printf("Thread Main -> Finalizando...\n");
    return 0;
}