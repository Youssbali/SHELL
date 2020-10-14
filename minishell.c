#include <stdio.h>   //pour les printf
#include <stdlib.h> //pour les constantes EXIT_SUCCESS et EXIT_FAILURE
#include <unistd.h> //pour les appels systemes
#include <sys/wait.h> // pour le wait
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h> //pour le pid_t
#include <string.h>
#include <stdbool.h>
#include "readcmd.h"
#include "chaine.h"

char path[100];
struct cmdline * cmnd;
int i=0;
char cmd[100]; // pour afficher la commande dans list
int pid_f; // pour stop et cont
int pid_fils; // pour le traitant SIGCHLD
pid_t pid; // pour le fork: c'est le processus père originale 
int id_proc = 0; // l'id du processus dans le shell
int etat_fils; // pour le traitant SIGCHLD
Etat etat; //pour màj l'état des procs
char test; //aiguillage
Liste_proc liste;
int cmd_multi = 0; //pour savoir le nbre de commandes composés
pid_t fils; //le petit-fils du processus originale
int c = 0;




void suivi_fils (int sig) {
    int etat_fils, pid_fils;
    do {
        pid_fils = waitpid(-1, &etat_fils, WNOHANG | WUNTRACED | WCONTINUED);
        if ((pid_fils == -1) && (errno != ECHILD)) {
            perror("waitpid");
            exit(EXIT_FAILURE);
 	}else if (pid_fils > 0) {

            if (WIFSTOPPED(etat_fils)) { //terminaison avec stop, signal SIGSTOP,ctrl-z
                /* traiter la suspension */
		maj (pid_fils, liste, SUSPENDU);	

            } else if (WIFCONTINUED(etat_fils)) {
                /* traiter la reprise */	
		etat = ACTIF;
		maj (pid_fils, liste, etat);
		if (test == 'A') {
			pid_fils = waitpid(pid_fils, &etat_fils, 0);
			//Je ne recois pas le signal de fin alors je mets à jour directement ici
			maj (pid_fils, liste, FINI);
			test = 'X';	        
		}
		
            } else if (WIFEXITED(etat_fils)) {
                /* traiter exit */
		pid_f = find_pid(id_proc, liste); //pour voir si le processus existe déjà dans ma liste
    		if(pid_f == pid_fils){
			maj (pid_fils, liste, FINI);
			id_proc--;
		}

            } else if (WIFSIGNALED(etat_fils)) { // le fils s'est terminé à cause d'un signal comme pour SIGINT
                /* traiter signal */
		//inserer_proc(&liste, id_proc, pid, FINI, cmd); ca sert a rien de l'insérer.
            }
        }
    } while (pid_fils > 0);
}

void traitant_STOP (int sig) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTSTP);
    sigset_t oldmask;
    sigprocmask(SIG_BLOCK, &mask, &oldmask); //le signal SIGTSTP est maintenant masqué
    printf("Arrêt du processus.\n");
    kill(pid, SIGSTOP);
    id_proc++;
    strcpy(cmd, cmnd->seq[0][0]);
    inserer_proc(&liste, id_proc, pid, SUSPENDU, cmd);
}

void traitant_INT (int sig) {
    printf("Terminaison du processus.\n");
    kill(pid, SIGKILL);
}


void entre(struct cmdline * cmnd, int in) {
  if (cmnd -> in != NULL) {
	  int e = open(cmnd -> in, O_RDONLY);
	  if (e < 0){
	  	perror("Erreur d'ouverture du fichier");
	  }
	  dup2(e,in); //on lie l'entrée standard au fichier qu'on a ouvert
	  close(e);
  }
}


void sortie(struct cmdline * cmnd, int out) {
  if (cmnd -> out != NULL) {
	  int s = creat(cmnd -> out, S_IRWXU);
	  if (s < 0){
	  	perror("Erreur d'ouverture du fichier");
	  }
	  dup2(s,out); //on lie la sortie standard au fichier qu'on a ouvert
	  close(s);
  }
}

void gest_proc(int n, t_pipe Tabpipe[cmd_multi-1]) {
	for (int h = 0; h< cmd_multi-1; h++){
		if ((h + 1) != n) { //on ferme les entrees et sorties qui ne seront pas utilisées
              		close(Tabpipe[h].desc[0]);
         	} if (h != n) {
              		close(Tabpipe[h].desc[1]);
		}
	}
	if (n==0) { //celui qui s'executera en premier
	    printf("ici1\n");
	    dup2(Tabpipe[n].desc[1], 1);
	    /*entre(cmnd, 0);
	    sortie(cmnd, 1);*/
	    execvp((cmnd->seq)[n][0], (cmnd->seq)[n]);
	}
	else if (n != cmd_multi-1) {
	   
	    for (int k =0; k<2; k++) {
		printf("ici2\n");
		    dup2(Tabpipe[c].desc[k], k);
		    entre(cmnd, 0);
		    sortie(cmnd, 1);
		    c++;
	    }
	    execvp((cmnd->seq)[n][0], (cmnd->seq)[n]);
	} else { //le dernier à s'executer
		printf("ici3\n");
	    dup2(Tabpipe[1].desc[0], 0);
	    entre(cmnd, 0);
	    sortie(cmnd, 1);
	    execvp((cmnd->seq)[n][0], (cmnd->seq)[n]);
	}
}

/*void signalisaton() {
	sigset_t sig_proc;
	struct sigaction action;
	sigemptyset(&sig_proc);
	action.sa_mask = sig_proc;
	action.sa_flags = 0;
	action.sa_handler = suivi_fils;
	struct sigaction action1;
	action1.sa_mask = sig_proc;
	action1.sa_flags = 0;
	action1.sa_handler = traitant_STOP;
	struct sigaction action2;
	action2.sa_mask = sig_proc;
	action2.sa_flags = 0;
	action1.sa_handler = traitant_INT;
	sigaction(SIGTSTP,&action1,0); //pour le ctrl-Z
	sigaction(SIGCHLD,&action,0);  //pour list, bg et fg	
	signal(SIGSTOP,suivi_fils);  //ca sert a rien finalement parce que l'envoi du signal SIGSTOP est déjà interprete dans suivi_fils
	sigaction(SIGINT,&action2,0); //pour le ctrl-c

}*/

int main () {
    //signalisation();
    signal(SIGCHLD, suivi_fils);
    signal(SIGTSTP, traitant_STOP);
    //signal(SIGINT, traitant_INT);
	
    /////// Question1 ////////
    while (1){
	
	strcpy(path, getcwd(NULL, 0)); 
	printf("\n%s ", path);
	
	do {
		do {
			cmnd = readcmd();
		}while (cmnd == NULL);

	}while ((cmnd -> seq)[0] == NULL); // Pour la Q3 il attend la fin de la lecture de la commande

	

  	while(cmnd -> seq[cmd_multi] != NULL) {
    		cmd_multi++;
  	}
	

	if (!strcmp(cmnd->seq[0][0], "exit")) {
		exit(0);	    
        }
    /////// Question4 ////////
	else if (!strcmp(cmnd->seq[0][0], "cd")) {

		if (cmnd->seq[0][1] == NULL) {
			chdir(getenv("HOME"));
		} 
		else {
			chdir(cmnd->seq[0][1]);
		}
	}
    /////// Question6 ////////
	else if (!strcmp(cmnd->seq[0][0], "list")) {
		afficher(liste);

	}else if (!strcmp(cmnd->seq[0][0], "stop")){

		if (cmnd->seq[0][1] != NULL) {
			pid_f = find_pid(atoi(cmnd->seq[0][1]), liste);
			if (pid_f != 404) {
				kill(pid_f,SIGSTOP);
			} else {
				printf("Rentrez un identifiant valable!!\n");
			}
		} else {
			printf("Précisez un identifiant!!\n");
		}
		
		
	}else if (!strcmp(cmnd->seq[0][0], "bg")){

		if (cmnd->seq[0][1] != NULL) {
			pid_f = find_pid(atoi(cmnd->seq[0][1]), liste);
			if (pid_f != 404) {
				kill(pid_f,SIGCONT);
			} else {
				printf("Rentrez un identifiant valable!!\n");
			}
		} else {
			printf("Précisez un identifiant!!\n");
		}

	}else if (!strcmp(cmnd->seq[0][0], "fg")){

		test = 'A';
		if (cmnd->seq[0][1] != NULL) {
			pid_f = find_pid(atoi(cmnd->seq[0][1]), liste);
			if (pid_f != 404) {
				kill(pid_f,SIGCONT);
			} else {
				printf("Rentrez un identifiant valable!!\n");
			}
		} else {
			printf("Précisez un identifiant!!\n");
		}
		
	}else {

		switch (pid = fork()){

			case -1 :  perror("\nErreur fork\n");
				   exit(1); 
		   
			case 0 :   /////////////// Question8 //////////////////

				   if (cmd_multi == 1) {

					entre(cmnd, 0);		
					sortie(cmnd, 1);
					execvp((cmnd->seq)[0][0], (cmnd->seq)[0]);
				   }
				   
				   /////////////// Fin Question8 //////////////////

				   else if (cmd_multi == 2) {

					/////////////// Question9 //////////////////

						int desc[2]; //le tableau de descripteur pour pipe
						int p = pipe(desc);
						if (p!=0) {
							perror("Erreur dans la création du tube");
						}
					   	fils = fork();
						if (fils == -1){
							perror("Erreur fork");
							exit(3);
						}
						if (fils == 0) { //on est dans le petit fils
							close(desc[0]); //on ferme la sortie du fils qui n'est pas utilisé
							dup2(desc[1],1);
							entre(cmnd, 0);
							sortie(cmnd, 1);
							execvp((cmnd->seq)[0][0], (cmnd->seq)[0]);
							close(desc[1]);
						} else { //on est dans le fils père du petit fils
							close(desc[1]); //on ferme l'entrée du père
							dup2(desc[0],0);
							entre(cmnd, 0);
							sortie(cmnd, 1);
							execvp((cmnd->seq)[1][0], (cmnd->seq)[1]);
							close(desc[0]);
						}
				   }
					/////////////// Fin Question9 //////////////////

				   else {

					/////////////// Question10 //////////////////
					int p[2];
					int q[2];
					pipe(p);
						if (!fork()) {
							for (int j = 1; j<cmd_multi; j++) { 
								pipe(q);
								if (!fork()) {	
									dup2(q[0], 0);
									entre(cmnd, 0);
									sortie(cmnd, 1);
									close(p[0]);close(p[1]);
									close(q[0]);close(q[1]);
									execvp((cmnd->seq)[j+1][0], (cmnd->seq)[j+1]);
								}
								
								dup2(p[0], 0);
								dup2(q[1], 1);
								close(p[0]);close(p[1]);
								close(q[0]);close(q[1]);
								execvp((cmnd->seq)[j][0], (cmnd->seq)[j]);
							}
						} else {
							dup2(p[1], 1); 
							close(p[1]);
							close(p[0]); 
							execvp((cmnd->seq)[0][0], (cmnd->seq)[0]);
							
						}
					
				   }
				   break;

			default :  
				   
				   if (cmnd -> backgrounded == NULL) { 					
						
					pause(); //j'attends la terminaison des fils, ca stop le pere jusqu'a ce qu'il reçoit un signal
					
				   }
				   else {
					id_proc ++;
					strcpy(cmd, cmnd->seq[0][0]);					
					strcat(cmd, " &");
					inserer_proc(&liste, id_proc, pid, ACTIF, cmd);
				   }
				   break;
		}
	}
   }
}

