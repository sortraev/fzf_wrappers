#include "myutils.h" // pipe_t, _dup2(), _close(), _exec()


int fd_child  (pipe_t p, int ignore);
int fzf_parent(pipe_t p, int ignore);


int main(int argc, char **argv) {

  // TODO: foolproof, but not very scalable hack to disable ignore.
  int ignore = argc == 1;

  pipe_t p;
  if (pipe(p.fds) != 0) {
    perror("pipe()");
    return 1;
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork()");
    _close(p.r_end);
    _close(p.w_end);
    return 1;
  }

  else if (pid == 0)
    return fd_child(p, ignore);
  fzf_parent(p, ignore);

  return 0;
}

int fd_child(pipe_t p, int ignore) {
  _close(p.r_end);

  if (_dup2(p.w_end, STDOUT_FILENO) == -1) {
    _close(p.w_end);
    return 1;
  }

  char *_argv[] = {
    "fd",
    "--type=directory",
    "--no-ignore-vcs",
    ignore ? NULL :
      "--hidden",
      "--exclude=.git",
      NULL
  };

  _exec(_argv);
  // only reached if _exec fails
  _close(p.w_end);
  return 1;
}

int fzf_parent(pipe_t p, int ignore) {
  _close(p.w_end);

  if (_dup2(p.r_end, STDIN_FILENO) == -1) {
    _close(p.r_end);
    return 1;
  }


  char *_argv[] = {
    "fzf",
    "--reverse",
    "--preview",
    ignore ?
        "eza --git --tree -L3 -a    -I=.git --group-directories-first --color=always {}"
      : "eza --git --tree -L3 -a -D -I=.git --group-directories-first --color=always {}"
    ,
    NULL
  };

  _exec(_argv);
  // only reached if _exec fails
  _close(p.r_end);
  return 0;
}
