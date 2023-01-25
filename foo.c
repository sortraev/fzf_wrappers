#include <stdlib.h> // malloc, exit
#include <stdio.h>  // printf, fprintf, fgets, fclose, etc.
#include <string.h> // strerror, strcpy, strlen.
#include <errno.h>  // errno.
#include <unistd.h> // fork, pipe, 
#include <sys/wait.h> // wait.

#define BUF_SIZE 128

int main(int argc, char **argv, char **envp) {

  // pipe_t pipe;
  // if (Pipe(&pipe) != 0)
  //   return 1;

  int fds[2];
  pipe(fds);
  pid_t pid = fork();
  if (pid == 0) {
    close(fds[1]);
    // if (Pipe_setup_reading(&pipe) != 0)
      // return 1;

    // if (dup2(fileno(pipe.r_end), STDIN_FILENO) == -1) {
    if (dup2(fds[0], STDIN_FILENO) == -1) {
      fprintf(stderr, "dup2() error during FD rerouting: %s\n",
                       strerror(errno));
      return 1;
    }

    char *bat_args[4] = {"/usr/bin/bat", "-pp", "-l", "c"};

    return execve(bat_args[0], bat_args, envp);
  }
  else if (pid == -1) {
    fprintf(stderr, "fork() error: %s\n", strerror(errno));
    return 1;
  }

  close(fds[0]);


  char *exe_name = argv[0];
  size_t n       = strlen(exe_name);
  char *src_name = malloc(n + 3);
  if (!src_name) {
    fprintf(stderr, "malloc() error: %s", strerror(errno));
    return 1;
  }
  strcpy(src_name, exe_name);
  src_name[n]     = '.';
  src_name[n + 1] = 'c';

  FILE *fp = fopen(src_name, "r");
  if (!fp) {
    fprintf(stderr, "fopen() error during opening of file '%s': %s\n",
                    src_name, strerror(errno));
    free(src_name);
    return 1;
  }

  char buf[BUF_SIZE] = { 0 };
  size_t num_read = 0;
  while ((num_read = fread(buf, sizeof(char), BUF_SIZE, fp)) > 0)
    write(fds[1], buf, num_read);

  close(fds[1]);

  if (!feof(fp))
    fprintf(stderr, "fread() error (reading stopped before EOF): %s\n",
                    strerror(errno));
  if (fclose(fp) != 0)
    fprintf(stderr, "fclose() error: %s", strerror(errno));

  free(src_name);

  int stat_loc;
  wait(&stat_loc);

  int child_normal_exit = WIFEXITED(stat_loc);
  if (!child_normal_exit)
    fprintf(stderr, "bat child did not exit normally: %s\n", strerror(errno));

  return child_normal_exit;
}

