#
# This simple script looks through libc-std.h file and collects either all
# gnu aliases.
#
# ARG1: path to libc-std.h
# ARG2: "" or "_".
#

BEGIN {
  fn = ARGV [1]"libc-std.h";
  want_underscore = (ARGV [2] == "_");

  while ((getline < fn) > 0)
    if (match($0, "_GNUALIAS\\(") != 0)
    {
       blank = match($0, " ");
       fun = substr($0, blank + 1, 255);
       blank = match(fun, " ");
       if (blank > 0)
         fun = substr(fun, 1, blank - 1);

       par = match($0, "\\(");
       fun2 = substr($0, par+1, 255);
       par = match(fun2, "\\)");
       if (par > 0)
         fun2 = substr(fun2, 1, par - 1);

       print fun " " fun2 " "
    }

  exit;
}
