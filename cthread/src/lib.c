#include <stdio.h>
#include <string.h>
#include "../include/support.h"
#include "../include/cthread.h"
#include "../include/cdata.h"

#define TAM_PILHA 4096

int Inicializa();


int PrimeiraExecucao = -1;
int UltimaTID = 1;

FILA2 AllThreads;
FILA2 BaixaPrio;
FILA2 MediaPrio;
FILA2 AltaPrio;

//-------------------

FILA2 bloqueadas;
FILA2 filaJoin;
TCB_t* threadExecutando;
ucontext_t yield;
ucontext_t threadTerminada;
int yieldBit;
int joinBit;




int ccreate (void* (*start)(void*), void *arg, int prio)
{

    int sucesso = 0;
    if(PrimeiraExecucao == -1)
    {
        sucesso = Inicializa();
        if(sucesso == -1)
            return -1;
    }

    TCB_t *newThread = NULL;

    newThread = (TCB_t*) malloc(sizeof(TCB_t));
    newThread -> state = PROCST_APTO;
    newThread -> prio = prio;
    newThread -> tid = UltimaTID;

    getcontext(&newThread->context);

    newThread->context.uc_stack.ss_sp = (char*) malloc(TAM_PILHA);
    newThread->context.uc_stack.ss_size = TAM_PILHA;
    newThread->context.uc_link = &threadTerminada;

    makecontext(&newThread->context, (void (*)(void))start, 1, arg);


    UltimaTID += 1;
    newThread -> tid = UltimaTID;

    switch(prio)
    {
    case 0:
        sucesso = AppendFila2(&AltaPrio,(void*)newThread);
        break;
    case 1:
        sucesso = AppendFila2(&MediaPrio,(void*)newThread);
        break;
    case 2:
        sucesso = AppendFila2(&BaixaPrio,(void*)newThread);
        break;
    default:
        return -1;
        break;
    }
    if (sucesso =! 0)
        return -1;

    sucesso = AppendFila2(&AllThreads,(void*)newThread);

    if (sucesso =! 0)
        return -1;

    return newThread->tid;
}



int csetprio(int tid, int prio)
{
    return -1;
}

int cyield(void)
{
    return -1;
}

int cjoin(int tid)
{
    return -1;
}

int csem_init(csem_t *sem, int count)
{
    return -1;
}

int cwait(csem_t *sem)
{
    return -1;
}

int csignal(csem_t *sem)
{
    return -1;
}

int cidentify (char *name, int size)
{
    strncpy (name, "Dieniffer Vargas, 261612 \nHenrique Capelatto, 230188 \nNicolas Cendron, 230281", size);

    if(name == NULL)
        return -1;
    else
        return 0;

}

//Funções auxiliares

int Inicializa()
{
    CreateFila2(&AllThreads);
    CreateFila2(&AltaPrio);
    CreateFila2(&MediaPrio);
    CreateFila2(&BaixaPrio);

    PrimeiraExecucao = 0;

    return 0;
}
