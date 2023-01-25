printself: printself.c
	gcc -o3 $< -o $@

run: printself
	./$<


debug: printself.c
	gcc -g3 $< -o $@

check: debug
	valgrind --track-fds=yes --leak-check=full ./$<

.PHONY: simple
simple: simple.c
	@gcc $< -o $@
	@$$(./$@ | diff $< -) && echo "(success)"


clean:
	rm -rf printself debug simple **vgcore**
