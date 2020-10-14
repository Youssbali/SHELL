#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h> //pour le pid_t
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include "chaine.h"
 

void inserer_proc(Liste_proc* liste, int id_proc, int pid, Etat etat, char* cmd){

	//On ajoute le processus à la tête de la liste    
    Liste_proc proc = (Liste_proc)malloc(sizeof(struct fiche_proc));
    proc -> id_proc = id_proc;
    proc -> pid = pid;
    proc -> etat = etat;
    proc -> cmd = cmd;
    proc -> suivant = *liste;
    *liste = proc;
     
}

void afficher(Liste_proc liste){

  
    if (liste == NULL) {
        printf("Pas de processus en attente\n");
    }
    //Parcourir la liste et afficher les différents processus
    else {
	while (liste!=NULL) {

		if(liste -> etat == ACTIF) {
			printf("[%d]  ",liste->id_proc);
			printf("PID=%d   ",liste->pid);
		        printf("ACTIF   ");
			printf("%s\n", liste->cmd);
		}else if(liste -> etat == SUSPENDU){
			printf("[%d]  ",liste->id_proc);
			printf("PID=%d   ",liste->pid);
		        printf("SUSPENDU   ");
			printf("%s\n", liste->cmd);
		}    
		liste = liste -> suivant;    	
	}
   }
}

int maj (int pid, Liste_proc liste, Etat new_etat){

	Liste_proc l = liste;
	while(l != NULL && l-> pid != pid){
	      l=l->suivant;
	}
	if (l == NULL ){
      		return 404;
	}
  	l -> etat = new_etat;
  return 0;
}

int find_pid(int id_proc, Liste_proc liste) {
	
	Liste_proc l = liste;
	while(l != NULL && l->id_proc != id_proc){
		l=l->suivant;
	}
	if (l==NULL) {
		return 404;
	}else{
		return (l->pid);
	}
	
}
 
