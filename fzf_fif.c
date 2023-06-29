#include "myutils.h" // pipe_t, _dup2(), _close(), _exec()

#define BUF_SIZE 128
#define FIELD_MATCH_SEPARATOR ":"

#ifndef DEBUG
#define DEBUG 0
#endif

int parent(pipe_t p, pid_t childpid);
int child1(pipe_t p);
int child2(pipe_t p);

int main() {
  pipe_t p;
  if (pipe(p.fds) != 0) {
    perror("pipe() error");
    return 1;
  }

  /*
   * FORK RG/FZF SUBPROCESS TREE
   */
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork() error");
    _close(p.r_end);
    _close(p.w_end);
    return 1;
  }
  else if (pid == 0)
    return child1(p);

  return parent(p, pid);
}


int parent(pipe_t p, pid_t childpid) {
  _close(p.w_end);

  FILE *r_fp = fdopen(p.r_end, "r");
  if (!r_fp) {
    perror("fdopen()");
    return 1;
  }

  char buf[BUF_SIZE] = { 0 };
  while (fgets(buf, BUF_SIZE, r_fp) != NULL);


  if (fclose(r_fp) != 0)
    // don't exit, just continue and let OS handle it.
    perror("fclose() error");

  if (!*buf) {
    // buf is empty at this point if fzf was interrupted *or* if the input to
    // fzf was empty (eg. if fzf_fif was initiated from an empty directory).
    return 0;
  }

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
  size_t i = 0;
  while (line[i++]);
  line[i] = '\0';
  while (--i > 0)
    line[i] = line[i - 1];
  *line = '+';

#if DEBUG
  printf("file: %s\nline: %s\nnvim %s %s\n", file, line, file, line);
  return 0;
#else

  // launch nvim at the specified file and line.
  char *_argv[] = {
    "nvim",
    file,
    line,
    NULL
  };

  return _exec(_argv);
#endif
}


int child1(pipe_t main_p) {
  _close(main_p.r_end);

  if (_dup2(main_p.w_end, STDOUT_FILENO) == -1) {
    _close(main_p.w_end);
    return 1;
  }

  pipe_t p;
  if (pipe(p.fds) != 0) {
    perror("pipe()");
    _close(main_p.w_end);
    return 1;
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("pipe()");
    _close(main_p.w_end);
    _close(p.r_end);
    _close(p.w_end);
    return 1;
  }
  else if (pid == 0)
    return child2(p);

  _close(p.w_end);


  if (_dup2(p.r_end, STDIN_FILENO) == -1) {
    // TODO: does anything else need to be closed here?
    _close(p.r_end);
    _close(main_p.w_end);
    return 1;
  }


  char *fzf_argv[] = {
    "fzf",
    "--ansi",
    "--reverse",
    "--delimiter="FIELD_MATCH_SEPARATOR,
    "--preview-window=~2,+{2}",
    "--preview",
    "bat --theme gruvbox-dark --color=always {1} --highlight-line {2} --style=header,numbers",
    NULL
  };

  _exec(fzf_argv);

  // TODO: should anything else be closed?
  _close(main_p.r_end);
  _close(main_p.w_end);
  return 1;
}


int child2(pipe_t p) {
  _close(p.r_end);

  if (_dup2(p.w_end, STDOUT_FILENO) == -1) {
    _close(p.w_end);
    return 1;
  }

  char *rg_argv[] = {
    "rg",
    "--line-number",
    // "-m1", // only one match per file (TODO: find out how this would work).
    "--with-filename",
    "--field-match-separator="FIELD_MATCH_SEPARATOR,
    "--color=always",
    "--colors=match:none",
    "--colors=path:fg:cyan",
    "--colors=line:fg:yellow",
    ".",
    NULL
  };

  return _exec(rg_argv);
}
