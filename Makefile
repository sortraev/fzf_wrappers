fzf_fif: fzf_fif.c
	gcc -O3 $< -o $@

fzf_changedir: fzf_wrap.c
	gcc -O3 -DCHANGEDIR_NO_IGNORE=0 $< -o $@
fzf_changedir_no_ignore: fzf_wrap.c
	gcc -O3 -DCHANGEDIR_NO_IGNORE=1 $< -o $@

deploy: fzf_fif fzf_changedir fzf_changedir_no_ignore
	mv $^ ~/.local/bin

clean:
	rm -rf fzf_changedir_no_ignore fzf_changedir fzf_fif **vgcore**
