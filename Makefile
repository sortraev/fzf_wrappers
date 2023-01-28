fzf_fif: fzf_fif.c
	gcc -O3 $< -o $@
run: fzf_fif
	./$<


fzf_changedir: fzf_wrap.c
	gcc -O3 -DCHANGEDIR_NO_IGNORE=0 $< -o $@
fzf_changedir_no_ignore: fzf_wrap.c
	gcc -O3 -DCHANGEDIR_NO_IGNORE=1 $< -o $@

debug: fzf_wrap.c
	gcc -g3 -DCHANGEDIR_NO_IGNORE=0 $< -o $@

check: debug
	valgrind --track-fds=yes --leak-check=full ./$<


clean:
	rm -rf fzf_changedir_no_ignore fzf_changedir fzf_fif debug simple **vgcore**
