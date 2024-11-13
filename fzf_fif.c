#include "myutils.h" // pipe_t, _dup2(), _close(), _exec()

// TODO: max path length is 4096 in Linux, so might want to change this one.
// note that BUF_SIZE should also account for the line length substring.
#define BUF_SIZE 512

#define RG_MAX_COLUMNS "128"
#define RG_FIELD_MATCH_SEPARATOR ":"

#ifndef DEBUG
#define DEBUG 0
#endif

int parent(pipe_t p, pid_t childpid);
int child1(pipe_t p);
int child2(pipe_t p);

int main() {
  pipe_t p;
  if (pipe(p.fds) != 0) {
    perror("pipe()");
    return 1;
  }

  /*
   * FORK RG/FZF SUBPROCESS TREE
   */
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork()");
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
  // TODO: should have some error handling here, in particular for the case
  // where a file name happens to be larger than BUF_SIZE, however unlikely.
  while (fgets(buf, BUF_SIZE, r_fp) != NULL);


  if (fclose(r_fp) != 0) {
    // don't exit, just continue and let OS handle it.
    // perror("fclose()");
  }

  if (!*buf) {
    // buf is empty at this point if fzf was interrupted *or* if the input to
    // fzf was empty (eg. if fzf_fif was initiated from an empty directory).
    return 0;
  }

#if DEBUG
  fprintf(stderr, "DEBUG: buf: %s\n", buf);
#endif
  // extract filename and line number.
  char *tmp;
  char *file = strtok_r(buf,  RG_FIELD_MATCH_SEPARATOR, &tmp);
  char *line = strtok_r(NULL, RG_FIELD_MATCH_SEPARATOR, &tmp);
  char *s = tmp + 1;

  int good_parse = file && line               // correctly parsed file and line.
                && strtol(line, &tmp, 10) > 0 // line nums should be positive.
                && *tmp == '\0';              // check strtol() success.

  if (!good_parse) {
    fprintf(stderr,
        "\x1b[31mfzf_fif error: Failed to parse file and/or line from rg output.\x1b[0m\n"
        "Got: file = \"%s\" and line = \"%s\".\n\n"
        "Note: fzf_fif compiled with field match separator \""RG_FIELD_MATCH_SEPARATOR"\", "
        "and hence does not support file names containing \""RG_FIELD_MATCH_SEPARATOR"\".\n",
        file, line);
    return 1;
  }

  // append '+' to start of line string.
  // we effectively increase the size of `line` by 1; this is always possible
  // because if `line` is correctly parsed, then it will always be followed by
  // *at least* two bytes (the original field/match separator and NULL-terminator).
  while (s > line) {
    *s = *(s - 1);
    s--;
  }
  *s = '+';

#if DEBUG
  fprintf(stderr, "DEBUG: file: %s\n", file);
  fprintf(stderr, "DEBUG: line: %s\n", line);
  fprintf(stderr, "DEBUG: 'nvim %s %s'\n", file, line);
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
    "--delimiter="RG_FIELD_MATCH_SEPARATOR,
    "--preview-window=~2,+{2}",
    "--preview",
    "bat --theme gruvbox-dark --color=always {1} --highlight-line {2} --style=header,numbers",
    // "bat --theme gruvbox-light --color=always {1} --highlight-line {2} --style=header,numbers",
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
    "--max-columns="RG_MAX_COLUMNS,
    "--with-filename",
    "--field-match-separator="RG_FIELD_MATCH_SEPARATOR,
    "--color=always",
    "--colors=match:none",
    "--colors=path:fg:cyan",
    "--colors=line:fg:yellow",
    ".",
    NULL
  };

  return _exec(rg_argv);
}
