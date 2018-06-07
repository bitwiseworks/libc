#!sh
ROOTDIR=`dirname $0`
OUTDIR=$ROOTDIR/../../obj/OS2/DEBUG
SRCDIR=`dirname $1`
EXT=`echo $1 | sed -e 's/.*\.\([a-zA-Z0-9]*\)$/\1/'`
cd $ROOTDIR
#echo ROOTDIR=$ROOTDIR
#echo OUTDIR=$OUTDIR
#echo 1=$1
#echo EXT=$EXT
#set -x

if [ "$EXT" = 's' -o "$EXT" = 'S' ]; then
    gcc -c -O3 -o $TMP/compileO3.o -fmessage-length=0 -x assembler-with-cpp \
     -DIN_INNOTEK_LIBC -D__IN_INNOTEK_LIBC__ -D_NFILES=20 -DHAVE_CONFIG_H \
     -I$ROOTDIR/include -I$ROOTDIR/src/include -I$ROOTDIR/src/lib/bsd/include -I$OUTDIR/emx \
     -I$ROOTDIR/src/lib/lgpl/ -I$ROOTDIR/src/lib/lgpl/sysdeps/os2 -I$ROOTDIR/src/lib/lgpl/sysdeps/i386/ -I$ROOTDIR/src/lib/lgpl/sysdeps/generic -I$ROOTDIR/src/lib/lgpl/include \
     -I$SRCDIR $1 \
 && gcc -c -O3 -o $TMP/compileLg.o -fmessage-length=0 -x assembler-with-cpp \
     -DIN_INNOTEK_LIBC -D__IN_INNOTEK_LIBC__ -D_NFILES=20 -DHAVE_CONFIG_H -DDEBUG_LOGGING -D__LIBC_STRICT \
     -I$ROOTDIR/include -I$ROOTDIR/src/include -I$ROOTDIR/src/lib/bsd/include -I$OUTDIR/emx \
     -I$ROOTDIR/src/lib/lgpl/ -I$ROOTDIR/src/lib/lgpl/sysdeps/os2 -I$ROOTDIR/src/lib/lgpl/sysdeps/i386/ -I$ROOTDIR/src/lib/lgpl/sysdeps/generic -I$ROOTDIR/src/lib/lgpl/include \
     -I$SRCDIR $1 \
 && echo succesfully built $1 \
 && nm -sS $TMP/compileO3.o | sort

else
       gcc -c -O3 -o $TMP/compileO3.o -fmessage-length=0 -std=gnu99 -Wundef -Wall -Wmissing-prototypes -pedantic -Wno-long-long \
        -DIN_INNOTEK_LIBC -D__IN_INNOTEK_LIBC__ -D_NFILES=20 -DHAVE_CONFIG_H \
        -I$ROOTDIR/include -I$ROOTDIR/src/include -I$ROOTDIR/src/lib/bsd/include -I$OUTDIR/emx \
        -I$ROOTDIR/src/lib/lgpl/ -I$ROOTDIR/src/lib/lgpl/sysdeps/os2 -I$ROOTDIR/src/lib/lgpl/sysdeps/i386/ -I$ROOTDIR/src/lib/lgpl/sysdeps/generic -I$ROOTDIR/src/lib/lgpl/include \
        -I$SRCDIR $1 \
    && gcc -c -O3 -o $TMP/compileLg.o -fmessage-length=0 -std=gnu99 -Wundef -Wall -Wmissing-prototypes -pedantic -Wno-long-long \
        -DIN_INNOTEK_LIBC -D__IN_INNOTEK_LIBC__ -D_NFILES=20 -DHAVE_CONFIG_H -DDEBUG_LOGGING -D__LIBC_STRICT \
        -I$SRCDIR -I$ROOTDIR/include -I$ROOTDIR/src/include -I$ROOTDIR/src/lib/bsd/include -I$OUTDIR/emx \
        -I$ROOTDIR/src/lib/lgpl/ -I$ROOTDIR/src/lib/lgpl/sysdeps/os2 -I$ROOTDIR/src/lib/lgpl/sysdeps/i386/ -I$ROOTDIR/src/lib/lgpl/sysdeps/generic -I$ROOTDIR/src/lib/lgpl/include \
        -I$SRCDIR $1 \
    && echo succesfully built $1 \
    && nm -sSg $TMP/compileO3.o | sort
fi
