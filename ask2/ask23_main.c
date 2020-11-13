#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"
#include "tree.h"


#define SLEEP_TREE_SEC  5

typedef struct tree_node* Tree;

void fork_newtree(Tree FirstNode) {
        unsigned children_nmbr = FirstNode->nr_children ;
	int status;

    
                pid_t new_pid;
                int i=0;
                for(i=0; i<children_nmbr; i++) { //if it has children then 

                       	new_pid = fork();	// create their processes
                        if (new_pid < 0) {

                                perror("Error with children.");
                                exit(1);
                        }
                        else if ( new_pid == 0 ) {	// child;s code

                                change_pname((FirstNode->children+i)->name);
                                printf("%s: Created.\n", (FirstNode->children+i)->name);
                                fork_newtree(FirstNode->children+i);	// anadormikh klhsh ths fork_newtree

				// afou teleiwsei h fork_newtree tou paidiou
				// tote auto exei dhmiourghsei ola tou ta paidia
				// ara mporei na koimhthei me SIGSTOP
				printf("PID = %ld, name = %s is going to stop now.\n",(long)getpid(), (FirstNode->children+i)->name);
				raise (SIGSTOP);
		
				exit(0);
                        }

			// keep this child's pid to send a SIGCONT to it later
			(FirstNode->children+i)->pid = new_pid ; 
                }

        	// wait for all of its children to get ready
                wait_for_ready_children(children_nmbr);

		// get to sleep
		raise(SIGSTOP);
		
		// now that a SIGCONT has arrived
		printf("PID = %ld, name = %s is now awake... \n" ,(long)getpid(), FirstNode->name );
		
		for ( i=0; i < children_nmbr; i++) { 	//every node sends a "wake up" signal to all of its child nodes with this loop
			
			kill((FirstNode->children+i)->pid , SIGCONT);
			int killedpid = wait(&status);
			explain_wait_status(killedpid , status);
			printf("\n");
		}
	
                printf("%s: Exiting now...\n",(FirstNode->name));
                exit( getpid() );
        }


int main(int argc, char *argv[])
{
        if (argc != 2) {
                fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
                exit(1);
        }

        pid_t  rootpid;

        Tree root = get_tree_from_file(argv[1]);	// diavasma dendrou eisodou apo to orisma
        int status;

        rootpid = fork() ;		// fitaxnoume thn riza
        if (rootpid < 0 ) {
                perror("Error with root.\n");
                exit(1);
        }
        else if (rootpid==0) {		// root's code

                change_pname(root->name);
                printf("%s: Created.\n", root->name);
                fork_newtree(root);	// kaloume thn fork_newtree me orisma thn rixa gia na ftiajei oloklhro to dendro 
                exit(getpid());
        }
	
	// father's code

	// perimenei to 1 tou paidi (root) na etoimastei (na einai se katastash stop)
	wait_for_ready_children(1);
	
	// ektypwnei to dendro twra pou oles oi diergasies koimountai
        show_pstree(rootpid);

	// stelnei SIGCONT sto root	
	kill(rootpid,SIGCONT);

	// perimenei na termatisei to root, ara kai oles oi apo katw diergasies
        rootpid = wait(&status);
        explain_wait_status(rootpid,status);

        return 0 ;
}
