all: response-1

response-1: response-1.exe

response-1.exe: response-1.c
	gcc -g -Zomf -Zmap $< -o $@

clean:
	rm -rf *.exe *.dll *.map

.PHONY: response-1
