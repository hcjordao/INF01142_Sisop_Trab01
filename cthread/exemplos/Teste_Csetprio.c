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
	printf("\n Eu sou a thread %d atualizada \n", *((int *)arg));
	return;
}

int main(int argc, char *argv[]) 
{
	int	id0;
	int i, prio;

	id0 = ccreate(func0, (void *)&i, 0);

    printf("\nPrioridade Atual da thread: %d ", id0->prio);

    prio = 1;

    if(csetprio(id0->tid,prio))
    {
        printf("\nPrioridade da thread Atualizada: %d", id0->prio);

    }

	printf("\nEu sou a main apos a alteração da prioridade \n");

    //Libera a exec
    cyield();
   
    return 0;
}