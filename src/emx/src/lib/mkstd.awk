# This AWK script will take a list of .c files (passed on the command line)
# and scan all of them for _STD(xxx) names. After this it will output the
# contents of the libc-std.h file which should be included in every libc
# sourcefile so that references to std names are handled internally within
# libc (and do not require to pull in -lc_alias and the -lc again after).

BEGIN{
  if (ARGC < 2)
  {
    print "Usage: mkstd.awk [file1.c{ file2.c{ ...}}]"
    exit 1;
  }
}

END {
  print "/* Auto-generated file, DO NOT EDIT!!! (see mkstd.awk) */"
  print "#ifndef __LIBC_STD_H__"
  print "#define __LIBC_STD_H__\n"
  print ""
  for (fun in std_fun)
  {
    print "#define " fun " _STD(" fun ")"
    if (!(fun in std_fun2))
      print "/*#define _" fun " _STD(" fun ")*/"
  }

  print ""
  print "/* BSD aliasing */"
  for (fun in bsd_aliases)
  {
    fun2 = bsd_aliases[fun];
    if (!(fun2 in std_fun))
      print "/*#define " fun2 " _BSDALIAS(" fun ")*/"
  }

  print ""
  print "/* GNU aliasing */"
  for (fun in gnu_aliases)
  {
    fun2 = gnu_aliases[fun];
    if (!(fun2 in std_fun))
      print "/*#define " fun2 " _GNUALIAS(" fun ")*/"
  }

  print "\n#endif /* __LIBC_STD_H__ */"
}

/_STD *\( *[_0-9A-Za-z]+ *\)/{
  while (match($0, "_STD *\\( *[_0-9A-Za-z ]+ *\\)"))
  {
    std_fun[gensub("_STD *\\( *([_0-9A-Za-z]+) *\\).*", "\\1", "", substr($0,RSTART,RLENGTH))]=1;
    $0 = substr($0, 1, RSTART-1) substr($0, RSTART+RLENGTH);
  }
}

/STDENTRY\( *[_0-9A-Za-z]+ *\)/{
  while (match($0, "STDENTRY\\( *[_0-9A-Za-z ]+ *\\)"))
  {
    std_fun[gensub("STDENTRY\\( *([_0-9A-Za-z]+) *\\).*", "\\1", "", substr($0,RSTART,RLENGTH))]=1;
    $0 = substr($0, 1, RSTART-1) substr($0, RSTART+RLENGTH);
  }
}

/__weak_reference\( *[_0-9A-Za-z]+ *, *[_0-9A-Za-z]+ *\)/{
  while (match($0, "__weak_reference\\( *[_0-9A-Za-z ]+ *, *[_0-9A-Za-z ]+ *\\)"))
  {
    fun1 = gensub("__weak_reference\\( *([_0-9A-Za-z]+) *, *[_0-9A-Za-z]+ *\\).*", "\\1", "", substr($0,RSTART,RLENGTH));
    fun2 = gensub("__weak_reference\\( *[_0-9A-Za-z]+ *, *([_0-9A-Za-z]+) *\\).*", "\\1", "", substr($0,RSTART,RLENGTH));
    bsd_aliases[fun1] = fun2;
    $0 = substr($0, 1, RSTART-1) substr($0, RSTART+RLENGTH);
  }
}

/__weak_alias\( *[_0-9A-Za-z]+ *, *[_0-9A-Za-z]+ *\)/{
  while (match($0, "__weak_alias\\( *[_0-9A-Za-z ]+ *, *[_0-9A-Za-z ]+ *\\)"))
  {
    fun1 = gensub("__weak_alias\\( *([_0-9A-Za-z]+) *, *[_0-9A-Za-z]+ *\\).*", "\\1", "", substr($0,RSTART,RLENGTH));
    fun2 = gensub("__weak_alias\\( *[_0-9A-Za-z]+ *, *([_0-9A-Za-z]+) *\\).*", "\\1", "", substr($0,RSTART,RLENGTH));
    bsd_aliases[fun2] = fun1;
    $0 = substr($0, 1, RSTART-1) substr($0, RSTART+RLENGTH);
  }
}

/^[^_0-9A-Za-z]*weak_alias *\( *[_0-9A-Za-z]+ *, *[_0-9A-Za-z]+ *\)/{
  while (match($0, "weak_alias *\\( *[_0-9A-Za-z ]+ *, *[_0-9A-Za-z ]+ *\\)"))
  {
    fun1 = gensub("weak_alias *\\( *([_0-9A-Za-z]+) *, *[_0-9A-Za-z]+ *\\).*", "\\1", "", substr($0,RSTART,RLENGTH));
    fun2 = gensub("weak_alias *\\( *[_0-9A-Za-z]+ *, *([_0-9A-Za-z]+) *\\).*", "\\1", "", substr($0,RSTART,RLENGTH));
    gnu_aliases[fun1] = fun2;
    $0 = substr($0, 1, RSTART-1) substr($0, RSTART+RLENGTH);
  }
}



/MATHSUFFIX1\( *[_0-9A-Za-z]+ *\)/{
  while (match($0, "MATHSUFFIX1\\( *[_0-9A-Za-z ]+ *\\)"))
  {
    fun = gensub("MATHSUFFIX1\\( *([_0-9A-Za-z]+) *\\).*", "\\1", "", substr($0,RSTART,RLENGTH))
    std_fun[fun]=1;
    std_fun[fun "f"]=1;
    std_fun[fun "l"]=1;
    $0 = substr($0, 1, RSTART-1) substr($0, RSTART+RLENGTH);
  }
}

/mkstd.awk: NOUNDERSCORE\( *[_0-9A-Za-z]+ *\)/{
  std_fun2[gensub(".*NOUNDERSCORE\\( *([_0-9A-Za-z]+) *\\).*", "\\1", "", $0)]=1;
}
