#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

/*
 * Create this process tree:
 * A-+-B---D
 *   `-C
 */
void fork_procs(void) {
        int status;
        pid_t B, C, D;
	//long int A2;
        /* Root A */
        change_pname("A");
        printf("Parent created root (A) with PID = %ld , waiting for it to terminate...\n", (long)getpid());
       // printf("A: Sleeping...\n");
       // sleep(SLEEP_PROC_SEC);
	//A2 = (long)getpid();
	//printf("-----------A2= %ld", A2);
        fprintf(stderr, "Parent (A), PID = %ld: Trying to creat child (B)...\n", (long)getpid());
        B = fork();
        if (B<0) {
                perror("fork");
                exit(1);
        }
        if (B==0) {
                /* Child B */
                change_pname("B");
                printf("Parent (A), created child (B) with PID = %ld, waiting for it to terminate...\n", (long)B);
               // printf("B: Sleeping...\n");
               // sleep(SLEEP_PROC_SEC);

                fprintf(stderr, "Parent (B), PID = %ld: Trying to creat child (D)...\n", (long)getpid());
                D = fork();
                if (D<0) {
                    perror("fork");
                    exit(1);
                }
                if (D==0) {
                    /*Child D*/
                    change_pname("D");
                    printf("Parent (B), created child (D) with PID = %ld, waiting for it to terminate...\n", (long)D);
                    printf("D: Sleeping...\n");
                    sleep(SLEEP_PROC_SEC);
                    printf("D: Exiting...\n");
                    exit(13);
                }
                /* back to root B */
               // printf("B: Sleeping...\n");
               // sleep(SLEEP_PROC_SEC);
                D = wait(&status);
                explain_wait_status(D, status);
                printf("Parent B: All done, exiting...\n");
                exit(19);
        }
        /* back to root A*/
        fprintf(stderr, "Parent (A), PID = %ld: Trying to creat child (C)...\n", (long)getpid());
        C = fork();
        if (C<0) {
            perror("fork");
            exit(1);
        }
        if (C==0) {
        /* child C */
        change_pname("C");
        printf("Parent (A), created child (C) with PID = %ld, waiting for it to terminate...\n", (long)C);
        printf("C: Sleeping...\n");
        sleep(SLEEP_PROC_SEC);
        printf("C: Exiting...\n");
        exit(17);
        }
        /* back to root A */

        /* Wait for the root of the process tree to terminate */
        C = wait(&status);
        explain_wait_status(C, status);
        B = wait(&status);
        explain_wait_status(B, status);
        printf("A: Exiting...\n");
        exit(16);
}

/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 * How to wait for the process tree to be ready?
 * In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.
 * In ask2-signals:
 *      use wait_for_ready_children() to wait until
 *      the first process raises SIGSTOP.
 */
int main(void) {
        pid_t A;
        int status;
        fprintf(stderr, "Parent, PID = %ld: Trying to creating root (A)...\n", (long)getpid());
        /* Fork root of process tree */
        A = fork();
        if (A < 0) {
                perror("main: fork");
                exit(1);
        }
        if (A == 0) {

                 /* trying to create the other children with function fork_procs */

                fork_procs();
               // exit(1);
        }

        /*
         * Father
         */


        sleep(SLEEP_TREE_SEC);

        /* Print the process tree root at pid */
        show_pstree(getpid());
        /* Wait for the root of the process tree to terminate */

        A = wait(&status);
        explain_wait_status(A, status);

        return 0;
}

