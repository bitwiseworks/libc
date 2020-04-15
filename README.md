# LIBC Next (kLIBC fork)

This repository contains the source code for the LIBC Next project maintained by bww bitwise works GmbH.

LIBC Next (or LIBCn, for short) is an attempt to provide an implementation of the C runtime library which is suitable for porting Unix and Linux applications to the OS/2 operating system using the GCC compiler. LIBC Next is heavily based on the kLIBC project. The original kLIBC home page is http://trac.netlabs.org/libc/wiki. The original kLIBC source code was imported into this repository as is (without the original commit history and with a few modifications mentioned below). This repository gets periodically synchronized with it to pick up latest changes. The modifications applied when importing include:

* Import code from branch `libc-0.6` only (the original `trunk` is at version 0.7 which was never finished).
* Remove the `/src/binutils` directory as we maintain a separate repository for binutils (https://github.com/bitwiseworks/binutils-os2).
* Remove the `/src/gcc` directory as we maintain a separate repository for a much newer version of GCC (https://github.com/bitwiseworks/gcc-os2).

Check `svn-import.sh` on the `vendor` branch for more details on what is imported and what is not.

A fork with a separate repository became necessary because the ongoing OS/2 development directed by bitwise works mostly uses the GCC compiler which requires kLIBC, but the original kLIBC project is not actively maintained anymore. As we at bitwise works work on our numerous projects, we find bugs in kLIBC and missing features that we want to implement. And although we usually submit all our patches to the kLIBC maintainer, new kLIBC versions are released very rarely and some of our patches are rejected. Since almost all our applications depend on kLIBC, we simply cannot wait that long. Therefore we have to make our own binary releases of this library from time to time.

We used to store our patches in DIFF files in source RPM archives but we got so many of them that this is simply not manageable any more because of inter-dependencies between these separate DIFFs. After all, this sort of things is exactly what version control systems are here for. So at some point it was decided to create a fork of kLIBC and host it at GitHub.

LIBC Next uses its own DLL name and versioning scheme in order to clearly distinguish from kLIBC as required by its maintainer. It is, however, fully backward compatible with the original kLIBC library version 0.6.6 (both ABI- and API-wise) and may be used as a drop-in replacement. Compatibility with earlier kLIBC releases is as good as that of kLIBC version 0.6.6 itself.

Backward compatibility is also maintained between different releases of LIBC Next itself as long as its major version (and the DLL name) does not change. Backward compatibility between different major versions of LIBC Next will be maintained using fowrader DLLs as needed.

Note that we also have a project named LIBCx (https://github.com/bitwiseworks/libcx) where we attempted to apply our fixes and improvements without forking kLIBC (mostly because kLIBC is a rather complex library all other libraries depend on and therefore it must be rock solid, any patch to it is potentially dangerous). However, some fixes and improvements are simply impossible to be implemented within a separate library so there are still patches to the original kLIBC code that we need to deploy.

The general plan for LIBC Next is to eventually incorporate all LIBCx code into this repository. And then merge it all with the original kLIBC project if it ever gets resurrected.

We want to thank Knut St. Osmundsen (aka Bird) for his effortful support of OS/2 software throughout many years and especially for his work on the kLIBC project LIBC Next is based on.
