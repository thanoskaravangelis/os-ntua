#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"
#include "tree.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  5

typedef struct tree_node* Tree;


void fork_newtree(Tree FirstNode) {
	unsigned children_nmbr = FirstNode->nr_children ;	// arithmos paidiwn tou komvou
	
	if (children_nmbr != 0) {		// an den einai fyllo

		pid_t new_pid;
		int i = 0 ;
		int status ; 
		for(i=0; i<children_nmbr; i++) {	// gia ola ta paidia pou prepei na apokthsei
			new_pid = fork();		// kane fork
	
			if (new_pid < 0) {
				perror("Error with children.");
				exit(1);
			}
	
			else if ( new_pid == 0 ) {	// this child's code
				change_pname((FirstNode->children+i)->name);
				printf("%s: Created.\n", (FirstNode->children[i]).name );
				fork_newtree(FirstNode->children+i);	// andromikh klhsh ths synarthshs gia to kathe paidi
			}
		}
		for (i=0; i<children_nmbr; i++) {	// o kathe komvos perimenei na termatisoun ola tou ta paidia
                        new_pid = wait(&status);
                	explain_wait_status(new_pid, status);
                }
		printf("%s: Exiting now...\n",FirstNode->name);		// kai meta kanei exit
		exit( getpid() );
		
	}
	else {		// einai fyllo 
		printf("%s: No children found...going to sleep.\n",(FirstNode->name));
		sleep(SLEEP_PROC_SEC);								// opws ola ta fylla koimatai prin termatisei
		printf("%s: Exiting now...\n",(FirstNode->name));
		exit( getpid() );
	}
}


int main(int argc, char *argv[])
{
        if (argc != 2) {
                fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
                exit(1);
        }
	
	pid_t  rootpid;

        Tree root = get_tree_from_file(argv[1]);	// diavasma dendrou eisodou apo to arxeio sto orisma
        int status;

	rootpid = fork() ; 				// ftiaxnoume to dendro diergasiwn
	if (rootpid < 0 ) {
		perror("Error with root.\n");
		exit(1);
	}
	else if (rootpid==0) {			// root's code
		change_pname(root->name);
		printf("%s: Created.\n", root->name);
		fork_newtree(root);		// kaloume thn fork_newtree
		exit(getpid());
	}
	
	// perimenoume merika seconds me thn elpida to dendro na einai etoimo gia ektypwsh otan auta perasoun
	sleep(SLEEP_TREE_SEC);
	
	// ektypwnoyme to dendro
	show_pstree(rootpid);

	// perimenoume na termatisei h riza
	rootpid = wait(&status);
	explain_wait_status(rootpid,status);

	return 0 ;
}
