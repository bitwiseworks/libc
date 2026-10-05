all: wl-1

wl-1: wl-1.exe wl-1-dll.dll

wl-1.exe: wl-1.c wl-1.def wl-1-dll.dll
	gcc -g -Zomf -Zmap $< wl-1.def -o $@

wl-1-dll.dll: wl-1-dll.c
	gcc -g -Zomf -Zmap -Zdll $< -o $@

clean:
	rm -rf *.exe *.dll *.map

.PHONY: wl-1