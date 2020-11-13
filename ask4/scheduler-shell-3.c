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
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */


struct process {
	pid_t mypid;
	struct process *next;
	char *name;
    	int serial_id;
	int priority;
};

struct process *head = NULL;
struct process *last = NULL;
struct process *running = NULL;
struct process *newnode = NULL;

int nproc = 0;
int serial_id_counter = 0;



/* Print a list of all tasks currently being scheduled.  */

static void sched_print_tasks(void) {
    struct process *temp;
    temp = head;
    running = head;
    while (temp != NULL) {
        printf("Serial_id: %d, PID: %d, Name: %s, Priority : %d",
                 temp->serial_id, temp->mypid, temp->name,temp->priority);
        if (temp->serial_id == running->serial_id) {
            printf(", I am the running process\n");
        }
        else {
            printf("\n");
        }
        temp = temp->next;
    }

}


/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */
static int sched_kill_task_by_id(int id) {

        struct process *temp;
        temp = head;

        while (temp != NULL) {
            if (temp->serial_id == id) {
                kill(temp->mypid, SIGKILL);
                return 0;
            }
            else {
                temp = temp->next;
            } 
        }

        return -ENOSYS;
}

/*fix the priorities*/
static int sched_set_high_p(int id){
	struct process * temp = head;
	//struct process * prev = NULL;
	//set the priority of a process to high and put it in the head of my list
        while (temp!=NULL){
                if (temp->serial_id == id ){
                        temp->priority=1;
			return 0;
                }
                else
                        if(temp->next!=NULL){
                                temp=temp->next;
                        }
			else{
                                break;
			}
        }
        return -ENOSYS;
}

static int sched_set_low_p(int id){
 	struct process * temp=head;
        while (temp!=NULL){
                if (temp->serial_id == id ){
                       temp->priority=0;
                       return 0;
                }
                else
                        if(temp->next!=NULL)
                                temp=temp->next;
                        else
                                break;
        }
        return -ENOSYS;

}


/* Create a new task.  */
static void sched_create_task(char *executable) { 

    pid_t my_pid;
    
    char *newargv[] = {executable, NULL, NULL, NULL};
    char *newenviron[] = {NULL};
    
    my_pid = fork();

    if (my_pid < 0) {
        printf("Error with forks\n");
    }
    if (my_pid == 0) {      // child's code
	raise(SIGSTOP);
        printf("I am %s, PID = %ld\n", executable, (long)getpid());
        sleep(2);
        execve(executable, newargv, newenviron);

        perror("execve");   // because it only returns on error
        exit(1);
    }
    else {      // scheduler's code

        newnode = (struct process *)malloc(sizeof(struct process));    // create space for its struct
	if (newnode==NULL) printf("Error with malloc\n");
		
	printf("-->Process with PID = %ld was just created.\n", (long)my_pid);

        newnode->mypid = my_pid;
        
        newnode->name=(char*)malloc(strlen(executable)+1);
		strcpy(newnode->name,executable);

        newnode->next = NULL;
        
	newnode->priority=0;

        newnode->serial_id = serial_id_counter;
        serial_id_counter = serial_id_counter+1;
	
        last->next = newnode;
        last = newnode;

        nproc = nproc+1;

    }

}


/* Process requests by the shell.  */
static int process_request(struct request_struct *rq) {

        switch (rq->request_no) {
                case REQ_PRINT_TASKS:
                        sched_print_tasks();
                        return 0;

                case REQ_KILL_TASK:
                        return sched_kill_task_by_id(rq->task_arg);

                case REQ_EXEC_TASK:
                        sched_create_task(rq->exec_task_arg);
                        return 0;
		
		case REQ_HIGH_TASK:
			sched_set_high_p(rq->task_arg);
			return 0;

		case REQ_LOW_TASK:
			sched_set_low_p(rq->task_arg);
			return 0;

                default:
                        return -ENOSYS;
        }
}


/*
 * SIGALRM handler
 */
static void sigalrm_handler(int signum) {
        kill(head->mypid, SIGSTOP);
}


/*
 * SIGCHLD handler
 */
static void sigchld_handler(int signum) {

        pid_t p;
	int status;
	int flag = 0;
	while( 1 ) {
		
        //perimene opoiodhpote paidi
        p=waitpid(-1,&status,WNOHANG|WUNTRACED);
		
        //error sthn waitpid
        if (p<0) {
		perror("waitpid");
		exit(1);
	}

		//no child has changed its state 
        if(p==0) break;		/////////
		
        //dhladh ean to p>0 pou shmainei oti h waitpid ekane return to pid tou child whose state has chenged
		explain_wait_status(p,status);
        
        //me vash to status thanatou ths p, pairnoume dyo kyries periptwseis
		if ( WIFEXITED(status) || WIFSIGNALED(status) )  {          //if finished REMOVE it from the list	
			struct process* temp;
			struct process* current;
			temp=head;
			
			while (temp != NULL) {

				if (temp->mypid==p && temp==head) {              //if temp == head -> got to delete the head
                                                if (temp->next == NULL){        //an yparxei mono to head sthn oura         MESW AUTOU TOU SHMEIOU THA GINEI H EJODOS APO TO PROGRAMMA
							 free(temp);
							 printf("I am done\n");   //telos h douleia
							 //exit(0);
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

				else {                                      // iterate till you find the temp that matches
					temp = temp -> next;
					continue;
				}
				break;
			}
			temp=head;
			struct process *current1 = head;
			//struct process * prev = NULL;
                        //search for high priorities
                        while (temp!=NULL){
                        	if (temp->priority!=1){ //move this node to the tail
                                	last->next=head;
                                        last=head;
                                        head=head->next;
                                        last->next=NULL;
                                        temp=head;
                                        if (temp == current1) 
						break;
                                        continue;
                             	}
                                else{
                            	      flag=1;
                                      break;
                              	}
			}

		}

		if ( WIFSTOPPED(status) ) {             // if stopped by the scheduler, SIGALRM -> sigalrm_hanlder() -> SIGSTOP sent to the head 
                                                // (only the head is being stopped by the scheduler)
// bring the head->next to the head                                                            // move the head to the last->next

			if(head==NULL) {
				printf("Empty List\n");
				exit(0);
			}	
			if(head->next != NULL) {
				
            			last->next=head;
				last=head;
				head = head->next;	//TO HEAD NA GINEI TO EPOMENO TOU
				last->next = NULL;
				struct process * current1;
				struct process * temp;
				
				current1 = head;
            			temp=head;                                             
       				//SEARCH FOR PRIORITY = HIGH
				while (temp!=NULL){
					if (temp->priority!=1){ //move this node to the end
						last->next=head;
						last=head;
						head=head->next;
						last->next=NULL;
						temp=head;
						if (temp == current1) 
							break;
						continue;
					}
					else{
						flag=1;
						break;
					}	
				}
			}
        	}

		  
        if ( WIFSTOPPED(status) ) {         // take care of the alarm time

            if (head->serial_id == 0 && (head->priority == 1 || flag == 0)) {
                printf("I am the shell: You have ten seconds to give another instruction\nShell> ");
                alarm(5*SCHED_TQ_SEC);
                kill(head->mypid,SIGCONT); 
            }

            else {
                alarm(SCHED_TQ_SEC);        // set the alarm-counter to the time quantum
                kill(head->mypid,SIGCONT);  // so we send a SIGCONT to the head procces, so that it continues from its pause
                                            // and after the alarm time has passed, is stops again because of the sigalrm_handler(int signum)
            } 
          
        }

        else {
            alarm(SCHED_TQ_SEC);            // set the alarm-counter to the time quantum
            kill(head->mypid,SIGCONT);      // so we send a SIGCONT to the head procces, so that it continues from its pause
                                            // and after the alarm time has passed, is stops again because of the sigalrm_handler(int signum)      
        }
                              
	}

}


/* Disable delivery of SIGALRM and SIGCHLD. */
static void signals_disable(void) {

        sigset_t sigset;

        sigemptyset(&sigset);
        sigaddset(&sigset, SIGALRM);
        sigaddset(&sigset, SIGCHLD);
        
        //              how=union  the set     
        if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
                perror("signals_disable: sigprocmask");
                exit(1);
        }
}


/* Enable delivery of SIGALRM and SIGCHLD.  */
static void signals_enable(void) {

        sigset_t sigset;

        sigemptyset(&sigset);
        sigaddset(&sigset, SIGALRM);
        sigaddset(&sigset, SIGCHLD);
        //              how=remove   the set
        if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
                perror("signals_enable: sigprocmask");
                exit(1);
        }
}


/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void install_signal_handlers(void) {

        sigset_t sigset;
        struct sigaction sa;

        sa.sa_handler = sigchld_handler;
        sa.sa_flags = SA_RESTART;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGCHLD);
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



static void do_shell(char *executable, int wfd, int rfd) {

        char arg1[10], arg2[10];
        char *newargv[] = { executable, NULL, NULL, NULL };
        char *newenviron[] = { NULL };

        // write to arg1,2 with format="%05d" from wfd, rfd
        sprintf(arg1, "%05d", wfd);
        sprintf(arg2, "%05d", rfd);
        newargv[1] = arg1;
        newargv[2] = arg2;

        raise(SIGSTOP);     // wait for the parent to send SIGCONT
        execve(executable, newargv, newenviron);    //execute the executable with arguments from array pointed to by newargv

        /* execve() only returns on error */
        perror("scheduler: child: execve");
        exit(1);
}


/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
static void sched_create_shell(char *executable, int *request_fd, int *return_fd) {

        pid_t p;
        int pfds_rq[2], pfds_ret[2];        // 2 pipes with these fds

        
        if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
                perror("pipe");
                exit(1);
        }

        p = fork();
        if (p < 0) {
                perror("scheduler: fork");
                exit(1);
        }

        if (p == 0) {
                /* Child */
                close(pfds_rq[0]);
                close(pfds_ret[1]);

                do_shell(executable, pfds_rq[1], pfds_ret[0]);
                assert(0);
        }
        /* Parent */
        close(pfds_rq[1]);
        close(pfds_ret[0]);
        *request_fd = pfds_rq[0];
        *return_fd = pfds_ret[1];

        // head <- shell
        // last <- head

        head->serial_id = 0;
        head->mypid = p;    
        head->name = "shell";
        head->next = NULL;
	head->priority = 0;



}


static void shell_request_loop(int request_fd, int return_fd) {

        int ret;
        struct request_struct rq;

        /*
         * Keep receiving requests from the shell.
         */
        for (;;) {

               //          from      to    size
                if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
                        perror("scheduler: read from shell");
                        fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
                        break;
                }

                signals_disable();
                ret = process_request(&rq);     // apokwdikopoiei me vash to rq->request_no kai kalei thn antistoixh func apo tis 
                signals_enable();               // sched_print_tasks, sched_kill_task_by_id, sched_create_task


                //           to      from   size
                if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
                        perror("scheduler: write to shell");
                        fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
                        break;
                }
        }
}



int main(int argc, char *argv[]) {

        int nproc;

        /* Two file descriptors for communication with the shell */
        static int request_fd, return_fd;

        head=(struct process *)malloc(sizeof(struct process));
        last = head;

        /* Create the shell. */
        sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);
        /* TODO: add the shell to the scheduler's tasks */

        /*
         * For each of argv[1] to argv[argc - 1],
         * create a new child process, add it to the process list.
         */

        nproc = argc-1; /* number of proccesses goes here */

        if (nproc==0) {
                fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
                exit(1);
        }

        char executable[] = "prog";
        char *newargv[] = {executable, NULL, NULL, NULL};
        char *newenviron[] = {NULL};

        pid_t my_pid;

        int i;
        for (i=0; i<nproc; i++) {
                
                my_pid = fork();
                
                if (my_pid<0) {
                        printf("Error with fork\n");
                }

                if (my_pid == 0) {              // child's code
                        
                        raise(SIGSTOP);         // and wait for scheduler's SIGCONT
                        printf("I am %s, PID = %ld\n", argv[0], (long)getpid());
                        printf("About to replace myself with the executable %s...\n", executable);
                        sleep(2);

                        execve (executable, newargv, newenviron);
                        //because execve only returns on error
                        perror("execve");
                        exit(1);

                }

                else {                          // father's code
                                                // the first time that this part is executed, the list has only the shell on the head
                        
                        newnode = (struct process*) malloc(sizeof(struct process));
                        
                        if (newnode == NULL) {
                                printf("Error with malloc\n");
                        }

                        newnode->mypid = my_pid;  
                        newnode->name = argv[i+1]; // i executable's name
                        newnode->next = NULL;
                        newnode->serial_id = i+1;
			newnode->priority = 0;

                        last->next = newnode;
                        last = newnode;
                       
                }


        }

       
        /* Wait for all children to raise SIGSTOP before exec()ing. */
        wait_for_ready_children(nproc);

        /* Install SIGALRM and SIGCHLD handlers. */
        install_signal_handlers();


        alarm(SCHED_TQ_SEC);    // begin the alarm countdown

        kill(head->mypid, SIGCONT);     //continue the head process (shell)

        shell_request_loop(request_fd, return_fd);

        /* Now that the shell is gone, just loop forever
         * until we exit from inside a signal handler.
         */
        while (pause())
                ;

        /* Unreachable */
        fprintf(stderr, "Internal error: Reached unreachable point\n");
        return 1;
}

