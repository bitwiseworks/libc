# This AWK script will take libc.def and extract the stuff between
# LIBGCCSTART and LIBGCCEND.

BEGIN{
#  if (ARGC != 3)
#  {
#    print "Usage: libgcc_d.awk <libcXY.def> <libc.def>";
#    exit 1;
#  }
  print "; Auto-generated file, DO NOT EDIT!!! (see libgcc_d.awk) */"
  in_exports = 0;
  in_libgcc = 0;
  done_library = 0;
}

/^LIBRARY .*/  { print $0; done_library = 1; }

/^EXPORTS$/    {
  if (!done_library)
  {
    print "No LIBRARY statement in file input file(s).";
    exit 1;
  }
  in_exports = 1;
  print "EXPORTS";
}

/LIBGCCEND/    { in_libgcc = 0; }
in_libgcc      { print $0; }
/LIBGCCSTART/  { in_libgcc = 1; }


