/*
 * mandel.c
 *
 * A program to draw the Mandelbrot Set on a 256-color xterm.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000

/***************************
 * Compile-time parameters *
 ***************************/
pthread_t *thread;

sem_t *sem;

int NTHREADS;

typedef struct thread_info_struct {			// struct with info of each thread

        pthread_t tid; /* POSIX thread id, as returned by the library */
        int thrid; /* Application-defined thread id */
        int N;
}thread_str;


/*
 * Output at the terminal is is x_chars wide by y_chars long
 */
int y_chars = 50;
int x_chars = 90;

/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
 */
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;

/*
 * Every character in the final output is
 * xstep x ystep units wide on the complex plane.
 */
double xstep;
double ystep;

/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
void compute_mandel_line(int line, int color_val[])
{
	/*
	 * x and y traverse the complex plane.
	 */
	double x, y;

	int n;
	int val;

	/* Find out the y value corresponding to this line */
	y = ymax - ystep * line;

	/* and iterate for all points on this line */
	for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {

		/* Compute the point's color value */
		val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
		if (val > 255)
			val = 255;

		/* And store it in the color_val[] array */
		val = xterm_color(val);
		color_val[n] = val;
	}
}

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
	int i;

	char point ='@';
	char newline='\n';

	for (i = 0; i < x_chars; i++) {
		/* Set the current color, then output the point */
		set_xterm_color(fd, color_val[i]);
		if (write(fd, &point, 1) != 1) {
			perror("compute_and_output_mandel_line: write point");
			exit(1);
		}
	}

	/* Now that the line is done, output a newline character */
	if (write(fd, &newline, 1) != 1) {
		perror("compute_and_output_mandel_line: write newline");
		exit(1);
	}
}

void  compute_and_output_mandel_line(int fd, int line, int N)
{
	/*
	 * A temporary array, used to hold color values for the line being drawn
	 */
	int color_val[x_chars];

	compute_mandel_line(line, color_val);	// we don't have to sync the computation of the lines
	sem_wait(&sem[((line)%N)]);		// lock before printint the line
	output_mandel_line(fd, color_val);	// print it
	sem_post(&sem[((line+1)%N)]);		// unlock
}

void usage(char *argv0) //function that prints info if argc!=2
{
	fprintf(stderr, "Usage: %s NTHREADS \n\n"
			"Exactly one argument required:\n"
			" NTHREADS: The number of threads to create.\n",
			argv0);
	exit(1);
}

void *safe_malloc(size_t size)
{
	void *p;
	if ((p = malloc(size)) == NULL) {
		fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n",size);
		exit(1);
	}
	return p;
}

int safe_th(char *s, int *val) //takes a string and separates the int and char parts 
{
	long l;
	char *endp;
	l = strtol(s, &endp, 10);
	if (s != endp && *endp == '\0') {
		*val = l;
		return 0;
	} else
		return -1;
}

void *thread_compute(void *arg)
{
	thread_str *thr = arg;
	int line;
	for (line = thr->thrid; line < y_chars; line+= thr->N) {
		compute_and_output_mandel_line(1, line, thr->N);
	}
	return NULL;
}

void handle_interrupt() //function to handle interrupts with Ctrl-C
{
	reset_xterm_color(1);
	printf("\n");
	exit(1);
}

int main(int argc , char **argv)
{
	int ist_thread,ret;
	
	thread_str *thr;
	
	if(argc!=2)
		usage(argv[0]);
	
	if(safe_th(argv[1] , &NTHREADS) < 0 || NTHREADS <=0) {
		fprintf(stderr, "`%s' is not valid for `NTHREADS'\n", argv[1]);
		exit(1);
	}

        //allocating memory for threads and semaphores
        thr = safe_malloc(NTHREADS * sizeof(*thr));	// make space for the threads' structs
        sem = safe_malloc(NTHREADS * sizeof(*sem));	// make space for the semaphores
	
	//initializing semaphores
        for(ist_thread=0; ist_thread<NTHREADS; ist_thread++){
                //	*sem_address  pshared initial value
		sem_init(&sem[ist_thread], 0, 0);	// we create the semaphores
        }

	xstep = (xmax - xmin) / x_chars;
	ystep = (ymax - ymin) / y_chars;

	/*
	* draw the Mandelbrot Set, one line at a time.
	* Output is sent to file descriptor '1', i.e., standard output.
	*/

	//incrementing the first semaphore, so that the process can begin
	sem_post(&sem[0]);

	for (ist_thread = 0; ist_thread < NTHREADS; ist_thread++) {
		thr[ist_thread].thrid = ist_thread;	// the in-application thread id
		thr[ist_thread].N = NTHREADS;
		/* Spawn new thread(s) */
			//		thread_id                 start_routine	  its arguments			    	
		ret = pthread_create(&thr[ist_thread].tid, NULL, thread_compute, &thr[ist_thread]);
		if (ret) {
			perror( "pthread_create");
			exit(1);
		}
	}
	
	signal(SIGINT, handle_interrupt); //when Ctrl-C is pressed 
	
	//joining threads
	for(ist_thread = 0; ist_thread< NTHREADS; ist_thread++){
		ret = pthread_join(thr[ist_thread].tid,NULL);
		if (ret) {
			perror("pthread_join");
			exit(1);
		}
	}

	//destroying semaphores
	for(ist_thread=0; ist_thread<NTHREADS; ist_thread++){
		sem_destroy(&sem[ist_thread]);
	}

	free(sem);
	free(thr);

	reset_xterm_color(1);
	return 0;
}
