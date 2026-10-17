all: mbcs-1.exe mbcs-2.exe

%.exe: %.c
	gcc -g -Zomf -Zmap -o $@ $^

clean:
	rm -rf *.exe *.dll *.map
