all: mbcs-1.exe mbcs-2.exe mbcs-3.exe mbcs-3-noinline.exe

mbcs-3.exe: mbcs-3.c
	gcc -g -Zomf -Zmap -o $@ $^ -los2_unicode_lazy

mbcs-3-noinline.exe: mbcs-3.c
	gcc -g -Zomf -Zmap -D_DONT_USE_CTYPE_INLINE_ -o $@ $^ -los2_unicode_lazy

%.exe: %.c
	gcc -g -Zomf -Zmap -o $@ $^

clean:
	rm -rf *.exe *.dll *.map
