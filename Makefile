fzf_fif: fzf_fif.c
	gcc -O3 $< -o $@

fzf_changedir: fzf_changedir.c
	gcc -O3 $< -o $@

fzf_changedir_no_ignore: fzf_changedir.c
	gcc -O3 -DCHANGEDIR_NO_IGNORE=1 $< -o $@

deploy: deploy_fif deploy_changedir

deploy_fif: fzf_fif
	mv $^ ~/.local/bin

deploy_changedir: fzf_changedir fzf_changedir_no_ignore
	mv $^ ~/.local/bin

clean:
	rm -rf fzf_changedir_no_ignore fzf_changedir fzf_fif **vgcore**
