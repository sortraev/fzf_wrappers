# fzf wrappers

* `fzf` wrappers written in C:
  - `fzf_changedir.c` pipes output of `fd` into `fzf` to search for directories
      and can eg. be used with `cd`.
  - `fzf_fif.c` pipes output of `rg` into `fzf` to search for keywords in files
      recursively and extract file and line number of search result; then runs
      `nvim <file> <linenumber>`. Can be modified to suit other editors.
