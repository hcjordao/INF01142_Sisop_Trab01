#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/support.h"
#include "../include/cthread.h"
#include "../include/cdata.h"

#define TAM_PILHA 4096

//Prototipos

int Inicializa();
int fimDeExecucao();
int escalonador();
int verificaSeThreadEstaNaFila(int, PFILA2);
void transicaoExecParaApto();
void transicaoBloqParaApto();
// Controles

int PrimeiraExecucao = -1;
int UltimaTID = 0;
int yieldBit = 0;
int joinBit = 0;
int yieldFlag = 0;
int debug = 1;
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

//Estrutura do join - thread + tid(thread aguardada)

typedef struct TCB_join
{
    TCB_t *tcb;
    int tid;
}TCB_join;

//Cria uma nova Thread. Não Preemptivo.
int ccreate (void* (*start)(void*), void *arg, int prio)
{

	if(debug)
	{
		printf("\n[ccreate]Inicio Criacao tid: %d com prioidade %d \n",UltimaTID+1,prio);
	}

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


	if(debug)
	{
		printf("\n[ccreate]Criado com sucesso");
	}

    return newThread->tid;
}


//Set Prioridade da Thread em Execução. Não preemptivo.
int csetprio(int tid, int prio)
{

	if(debug)
	{
		printf("\n[csetprio]: Processo %d passará da prioridade %d para %d",threadExecutando->tid,threadExecutando->prio, prio);
	}

    int sucesso = 0;
    if(PrimeiraExecucao == -1)
    {
        sucesso = Inicializa();
        if(sucesso == -1)
            return -1;
    }

    tid = -1;// ESSE TID NAO È USADO NA versão 2019/1

    threadExecutando->prio = prio;

    if(debug)
	{
		printf("\n[csetprio]: Prioridade atualizada com sucesso");
	}

    //Não É Preemptivo
    return 0;
}

//Libera o Controle
int cyield(void)
{
    if(debug)
	{
		printf("\n[cyield]: Processo %d abriu mão do processador",threadExecutando->tid);
	}

    int sucesso = 0;
    if(PrimeiraExecucao == -1)
    {
        sucesso = Inicializa();
        if(sucesso == -1)
            return -1;
    }
    // Troca do Contexto em Execução para o Contexto Yield
    yieldFlag = 1; //Processo é irá liberar CPU pelo yield
    swapcontext(&threadExecutando->context,&contextoYield);

    return 0;
}


//Thread de exec para bloqueado, espera thread = tid

int cjoin(int tid)
{
    if(debug)
	{
		printf("\n[cjoin]: Processo %d quer aguardar o termino do processo %d",threadExecutando->tid,tid);
	}


    int sucesso = 0;

    if(PrimeiraExecucao == -1)
    {
        sucesso = Inicializa();
        if(sucesso == -1)
            return -1;
    }

    //Verifica se eh a thread main Nao pode dar join na main
    if(tid == 0)
    {
        return -1;
    }

    //Tid nao esta nas filas
    if(verificaSeThreadEstaNaFila(tid,&AltaPrio) == 0 &&
            verificaSeThreadEstaNaFila(tid,&MediaPrio) == 0 &&
            verificaSeThreadEstaNaFila(tid,&BaixaPrio) == 0 &&
            verificaSeThreadEstaNaFila(tid,&bloqueadas) == 0)
    {
        return -1;
    }

    //Representa a Thread Bloqueada pelo semaforo que estamos analisando.
    TCB_join *joinAtual = NULL;


    if(NextFila2(&filaJoin) != -NXTFILA_VAZIA) //Fila não vazia
    {
		 if(FirstFila2(&filaJoin) != 0) // Não conseguiupegar o First
			{
				return -1;
			}
		
        //Como sabemos que tem gente esperando. Pega a primeira Thread
        joinAtual = GetAtIteratorFila2(&filaJoin);
		
		
        if(joinAtual == NULL)
        {
            return -1; // Deu pau no iterador
        }
        
		//Tenta Achar o primeiro com prioridade 0 (Alta) que encontrar
        while(joinAtual != NULL)
        {
            if(joinAtual->tid == tid)
            {
            	if(debug)
				{
					printf("\n[cjoin]: Ja Existia uma thread aguardando o tid %d",tid);
				}

                return -2;
            }
            NextFila2(&filaJoin);
            joinAtual = (TCB_join*) GetAtIteratorFila2(&filaJoin);
        }
	}

    //Status da thread para bloqueado
    threadExecutando->state = PROCST_BLOQ;

    //Cria estrutura da fila de join
    TCB_join *tjoin = (TCB_join*)malloc(sizeof(TCB_join));
    tjoin->tcb = threadExecutando;
    tjoin->tid = tid;

    //Insere na fila de join
    if((AppendFila2(&filaJoin, tjoin)) != 0)
    {
        return -1;
    }

    //Insere a thread que chamou na fila bloqueado
    if((AppendFila2(&bloqueadas, threadExecutando)) != 0)
    {
        return -1;
    }

    if(debug)
	{
		printf("\n[cjoin]: Join Reaizado Com sucesso");
	}


	escalonador();
    return 0;
}


//Inicializa um semforo.
int csem_init(csem_t *sem, int count)
{

	if(debug)
	{
		printf("\n[csem_init]: Semaforo Começou a ser inicializado");
	}

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

    	if(debug)
		{
			printf("\n[csem_init]: Semaforo Inicilizado com sucesso");
		}
        return 0;
    }
    else
    {
        return -1;
    }
}

int cwait(csem_t *sem)
{
	if(debug)
	{
		printf("\n[cwait]:Processo atual soliciou acesso a recurso");
	}

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
		if(debug)
		{
			printf("\n[cwait]:Semaforo estava livre");
		}

        sem->count -=1;
        return 0;
    }

    //Adiciona o tid da thread executando na lista aguardando semaforo

    sucesso = AppendFila2(sem->fila,(void*)threadExecutando);// Ou seria threadExecutando->tid

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


	if(debug)
	{
		printf("\n[cwait]:Processo ficará aguardando o semaforo");
	}

    return 0;
}


//Utilizado para avisar o Semaforo que um recurso foi liberado.
int csignal(csem_t *sem)
{
    if(sem == NULL)
    {
        return -1;
    }

    int sucesso = 0;
    if(PrimeiraExecucao == -1)
    {
        sucesso = Inicializa();
        if(sucesso == -1)
            return -1;
    }

    if(debug)
	{
		printf("\n[csignal]:Liberado recurso no semaforo");
	}


    //Representa a Thread Bloqueada pelo semaforo que estamos analisando.
    TCB_t *threadBloqueada = NULL;

    if(FirstFila2(sem->fila) != 0) // Não conseguiupegar o First
    {
        if(NextFila2(sem->fila) == -NXTFILA_VAZIA) // FIla Vazia
        {

       	    if(debug)
			{
				printf("\n[csignal]:Como não havia ninguem esperando, apenas atualiza o count");
			}

            sem->count +=1; //Libera um Recurso
            return 0;
        }
        else  // Não pegou First e Fila Não Vazia --> ERRO
        {
            return -1;
        }
    }

    //Como sabemos que tem gente esperando. Pega a primeira Thread
    threadBloqueada = GetAtIteratorFila2(sem->fila);

    if(threadBloqueada == NULL)
    {
        return -1; // Deu pau no iterador
    }

    //Tenta Achar o primeiro com prioridade 0 (Alta) que encontrar
    while(threadBloqueada != NULL && threadBloqueada->prio != 0)
    {
        NextFila2(sem->fila);
        threadBloqueada = (TCB_t*) GetAtIteratorFila2(sem->fila);
    }

    // Se encontrou uma com prioridade Alta
    if(threadBloqueada->prio == 0)
    {
        //Adiciona a bloqueada na Fila de Aptos
        threadBloqueada-> state = PROCST_APTO;
        AppendFila2(&AltaPrio,threadBloqueada);

        //Remove ela da Lista de Bloqueados pelo semaforo
        DeleteAtIteratorFila2(sem->fila);

        //Libera um Recurso
        sem->count +=1;

        if(debug)
		{
			printf("\n[csignal]:Liberada thread bloqueada com prioridade alta");
		}

        //Não Preempta
        return 0;
    }

    //Se não tinha nenhum com prioridade Alta, Voltamos Para o Inicio da Fila
    if(FirstFila2(sem->fila) == 0)
    {
        threadBloqueada = (TCB_t*) GetAtIteratorFila2(sem->fila);
    }
    else
    {
        return -1;
    }

    //Tenta Achar o primeiro com prioridade 1 (Media) que encontrar
    while(threadBloqueada != NULL && threadBloqueada->prio != 1)
    {
        NextFila2(sem->fila);
        threadBloqueada = (TCB_t*) GetAtIteratorFila2(sem->fila);
    }

    // Se encontrou uma com prioridade Media
    if(threadBloqueada->prio == 1)
    {
        //Adiciona a bloqueada na Fila de Aptos
        threadBloqueada-> state = PROCST_APTO;
        AppendFila2(&MediaPrio,threadBloqueada);

        //Remove ela da Lista de Bloqueados pelo semaforo
        DeleteAtIteratorFila2(sem->fila);

        //Libera um Recurso
        sem->count +=1;

        if(debug)
		{
			printf("\n[csignal]:Liberada thread bloqueada com prioridade media");
		}


        //Não Preempta
        return 0;
    }


    // Se não encontrou nem com prioridade Alta Nem Media. Vamos mandar o primeiro item da Lista.

    if(FirstFila2(sem->fila) == 0)
    {
        threadBloqueada = GetAtIteratorFila2(sem->fila);
        AppendFila2(&BaixaPrio,threadBloqueada);
        DeleteAtIteratorFila2(sem->fila);
        sem->count+=1;

		if(debug)
		{
			printf("\n[csignal]:Liberada thread bloqueada com prioridade baixa");
		}

        return 0;
    }
    else
    {

    	if(debug)
		{
			printf("\n[csignal]:Nao havia thread com prioridade permitida");
		}
        // Se chegou aqui entao algo errado aconteceu.
        //Por logica nunca chegaria aqui a menos que houvesse prioridades erradas.
        return -1;
    }
}

int cidentify (char *name, int size)
{
    strncpy (name, "Dieniffer Vargas, 261612 \nHenrique Capelatto, 230188 \nNicolas Cendron, 230281", size);

    if(name == NULL)
        return -1;
    else
    	if(debug)
		{
			printf("\n[cidentify]:Nome printado com sucesso");
		}
        return 0;

}

//Funções auxiliares

int Inicializa()
{
	if(debug)
	{
		printf("\n[Inicializa]:Começa a Inicializacao");
	}

    // Cria as filas
    if(
        CreateFila2(&AllThreads) != 0 ||
        CreateFila2(&AltaPrio)   != 0 ||
        CreateFila2(&MediaPrio)  != 0 ||
        CreateFila2(&BaixaPrio)  != 0 ||
        CreateFila2(&bloqueadas) != 0 ||
        CreateFila2(&filaJoin)  != 0)
    {
        return -1;
    }

    if(debug)
	{
		printf("\n[Inicializa]:Filas Criadas com sucesso. Proximo Passo, Criar contexto para Main");
	}

    // Cria Contexto para a Main
    getcontext(&mainThread->context);
    mainThread = (TCB_t*)malloc(sizeof(TCB_t));
    mainThread -> state = PROCST_EXEC;
    mainThread -> tid = 0;

    //Coloca a Thread Main na lista de todas as threads
    AppendFila2(&AllThreads,(void*)mainThread);
    //Coloca a Thread Main na lista de Baixa prioridade
    AppendFila2(&BaixaPrio,(void*)mainThread);

    if(debug)
	{
		printf("\n[Inicializa]:Criada Thread Main e add em baixa prioridade.");
	}

    //Define a Main thread como a thread executando no momento
    threadExecutando = mainThread;

    //Contexto de Finalização
    getcontext(&threadTerminada);
    threadTerminada.uc_link = NULL;
    threadTerminada.uc_stack.ss_sp = (char *)malloc(TAM_PILHA);
    threadTerminada.uc_stack.ss_size = TAM_PILHA;
    makecontext(&threadTerminada, (void(*)(void)) fimDeExecucao, 0);

    if(debug)
	{
		printf("\n[Inicializa]:Criado contexto threadTerminada");
	}

    //Contexto de Yield
    getcontext(&contextoYield);
    contextoYield.uc_link = 0;
    contextoYield.uc_stack.ss_sp = (char *) malloc(TAM_PILHA);
    contextoYield.uc_stack.ss_size = TAM_PILHA;
    makecontext(&contextoYield, (void(*)(void)) escalonador, 0);

    if(debug)
	{
		printf("\n[Inicializa]:Criado contexto yield");
	}

    PrimeiraExecucao = 0;
    return 0;
}

int fimDeExecucao()
{
	if(debug)
	{
		printf("\n[fimDeExecucao]:a thread %d esta encerrando sua execução.",threadExecutando->tid);
	}

	int tid = threadExecutando->tid;
	TCB_t* threadLiberada = NULL;
    TCB_join *joinAtual = NULL;

    ////////******* Verifica Se Existe uma Thread esperando por esta que acabou ***********************

    if(NextFila2(&filaJoin) != -NXTFILA_VAZIA) //Fila não vazia
    {
		 if(FirstFila2(&filaJoin) != 0) // Não conseguiu pegar o First
			{
				return -1;
			}
		
        //Como sabemos que a fila nao esta vazia. Pega o primeiro join
        joinAtual = GetAtIteratorFila2(&filaJoin);
		
		
        if(joinAtual == NULL)
        {
            return -1; // Deu pau no iterador
        }
        
        

		//Tenta Achar um join aguardando a tid do processo em execução
        while(joinAtual != NULL)
        {
            if(joinAtual->tid == tid)
            {
                threadLiberada = joinAtual->tcb;
                break;
            }
            NextFila2(&filaJoin);
            joinAtual = (TCB_join*) GetAtIteratorFila2(&filaJoin);
        }
	}


	if(threadLiberada != NULL)
	{
		if(debug)
		{
			printf("\n[fimDeExecucao]:thread %d será liberada.",threadLiberada->tid);
		}

		transicaoBloqParaApto(threadLiberada);
		DeleteAtIteratorFila2(&filaJoin);

		if(debug)
		{
			printf("\n[fimDeExecucao]:thread foi liberada.",threadLiberada->tid);
		}
	}

    free(threadExecutando);
    escalonador();
    return 0;
}

int verificaSeThreadEstaNaFila(int tid, PFILA2 filaEntrada)
{

	if(debug)
	{
		printf("\n[verificaSeThreadEstaNaFila]: Inicio");
	}

    TCB_t *threadAtual = NULL;
    int found = 0;

    FirstFila2(filaEntrada);

    threadAtual = (TCB_t*) GetAtIteratorFila2(filaEntrada);

    while(found == 0 && threadAtual != NULL)
    {
        if(threadAtual->tid == tid)
        {
            found = 1;
            break;

        }
        else
        {
            NextFila2(filaEntrada);
            threadAtual = (TCB_t*) GetAtIteratorFila2(filaEntrada);
        }
    }

    if(debug){

    	if(found == 1){
			printf("\n[verificaSeThreadEstaNaFila]: Encontrou");
    	}else{
    		printf("\n[verificaSeThreadEstaNaFila]: Não Encontrou");
    	}



    }
    return found;

}

//Yield
//Wait
//Join - Bloq ate terminar a thread de espera. Entro no escalonador após.
//Fim de Processo
int escalonador(){

	if(debug)
	{
		printf("\n[escalonador]: Inicio");
	}

	//Em caso de yield apenas? E em caso de join? Precisa de flag de verificação.
	if(FirstFila2(&AltaPrio) == 0){//Achei thread com alta prioridade 
        if(yieldFlag == 1) transicaoExecParaApto(); //Caso seja do tipo yield transfere processo atual pra fila de aptos. Caso venha de outro local a troca de filas ja foi feita, portanto so localizar qual processo por na CPU.
		threadExecutando = (TCB_t *)GetAtIteratorFila2(&AltaPrio);
		DeleteAtIteratorFila2(&AltaPrio);//Deleto da fila de prioridades
		threadExecutando->state = PROCST_EXEC;//Seto pra executando
		setcontext(&threadExecutando->context);//Seto o contexto
        yieldFlag = 0;
		return 0;
	} else {
		if(FirstFila2(&MediaPrio) == 0){//Achei thread com media prioridade 
			if(yieldFlag == 1) transicaoExecParaApto();
			threadExecutando = (TCB_t *)GetAtIteratorFila2(&MediaPrio);
			DeleteAtIteratorFila2(&MediaPrio);
			threadExecutando->state = PROCST_EXEC;
			setcontext(&threadExecutando->context);
            yieldFlag = 0;
			return 0;
		} else { //Nao encontrou nenhuma com media prioridade busca na de baixa
			if(FirstFila2(&BaixaPrio) == 0){ //Achei thread com baixa prioridade 
				if(yieldFlag == 1) transicaoExecParaApto();
				threadExecutando = (TCB_t *)GetAtIteratorFila2(&BaixaPrio);
				DeleteAtIteratorFila2(&BaixaPrio);
				threadExecutando->state = PROCST_EXEC;
				setcontext(&threadExecutando->context);
                yieldFlag = 0;
				return 0;
			} else { //Nao encontrei nenhuma de baixa prioridade...
				return 0;			
			}	
		}
	}
}


//Realiza a transição da thread de execução atual para o estado de sua prioridade especifica.
//Depois ver se causa erro trocar o estado antes do switch.
void transicaoExecParaApto(){

	if(debug)
	{
		printf("\n[transicaoExecParaApto]");
	}

	threadExecutando->state = PROCST_APTO;	
	int prioridade = threadExecutando->prio;
	switch (prioridade){
		case 0:{
			AppendFila2(&AltaPrio, threadExecutando);
			break;
		}
		case 1:{
			AppendFila2(&MediaPrio, threadExecutando);
			break;
		}
		case 2:{
			AppendFila2(&BaixaPrio, threadExecutando);
			break;
		}
	}
}

//Realiza a transição da thread bloqueada para o estado de sua prioridade especifica.
//Depois ver se causa erro trocar o estado antes do switch.
void transicaoBloqParaApto(TCB_t *threadLiberada){

	if(debug)
	{
		printf("\n[transicaoBloqParaApto]");
	}

	threadLiberada->state = PROCST_APTO;	
	int prioridade = threadLiberada->prio;
	switch (prioridade){
		case 0:{
			AppendFila2(&AltaPrio, threadLiberada);
			break;
		}
		case 1:{
			AppendFila2(&MediaPrio, threadLiberada);
			break;
		}
		case 2:{
			AppendFila2(&BaixaPrio, threadLiberada);
			break;
		}
	}
}
