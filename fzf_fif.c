#include <stdlib.h> // malloc, exit
#include <stdio.h>  // printf, fprintf, fgets, fclose, etc.
#include <string.h> // strerror, strcpy, strlen.
#include <errno.h>  // errno.
#include <unistd.h> // fork, pipe,
#include <sys/wait.h>

#include "myutils.h"

#define BUF_SIZE 128
#define FIELD_MATCH_SEPARATOR ":"

#ifndef DEBUG
#define DEBUG 0
#endif

int parent(int fds[2], pid_t childpid);
int child1(int fds[2]);
int child2(int fds[2]);


int main() {
  int fds[2];
  if (pipe(fds) != 0) {
    perror("pipe() error");
    return 1;
  }

  /*
   * FORK RG/FZF SUBPROCESS TREE
   */
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork() error");
    __close(fds[0]);
    __close(fds[1]);
    return 1;
  }
  else if (pid == 0)
    return child1(fds);

  parent(fds, pid);
}


int parent(int fds[2], pid_t childpid) {
  __close(fds[1]);

  FILE *r_fp = fdopen(fds[0], "r");
  if (!r_fp) {
    perror("fdopen()");
    return 1;
  }

  char buf[BUF_SIZE] = { 0 };
  while (fgets(buf, BUF_SIZE, r_fp) != NULL);

  if (fclose(r_fp) != 0)
    // don't exit, just continue and let OS handle it.
    perror("fclose() error");


  // extract filename and line number.
#if DEBUG
  printf("buf: %s\n", buf);
#endif
  char *tmp;
  char *file = strtok_r(buf,  FIELD_MATCH_SEPARATOR, &tmp);
  char *line = strtok_r(NULL, FIELD_MATCH_SEPARATOR, &tmp);

  int good_parse = file && line               // correctly parsed file and line.
                && strtol(line, &tmp, 10) > 0 // line nums should be positive.
                && *tmp == '\0';              // check strtol() success.

  if (!good_parse) {
    fprintf(stderr, "\x1b[31mfzf_fif error: Failed to parse file and/or line from rg "
                    "output.\x1b[0m\n"
                    "Got: file = \"%s\", line = \"%s\".\n\n"
                    "Note: fzf_fif does not work with file names containing "
                    "'"FIELD_MATCH_SEPARATOR"'.\n", file, line);
    return 1;
  }

  // append '+' to start of line string.
  // we effectively increase the size of `line` by 1; this is always possible
  // because if `line` is parsed, then it will always *at least* have a newline
  // char following the number string.
  char *s = line;
  while (*(s++));
  *(s + 1) = '\0';
  while (s > line) {
    *s = *(s - 1);
    s--;
  }
  *s = '+';

#if DEBUG
  printf("file: %s\nline: %s\nnvim %s %s\n", file, line, file, line);
  return 0;
#else

#if 0
  // TODO: why wait here?
  int stat_loc;
  pid_t _childpid = waitpid(childpid, &stat_loc, 0);
  if (_childpid == -1)
    perror("waitpid() error");
  else if (_childpid != childpid) {
    ;
    // TODO: how to handle this?? probably not even necessary.
  }
  else if (!WIFEXITED(stat_loc))
    fprintf(stderr, "Child exited abnormally with exit code %d\n",
                    WEXITSTATUS(stat_loc));
#endif

  // launch nvim at the specified file and line.
  char *_argv[] = {
    "nvim",
    file,
    line,
    NULL
  };

  return __myexec(_argv);
#endif
}


int child1(int main_fds[2]) {
  __close(main_fds[0]);

  if (__dup2(main_fds[1], STDOUT_FILENO) == -1) {
    __close(main_fds[1]);
    return 1;
  }

  int fds[2];
  if (pipe(fds) != 0) {
    perror("pipe()");
    __close(main_fds[1]);
    return 1;
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("pipe()");
    __close(main_fds[1]);
    __close(fds[0]);
    __close(fds[1]);
    return 1;
  }
  else if (pid == 0)
    return child2(fds);

  __close(fds[1]);


  if (__dup2(fds[0], STDIN_FILENO) == -1) {
    // TODO: does anything else need to be closed here?
    __close(fds[0]);
    __close(main_fds[1]);
    return 1;
  }


  char *fzf_argv[] = {
    "fzf",
    "--ansi",
    "--reverse",
    "--preview-window=~2,+{2}",
    "--preview",
    "bat --theme gruvbox-dark --color=always {1} --highlight-line {2}",
    NULL
  };

  __myexec(fzf_argv);

  // TODO: should anything else be closed?
  __close(main_fds[0]);
  __close(main_fds[1]);
  return 1;
}


int child2(int fds[2]) {
  __close(fds[0]);

  if (__dup2(fds[1], STDOUT_FILENO) == -1) {
    __close(fds[1]);
    return 1;
  }

  char *rg_argv[] = {
    "rg",
    "--line-number",
    // "-m1", // only one match per file
    "--with-filename",
    "--field-match-separator="FIELD_MATCH_SEPARATOR,
    "--color=always",
    "--colors=match:none",
    "--colors=path:fg:cyan",
    "--colors=line:fg:yellow",
    ".",
    NULL
  };

  return __myexec(rg_argv);
}
