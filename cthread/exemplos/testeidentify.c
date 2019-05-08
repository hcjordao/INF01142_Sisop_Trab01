/*
 *	Programa de exemplo de uso da biblioteca cthread
 *
 * 	Sistemas Operacionais I - www.inf.ufrgs.br
 * 
 * Teste da funcao cidentify
 *
 */

#include "../include/support.h"
#include "../include/cthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    char name[100];
    
    cidentify(&name,100);

    puts(name);

return 0;
}
