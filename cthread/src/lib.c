
#include <stdio.h>
#include <string.h>
#include "../include/support.h"
#include "../include/cthread.h"
#include "../include/cdata.h"


/*
  int init;
int UltimaTID = 1;
  PFILA2 todasThreads;
  PFILA2 aptosBaixa;
  PFILA2 aptosMedia;
  PFILA2 aptosAlta;
  PFILA2 bloqueadas;
  PFILA2 filaJoin;
  TCB_t* threadExecutando;
  ucontext_t yield;
  ucontext_t threadTerminada;
  int yieldBit;
  int joinBit;
*/



int ccreate (void* (*start)(void*), void *arg, int prio) {
/*
	TCB_t *newThread = NULL;

	newThread = (TCB_t*) malloc(sizeof(TCB_t));
	newThread -> state = PROCST_APTO;
	newThread -> prio = prio;
	newThread -> tid = UltimaTID;








	UltimaTID += 1;*/
	return 1;//newThread->tid;
}

int csetprio(int tid, int prio) {
	return -1;
}

int cyield(void) {
	return -1;
}

int cjoin(int tid) {
	return -1;
}

int csem_init(csem_t *sem, int count) {
	return -1;
}

int cwait(csem_t *sem) {
	return -1;
}

int csignal(csem_t *sem) {
	return -1;
}

int cidentify (char *name, int size) {
	strncpy (name, "Dieniffer Vargas, 261612 \nHenrique Capelatto, 230188 \nNicolas Cendron, 230281", size);

	if(name == NULL)
        return -1;
    else
        return 0;

}


