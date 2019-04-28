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
int verificaSeThreadEstaNaFila(int, PFILA2);
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

ucontext_t contextoYield;
ucontext_t threadTerminada;

//Threads

TCB_t* threadExecutando;
TCB_t* mainThread;

//Cria uma nova Thread. Não Preemptivo.
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


//Set Prioridade da Thread em Execução. Não preemptivo.
int csetprio(int tid, int prio)
{
    int sucesso = 0;
    if(PrimeiraExecucao == -1)
    {
        sucesso = Inicializa();
        if(sucesso == -1)
            return -1;
    }

    tid = -1;// ESSE TID NAO È USADO NA versão 2019/1

    threadExecutando->prio = prio;

    //Não É Preemptivo
    return 0;
}

//Libera o Controle
int cyield(void)
{
    int sucesso = 0;
    if(PrimeiraExecucao == -1)
    {
        sucesso = Inicializa();
        if(sucesso == -1)
            return -1;
    }
    // Troca do Contexto em Execução para o Contexto Yield
    swapcontext(&threadExecutando->context,&contextoYield);

    return 0;
}

int cjoin(int tid)
{
    int sucesso = 0;
    if(PrimeiraExecucao == -1)
    {
        sucesso = Inicializa();
        if(sucesso == -1)
            return -1;
    }
    return -1;
}

//Inicializa um semforo.
int csem_init(csem_t *sem, int count)
{
    int sucesso = 0;
    if(PrimeiraExecucao == -1)
    {
        sucesso = Inicializa();
        if(sucesso == -1)
            return -1;
    }

    sem->count = count;
    sem->fila = (PFILA2) malloc(sizeof(PFILA2));

    int sucessoCriarFila = CreateFila2(sem->fila);

    if(sucessoCriarFila == 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int cwait(csem_t *sem)
{
    int sucesso = 0;
    if(PrimeiraExecucao == -1)
    {
        sucesso = Inicializa();
        if(sucesso == -1)
            return -1;
    }

    //Caso Semaforo Esteja Livre
    if(sem->count > 0) //Semaforo Livre
    {
        sem->count -=1;
        return 0;
    }

    //Adiciona o tid da thread executando na lista aguardando semaforo

    sucesso = AppendFila2(sem->fila,(void*) threadExecutando->tid);
    
    if(sucesso != 0) return -1;

    //Muda o Estado de Executando para Bloqueado
    threadExecutando->state = PROCST_BLOQ;

    //Adiciona o contexto da thread executando na lista de bloqueados

    sucesso = AppendFila2(&bloqueadas,(void*)threadExecutando);
    if(sucesso != 0) return -1;

    //Decrementa o Contador do Semaforo ???
    sem->count -= 1;

    //Troca de contexto por que perdeu o processador
    swapcontext(&threadExecutando->context,&contextoYield);

    return 0;
}

int csignal(csem_t *sem)
{
    int sucesso = 0;
    if(PrimeiraExecucao == -1)
    {
        sucesso = Inicializa();
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
    if(
        CreateFila2(&AllThreads) != 0 ||
        CreateFila2(&AltaPrio)   != 0 ||
        CreateFila2(&MediaPrio)  != 0 ||
        CreateFila2(&BaixaPrio)  != 0 )
    {
        return -1;
    }

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
    getcontext(&contextoYield);
    contextoYield.uc_link = 0;
    contextoYield.uc_stack.ss_sp = (char *) malloc(TAM_PILHA);
    contextoYield.uc_stack.ss_size = TAM_PILHA;
    makecontext(&contextoYield, (void(*)(void)) escalonador, 0);

    PrimeiraExecucao = 0;
    return 0;
}

void fimDeExecucao()
{
    free(threadExecutando);
    escalonador();
}

void escalonador()
{

}

int verificaSeThreadEstaNaFila(int tid, PFILA2 filaEntrada)
{
	TCB_t *threadAtual = NULL;
	int found = 0;

	FirstFila2(filaEntrada);

	threadAtual = (TCB_t*) GetAtIteratorFila2(filaEntrada);

	while(found == 0 && threadAtual != NULL)
	{
		if(threadAtual->tid == tid)
		{
			found = 1;
		}
		else{
			NextFila2(filaEntrada);
			threadAtual = (TCB_t*) GetAtIteratorFila2(filaEntrada);
		}
	}
	return found;

}


