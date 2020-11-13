#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "write_file.h"

int main (int argc, char ** argv) {
  char default_output []  = "fconc.out";
  if ((argc<3) || (argc>4)) {  //only one input file or more than 3 files given...
    fprintf(stderr, "Usage: %s infile1 infile2 [outfile (default:%s)]\n", argv[0], default_output);
    exit(1);
  }
  else {  //legal input...
    if (open(argv[1], O_RDONLY, S_IRUSR)<0) {
      perror(argv[1]);
      exit(1);
    }
    if (open(argv[2], O_RDONLY, S_IRUSR)<0) {
      perror(argv[2]);
      exit(1);    
    }
    char* outfile;
    if (argc==4)
      outfile = argv[3];  //htan fconc.out anti gia defout...
    else 
      outfile = default_output;
    int fd_out = open(outfile, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
    write_file(fd_out, argv[1]);
    write_file(fd_out, argv[2]);
    close(fd_out);
    return 0;  
  }
} 
