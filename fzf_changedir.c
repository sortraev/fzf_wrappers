#include <stdio.h>  // perror.
#include <errno.h>  // errno.
#include <unistd.h> // fork, pipe,

#include "myutils.h"

#ifndef CHANGEDIR_NO_IGNORE
#define CHANGEDIR_NO_IGNORE 0
#endif

int fd_child(int fds[2], int no_ignore);
int fzf_parent(int fds[2], int no_ignore);

int main(int argc, char **argv) {

  // TODO: foolproof, but not very scalable hack to enable no_ignore.
  int no_ignore = argc > 1;

  int fds[2];
  if (pipe(fds) != 0) {
    perror("pipe()");
    return 1;
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork()");
    __close(fds[0]);
    __close(fds[1]);
    return 1;
  }

  else if (pid == 0)
    return fd_child(fds, no_ignore);
  fzf_parent(fds, no_ignore);

  return 0;
}

int fd_child(int fds[2], int no_ignore) {
  __close(fds[0]);
  int w_end = fds[1];

  if (__dup2(w_end, STDOUT_FILENO) == -1) {
    __close(w_end);
    return 1;
  }

  char *_argv[] = {
    "fd",
    "--type=directory",
    "--hidden",
    "--exclude=.git",
    no_ignore ? "--no-ignore" : "--exclude=.*",
    NULL
  };

  execvp(_argv[0], _argv);
  perror("execvp() (fd)");
  __close(w_end);
  return 1;
}

int fzf_parent(int fds[2], int no_ignore) {
  __close(fds[1]);
  int r_end = fds[0];

  if (__dup2(r_end, STDIN_FILENO) == -1) {
    __close(r_end);
    return 1;
  }


  // CHANGEDIR_NO_IGNORE
  char *_argv[] = {
    "fzf",
    "--reverse",
    "--preview",
    no_ignore ?
      "exa --git --tree -L3 -a -D -I=.git --group-directories-first --color=always {}"
      : "exa --git --tree -L3 -a -I=.git --group-directories-first --color=always {}"
    ,
    NULL
  };

  execvp(_argv[0], _argv);
  perror("execvp() (fzf)");

  // this is only reached if exec does not succeed.
  __close(r_end);
  return 0;
}
