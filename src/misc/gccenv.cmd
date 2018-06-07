/* $Id: gccenv.cmd 3376 2007-06-05 21:05:46Z bird $
 *
 * GCC environment script.
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with This program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * Typical init.
 */
signal on novalue name NoValueHandler
if (RxFuncQuery('SysLoadFuncs') = 1) then
do
    call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs';
    call SysLoadFuncs;
end
call FixCMDEnv

/*
 * Parse any args.
 */
parse arg sPath sLinker sDummy
if (sPath = '') then
do
    /* Default is that this script resides in /usr/bin/. */
    parse source . . sSrc
    sPath = filespec('drive', sSrc)||strip(filespec('path', strip(filespec('path', sSrc), 'T', '\')), 'T', '\');
end

sLinker = translate(sLinker);
if (sLinker <> '' & sLinker <> 'VAC308' & sLinker <> 'VAC365' & sLinker <> 'LINK386' & sLinker <> 'WLINK') then
do
    say 'Syntax error: Invalid linker '''sLinker''' specified.';
    say ''
    call Usage;
    exit 8;
end

if (stream(sPath'\bin\gcc.exe', 'c', 'query exist') = '') then
do
    say 'Error: cannot find gcc.exe in '''sPath'\usr''!';
    say ''
    call Usage;
    exit 8;
end


/*
 * Do work.
 */
call GCC322plus sPath, 'gcc335', 0, sLinker;
exit 0;


/**
 * Display usage info for script.
 */
Usage: procedure
    say 'syntax: gccenv.cmd [gcc-usr-path [linker]]';
    say ''
    say '[gcc-usr-path] is the path to the /usr directory of the gcc distro.'
    say 'If [gcc-usr-path] isn''t specified, the script assumes it''s located'
    say 'in /usr/bin of the gcc distro and deducts the path based on that.'
    say ''
    say '[linker] is the EMXOMFLD_TYPE as described by emxomfld.exe usage. It''s'
    say 'thus either LINK386, VAC308 or VAC365.'
    say ''
    say 'Copyright (c) 2003 InnoTek Systemberatung GmbH'
    say ''
return 0;



/**
 * Innotek GCC 3.2.x and higher - this environment is EMX RT free.
 * @param   sPath       The path of the /usr directory in the GCC distro.
 * @param   sToolId     Tool identificator. gcc335
 * @param   fRM         If set we'll remove the environment setup.
 *                      If clear we'll install it.
 * @param   sLinker     Which linker to use. If blank use emxomfld default linker.
 */
GCC322plus: procedure
    parse arg sPath, sToolId, fRM, sLinker

    /*
     * EMX/GCC main directory.
     */

    /* parse out the version / constants */
    chMajor = '3';
    chMinor = left(right(sToolId, 2), 1);
    chRel   = right(sToolId, 1);
    sVer    = chMajor'.'chMinor'.'chRel
    sVerShrt= chMajor||chMinor||chRel;
    sTrgt   = 'i386-pc-os2-emx'

    sGCCBack    = translate(sPath, '\', '/');
    sGCCForw    = translate(sPath, '/', '\');
    call EnvSet      fRM, 'PATH_EMXPGCC',   sGCCBack;
    call EnvSet      fRM, 'CCENV',          'EMX'
    call EnvSet      fRM, 'BUILD_ENV',      'EMX'
    call EnvSet      fRM, 'BUILD_PLATFORM', 'OS2'

    call EnvAddFront fRM, 'BEGINLIBPATH',       sGCCBack'\'sTrgt'\lib;'sGCCBack'\lib;'
    call EnvAddFront fRM, 'DPATH',              sGCCBack'\lib;'
    /*call EnvAddFront fRM, 'HELP',               sGCCBack'\lib;'*/
    call EnvAddFront fRM, 'PATH',               sGCCBack'\'sTrgt'\bin;'sGCCBack'\bin;'
    /*call EnvAddFront fRM, 'DPATH',              sGCCBack'\book;'
    call EnvAddFront fRM, 'BOOKSHELF',          sGCCBack'\book;'
    call EnvAddFront fRM, 'HELP',               sGCCBack'\help;' */
    call EnvAddFront fRM, 'C_INCLUDE_PATH',     sGCCForw'/include;'
    call EnvAddFront fRM, 'C_INCLUDE_PATH',     sGCCForw'/lib/gcc-lib/'sTrgt'/'sVer'/include;'
    call EnvAddFront fRM, 'C_INCLUDE_PATH',     sGCCForw'/lib/gcc-lib/'sTrgt'/'sVer'/include;'
    call EnvAddFront fRM, 'CPLUS_INCLUDE_PATH', sGCCForw'/include;'
    call EnvAddFront fRM, 'CPLUS_INCLUDE_PATH', sGCCForw'/include/c++/'sVer'/backward;'
    call EnvAddFront fRM, 'CPLUS_INCLUDE_PATH', sGCCForw'/include/c++/'sVer'/'sTrgt';'
    call EnvAddFront fRM, 'CPLUS_INCLUDE_PATH', sGCCForw'/include/c++/'sVer'/;'
    call EnvAddFront fRM, 'LIBRARY_PATH',       sGCCForw'/lib'
    call EnvAddFront fRM, 'LIBRARY_PATH',       sGCCForw'/lib/gcc-lib/'sTrgt'/'sVer';'
    call EnvAddFront fRM, 'INFOPATH',           sGCCForw'/info'
    /* is this used? */
    call EnvSet      fRM, 'PROTODIR',           sGCCForw'/include/c++/gen'

    /* use wlink? */
    if (sLinker = 'WLINK') then
    do
        call EnvSet  fRM, 'EMXOMFLD_TYPE',      'WLINK';
        call EnvSet  fRM, 'EMXOMFLD_LINKER',    'wl.exe';
        /* call EnvSet  fRM, 'EMXOMFLD_LINKER',    'wl.exe'; - comment this in if you use an official OpenWatcom wlink. */
    end

    /* use link386? */
    if (sLinker = 'LINK386') then
    do
        call EnvSet  fRM, 'EMXOMFLD_TYPE',      'LINK386';
        call EnvSet  fRM, 'EMXOMFLD_LINKER',    'link386.exe';
    end

    /* use vac308? */
    if (sLinker = 'VAC308') then
    do
        call EnvSet  fRM, 'EMXOMFLD_TYPE',      'VAC308';
        call EnvSet  fRM, 'EMXOMFLD_LINKER',    'ilink.exe';
    end

    /* use default or VAC365? */
    if (sLinker = '' | sLinker = 'VAC365') then
    do
        call EnvSet  1, 'EMXOMFLD_TYPE';
        call EnvSet  1, 'EMXOMFLD_LINKER';
    end

return 0;



/**
 * Add sToAdd in front of sEnvVar.
 * Note: sToAdd now is allowed to be alist!
 *
 * Known features: Don't remove sToAdd from original value if sToAdd
 *                 is at the end and don't end with a ';'.
 */
EnvAddFront: procedure
    parse arg fRM, sEnvVar, sToAdd, sSeparator

    /* sets default separator if not specified. */
    if (sSeparator = '') then sSeparator = ';';

    /* checks that sToAdd ends with an ';'. Adds one if not. */
    if (substr(sToAdd, length(sToAdd), 1) <> sSeparator) then
        sToAdd = sToAdd || sSeparator;

    /* check and evt. remove ';' at start of sToAdd */
    if (substr(sToAdd, 1, 1) = ';') then
        sToAdd = substr(sToAdd, 2);

    /* loop thru sToAdd */
    rc = 0;
    i = length(sToAdd);
    do while i > 1 & rc = 0
        j = lastpos(sSeparator, sToAdd, i-1);
        rc = EnvAddFront2(fRM, sEnvVar, substr(sToAdd, j+1, i - j), sSeparator);
        i = j;
    end

return rc;

/**
 * Add sToAdd in front of sEnvVar.
 *
 * Known features: Don't remove sToAdd from original value if sToAdd
 *                 is at the end and don't end with a ';'.
 */
EnvAddFront2: procedure
    parse arg fRM, sEnvVar, sToAdd, sSeparator

    /* sets default separator if not specified. */
    if (sSeparator = '') then sSeparator = ';';

    /* checks that sToAdd ends with a separator. Adds one if not. */
    if (substr(sToAdd, length(sToAdd), 1) <> sSeparator) then
        sToAdd = sToAdd || sSeparator;

    /* check and evt. remove the separator at start of sToAdd */
    if (substr(sToAdd, 1, 1) = sSeparator) then
        sToAdd = substr(sToAdd, 2);

    /* Get original variable value */
    sOrgEnvVar = EnvGet(sEnvVar);

    /* Remove previously sToAdd if exists. (Changing sOrgEnvVar). */
    i = pos(translate(sToAdd), translate(sOrgEnvVar));
    if (i > 0) then
        sOrgEnvVar = substr(sOrgEnvVar, 1, i-1) || substr(sOrgEnvVar, i + length(sToAdd));

    /* set environment */
    if (fRM) then
        return EnvSet(0, sEnvVar, sOrgEnvVar);
return EnvSet(0, sEnvVar, sToAdd||sOrgEnvVar);


/**
 * Add sToAdd as the end of sEnvVar.
 * Note: sToAdd now is allowed to be alist!
 *
 * Known features: Don't remove sToAdd from original value if sToAdd
 *                 is at the end and don't end with a ';'.
 */
EnvAddEnd: procedure
    parse arg fRM, sEnvVar, sToAdd, sSeparator

    /* sets default separator if not specified. */
    if (sSeparator = '') then sSeparator = ';';

    /* checks that sToAdd ends with a separator. Adds one if not. */
    if (substr(sToAdd, length(sToAdd), 1) <> sSeparator) then
        sToAdd = sToAdd || sSeparator;

    /* check and evt. remove ';' at start of sToAdd */
    if (substr(sToAdd, 1, 1) = sSeparator) then
        sToAdd = substr(sToAdd, 2);

    /* loop thru sToAdd */
    rc = 0;
    i = length(sToAdd);
    do while i > 1 & rc = 0
        j = lastpos(sSeparator, sToAdd, i-1);
        rc = EnvAddEnd2(fRM, sEnvVar, substr(sToAdd, j+1, i - j), sSeparator);
        i = j;
    end

return rc;

/**
 * Add sToAdd as the end of sEnvVar.
 *
 * Known features: Don't remove sToAdd from original value if sToAdd
 *                 is at the end and don't end with a ';'.
 */
EnvAddEnd2: procedure
    parse arg fRM, sEnvVar, sToAdd, sSeparator

    /* sets default separator if not specified. */
    if (sSeparator = '') then sSeparator = ';';

    /* checks that sToAdd ends with a separator. Adds one if not. */
    if (substr(sToAdd, length(sToAdd), 1) <> sSeparator) then
        sToAdd = sToAdd || sSeparator;

    /* check and evt. remove separator at start of sToAdd */
    if (substr(sToAdd, 1, 1) = sSeparator) then
        sToAdd = substr(sToAdd, 2);

    /* Get original variable value */
    sOrgEnvVar = EnvGet(sEnvVar);

    if (sOrgEnvVar <> '') then
    do
        /* Remove previously sToAdd if exists. (Changing sOrgEnvVar). */
        i = pos(translate(sToAdd), translate(sOrgEnvVar));
        if (i > 0) then
            sOrgEnvVar = substr(sOrgEnvVar, 1, i-1) || substr(sOrgEnvVar, i + length(sToAdd));

        /* checks that sOrgEnvVar ends with a separator. Adds one if not. */
        if (sOrgEnvVar = '') then
            if (right(sOrgEnvVar,1) <> sSeparator) then
                sOrgEnvVar = sOrgEnvVar || sSeparator;
    end

    /* set environment */
    if (fRM) then return EnvSet(0, sEnvVar, sOrgEnvVar);
return EnvSet(0, sEnvVar, sOrgEnvVar||sToAdd);


/**
 * Sets sEnvVar to sValue.
 */
EnvSet: procedure
    parse arg fRM, sEnvVar, sValue

    /* if we're to remove this, make valuestring empty! */
    if (fRM) then
        sValue = '';
    sEnvVar = translate(sEnvVar);

    /*
     * Begin/EndLibpath fix:
     *      We'll have to set internal these using both commandline 'SET'
     *      and internal VALUE in order to export it and to be able to
     *      get it (with EnvGet) again.
     */
    if ((sEnvVar = 'BEGINLIBPATH') | (sEnvVar = 'ENDLIBPATH')) then
    do
        if (length(sValue) >= 1024) then
            say 'Warning: 'sEnvVar' is too long,' length(sValue)' char.';
        return SysSetExtLibPath(sValue, substr(sEnvVar, 1, 1));
    end

    if (length(sValue) >= 1024) then
    do
        say 'Warning: 'sEnvVar' is too long,' length(sValue)' char.';
        say '    This may make CMD.EXE unstable after a SET operation to print the environment.';
    end
    sRc = VALUE(sEnvVar, sValue, 'OS2ENVIRONMENT');
return 0;

/**
 * Gets the value of sEnvVar.
 */
EnvGet: procedure
    parse arg sEnvVar
    if ((translate(sEnvVar) = 'BEGINLIBPATH') | (translate(sEnvVar) = 'ENDLIBPATH')) then
        return SysQueryExtLibPath(substr(sEnvVar, 1, 1));
return value(sEnvVar,, 'OS2ENVIRONMENT');


/**
 *  Workaround for bug in CMD.EXE.
 *  It messes up when REXX have expanded the environment.
 */
FixCMDEnv: procedure
    /* check for 4OS2 first */
    Address CMD '@set 4os2test_env=%@eval[2 + 2]';
    if (value('4os2test_env',, 'OS2ENVIRONMENT') = '4') then
        return 0;

    /* force environment expansion by setting a lot of variables and freeing them. */
    do i = 1 to 100
        Address CMD '@set dummyenvvar'||i'=abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
    end
    do i = 1 to 100
        Address CMD '@set dummyenvvar'||i'=';
    end
return 0;

