#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/support.h"
#include "../include/cthread.h"
#include "../include/cdata.h"

#define TAM_PILHA 4096

//Prototipos

int Inicializa();
void fimDeExecucao();
void escalonador();

// Controles

int PrimeiraExecucao = -1;
int UltimaTID = 0;
int yieldBit = 0;
int joinBit = 0;

//Filas

FILA2 AllThreads;
FILA2 BaixaPrio;
FILA2 MediaPrio;
FILA2 AltaPrio;
FILA2 bloqueadas;
FILA2 filaJoin;

//Contextos

ucontext_t yield;
ucontext_t threadTerminada;

//Threads

TCB_t* threadExecutando;
TCB_t* mainThread;


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
    if (sucesso != 0)
        return -1;

    sucesso = AppendFila2(&AllThreads,(void*)newThread);

    if (sucesso != 0)
        return -1;

    return newThread->tid;
}



int csetprio(int tid, int prio)
{
    if(PrimeiraExecucao == -1)
    {
        int sucesso = Inicializa();
        if(sucesso == -1)
            return -1;
    }

    return -1;
}

int cyield(void)
{
    if(PrimeiraExecucao == -1)
    {
        int sucesso = Inicializa();
        if(sucesso == -1)
            return -1;
    }
    return -1;
}

int cjoin(int tid)
{
    if(PrimeiraExecucao == -1)
    {
        int sucesso = Inicializa();
        if(sucesso == -1)
            return -1;
    }
    return -1;
}

int csem_init(csem_t *sem, int count)
{
    if(PrimeiraExecucao == -1)
    {
        int sucesso = Inicializa();
        if(sucesso == -1)
            return -1;
    }
    return -1;
}

int cwait(csem_t *sem)
{
    if(PrimeiraExecucao == -1)
    {
        int sucesso = Inicializa();
        if(sucesso == -1)
            return -1;
    }
    return -1;
}

int csignal(csem_t *sem)
{
    if(PrimeiraExecucao == -1)
    {
        int sucesso = Inicializa();
        if(sucesso == -1)
            return -1;
    }
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
    // Cria as filas

    CreateFila2(&AllThreads);
    CreateFila2(&AltaPrio);
    CreateFila2(&MediaPrio);
    CreateFila2(&BaixaPrio);

    // Cria Contexto para a Main
    getcontext(&mainThread->context);
    mainThread = (TCB_t*)malloc(sizeof(TCB_t));
    mainThread -> state = PROCST_EXEC;
    mainThread -> tid = 0;

    //Coloca a Thread Main na lista de todas as threads
    AppendFila2(&AllThreads,(void*)mainThread);

    //Define a Main thread como a thread executando no momento
    threadExecutando = mainThread;

    //Contexto de Finalização
    getcontext(&threadTerminada);
    threadTerminada.uc_link = NULL;
    threadTerminada.uc_stack.ss_sp = (char *)malloc(TAM_PILHA);
    threadTerminada.uc_stack.ss_size = TAM_PILHA;
    makecontext(&threadTerminada, (void(*)(void)) fimDeExecucao, 0);

    //Contexto de Yield
    getcontext(&yield);
    yield.uc_link = 0;
    yield.uc_stack.ss_sp = (char *) malloc(TAM_PILHA);
    yield.uc_stack.ss_size = TAM_PILHA;
    makecontext(&yield, (void(*)(void)) escalonador, 0);

    PrimeiraExecucao = 0;
    return 0;
}

void fimDeExecucao(){
  free(threadExecutando);
  escalonador();
}

void escalonador()
{

}


