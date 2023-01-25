#include <stdlib.h> // malloc, exit
#include <stdio.h>  // printf, fprintf, fgets, fclose, etc.
#include <string.h> // strerror, strcpy, strlen.
#include <errno.h>  // errno.
#include <unistd.h> // fork, pipe, 
#include <sys/wait.h> // wait.

#define BUF_SIZE 256

int __close(int fildes);

int main(int argc, char **argv, char **envp) {

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
    __close(fds[1]); // TODO: error handling.
    if (dup2(fds[0], STDIN_FILENO) == -1) {
      fprintf(stderr, "dup2() error during FD rerouting: %s\n",
                       strerror(errno));
      __close(fds[0]);
      return 1;
    }

    char *bat_args[] = {"/usr/bin/bat", "-pp", "-l", "c", NULL};
    if (execve(bat_args[0], bat_args, envp) == -1) {

      // TODO: error handling (??)
      char *cat_args[] = {"/usr/bin/cat", NULL};
      execve(cat_args[0], cat_args, envp);
    }

    __close(fds[0]);
    return 0;
  }

  /*
   * PARENT.
   */
  __close(fds[0]);

  char *src_name = __FILE__;

  FILE *fp = fopen(src_name, "r");
  if (!fp) {
    fprintf(stderr, "fopen() error during opening of file '%s': %s\n",
                    src_name, strerror(errno));
    return 1;
  }


  char buf[BUF_SIZE] = { 0 };
  size_t num_read = 0;
  while ((num_read = fread(buf, sizeof(char), BUF_SIZE, fp)) > 0)
    write(fds[1], buf, num_read);

  __close(fds[1]);

  if (!feof(fp))
    fprintf(stderr, "fread() error (reading stopped before EOF): %s\n",
                    strerror(errno));
  if (fclose(fp) != 0)
    fprintf(stderr, "fclose() error: %s", strerror(errno));


  /*
   * CHILD EXIT HANDLING.
   */
  int stat_loc;
  pid_t _childpid = waitpid(childpid, &stat_loc, 0);
  if (_childpid == -1)
    fprintf(stderr, "waitpid() error: %s\n", strerror(errno));
  else if (_childpid != childpid) {
    ;
    // TODO: how to handle this??
  }
  else if (!WIFEXITED(stat_loc))
    fprintf(stderr, "Child exited abnormally with exit code %d\n",
                    WEXITSTATUS(stat_loc));


  return 0;
}


int __close(int fildes) {
  int r = close(fildes);
  if (r != 0)
    fprintf(stderr, "close() error: %s\n", strerror(errno));
  return r;
}
