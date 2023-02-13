# compilation
fzf_fif: fzf_fif.c
	gcc -O3 $< -o $@

fzf_changedir: fzf_changedir.c
	gcc -O3 $< -o $@

# debug
fzf_fif_debug: fzf_fif.c
	gcc -g3 -DDEBUG=1 $< -o $@


# "deployment"
deploy: deploy_fif deploy_changedir
deploy_fif: fzf_fif
	mv $^ ~/.local/bin
deploy_changedir: fzf_changedir
	mv $^ ~/.local/bin

clean:
	rm -rf fzf_changedir fzf_fif fzf_fif_debug **vgcore**
