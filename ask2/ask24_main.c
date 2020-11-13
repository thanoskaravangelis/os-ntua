#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>

#include <sys/wait.h>
#include "tree.h"
#include "proc-common.h"

#define SLEEP_PROC_SEC 3


void my_pipe (struct tree_node* current, int fd) {
        
	printf("PID = %ld, name = %s, Starting...\n", (long)getpid(), current->name);
        
	change_pname(current->name);
        
	double val_to_write;
        
	if (current->nr_children!=0) {	// ean den einai termatikos komvos

                pid_t p1, p2;
                int status;
                int pfd0[2];
                double data1;
                printf("%s: Creating pipe...\n", current->name);
                if (pipe(pfd0)<0) {
                        perror("pipe");
                        exit(1);
                }
 
                p1=fork();		// dhmiourgia 1ou paidiou
                if (p1<0) {
                        /*fork failed*/
                        perror("fork");
                        exit(1);
                }
                if (p1==0) {		// 1st child's code
                        printf("%s: Created 1st child process with PID=%ld\n", current->name, (long)getpid());
                        my_pipe(current->children, pfd0[1]);	// kaloume anadromika thn my_pipe gia to 1o paidi
                }


                p2=fork();		// dhmiourgia 2ou paidiou
                if (p2<0) {
                        /* fork failed...*/
                        perror("fork");
                        exit(1);
                }
                if (p2==0) {		// 2nd child's code
                        printf("%s: Created the 2nd child process with PID=%ld\n", current->name, (long)getpid());
                        my_pipe(current->children+1, pfd0[1]);
                }



	       // wait for your 2 children
               // waitpid(p1, &status, 0);
               // explain_wait_status(p1, status);

               // waitpid(p2, &status, 0);
               // explain_wait_status(p2, status);


		p1=wait(&status);
	        explain_wait_status(p1, status);

		p2=wait(&status);
	        explain_wait_status(p2, status);



                if (read (pfd0[0], &data1, sizeof(data1))!=sizeof(data1)) {	// read from the first child
                        perror("Read from pipe the 1st number");
                        exit(1);
                }
                val_to_write=data1;
               
                printf("%s: Read %f from 1st child process\n", current->name, data1);

                if (read (pfd0[0], &data1, sizeof(data1))!=sizeof(data1)) {	// read from the second child
                        perror("Read from pipe the 2nd number");
                        exit(1);
                }
                printf("%s: Read %f from 2nd child process\n", current->name, data1);

                if ( *(current->name)=='+') {					// compute the result acoording to current node's data
                        val_to_write = val_to_write+data1;
                }
                else {
                        val_to_write = val_to_write*data1;
                }


                if (write(fd, &val_to_write, sizeof(val_to_write))!=sizeof(val_to_write)) {	// write the result to the pipe connecting you with your parent node
                        perror("write to pipe");
                        exit(1);
                }
                printf("%s: Wrote %f to the pipe\n", current->name, val_to_write);

               // printf("%s: Sleeping...\n", current->name);
               // sleep(SLEEP_PROC_SEC);
                

		printf("%s: All done, exiting...\n", current->name);
                exit(val_to_write);
        }

        else {		// leaf node, we just have to read a number

                val_to_write=atoi(current->name); 	// read this number
                
		if (write(fd, &val_to_write, sizeof(val_to_write))!=sizeof(val_to_write)) {	// send it to the parent
                        perror(" write to pipe");
                        exit(1);
                }
                printf("Leaf %s: Wrote %f to the pipe\n", current->name, val_to_write);

                printf("%s: Sleeping...\n", current->name);
                sleep(SLEEP_PROC_SEC);
                printf("%s: Exiting...\n", current->name);
                exit(val_to_write);
        }
}

int main (int argc, char* argv[]) {

        pid_t pid;
        int status;

        int pfd_main[2];	// dyo akra tou pipe

        double output;
        struct tree_node* root;

        if (argc != 2) {
                fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
        }

        printf("Parent: Creating pipe...\n");	// ftiaxnoume to pipe
        if (pipe(pfd_main) <0) {
                perror("pipe");
                exit(1);
        }

        root=get_tree_from_file(argv[1]);	// diavasma tou dendrou eisodou apo to orisma

        printf("Parent: Creating child process...\n");
        pid=fork();
        if (pid<0) {
                perror("main: fork");
                exit(1);
        }
        if (pid==0) {		// root's code
                printf("Parent created the root with PID=%ld\n", (long)getpid());
     
                printf("Parent: Creating pipe...\n");
                my_pipe(root, pfd_main[1]);	
                exit(1);
        }

        if (read(pfd_main[0], &output, sizeof(output))!=sizeof(output)) {	// when the result is ready
                perror("parent: Read from pipe");
                exit(1);
        }
        printf("The final result is: %f\n", output);

	// wait for the root to terminate
        pid=wait(&status);
        explain_wait_status(pid, status);
       
	 printf("Parent: All done, exiting\n");
        return 0;

}

