#include <unistd.h>
#include <string.h>

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
  char BUF[32] = "execvp() ";
  strcat(BUF, _argv[0]);
  perror(BUF);
  return 1;
}
