#
# This simple script looks through libc-std.h file and collects either all
# non-underscored aliases or underscored aliases to their __std_ equivalents.
#
# ARG1: path to libc-std.h
# ARG2: "" or "_".
#

BEGIN {
  fn = ARGV [1]"libc-std.h";
  want_underscore = (ARGV [2] == "_");

  while ((getline < fn) > 0)
    if (match($0, "_STD\\(") != 0)
    {
      blank = match($0, " ");
      fun = substr($0, blank + 1, 255);
      blank = match(fun, " ");
      if (blank > 0)
        fun = substr(fun, 1, blank - 1);

      has_underscore = (substr(fun, 1, 1) == "_");
      if (has_underscore == want_underscore)
        print has_underscore ? substr(fun, 2) : fun;
    }

  exit;
}
