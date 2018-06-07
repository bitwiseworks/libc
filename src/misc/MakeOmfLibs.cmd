/* $Id: MakeOmfLibs.cmd,v 1.8 2004/11/17 08:08:03 bird Exp $
 *
 * Generates the OMF libraries.
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of GCC.
 *
 * GCC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GCC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GCC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

    call hlpRexxUtilInit;
    signal on novalue name NoValueHandler_label_doesnt_exist_btw;

    /*
     * Figure out the usr/lib directory location.
     * Either as parameter or we assume this script is located there.
     */
    parse arg sDir sDummy;
    if (sDir = '') then
    do
        parse source . . sSrc
        sDir = filespec('drive', sSrc) || strip(filespec('path', sSrc), 'T', '\');
    end

    /*
     * Setup the environment
     */
    Address CMD '@setlocal';
    sEMXOMF = sDir'\..\bin\emxomf.exe';
    sOldBeginLibPath = hlpEnvGet('BEGINLIBPATH');
    call hlpEnvSet 'BEGINLIBPATH', sDir'..\lib;'sOldBeginLibPath
    if (hlpEnvGet('GCC_WEAKSYMS') <> '') then
        say 'warning: GCC_WEAKSYMS is set, sure you wanna use old weak handling?';

    /*
     * Do Da work.
     */
    call SysFileDelete sDir'\weaksyms.omf'; /* don't need this around! */
    iRc = doDir(sDir, sEMXOMF' -s');
    iRc2= doDir(sDir'\tcpipv4', sEMXOMF' -s');
    if (iRc2 > iRc) then
        iRc = iRc2;
    iRc2= doDir(sDir'\dbg', sEMXOMF);
    if (iRc2 > iRc) then
        iRc = iRc2;
    iRc2= doDir(sDir'\dbg\tcpipv4', sEMXOMF);
    if (iRc2 > iRc) then
        iRc = iRc2;
    iRc2= doDir(sDir'\gcc-lib\i386-pc-os2-emx\3.3.5', sEMXOMF' -s');
    if (iRc2 > iRc) then
        iRc = iRc2;

    /*
     * restore environment
     */
    call hlpEnvSet 'BEGINLIBPATH', sOldBeginLibPath
    Address CMD '@endlocal';

    if (iRc <> 0) then
        say 'warning: one or more operations failed, check the above output. :-)'.
exit(iRc);

/**
 * Process a given directory.
 */
doDir: procedure
parse arg sDir, sEMXOMF
    say 'info: processing directory 'sDir'...';
    iRc = 0;

    /* Is libstdc++.a in this directory, if so, do it first. */
    asLibs.0 = 0;
    rc = SysFileTree(sDir'\libstdc++.a', 'asLibs', 'OF');
    if (rc = 0 & asLibs.0 > 0) then
    do
        Address CMD sEMXOMF '-p128' asLibs.1;
        if (rc <> 0 & rc > iRc) then
            iRc = rc;
    end

    /* do all */
    asLibs.0 = 0;
    rc = SysFileTree(sDir'\*.a', 'asLibs', 'OF');
    if (rc = 0) then
    do
        do i = 1 to asLibs.0
            Address CMD sEMXOMF '-p128' asLibs.i;
            if (rc <> 0 & rc > iRc) then
                iRc = rc;
        end
    end
    else
    do
        say 'error: SysFileTree failed. rc='rc;
        iRc = 12;
    end

return iRc;

/**
 * Sets the value of sEnvVar to sValue.
 * @returns Old value of sEnvVar.
 * @returns '' if not found.
 * @param   sEnvVar     Environment variable to set.
 * @param   sValue      The new value.
 */
hlpEnvSet: procedure
parse arg sEnvVar, sValue
    parse upper source sRexxVer . .
    if (sRexxVer = 'OS/2') then
    do
        if ((translate(sEnvVar) = 'BEGINLIBPATH') | (translate(sEnvVar) = 'ENDLIBPATH')) then
            return SysSetExtLibPath(sValue, substr(sEnvVar, 1, 1));
        if (length(sValue) >= 1024) then
        do
            say 'Warning: 'sEnvVar' is too long,' length(sValue)' char.';
            say '    This may make CMD.EXE unstable after a SET operation to print the environment.';
        end
        sEnv = 'OS2ENVIRONMENT';
    end
    else
        sEnv = 'ENVIRONMENT';
return value(sEnvVar, sValue, sEnv);

/**
 * Gets the value of sEnvVar.
 * @returns Value of sEnvVar if found.
 * @returns '' if not found.
 * @param   sEnvVar     Environment variable to get.
 */
hlpEnvGet: procedure
parse arg sEnvVar
    parse upper source sRexxVer . .
    if (sRexxVer = 'OS/2') then
    do
        if ((translate(sEnvVar) = 'BEGINLIBPATH') | (translate(sEnvVar) = 'ENDLIBPATH')) then
            return SysQueryExtLibPath(substr(sEnvVar, 1, 1));
        sEnv = 'OS2ENVIRONMENT';
    end
    else
        sEnv = 'ENVIRONMENT';
return value(sEnvVar,, sEnv);

/**
 * Loads the RexxUtil dll.
 */
hlpRexxUtilInit: procedure
    if (RxFuncQuery('SysLoadFuncs') = 1) then
    do
        call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs';
        call SysLoadFuncs;
    end
return 0;


