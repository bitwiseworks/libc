# kLIBC (bww flavor)

This repository contains the source code for the kLIBC project with patches maintained by bww bitwise works GmbH.

kLIBC is an attempt to provide an implementation of the C runtime library which is suitable for porting Unix and Linux applications to the OS/2 operating system using the GCC compiler. The original kLIBC home page is http://trac.netlabs.org/libc/wiki. The original kLIBC source code is imported into this repository as is (without the original commit history and with a few modifications mentioned below). It gets periodically synchronized to pick up latest changes from upstream. The modifications applied when importing include:

* Import code from branch `libc-0.6` only (the original `trunk` is at version 0.7 which was never finished).
* Remove the `/src/binutils` directory as we maintain a separate repository for binutils (http://trac.netlabs.org/ports/browser/binutils/trunk).
* Remove the `/src/gcc` directory as we maintain a separate repository for a newer version of GCC (https://github.com/psmedley/gcc).

Check `svn-import.sh` on the `vendor` branch for more details on what is imported and what is not.

A separate repository for bitwiseworks patches became necessary because the ongoing OS/2 development directed by bitwiseworks mostly uses the GCC compiler which requires kLIBC, but the original kLIBC project is not actively maintained anymore. As we work on our numerous projects, we find bugs in kLIBC and missing features that we want to implement. But although we submit all our patches to the original maintainer, new kLIBC versions are released very rarely and some of our patches are rejected. Since almost all our applications depend on kLIBC, we simply cannot wait that long. Therefore we have to make our own binary releases of this library from time to time.

We used to store our patches in DIFF files in source RPM archives but we got so many of them that this is simply not manageable any more because of inter-dependencies between these separate DIFFs. After all, this sort of things is exactly what version control systems are here for. So at some point it was decided to create a fork of kLIBC and host it at GitHub.

Note that we also have a project named LIBCx (https://github.com/bitwiseworks/libcx) where we attempted to apply our fixes and improvements without forking kLIBC (mostly because kLIBC is a rather complex library all other libraries depend on and therefore it must be rock solid, any patch to it is potentially dangerous). However, some fixes and improvements are simply impossible to be implemented within a separate library so there are still patches to kLIBC that we need to deploy.

The general plan for our kLIBC fork is to eventually incorporate all LIBCx code into this repository. And then merge it all with the original kLIBC project if it ever gets resurrected.
