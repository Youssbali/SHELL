typedef enum {FINI,ACTIF,SUSPENDU} Etat;
typedef struct fiche_proc *Liste_proc; //La liste de processus pointe sur une variable de type fiche_proc
struct fiche_proc
{
    int id_proc;
    int pid;
    Etat etat;
    char* cmd;
    Liste_proc suivant;  
};

typedef struct  //le tableau de pipe pour la question 10 que j'ai finalement pas utilisé
{ 
  int desc[2];
} t_pipe ;


void inserer_proc(Liste_proc* liste, int id_proc, int pid, Etat etat, char* cmd);
 
void afficher(Liste_proc liste);

int maj (int pid, Liste_proc liste, Etat new_etat);

int find_pid(int id_proc, Liste_proc liste);
 
int detruire_liste_proc(int pid, Liste_proc* liste);

