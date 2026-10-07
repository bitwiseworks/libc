all: version-1.exe inode-copy-1.exe

%.exe: %.c
	gcc -g -Zomf -Zmap -o $@ $^

clean:
	rm -rf *.exe *.dll *.map
