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
    cwait(&semaforo);
    printf("Usando recurso na funcao  %d\n", *((int *)arg));
    csignal(&semaforo);
	return;
}

void* func1(void *arg) {
	cwait(&semaforo);
    printf("Usando recurso na funcao  %d\n", *((int *)arg));
    csignal(&semaforo);
	return;
}


int main(int argc, char *argv[]) 
{
    int id0, id1;
	int i ;
    
    //Inicializa o semaforo
    csem_init(&semaforo,TAM_SEM);

    //Utiliza um recurso
    cwait(&semaforo);

    //Cria as threads
    id0 = ccreate(func0,(void *)&i, 0);
    id1 = ccreate(func1,(void *)&i, 0);

    cyield();

    printf("Criei o semaforo as threads e estou usando um recurso");

    csignal(&semaforo);

    cyield();
    cyield();

    return 0;
}



