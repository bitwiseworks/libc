#!/bin/sh -v

echo "arg[0] = \"$0\""

: "$((i=1))"

while [ -n "$1" ]
do
  echo "arg[$i] = \"$1\""
  shift
  : "$((i = i + 1))"
done
