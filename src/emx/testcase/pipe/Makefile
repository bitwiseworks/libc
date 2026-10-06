all: namedpipe-1

namedpipe-1: namedpipe-1.exe

namedpipe-1.exe: namedpipe-1.c
	gcc -g -Zomf -Zmap $< -o $@

clean:
	rm -rf *.exe *.dll *.map

.PHONY: namedpipe-1
