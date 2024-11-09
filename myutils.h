#pragma once
#include <stdlib.h> // calloc.
#include <stdio.h>  // perror.
#include <unistd.h> // close.
#include <string.h> // strcat, strcpy.
#include <errno.h>  // errno.


typedef union {
  int fds[2];
  struct {
    int r_end, w_end;
  };
} pipe_t;

#define DUP2_TIMEOUT 20
int _dup2(int a, int b) {
  int k = 0;
  int r;
  while (k++ < DUP2_TIMEOUT &&
         ((r = dup2(a, b)) == -1) && errno == EINTR
        );

  if (r == -1)
    perror("dup2()");
  return r;
}

int _close(int fd) {
  int r = close(fd);
  if (r != 0)
    perror("close()");
  return r;
}

int _exec(char *const _argv[]) {
  char *file = _argv[0];
  execvp(file, _argv);

  char *err_msg = malloc((9 + strlen(file) + 1) * sizeof(char));
  if (!err_msg)
    perror("malloc()");

  sprintf(err_msg, "execvp() %s", file);
  perror(err_msg);
  free(err_msg);
  return 1;
}
