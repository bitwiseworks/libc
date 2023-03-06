all: spawnvpe-1.exe spawnvpe-1-child.exe

%.exe: %.c
	gcc -g -Zomf -Zmap -lcx -o $@ $^
