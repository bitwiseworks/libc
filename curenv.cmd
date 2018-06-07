/* $Id: env.cmd,v 1.57 2005/12/18 13:25:50 bird Exp $
 *
 * GCC/2 Build Environment.
 *
 * InnoTek Systemberatung GmbHconfidential
 *
 * Copyright (c) 1999-2005 Knut St. Osmundsen
 * Copyright (c) 2003-2004 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird@anduin.net>
 *
 * All Rights Reserved
 *
 */

    /*
     * Setup the usual stuff...
     */
    Address CMD '@echo off';
    signal on novalue name NoValueHandler
    if (RxFuncQuery('SysLoadFuncs') = 1) then
    do
        call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs';
        call SysLoadFuncs;
    end

    /*
     * Constants.
     */
    asBuildModes.0 = 2;
    asBuildModes.1 = 'DEBUG';
    asBuildModes.2 = 'RELEASE';
    asStages.0 = 3
    asStages.1 = 'stage1'
    asStages.2 = 'stage2'
    asStages.3 = 'stage3'

    /*
     * Apply CMD.EXE workaround.
     */
    call FixCMDEnv;


    /*
     * Parse argument
     */
    parse arg sArgs
    fRm = 0;
    fUseStagedGcc = 1;
    sBuildMode = 'DEBUG';
    do i = 1 to words(sArgs)
        /* extract word */
        sArg = word(sArgs, i)

        /* */
        if (left(sArg, 1) = '/' | (left(sArg, 1) = '-')) then
        do
            sArg = strip(strip(sArg, 'L', '/'), 'L', '-');
            select
                when (sArg = 'disable-staged-gcc') then
                    fUseStagedGcc = 0;
                when (sArg = 'enable-staged-gcc') then
                    fUseStagedGcc = 1;

                when (sArg = 'uninstall') then
                    fRM = 1;
                when (sArg = 'install') then
                    fRM = 0;

                otherwise
                do
                    call syntax;
                    exit(12);
                end
            end
        end
        else
            sBuildMode = translate(sArg);
    end

    if (sBuildMode <> 'RELEASE' & sBuildMode <> 'DEBUG') then
    do
        call syntax;
        exit(12);
    end


    /*
     * Figure out our location which will give us the place of the tools
     * directory.
     */
    parse source . . sSrc
    sPathTools  = filespec('drive', sSrc) || strip(filespec('path', sSrc), 'T', '\') || '\tools';
    sPathToolsF = translate(sPathTools, '/', '\');
    sPathToolsFD= translate(strip(filespec('path', sSrc), 'T', '\'), '/', '\') || '\tools';
    sPathRoot   = filespec('drive', sPathTools) || strip(filespec('path', sPathTools), 'T', '\');
    sPathRootF  = translate(sPathRoot, '/', '\');

    /*
     * Setup the environment.
     */
    /* cleanup */
    do i = 1 to asBuildModes.0
        sMode = asBuildModes.i;
        call EnvAddFront 1, 'PATH',                 sPathRoot'\obj\OS2\'sMode'\builtunix\bin;'
        call EnvAddFront 1, 'PATH',                 sPathRoot'\obj\OS2\'sMode'\builtunix\usr\omfhackbin;'
        call EnvAddFront 1, 'PATH',                 sPathRoot'\obj\OS2\'sMode'\builtunix\usr\bin;'
        call EnvAddFront 1, 'PATH',                 sPathRootF'/obj/OS2/'sMode'/builtunix/bin;'
        call EnvAddFront 1, 'PATH',                 sPathRootF'/obj/OS2/'sMode'/builtunix/usr/bin;'
        call EnvAddFront 1, 'C_INCLUDE_PATH',       sPathRootF'/obj/OS2/'sMode'/builtunix/usr/include;'
        call EnvAddFront 1, 'CPLUS_INCLUDE_PATH',   sPathRootF'/obj/OS2/'sMode'/builtunix/usr/include;'
        call EnvAddFront 1, 'COBJ_INCLUDE_PATH',    sPathRootF'/obj/OS2/'sMode'/builtunix/usr/include;'
        call EnvAddFront 1, 'LIBRARY_PATH',         sPathRootF'/obj/OS2/'sMode'/builtunix/lib;'
        call EnvAddFront 1, 'LIBRARY_PATH',         sPathRootF'/obj/OS2/'sMode'/builtunix/usr/lib;'
        call EnvAddFront 1, 'LIBRARY_PATH',         sPathRootF'/obj/OS2/'sMode'/builtunix/usr/lib/gcc-lib/i386-pc-os2-emx/3.3.5;'
        do j = 1 to asStages.0
            /* #424: no mt or st directories.
             */
            call EnvAddFront 1,'BEGINLIBPATH',      sPathRoot'\obj\OS2\'sMode'\gcc\gcc\'asStages.j'\mt;'sPathRoot'\obj\OS2\'sMode'\gcc\gcc\'asStages.j'\st;'
            call EnvAddFront 1,'BEGINLIBPATH',      sPathRoot'\obj\OS2\'sMode'\gcc\gcc\'asStages.j';'
        end
        call EnvAddFront 1,  'BEGINLIBPATH',        sPathRoot'\obj\OS2\'sMode'\builtunix\lib;'
        call EnvAddFront 1,  'BEGINLIBPATH',        sPathRoot'\obj\OS2\'sMode'\builtunix\usr\lib;'
    end

    /* constants (more) */
    sPathObj  = sPathRoot'\obj\OS2\'sBuildMode
    sPathObjF = sPathRootF'/obj/OS2/'sBuildMode
    sPathBin  = sPathRoot'\bin\OS2\'sBuildMode
    sPathBinF = sPathRootF'/bin/OS2/'sBuildMode

    do i = 1 to asStages.0
        /* #424: no mt or st directories.
         * call EnvAddFront fRm,'BEGINLIBPATH',    sPathObj'\gcc\gcc\'asStages.i'\mt;'sPathObj'\gcc\gcc\'asStages.i'\st;'
         */
        call EnvAddFront fRm,'BEGINLIBPATH',    sPathObj'\gcc\gcc\'asStages.i';'
    end

    /* build tools and 1stage compiler? */
    call EnvSet      fRm, 'PATH_BUILTUNIX',     sPathObjF'/builtunix'
    call EnvAddFront fRm, 'PATH',               sPathObjF'/builtunix/usr/bin;'
    call EnvAddFront fRm, 'PATH',               sPathObj'\builtunix\usr\bin;'
    call EnvAddFront fRm, 'PATH',               sPathObjF'/builtunix/bin;'
    call EnvAddFront fRm, 'PATH',               sPathObj'\builtunix\bin;'
    call EnvAddFront fRm, 'PATH',               sPathObj'\builtunix/usr\omfhackbin;'
    call EnvAddFront fRm, 'BEGINLIBPATH',       sPathObj'\builtunix\usr\lib;'
    call EnvAddFront fRm, 'C_INCLUDE_PATH',     sPathObjF'/builtunix/usr/include;'
    call EnvAddFront fRm, 'CPLUS_INCLUDE_PATH', sPathObjF'/builtunix/usr/include;'
    call EnvAddFront fRm, 'OBJC_INCLUDE_PATH',  sPathObjF'/builtunix/usr/include;'
    call EnvAddFront fRm, 'LIBRARY_PATH',       sPathObjF'/builtunix/usr/lib;'
    call EnvAddFront fRm, 'LIBRARY_PATH',       sPathObjF'/builtunix/usr/lib/gcc-lib/i386-pc-os2-emx/3.3.5;'

    if (\fRm) then
    do
        sTmp = EnvGet('LIBRARY_PATH');
        iPos = pos(';;', sTmp);
        do while (iPos > 0)
            sTmp = left(sTmp, iPos - 1)||substr(sTmp, iPos+1);
            iPos = pos(';;', sTmp);
        end
        sTmp = strip(sTmp, 'B', ';');
        call EnvSet fRm, 'LIBRARY_PATH', sTmp;
        drop sTmp;
    end


    /*
     * Check for pitfalls....
     */
    rc = 0;
    sHome = EnvGet('HOME');
    if (sHome <> '') then
    do
        sHome = translate(sHome, '\', '/');
        if (FileExists(sHome'\.bashrc')) then
        do
            say 'warning: HOME includes a .bashrc, pray you have no weird aliases...'
            if (rc < 4) then rc = 4;
        end
    end
    else
    do
        say 'warning: no home path!?!'
        if (rc < 4) then rc = 4;
    end

exit(rc);


/*******************************************************************************
*   Procedure Section                                                          *
*******************************************************************************/

/**
 * Give the script syntax
 */
syntax: procedure
    say 'syntax: env.cmd [options] [mode]'
    say ''
    say 'Mode:'
    say '    The build mode, DEBUG or RELEASE. Default it DEBUG.'
    say ''
    say 'Options:'
    say '    enable-staged-gcc      Enable the staged GCC build. (default)'
    say '    disable-staged-gcc     Use buildenv gcc335.'
    say ''
return 0;


/**
 * No value handler
 */
NoValueHandler:
    say 'NoValueHandler: line 'SIGL;
exit(16);



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
 * Checks if a file exists.
 * @param   sFile       Name of the file to look for.
 * @param   sComplain   Complaint text. Complain if non empty and not found.
 * @returns TRUE if file exists.
 *          FALSE if file doesn't exists.
 */
FileExists: procedure
    parse arg sFile, sComplain
    rc = stream(sFile, 'c', 'query exist');
    if ((rc = '') & (sComplain <> '')) then
        say sComplain ''''sFile'''.';
return rc <> '';


/**
 * Checks if a directory exists.
 * @param   sDir        Name of the directory to look for.
 * @param   sComplain   Complaint text. Complain if non empty and not found.
 * @returns TRUE if file exists.
 *          FALSE if file doesn't exists.
 */
DirExists: procedure
    parse arg sDir, sComplain
    rc = SysFileTree(sDir, 'sDirs', 'DO');
    if (rc = 0 & sDirs.0 = 1) then
        return 1;
    if (sComplain <> '') then do
        say sComplain ''''sDir'''.';
return 0;



/*
 * EMX/GCC 3.x.x - this environment must be used 'on' the ordinary EMX.
 * Note! bin.new has been renamed to bin!
 * Note! make .lib of every .a! in 4OS2: for /R %i in (*.a) do if not exist %@NAME[%i].lib emxomf %i
 */
GCC3xx: procedure expose aCfg. aPath. sPathFile sPathTools sPathToolsF
    parse arg sToolId,sOperation,fRM,fQuiet,sPathId

    /*
     * EMX/GCC main directory.
     */
    /*sGCC = PathQuery(sPathId, sToolId, sOperation);
    if (sGCC = '') then
        return 1;
    /* If config operation we're done now. */
    if (pos('config', sOperation) > 0) then
        return 0;
    */
    sGCC = sPathTools'\x86.os2\gcc\staged'
    sGCCBack    = translate(sGCC, '\', '/');
    sGCCForw    = translate(sGCC, '/', '\');
    chMajor     = '3';
    chMinor     = left(right(sToolId, 2), 1);
    chRel       = right(sToolId, 1);
    sVer        = chMajor'.'chMinor'.'chRel

    call EnvSet      fRM, 'PATH_IGCC',      sGCCBack;
    call EnvSet      fRM, 'CCENV',          'IGCC'
    call EnvSet      fRM, 'BUILD_ENV',      'IGCC'
    call EnvSet      fRM, 'BUILD_PLATFORM', 'OS2'

    call EnvAddFront fRM, 'BEGINLIBPATH',       sGCCBack'\lib;'
    call EnvAddFront fRM, 'PATH',               sGCCForw'/bin;'sGCCBack'\bin;'
    call EnvAddFront fRM, 'C_INCLUDE_PATH',     sGCCForw'/include'
    call EnvAddFront fRM, 'LIBRARY_PATH',       sGCCForw'/lib/gcc-lib/i386-pc-os2-emx/'sVer';'sGCCForw'/lib;'sGCCForw'/lib/mt;'
    call EnvAddFront fRm, 'CPLUS_INCLUDE_PATH', sGCCForw'/include;'
    call EnvAddFront fRm, 'CPLUS_INCLUDE_PATH', sGCCForw'/include/c++/'sVer'/backward;'
    call EnvAddFront fRm, 'CPLUS_INCLUDE_PATH', sGCCForw'/include/c++/'sVer'/i386-pc-os2-emx;'
    call EnvAddFront fRm, 'CPLUS_INCLUDE_PATH', sGCCForw'/include/c++/'sVer';'
    call EnvSet      fRM, 'PROTODIR',           sGCCForw'/include/cpp/gen'
    call EnvSet      fRM, 'OBJC_INCLUDE_PATH',  sGCCForw'/include'
    call EnvAddFront fRM, 'INFOPATH',           sGCCForw'/info'
    call EnvSet      fRM, 'EMXBOOK',            'emxdev.inf+emxlib.inf+emxgnu.inf+emxbsd.inf'
    call EnvAddFront fRM, 'HELPNDX',            'emxbook.ndx', '+', 1

    /*
     * Verify.
     */
    if (pos('verify', sOperation) <= 0) then
        return 0;

    if (rc = 0) then
        rc = CheckCmdOutput('g++ --version', 0, fQuiet, sVer);
    if (rc = 0) then
    do
        sVerAS = '2.14';
        rc = CheckCmdOutput('as --version', 0, fQuiet, 'GNU assembler 'sVerAS);
    end
return rc;



/**
 *  Workaround for bug in CMD.EXE.
 *  It messes up when REXX have expanded the environment.
 */
FixCMDEnv: procedure
/* do this anyway
    /* check for 4OS2 first */
    Address CMD 'set 4os2test_env=%@eval[2 + 2]';
    if (value('4os2test_env',, 'OS2ENVIRONMENT') = '4') then
        return 0;
*/

    /* force environment expansion by setting a lot of variables and freeing them.
     * ~6600 (bytes) */
    do i = 1 to 100
        Address CMD '@set dummyenvvar'||i'=abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
    end
    do i = 1 to 100
        Address CMD '@set dummyenvvar'||i'=';
    end
return 0;

