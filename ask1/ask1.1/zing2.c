#include <stdio.h>
#include <unistd.h>
void zing(void) {
  char* name = getlogin();
  printf ("Hi, %s\n" , name );
}
