#include <stdlib.h> // malloc, exit
#include <stdio.h>  // printf, fprintf, fgets, fclose, etc.
#include <string.h> // strerror, strcpy, strlen.
#include <errno.h>  // errno.
#include <unistd.h> // fork, pipe,
#include <sys/wait.h>

/*
 * unfinished ideas:
 *   main: open rg_pipe;
 *         fork() rg_child;
 *         close rg_pipe.write;
 *         open fzf_pipe;
 *         fork() fzf_child;
 *         close fzf_pipe.read;
 *         exec fzf.
 *   child1:
 *
 *   main:   read from child1; write to child2; exec fzf.
 *   child1: close reading; write to main; exec rg.
 */

#ifndef CHANGEDIR_NO_IGNORE
#define CHANGEDIR_NO_IGNORE 0
#endif

int __close(int fildes);
int rg_child(int fds[2]);
int cut_child(int fds[2]);
int parent_f(int rg_r_end, int cut_w_end);

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
    (void) rg_child(fds);
    return 0;
  }

  __close(fds[1]);
  int rg_r_end = fds[0];

  int fds2[2];
  if (pipe(fds2) != 0) {
    fprintf(stderr, "pipe() error: %s\n", strerror(errno));
    return 1;
  }

  pid_t childpid2 = fork();
  if (childpid2 == -1) {
    fprintf(stderr, "fork() error: %s\n", strerror(errno));
    // TODO: close fds
    __close(rg_r_end);
    __close(fds2[0]);
    __close(fds2[1]);
    return 1;
  }
  else if (childpid2 == 0) {
    (void) cut_child(fds2);
    return 0;
  }

  // TODO: correct??
  __close(fds2[0]);
  int cut_w_end = fds2[1];

  /*
   * PARENT.
   */
  (void) parent_f(rg_r_end, cut_w_end);

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


int rg_child(int fds[2]) {
  __close(fds[0]);
  int w_end = fds[1];

  if (dup2(w_end, STDOUT_FILENO) == -1) {
    fprintf(stderr, "dup2() error during stdout rerouting: %s\n",
                     strerror(errno));
    __close(w_end);
    return 1;
  }

  char *_argv[] = {
    "rg",
    "--line-number",
    "--with-filename",
    "--field-match-separator= ",
    "--color=always",
    "--colors=match:none",
    "--colors=path:fg:cyan",
    "--colors=line:fg:yellow",
    ".",
    NULL
  };


  if (execvp(_argv[0], _argv) == -1) {
    // TODO: how to handle exec() errors??
    fprintf(stderr, "execvp() error: %s\n", strerror(errno));
  }
  // this is only reached if exec does not succeed.
  __close(w_end);
  return 1;
}

int fzf_child(int fds[2]) {
  __close(fds[0]);
  int w_end = fds[1];

  if (dup2(w_end, STDOUT_FILENO) == -1) {
    fprintf(stderr, "dup2() error during stdout rerouting: %s\n",
                     strerror(errno));
    __close(w_end);
    return 1;
  }

  char *_argv[] = {
    "rg",
    "--line-number",
    "--with-filename",
    "--field-match-separator= ",
    "--color=always",
    "--colors=match:none",
    "--colors=path:fg:cyan",
    "--colors=line:fg:yellow",
    ".",
    NULL
  };


  if (execvp(_argv[0], _argv) == -1) {
    // TODO: how to handle exec() errors??
    fprintf(stderr, "execvp() error: %s\n", strerror(errno));
  }
  // this is only reached if exec does not succeed.
  __close(w_end);
  return 1;
}

int cut_child(int fds[2]) {
  __close(fds[1]);
  int r_end = fds[0];

  FILE *r_end_fp = fdopen(r_end, "r");
  // TODO: error handle fdopen

#define BUF_SIZE 128
  char buf[BUF_SIZE] = { 0 };
  while (fgets(buf, BUF_SIZE, r_end_fp) != NULL);

  if (fclose(r_end_fp) != 0)
    fprintf(stderr, "fclose() error: %s\n", strerror(errno));

  char *saveptr;
  char *file = strtok_r(buf,  " ", &saveptr);
  char *line = strtok_r(NULL, " ", &saveptr);

  if (!(file && line)) {
    // TODO: error handle this case.
    return 1;
  }

#define JUST_PRINT_FILE_AND_LINE 1
  // launch nvim directly or simply print "<file> +<line>"?
#if JUST_PRINT_FILE_AND_LINE
  printf("%s +%s", file, line);
  return 0;
#else
  // add a '+' in front of the line number string.
  char *s = line;
  while (*(s++));
  *(s + 1) = '\0';
  while (s > line) {
    *s = *(s - 1);
    s--;
  }
  *s = '+';

  char *_argv[] = {
    "echo",
    file,
    line,
    NULL
  };

  if (execvp(_argv[0], _argv) == -1) {
    // TODO: how to handle exec() errors??
    fprintf(stderr, "execvp() error: %s\n", strerror(errno));
  }
  // this is only reached if exec does not succeed.
  return 1;
#endif
}



int parent_f(int rg_r_end, int cut_w_end) {

  if (dup2(rg_r_end, STDIN_FILENO) == -1) {
    fprintf(stderr, "dup2() error during stdin rerouting: %s\n",
                     strerror(errno));
    __close(cut_w_end);
    __close(rg_r_end);
    return 1;
  }
  if (dup2(cut_w_end, STDOUT_FILENO) == -1) {
    fprintf(stderr, "dup2() error during stdin rerouting: %s\n",
                     strerror(errno));
    __close(cut_w_end);
    __close(rg_r_end);
    return 1;
  }


  // CHANGEDIR_NO_IGNORE
  char *_argv[] = {
    "/usr/bin/fzf",
    "--ansi",
    "--reverse",
    "--preview-window=~2,+{2}",
    "--preview",
    "bat --theme gruvbox-dark --color=always {1} --highlight-line {2}",
    NULL};

  if (execvp(_argv[0], _argv) == -1) {
    // TODO: how to handle exec() errors??
    fprintf(stderr, "execvp() error: %s\n", strerror(errno));
  }
  // this is only reached if exec fails.
  __close(rg_r_end);
  __close(cut_w_end);
  return 1;
}


int __close(int fildes) {
  int r = close(fildes);
  if (r != 0)
    fprintf(stderr, "close() error: %s\n", strerror(errno));
  return r;
}
