fzf_fif: fzf_fif.c
	gcc -O3 $^ -o $@
fzf_dirfind: fzf_dirfind.c
	gcc -O3 $^ -o $@
fzf_fif_debug: fzf_fif.c
	gcc -g3 -DDEBUG=1 $< -o $@

# "install"
install: fzf_fif fzf_dirfind
	mv $^ ~/.local/bin

clean:
	rm -rf fzf_dirfind fzf_fif fzf_fif_debug **vgcore**
