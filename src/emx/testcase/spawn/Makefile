all: spawnvpe-1.exe spawnvpe-1-child.exe spawnvpe-2.exe spawnvpe-3.exe

%.exe: %.c
	gcc -g -Zomf -Zmap -lcx -o $@ $^
