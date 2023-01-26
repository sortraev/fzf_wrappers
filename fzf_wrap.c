#include <stdlib.h> // malloc, exit
#include <stdio.h>  // printf, fprintf, fgets, fclose, etc.
#include <string.h> // strerror, strcpy, strlen.
#include <errno.h>  // errno.
#include <unistd.h> // fork, pipe,
#include <sys/wait.h>


#ifndef CHANGEDIR_NO_IGNORE
#define CHANGEDIR_NO_IGNORE 0
#endif

int __close(int fildes);
int child_f(int fds[2]);
int parent_f(int fds[2]);

int main(int argc, char **argv) {
  (void) argc;
  (void) argv;

  int fds[2];
  if (pipe(fds) != 0) {
    fprintf(stderr, "pipe() error: %s\n", strerror(errno));
    return 1;
  }

  pid_t childpid = fork();
  if (childpid == -1) {
    fprintf(stderr, "fork() error: %s\n", strerror(errno));
    __close(fds[0]);
    __close(fds[1]);
    return 1;
  }

  else if (childpid == 0) {
    /*
     * CHILD.
     */
    (void) child_f(fds);
    return 0;
  }

  /*
   * PARENT.
   */
  parent_f(fds);

  /*
   * CHILD EXIT HANDLING.
   * TODO: if execvp succeeded, then this code will never be reached. in this
   * case, how is the child handled? I'm guessing waiting is only necessary when
   * we are interested in/dependent on the child's result somehow.
   *
   * for now, let's just leave it here.
   */
  int stat_loc;
  pid_t _childpid = waitpid(childpid, &stat_loc, 0);
  if (_childpid == -1)
    fprintf(stderr, "waitpid() error: %s\n", strerror(errno));
  else if (_childpid != childpid) {
    ;
    // TODO: how to handle this?? probably not even necessary.
  }
  else if (!WIFEXITED(stat_loc))
    fprintf(stderr, "Child exited abnormally with exit code %d\n",
                    WEXITSTATUS(stat_loc));

  return 0;
}


int child_f(int fds[2]) {
  __close(fds[0]);
  int w_end = fds[1];

  if (dup2(w_end, STDOUT_FILENO) == -1) {
    fprintf(stderr, "dup2() error during stdout rerouting: %s\n",
                     strerror(errno));
    __close(w_end);
    return 1;
  }

  char *_argv[] = {
    "/usr/bin/fd",
    "--type=directory",
    "--hidden",
    "--exclude=.git",
#if CHANGEDIR_NO_IGNORE
    "--no-ignore",
#else
    "--exclude=.*",
#endif
    NULL};

  if (execvp(_argv[0], _argv) == -1) {
    // TODO: how to handle exec() errors??
    fprintf(stderr, "execvp() error: %s\n", strerror(errno));
  }
  // this is only reached if exec does not succeed.
  __close(w_end);
  return 1;
}

int parent_f(int fds[2]) {
  __close(fds[1]);
  int r_end = fds[0];

  if (dup2(r_end, STDIN_FILENO) == -1) {
    fprintf(stderr, "dup2() error during stdin rerouting: %s\n",
                     strerror(errno));
    __close(r_end);
    return 1;
  }


  // CHANGEDIR_NO_IGNORE
  char *_argv[] = {
    "/usr/bin/fzf",
    "--reverse",
    "--preview", 
#if CHANGEDIR_NO_IGNORE
    "exa --git --tree -L3 -a -D -I=.git --group-directories-first --color=always {}",
#else
    "exa --git --tree -L3 -a -I=.git --group-directories-first --color=always {}",
#endif
  };

  if (execvp(_argv[0], _argv) == -1) {
    // TODO: how to handle exec() errors??
    fprintf(stderr, "execvp() error: %s\n", strerror(errno));
  }
  // this is only reached if exec does not succeed.
  __close(r_end);
  return 1;
}


int __close(int fildes) {
  int r = close(fildes);
  if (r != 0)
    fprintf(stderr, "close() error: %s\n", strerror(errno));
  return r;
}
