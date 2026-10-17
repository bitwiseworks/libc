all: mbcs-1.exe

%.exe: %.c
	gcc -g -Zomf -Zmap -o $@ $^

clean:
	rm -rf *.exe *.dll *.map
