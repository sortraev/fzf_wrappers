printself: printself.c
	gcc -g3 $< -o $@

run: printself
	./$<

.PHONY: simple
simple: simple.c
	@gcc $< -o $@
	@$$(./$@ | diff $< -) && echo "(success)"


