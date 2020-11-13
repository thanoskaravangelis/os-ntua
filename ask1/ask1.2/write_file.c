#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>  
#include <stdio.h>
#include <unistd.h>  
#include <fcntl.h>   

#include "doWrite.h"

void write_file (int fd_out, const char *infile) {
  int fd_in = open (infile, O_RDONLY);
  char buff[4096];
  ssize_t  rcnt;
  for (;;) {	
  rcnt = read (fd_in, buff, sizeof(buff)-1);
  if (rcnt==0)
    return;
  if (rcnt==-1) {
    perror ("reading file error");
    exit (1);
  }                                           
  doWrite(fd_out, buff, rcnt);
  }
  close (fd_in);	
}
