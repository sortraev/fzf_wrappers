
#include <stdlib.h> // malloc, exit
#include <stdio.h>  // printf, fprintf, fgets, fclose, etc.
#include <string.h> // strerror, strcpy, strlen.
#include <errno.h>  // errno.
#include <unistd.h> // fork, pipe, 
#include <sys/wait.h> // wait.

#define BUF_SIZE 256

int main(int argc, char **argv, char **envp) {

  int fds[2];
  pipe(fds); // TODO: handle pipe() errors.
             //
  pid_t pid = fork();
  if (pid == -1) {
    fprintf(stderr, "fork() error: %s\n", strerror(errno));
    return 1;
  }

  else if (pid == 0) {
    /*
     * CHILD.
     */
    close(fds[1]); // TODO: handle close() errors.

    printf("duppping\n");
    if (dup2(fds[0], STDIN_FILENO) == -1) {
      fprintf(stderr, "dup2() error during FD rerouting: %s\n",
                       strerror(errno));
      return 1;
    }
    printf("successfully duped\n");

    char *bat_args[4] = {"/usr/bin/bat", "-pp", "-l", "c"};
    // TODO: handle execve errors (??)
    execve(bat_args[0], bat_args, envp);
    return  0;
  }

  /*
   * PARENT.
   */

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
  else {
    printf("child exited normally\n");
  }

  return child_normal_exit;
}



/*
 * Pipe stuff.
 */
// int Pipe(pipe_t *_pipe) {
//   int fds[2];
//
//   int pipe_ret = pipe(fds);
//
//   if (pipe_ret != 0) {                   // pipe returns 0 on success
//     fprintf(stderr, "Pipe() error during opening of pipe: %s\n",
//         strerror(errno));
//     return 1;
//   }
//
//   _pipe->r_end = fdopen(fds[0], "r");    // open new, read/write-restricted streams
//   _pipe->w_end = fdopen(fds[1], "w");
//
//   if (!(_pipe->r_end && _pipe->w_end)) { // if either fdopen call failed, exit 0
//     fprintf(stderr, "Pipe() error during opening of FD's for pipe: %s\n",
//                     strerror(errno));
//     return 1;
//   }
//   return 0;
// }
//
// int Pipe_setup_reading(pipe_t *pipe) {
//   int r = fclose(pipe->w_end);
//   if (r != 0) {
//     fprintf(stderr, "Pipe_setup_reading() error: %s\n", strerror(errno));
//     exit(r);
//   }
//   return 0;
// }
//
// int Pipe_setup_writing(pipe_t *pipe) {
//   int r = fclose(pipe->r_end);
//   if (r != 0) {
//     fprintf(stderr, "Pipe_setup_writing() error: %s\n", strerror(errno));
//     exit(r);
//   }
//   return 0;
// }
