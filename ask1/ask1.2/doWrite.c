#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void doWrite( int fd, const char *buff, int len) {
  ssize_t wcnt;
  size_t idx;
  idx = 0;
  do {
    wcnt = write(fd, buff+idx, len - idx);
    if ( wcnt == -1){
      perror("write");
      exit(1);
    }
    idx += wcnt;
  } while (idx < len);
}

