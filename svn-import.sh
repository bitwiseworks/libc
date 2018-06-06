#! /bin/sh

# This script is to import the original kLIBC sources and is not
# part of them.

REV=3947
URL=http://svn.netlabs.org/repos/libc/branches/libc-0.6
SUBDIRS="doc src/emx src/libctests src/misc testcase"

SCRIPT="${0##*/}"

if [ "$(git rev-parse --abbrev-ref HEAD)" != "vendor" ] ; then
  echo "Must be on vendor branch."
  exit 1
fi

if [ -n "$(git status -s | grep -v "^ M $SCRIPT$")" ] ; then
  echo "Repository state must be clean (except $SCRIPT)."
  exit 1
fi

# 1. Remove everything but .git and this script,
# 2. Import all files from root,
# 3. Import selected directories recursively.
# 4. Commit.

find . -mindepth 1 -maxdepth 1 ! -name "$SCRIPT" ! -name ".git" \( -type f -o -type d \) -exec rm -rf {} + && \
svn export "$URL" . -r$REV --depth files --force && \
for d in $SUBDIRS ; do svn export "$URL/$d" "$d" -r$REV --depth infinity ; done && \
git add -A . && \
git commit -e -m "vendor: Import r$REV from $URL." \
-m "Imported are all files from the root and the following subdirectories:" \
-m "$SUBDIRS" 

exit 0
