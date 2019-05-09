#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/support.h"
#include "../include/cthread.h"
#include "../include/cdata.h"

#define TAM_PILHA 4096

//Prototipos
int inicializa();
int fimDeExecucao();
int escalonador();
int verificaSeThreadEstaNaFila(int, PFILA2);
void transicaoExecParaApto();
void transicaoBloqParaApto();

// Controles
int primeiraExec = -1;
int ultimaTID = 0;
int yield = 0;
int debug = 0;

//Filas
FILA2 todasThreads;
FILA2 filaBaixaPrio;
FILA2 filaMediaPrio;
FILA2 filaAltaPrio;
FILA2 filaBloqueadas;
FILA2 filaJoin;

//Contextos
ucontext_t contextoYield;
ucontext_t threadTerminada;

//Threads
TCB_t* threadExecutando;
TCB_t* mainThread;

//Estrutura do join - thread + tid(thread aguardada)
typedef struct TCB_join{
    TCB_t *tcb;
    int tid;
}TCB_join;

//Funções
int ccreate (void* (*start)(void*), void *arg, int prio){
	if(debug) printf("\n[ccreate]Inicio Criacao tid: %d com prioidade %d \n",ultimaTID+1,prio);

    int sucesso = 0;
    if(primeiraExec == -1){
        sucesso = inicializa();
        if(sucesso == -1) return -1;
    }

    TCB_t *newThread = NULL;

    newThread = (TCB_t*) malloc(sizeof(TCB_t));
    newThread -> state = PROCST_APTO;
    newThread -> prio = prio;
    newThread -> tid = ultimaTID;

    getcontext(&newThread->context);

    newThread->context.uc_stack.ss_sp = (char*) malloc(TAM_PILHA);
    newThread->context.uc_stack.ss_size = TAM_PILHA;
    newThread->context.uc_link = &threadTerminada;

    makecontext(&newThread->context, (void (*)(void))start, 1, arg);

    ultimaTID += 1;
    newThread -> tid = ultimaTID;

    switch(prio){
        case 0:
            sucesso = AppendFila2(&filaAltaPrio,(void*)newThread);
            break;
        case 1:
            sucesso = AppendFila2(&filaMediaPrio,(void*)newThread);
            break;
        case 2:
            sucesso = AppendFila2(&filaBaixaPrio,(void*)newThread);
            break;
        default:
            return -1;
            break;
    }

    if (sucesso != 0){
        return -1;
    }

    sucesso = AppendFila2(&todasThreads,(void*)newThread);

    if (sucesso != 0){
        return -1;
    }

	if(debug) printf("\n[ccreate]Criado com sucesso\n");

    return newThread->tid;
}

int csetprio(int tid, int prio){

	if(debug) printf("\n[csetprio]: Processo %d passará da prioridade %d para %d\n",threadExecutando->tid,threadExecutando->prio, prio);

    int sucesso = 0;
    if(primeiraExec == -1){
        sucesso = inicializa();
        if(sucesso == -1){
            return -1;
        }
    }

    threadExecutando->prio = prio;

    if(debug) printf("\n[csetprio]: Prioridade atualizada com sucesso\n");

    return 0;
}

int cyield(void){
    if(debug) printf("\n[cyield]: Processo %d abriu mão do processador\n",threadExecutando->tid);

    int sucesso = 0;
    if(primeiraExec == -1){
        sucesso = inicializa();
        if(sucesso == -1){
            return -1;
        }
    }

    yield = 1;
    swapcontext(&threadExecutando->context,&contextoYield);

    return 0;
}

int cjoin(int tid){
    if(debug) printf("\n[cjoin]: Processo %d quer aguardar o termino do processo %d\n",threadExecutando->tid,tid);

    int sucesso = 0;

    if(primeiraExec == -1){
        sucesso = inicializa();
        if(sucesso == -1){
            return -1;
        }
    }

    if(tid == 0){
        return -1;
    }

    if( verificaSeThreadEstaNaFila(tid,&filaAltaPrio) == 0 &&
        verificaSeThreadEstaNaFila(tid,&filaMediaPrio) == 0 &&
        verificaSeThreadEstaNaFila(tid,&filaBaixaPrio) == 0 &&
        verificaSeThreadEstaNaFila(tid,&filaBloqueadas) == 0){
        return -1;
    }

    TCB_join *joinAtual = NULL;

    if(NextFila2(&filaJoin) != -NXTFILA_VAZIA) {
	    if(FirstFila2(&filaJoin) != 0) {
            return -1;
        }

        joinAtual = GetAtIteratorFila2(&filaJoin);
		
        if(joinAtual == NULL) {
            return -1;
        }

        while(joinAtual != NULL){
            if(joinAtual->tid == tid){
            	if(debug) printf("\n[cjoin]: Ja Existia uma thread aguardando o tid %d\n",tid);

                return -2;
            }
            NextFila2(&filaJoin);
            joinAtual = (TCB_join*) GetAtIteratorFila2(&filaJoin);
        }
	}

    threadExecutando->state = PROCST_BLOQ;

    TCB_join *tjoin = (TCB_join*)malloc(sizeof(TCB_join));
    tjoin->tcb = threadExecutando;
    tjoin->tid = tid;

    if((AppendFila2(&filaJoin, tjoin)) != 0){
        return -1;
    }

    if((AppendFila2(&filaBloqueadas, threadExecutando)) != 0){
        return -1;
    }

    if(debug) printf("\n[cjoin]: Join Reaizado Com sucesso\n");

	escalonador();
    return 0;
}

int csem_init(csem_t *sem, int count){
	if(debug) printf("\n[csem_init]: Semaforo Começou a ser inicializado\nn");

    int sucesso = 0;
    if(primeiraExec == -1){
        sucesso = inicializa();
        if(sucesso == -1){
            return -1;
        }
    }

    sem->count = count;
    sem->fila = (PFILA2) malloc(sizeof(PFILA2));

    int sucessoCriarFila = CreateFila2(sem->fila);

    if(sucessoCriarFila == 0){
    	if(debug) printf("\n[csem_init]: Semaforo Inicilizado com sucesso\n");
        return 0;
    } else {
        return -1;
    }
}

int cwait(csem_t *sem){
	if(debug) printf("\n[cwait]:Processo atual soliciou acesso a recurso\n");

    int sucesso = 0;
    if(primeiraExec == -1){
        sucesso = inicializa();
        if(sucesso == -1){
            return -1;
        }
    }

    if(sem->count > 0){
		if(debug) printf("\n[cwait]:Semaforo estava livre\n");

        sem->count -=1;
        return 0;
    }

    sucesso = AppendFila2(sem->fila,(void*)threadExecutando);

    if(sucesso != 0) {
        return -1;
    }

    //Processo agora está em bloqueado
    threadExecutando->state = PROCST_BLOQ;

    sucesso = AppendFila2(&filaBloqueadas,(void*)threadExecutando);

    if(sucesso != 0) {
        return -1;
    }

    sem->count -= 1;
    
	if(debug) printf("\n[cwait]:Processo ficará aguardando o semaforo\n");

    yield = 0;
    swapcontext(&threadExecutando->context,&contextoYield);

    return 0;
}

int csignal(csem_t *sem){
    if(sem == NULL){
        return -1;
    }

    int sucesso = 0;
    if(primeiraExec == -1){
        sucesso = inicializa();
        if(sucesso == -1){
            return -1;
        }
    }

    if(debug) printf("\n[csignal]:Liberado recurso no semaforo");

    TCB_t *threadBloqueada = NULL;

    if(FirstFila2(sem->fila) != 0){
        if(NextFila2(sem->fila) == -NXTFILA_VAZIA){
       	    if(debug) printf("\n[csignal]:Como não havia ninguem esperando, apenas atualiza o count");

            sem->count +=1;
            return 0;
        } else {
            return -1;
        }
    }

    threadBloqueada = GetAtIteratorFila2(sem->fila);

    if(threadBloqueada == NULL){
        return -1;
    }

    while(threadBloqueada != NULL && threadBloqueada->prio != 0){
        NextFila2(sem->fila);
        threadBloqueada = (TCB_t*) GetAtIteratorFila2(sem->fila);
    }

    if(threadBloqueada->prio == 0){
        threadBloqueada-> state = PROCST_APTO;
        AppendFila2(&filaAltaPrio,threadBloqueada);

        DeleteAtIteratorFila2(sem->fila);

        sem->count +=1;

        if(debug) printf("\n[csignal]:Liberada thread bloqueada com prioridade alta\n");

        return 0;
    }

    if(FirstFila2(sem->fila) == 0){
        threadBloqueada = (TCB_t*) GetAtIteratorFila2(sem->fila);
    } else{
        return -1;
    }

    while(threadBloqueada != NULL && threadBloqueada->prio != 1){
        NextFila2(sem->fila);
        threadBloqueada = (TCB_t*) GetAtIteratorFila2(sem->fila);
    }

    if(threadBloqueada->prio == 1){
        threadBloqueada-> state = PROCST_APTO;
        AppendFila2(&filaMediaPrio,threadBloqueada);

        DeleteAtIteratorFila2(sem->fila);

        sem->count +=1;

        if(debug) printf("\n[csignal]:Liberada thread bloqueada com prioridade media\n");

        return 0;
    }

    if(FirstFila2(sem->fila) == 0){
        threadBloqueada = GetAtIteratorFila2(sem->fila);
        AppendFila2(&filaBaixaPrio,threadBloqueada);
        DeleteAtIteratorFila2(sem->fila);
        sem->count+=1;

		if(debug) printf("\n[csignal]:Liberada thread bloqueada com prioridade baixa\n");

        return 0;
    } else{
    	if(debug) printf("\n[csignal]:Nao havia thread com prioridade permitida\n");

        return -1;
    }
}

int cidentify (char *name, int size){
    strncpy (name, "Dieniffer Vargas, 261612 \nHenrique Capelatto, 230188 \nNicolas Cendron, 230281", size);

    if(name == NULL){
        return -1;
    } else {
    	if(debug) printf("\n[cidentify]:Nome printado com sucesso\n");

        return 0;
    }
}

//Funções auxiliares
/*-------------------------------------------------------------------
Função:	inicializa filas e primeiras execuções.
Ret:	Retorna 0 se foi um sucesso ou menos -1 caso contrário
-------------------------------------------------------------------*/
int inicializa(){
	if(debug) printf("\n[inicializa]:Começa a inicializacao\n");

    if( CreateFila2(&todasThreads) != 0 ||
        CreateFila2(&filaAltaPrio)   != 0 ||
        CreateFila2(&filaMediaPrio)  != 0 ||
        CreateFila2(&filaBaixaPrio)  != 0 ||
        CreateFila2(&filaBloqueadas) != 0 ||
        CreateFila2(&filaJoin)  != 0){
        return -1;
    }

    if(debug){
		printf("\n[inicializa]:Filas Criadas com sucesso. Proximo Passo, Criar contexto para Main\n");
		fflush(stdout);
	}

    primeiraExec = 0;

    if(debug){
	    printf("\n[inicializa]:Criada Thread Main e add em baixa prioridade.\n");
	    fflush(stdout);
	}

    mainThread = (TCB_t*)malloc(sizeof(TCB_t));
    getcontext(&mainThread->context);
    mainThread -> state = PROCST_EXEC;
    mainThread -> tid = 0;
    mainThread -> prio = 2;

    AppendFila2(&todasThreads,(void*)mainThread);
    AppendFila2(&filaBaixaPrio,(void*)mainThread);

    threadExecutando = mainThread;

    getcontext(&threadTerminada);
    threadTerminada.uc_link = NULL;
    threadTerminada.uc_stack.ss_sp = (char *)malloc(TAM_PILHA);
    threadTerminada.uc_stack.ss_size = TAM_PILHA;
    makecontext(&threadTerminada, (void(*)(void)) fimDeExecucao, 0);

    if(debug) printf("\n[inicializa]:Criado contexto threadTerminada\n");

    getcontext(&contextoYield);
    contextoYield.uc_link = 0;
    contextoYield.uc_stack.ss_sp = (char *) malloc(TAM_PILHA);
    contextoYield.uc_stack.ss_size = TAM_PILHA;
    makecontext(&contextoYield, (void(*)(void)) escalonador, 0);

    if(debug) printf("\n[inicializa]:Criado contexto yield\n");

    return 0;
}

/*-------------------------------------------------------------------
Função: Coordenação do fim de execuções de uma thread
Ret:	Retorna 0 se foi um sucesso ou menos -1 caso contrário
-------------------------------------------------------------------*/
int fimDeExecucao(){
	yield = 0;
	if(debug) printf("\n[fimDeExecucao]:a thread %d esta encerrando sua execução.\n",threadExecutando->tid);

	int tid = threadExecutando->tid;
	TCB_t* threadLiberada = NULL;
    TCB_join *joinAtual = NULL;

    if(NextFila2(&filaJoin) != -NXTFILA_VAZIA){
		if(FirstFila2(&filaJoin) != 0){
			return -1;
		}

        joinAtual = GetAtIteratorFila2(&filaJoin);
		
        if(joinAtual == NULL){
            return -1; 
        }

        while(joinAtual != NULL){
            if(joinAtual->tid == tid){
                threadLiberada = joinAtual->tcb;
                break;
            }
            NextFila2(&filaJoin);
            joinAtual = (TCB_join*) GetAtIteratorFila2(&filaJoin);
        }
	}

	if(threadLiberada != NULL){
		if(debug) printf("\n[fimDeExecucao]:thread %d será liberada.\n",threadLiberada->tid);

		transicaoBloqParaApto(threadLiberada);
		DeleteAtIteratorFila2(&filaJoin);

		if(debug) printf("\n[fimDeExecucao]:thread foi liberada.\n");
	}

    escalonador();
    return 0;
}

/*-------------------------------------------------------------------
Função: Recebe uma fila e procura se o processo está nessa fila
Ret:	Retorna 0 se foi um sucesso encontrar a thread na fila.
-------------------------------------------------------------------*/
int verificaSeThreadEstaNaFila(int tid, PFILA2 filaEntrada){
    TCB_t *threadAtual = NULL;
    int found = 0;

    FirstFila2(filaEntrada);

    threadAtual = (TCB_t*) GetAtIteratorFila2(filaEntrada);

    while(found == 0 && threadAtual != NULL){
        if(threadAtual->tid == tid){
            found = 1;
            break;
        } else{
            NextFila2(filaEntrada);
            threadAtual = (TCB_t*) GetAtIteratorFila2(filaEntrada);
        }
    }

    return found;
}

/*-------------------------------------------------------------------
Função: Escalonador
Ret:	Retorna 0 se foi um sucesso o processo de escalonamento
-------------------------------------------------------------------*/
int escalonador(){
	if(debug){
		printf("\n[escalonador]:");
		fflush(stdout);
	}

	if(FirstFila2(&filaAltaPrio) == 0){
	    if(debug) printf("\nALTA\n"); 

		if(yield == 1){
            printf("\nTransitando de Exec para\n");
            transicaoExecParaApto();
        }

		threadExecutando = (TCB_t *)GetAtIteratorFila2(&filaAltaPrio);
		DeleteAtIteratorFila2(&filaAltaPrio);
		threadExecutando->state = PROCST_EXEC;

		if(debug) printf("\nthreadExecutando TID: %d\n",threadExecutando->tid);fflush(stdout);

		setcontext(&threadExecutando->context);
        yield = 0;

		return 0;
	} else {
		if(FirstFila2(&filaMediaPrio) == 0){
            if(debug) printf("\nMEDIA\n");

			if(yield == 1){
                transicaoExecParaApto();
            }

			threadExecutando = (TCB_t *)GetAtIteratorFila2(&filaMediaPrio);
			DeleteAtIteratorFila2(&filaMediaPrio);
			threadExecutando->state = PROCST_EXEC;

            if(debug) printf("\nthreadExecutando TID: %d\n",threadExecutando->tid);fflush(stdout);

			setcontext(&threadExecutando->context);
            yield = 0;

			return 0;
		} else { 
			if(FirstFila2(&filaBaixaPrio) == 0){
                if(debug) printf("\nBAIXA\n");

				if(yield == 1){ 
                    transicaoExecParaApto();
                }

				threadExecutando = (TCB_t *)GetAtIteratorFila2(&filaBaixaPrio);
				DeleteAtIteratorFila2(&filaBaixaPrio);
				threadExecutando->state = PROCST_EXEC;

                if(debug) printf("\nthreadExecutando TID: %d\n",threadExecutando->tid);fflush(stdout);

				setcontext(&threadExecutando->context);
                yield = 0;

				return 0;
			} else { 			
				return 0;			
			}	
		}
	}
}

/*-------------------------------------------------------------------
Função: Realiza a transição de uma thread de execução para apto
Ret:	Sem retorno
-------------------------------------------------------------------*/
void transicaoExecParaApto(){
	if(debug){
		printf("\n[transicaoExecParaApto]");
		fflush(stdout);
	}

	threadExecutando->state = PROCST_APTO;	
	int prioridade = threadExecutando->prio;

	switch (prioridade){
		case 0:
			AppendFila2(&filaAltaPrio, threadExecutando);
			break;
		case 1:
			AppendFila2(&filaMediaPrio, threadExecutando);
			break;
		case 2:
			AppendFila2(&filaBaixaPrio, threadExecutando);
			break;
	}
}

/*-------------------------------------------------------------------
Função: Transição de uma thread da fila bloqueado para apto
Ret:	Sem retorno
-------------------------------------------------------------------*/
void transicaoBloqParaApto(TCB_t *threadLiberada){

	if(debug) printf("\n[transicaoBloqParaApto]");

	threadLiberada->state = PROCST_APTO;	
	int prioridade = threadLiberada->prio;

	switch (prioridade){
		case 0:
			AppendFila2(&filaAltaPrio, threadLiberada);
			break;
		case 1:
			AppendFila2(&filaMediaPrio, threadLiberada);
			break;
		case 2:
			AppendFila2(&filaBaixaPrio, threadLiberada);
			break;
	}
}

