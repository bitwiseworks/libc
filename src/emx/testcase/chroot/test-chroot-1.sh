#!/bin/sh

set -e

unixdir="$(mktemp -d --tmpdir unix.XXXXX)"
rootdir="$(mktemp -d --tmpdir root.XXXXX)"

# Normalize slashes for comparison
unixdir=$(printf "%s" "$unixdir" | tr \\\\ /)
rootdir=$(printf "%s" "$rootdir" | tr \\\\ /)

trap 'rm -rf "$unixdir" "$rootdir"' EXIT HUP INT TERM

echo "1. Unofficial unixroot"
UNIXROOT="$unixdir" chroot-1.exe "$rootdir"

echo "2. No unixroot"
UNIXROOT="" chroot-1.exe "$rootdir"

echo "3. Unofficial unixroot, chrooted"
UNIXROOT="$unixdir" UNIXROOT_CHROOTED=1 chroot-1.exe "$rootdir"

echo "4. Unofficial unixroot, chroot(NULL)"
UNIXROOT="$unixdir" TEST_CHROOT_NULL=1 chroot-1.exe "$rootdir"

echo "5. Unofficial unixroot, chrooted, chroot(NULL)"
UNIXROOT="$unixdir" UNIXROOT_CHROOTED=1 TEST_CHROOT_NULL=1 chroot-1.exe "$rootdir"
