/*
 *	Programa de exemplo de uso da biblioteca cthread
 *
 * 	Sistemas Operacionais I - www.inf.ufrgs.br
 * 
 * Teste da funcao csetprio
 *
 */

#include "../include/support.h"
#include "../include/cthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* func0(void *arg) {

    int novaPrioridade = 1;
    int tid = 0; 

    printf("Thread 1 -> Executando\n");

    printf("Thread 1 -> Alterando Prioridade Própria\n");
    if(csetprio(tid,novaPrioridade) == 0)
    {
        printf("Thread 1 -> Prioridade alterada para %d\n", novaPrioridade);
    } else {
        printf("Thread 1 -> Prioridade não foi alterada\n");
    }

    printf("Thread 1 -> Finalizando\n");
	return (NULL);
}

int main(int argc, char *argv[]) 
{
	int id0, i, threadPrio;
    printf("Thread Main -> Executando...\n");

    threadPrio = 0;

	id0 = ccreate(func0, (void *)&i, threadPrio);
    if(id0 != -1) printf("Thread 1 criada com prioridade %d...\n", threadPrio); 

    printf("Thread Main -> Perdeu Processador\n");
    cyield();
    
    printf("Thread Main -> Finalizando...\n");
    exit(0);
}
