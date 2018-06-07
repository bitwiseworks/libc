/* $Id: checkhdrs.cmd 362 2003-07-11 15:34:00Z bird $
 *
 * Check all the headers if they can be included alone.
 * There is an exclusion list to skip those that requires company.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird@anduin.net>
 *
 * All Rights Reserved
 *
 */

parse arg sInclude sFile sOther
if (sInclude = '') then
do
    say 'syntax: checkhdrs <includedir> [file]';
    exit 8;
end

/*
 * Normalize input.
 */
asFiles.0 = 0;
rc = SysFileTree(sInclude, 'asFiles', 'OD');
if (asFiles.0 <> 1) then
do
    say 'error!: failed to find '''sInclude'''.';
    exit 8;
end
sInclude = asFiles.1;


/*
 * Exclude list.
 */
i = 1;
asExcludes.i = 'os2tk.h'; i=i+1;
asExcludes.i = 'sys\moddef.h'; i=i+1;   /* f**kup */
asExcludes.i = 'objc\encoding.h'; i=i+1;
asExcludes.i = 'objc\hash.h'; i=i+1;
asExcludes.i = 'objc\NXConstS.h'; i=i+1;
asExcludes.i = 'objc\objc-api.h'; i=i+1;
asExcludes.i = 'objc\objc-lis.h'; i=i+1;
asExcludes.i = 'objc\objc.h'; i=i+1;
asExcludes.i = 'objc\Object.h'; i=i+1;
asExcludes.i = 'objc\Protocol.h'; i=i+1;
asExcludes.i = 'objc\sarray.h'; i=i+1;
asExcludes.i = 'objc\thr.h'; i=i+1;
asExcludes.i = 'objc\typedstr.h'; i=i+1;
asExcludes.i = '386\types.h'; i=i+1;
asExcludes.i = 'machine\types.h'; i=i+1;
asExcludes.0 = i - 1;

/*
 * Require Resolving.
 */
i = 1;
aReq.i.msHdr = 'dyn-string.h';  aReq.i.msIncs = 'ansidecl.h'; i=i+1;
aReq.i.msHdr = 'emx\locale.h';  aReq.i.msIncs = 'sys/types.h'; i = i + 1;
aReq.i.msHdr = 'emx\umalloc.h'; aReq.i.msIncs = 'stddef.h umalloc.h sys/builtin.h sys/fmutex.h sys/rmutex.h'; i = i + 1;
aReq.i.msHdr = 'fibheap.h';     aReq.i.msIncs = 'sys/types.h'; i=i+1;
aReq.i.msHdr = 'hashtab.h';     aReq.i.msIncs = 'sys/types.h'; i=i+1;
aReq.i.msHdr = 'net\if.h';      aReq.i.msIncs = 'sys/socket.h'; i=i+1;
aReq.i.msHdr = 'net\if_arp.h';  aReq.i.msIncs = 'sys/socket.h'; i=i+1;
aReq.i.msHdr = 'net\if_dl.h';   aReq.i.msIncs = 'sys/socket.h'; i=i+1;
aReq.i.msHdr = 'net\route.h';   aReq.i.msIncs = 'sys/socket.h'; i=i+1;
aReq.i.msHdr = 'netinet\ip_icmp.h';     aReq.i.msIncs = 'sys/types.h netinet/in_systm.h netinet/in.h netinet/ip.h'; i=i+1;
aReq.i.msHdr = 'netinet\icmp_var.h';    aReq.i.msIncs = 'sys/types.h netinet/in_systm.h netinet/in.h netinet/ip.h netinet/ip_icmp.h'; i=i+1;
aReq.i.msHdr = 'netinet\if_ether.h';    aReq.i.msIncs = 'sys/types.h netinet/in.h sys/socket.h net/if_arp.h net/if.h '; i=i+1;
aReq.i.msHdr = 'netinet\igmp.h';        aReq.i.msIncs = 'sys/types.h netinet/in.h'; i=i+1;
aReq.i.msHdr = 'netinet\igmp_var.h';    aReq.i.msIncs = 'sys/types.h netinet/in.h netinet/igmp.h'; i=i+1;
aReq.i.msHdr = 'netinet\in_systm.h';    aReq.i.msIncs = 'sys/types.h'; i=i+1;
aReq.i.msHdr = 'netinet\ip.h';          aReq.i.msIncs = 'sys/types.h netinet/in_systm.h netinet/in.h'; i=i+1;
aReq.i.msHdr = 'netinet\ip_var.h';      aReq.i.msIncs = 'sys/types.h netinet/in.h'; i=i+1;
aReq.i.msHdr = 'netinet\ip_mrout.h';    aReq.i.msIncs = 'sys/types.h netinet/in.h'; i=i+1;
aReq.i.msHdr = 'netinet\ip_mroute.h';   aReq.i.msIncs = 'sys/types.h netinet/in.h'; i=i+1;
aReq.i.msHdr = 'netinet\tcp.h';         aReq.i.msIncs = 'sys/types.h'; i=i+1;
aReq.i.msHdr = 'netinet\tcpip.h';       aReq.i.msIncs = 'sys/types.h netinet/in.h netinet/ip_var.h netinet/tcp.h'; i=i+1;
aReq.i.msHdr = 'netinet\tcp_var.h';     aReq.i.msIncs = 'sys/types.h'; i=i+1;
aReq.i.msHdr = 'netinet\udp.h';         aReq.i.msIncs = 'sys/types.h'; i=i+1;
aReq.i.msHdr = 'netinet\udp_var.h';     aReq.i.msIncs = 'sys/types.h netinet/udp.h netinet/in.h netinet/ip_var.h'; i=i+1;

aReq.i.msHdr = 'protocol\routed.h';     aReq.i.msIncs = 'sys/types.h sys/param.h sys/time.h sys/socket.h'; i=i+1;
aReq.i.msHdr = 'protocol\rwhod.h';      aReq.i.msIncs = 'sys/types.h sys/param.h sys/time.h'; i=i+1;
aReq.i.msHdr = 'protocol\talkd.h';      aReq.i.msIncs = 'sys/types.h sys/param.h sys/time.h sys/socket.h'; i=i+1;
aReq.i.msHdr = 'protocol\timed.h';      aReq.i.msIncs = 'sys/types.h sys/param.h sys/time.h'; i=i+1;
aReq.i.msHdr = 'protocols\routed.h';    aReq.i.msIncs = 'sys/types.h sys/param.h sys/time.h sys/socket.h'; i=i+1;
aReq.i.msHdr = 'protocols\rwhod.h';     aReq.i.msIncs = 'sys/types.h sys/param.h sys/time.h'; i=i+1;
aReq.i.msHdr = 'protocols\talkd.h';     aReq.i.msIncs = 'sys/types.h sys/param.h sys/time.h sys/socket.h'; i=i+1;
aReq.i.msHdr = 'protocols\timed.h';     aReq.i.msIncs = 'sys/types.h sys/param.h sys/time.h'; i=i+1;
aReq.i.msHdr = 'resolv.h';              aReq.i.msIncs = 'sys/socket.h netinet/in.h arpa/nameser.h'; i=i+1;
aReq.i.msHdr = 'sys\fmutex.h';          aReq.i.msIncs = 'sys/builtin.h'; i = i + 1;
aReq.i.msHdr = 'sys\moddef.h';          aReq.i.msIncs = 'stdio.h stdlib.h alloca.h errno.h string.h process.h io.h fcntl.h sys/types.h'; i=i+1;
aReq.i.msHdr = 'sys\rmutex.h';          aReq.i.msIncs = 'sys/builtin.h sys/fmutex.h'; i = i + 1;
aReq.i.msHdr = 'sys\smutex.h';          aReq.i.msIncs = 'sys/builtin.h'; i = i + 1;
aReq.i.msHdr = 'sys\un.h';              aReq.i.msIncs = 'sys/types.h'; i=i+1; /* TCPV40HDRS only */
aReq.i.msHdr = 'ternary.h';             aReq.i.msIncs = 'ansidecl.h'; i=i+1;
aReq.0 = i;

/*
 * Get files.
 */
asFiles.0 = 0;
if (sFile <> '') then
do
    rc = SysFileTree(sFile, 'asFiles', 'OSF');
    if (asFiles.0 = 0) then
        rc = SysFileTree(sInclude'\'sFile, 'asFiles', 'OSF');
end
else
    rc = SysFileTree(sInclude'\*.h', 'asFiles', 'OSF');
if (rc <> 0) then
do
    say 'error! SysFileTree failed with rc='rc'!';
    exit 8;
end
say 'info: Headers to test 'asFiles.0'...';


/*
 * Do testing.
 */
asFailed.0 = 0;
do i = 1 to asFiles.0
    call testfile sInclude, asFiles.i;
end

if (asFailed.0 > 0) then
do
    say 'The following files failed:'
    do i = 1 to asFailed.0
        say '  'asFailed.i;
    end
end

exit(asFailed.0);



/**
 * Test one file.
 * @returns 0
 * @param   sDir    Include directory (for the -I option and for basing sFile).
 * @param   sFile   The include file to test, full path.
 */
testfile: procedure expose asFailed. asExcludes. aReq.
parse arg sDir, sFile
    sName = substr(sFile, length(sDir) + 2);
    if (isExcluded(sName)) then
    do
        say 'info: skipping 'sName'...';
        return 0;
    end

    say 'info: testing 'sName'...';
    sTmp = '.\tmpfile.cpp';
    sTmpS = '.\tmpfile.s';
    call SysFileDelete sTmp;
    call writeReqs sTmp, sName;
    call lineout sTmp, '#include <'sName'>';
    call lineout sTmp, 'int main() {return 0;}';
    call lineout sTmp
    /*Address CMD 'type 'sTmp; */
    Address CMD 'gcc -S -O -Wall -I'sDir sTmp;
    if (rc = 0) then
        Address CMD 'gcc -S -DTCPV40HDRS -O -Wall -I'sDir sTmp;
    if (rc <> 0) then
    do
        j = asFailed.0 + 1;
        asFailed.j = sName;
        asFailed.0 = j;
    end
    call SysFileDelete sTmp;
    call SysFileDelete sTmpS;
return 0;


/**
 * Check if sFile is excluded from testing.
 */
isExcluded: procedure expose asExcludes.
parse arg sFile
    do i = 1 to asExcludes.0
        if (sFile = asExcludes.i) then
            return 1;
    end
return 0;


/**
 * Writes required code (#includes) for a given header.
 */
writeReqs: procedure expose aReq.
parse arg sOutput, sFile
    do i = 1 to aReq.0
        if (sFile = aReq.i.msHdr) then
        do
            do j = 1 to words(aReq.i.msIncs)
                call lineout sOutput, '#include <'word(aReq.i.msIncs, j)'>';
            end
            leave i;
        end
    end
return 0;
