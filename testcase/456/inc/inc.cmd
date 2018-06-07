/* */

parse arg sInc

say 'extern int giInc_'sNum';';
say ''
say '#ifdef INC_CODE'
say 'int inc_'sInc'(int i)'
say '{'
say '    i += 1;'
say '    return i;'
say '}'
say '#endif'

