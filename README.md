# fzf wrappers

* `fzf` wrappers written in C:
  - `fzf_changedir.c` pipes output of `fd` into `fzf` to search for directories
      and can eg. be used with `cd`.
  - `fzf_fif.c` pipes output of `rg` into `fzf` to search for keywords in files
      recursively and extract file and line number of search result; then runs
      `nvim <file> <linenumber>`. Can be modified to suit other editors.

### Requirements

* `fzf_changedir`: `fzf`, `fd`, `exa`
* `fzf_fif`: `fzf`, `bat`, `ripgrep`

TODO: the programs should ideally fallback to compile with using `find`, `ls`,
`cat`, and `grep` instead, if any of the above do not exist (except for `fzf`,
which will always be a hard requirement).
