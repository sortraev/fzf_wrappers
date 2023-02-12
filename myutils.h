#include <stdlib.h> // calloc.
#include <stdio.h>  // perror.
#include <unistd.h> // close.
#include <string.h> // strcat, strcpy.
#include <errno.h>  // errno.

#define DUP2_TIMEOUT 20
int __dup2(int a, int b) {
  int k = 0;
  int r;
  while (k++ < DUP2_TIMEOUT &&
         ((r = dup2(a, b)) == -1) && errno == EINTR
        );

  if (r == -1)
    perror("dup2() error");
  return r;
}

int __close(int fd) {
  int r = close(fd);
  if (r != 0)
    perror("close() error");
  return r;
}

int __myexec(char *const _argv[]) {
  execvp(_argv[0], _argv);

  const char *base = "execvp() ";
  char *err_msg = calloc(sizeof(char), strlen(base) + strlen(_argv[0]) + 1);
  strcpy(err_msg, base);
  strcat(err_msg, _argv[0]);
  perror(err_msg);
  free(err_msg);
  return 1;
}
