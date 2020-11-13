#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"


/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */


// to next afora thn oura, deikths sto struct ths epomenhs diergasias pou tha xronodromologhthei

struct process {

	pid_t mypid;
	struct process *next;
	char *name;

};



/*Definition of my list nodes*/

struct process *head=NULL;
struct process *newnode=NULL;
struct process *last=NULL;




// o SIGALRM handler kaleitai gia otan teleiwnei to kvanto xronou, afou tote o xronodromologhths stelnei SIGALRM, ara tote prepei na skotwsoume thn diergasia pou trexei twra
// dhladh thn diergasia pou einai sto head

static void sigalrm_handler(int signum) {
	kill(head->mypid,SIGSTOP);
}


// o SIGCHLD handler kaleitai gia otan fernoume allh diergasia sto head ths ouras, dhl allh diergasia gia ektelesh

static void sigchld_handler(int signum) {

	pid_t p;
	int status;

	while( 1 ) {

		//perimene opoiodhpote paidi
		p=waitpid(-1,&status,WNOHANG|WUNTRACED);

		//error sthn waitpid
		if (p<0){
			perror("waitpid");
			exit(1);
		}

		//no child has changed its state 
		if(p==0) break;		/////////

		//dhladh ean to p>0 pou shmainei oti h waitpid ekane return to pid tou child whose state has chenged
		explain_wait_status(p,status);

		//me vash to status thanatou ths p, pairnoume dyo kyries periptwseis


		if ( WIFEXITED(status) | WIFSIGNALED(status) )  {          //if finished REMOVE it from the list	
			struct process * temp;
			struct process* current;
			temp=head;


			while (temp != NULL) {
				if (temp->mypid==p && temp==head) {              //if temp == head -> got to delete the head

					if (temp->next == NULL){        //an yparxei mono to head sthn oura         MESW AUTOU TOU SHMEIOU THA GINEI H EJODOS APO TO PROGRAMMA
						free(temp);
						printf("I am done\n");   //telos h douleia
						exit(0);
					}

					else{
						head=temp->next;    //an yparxoyn ki alla, head = to pisw tou
						free(temp);
					}
				}


				else if (temp->mypid==p && temp==last) {         //if TEMP == last -> apla to diagrafoume kanontas to NULL kai auto kai to epomeno tou
					current = head;

					while (current->next->next != NULL) {
						current = current->next;
					}

					free( temp );
					last = current;
					last->next = NULL;
				}

				else if (temp->mypid==p) {                        //if temp == random process of the queue

					current = head;

					while (current->next != temp) {
						current = current->next;
					}

					current->next = temp->next;
					temp->next = NULL;
					free (temp);

				}

				else {
					temp = temp -> next;
					continue;
				}

			}
		}

		if ( WIFSTOPPED(status) ) {             // if stopped by the scheduler, SIGALRM -> sigalrm_hanlder() -> SIGSTOP sent to the head 
			// (only the head is being stopped by the scheduler)
			// bring the head->next to the head                                                               // move the head to the last->next
			last->next=head;
			last=head;

			struct process * temp;

			temp=head;                                             
			head=head->next;                    // to head na ginei to epomeno tou

			temp->next=NULL;			// to palio head = temp, na exei next = null

		}

		alarm(SCHED_TQ_SEC);                        //set the alarm-counter to the time quantum
		//printf("%s\n",running->name);
		kill(head->mypid,SIGCONT);                  // so we send a SIGCONT to the head procces, so that it continues from its pause
		// and after the alarm time has passed, is stops again because of the sigalrm_handler(int signum)

	}

}


static void install_signal_handlers(void) {

	sigset_t sigset;
	struct sigaction sa;

	sa.sa_handler = sigchld_handler;
	sa.sa_flags = SA_RESTART;

	// we make the set of the flags that will be in the mask
	// os the sigaction struct

	sigemptyset(&sigset);                   // make it empty        
	sigaddset(&sigset, SIGCHLD);            // add SIGCHLD, SIGALRM
	sigaddset(&sigset, SIGALRM);

	sa.sa_mask = sigset;

	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction: sigchld");
		exit(1);
	}

	sa.sa_handler = sigalrm_handler;
	if (sigaction(SIGALRM, &sa, NULL) < 0) {
		perror("sigaction: sigalrm");
		exit(1);
	}

	/*
	 * Ignore SIGPIPE, so that write()s to pipes
	 * with no reader do not result in us being killed,
	 * and write() returns EPIPE instead.
	 */
	if (signal(SIGPIPE, SIG_IGN) < 0) {
		perror("signal: sigpipe");
		exit(1);
	}
}



int main(int argc, char *argv[]) {

	int nproc;      // number of processes that we will organise

	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */

	char executable[] = "prog";                             // name of the executable which will replace the process
	char *newargv[] = { executable, NULL, NULL, NULL };     // pointer to the table with the executable's arguments (must end with NULL)
	char *newenviron[] = { NULL };                          // pointer to environment variables


	nproc = argc-1;                                         // number of proccesses

	if (nproc == 0) {
		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
		exit(1);
	}

	pid_t mypid;
	int i;

	/*Fork and create the list*/
	for (i=0;i<nproc;i++) {

		mypid = fork();                                                // create the process for the current executable

		if (mypid<0){                                                  // fork error
			printf("Error with forks\n");
		}

		if(mypid==0){                                                   // code of the child process

			raise(SIGSTOP);                                             // pauses, waits for the parent to send the SIGCONT

			printf("I am %s, PID = %ld\n", argv[0], (long)getpid());

			printf("About to replace myself with the executable %s...\n", executable);

			sleep(2);

			execve(executable, newargv, newenviron);                    // replace current procces with the executable using arguments and environment variables
			// 

			/* because execve() only returns on error */
			perror("execve");
			exit(1);

		}

		else{                                                               // parent's code after child's fork


			if (i==0){                                                      // i=0 -> first process was just forked, so it's the head

				head = (struct process *) malloc(sizeof(struct process));    // create space for its struct
				if (head==NULL) printf("Error with malloc\n");

				head->mypid=mypid;                                          // this process is the head and the last process of the list-queue
				head->next=NULL;
				head->name=argv[i+1];					// name of the process = name of the exec

				last=head;

			}

			else{                                                           // no first process was just made by the scheduler process

				newnode=(struct process *) malloc(sizeof(struct process));   // make space for its struct using malloc
				if (newnode==NULL) printf("Error with malloc\n");

				newnode->mypid=mypid;
				newnode->next=NULL;
				newnode->name=argv[i+1];

				last->next=newnode;                 // connect it to the end of the list-queue
				last=newnode;                       // make it the last process of the list-queue

			}	

		}


	}

	// after all the nproc processes have been forked, the father-scheduler waits for them

	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

	/*Set the alarm on*/
	alarm(SCHED_TQ_SEC);

	/*Start the first process*/
	kill(head->mypid,SIGCONT);

	// loop forever  until we exit from inside a signal handler. -> from line 83
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}



