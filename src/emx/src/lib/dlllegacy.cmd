/* $Id: dlllegacy.cmd 784 2003-10-02 01:11:55Z bird $
 *
 * REXX script for handling legacy we create with the libc dll.
 * It's to make sure that we preserve old ordinals.
 * Both input files may be updated.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */

/*
 * REXX stuff.
 */
signal on novalue name NoValueHandler
call hlpRexxUtilInit;


/*
 * Arguments and such.
 */
sDefFile    = '';
sLegacyFile = '';
sExcludes   = ';';


/*
 * Loop thru arguments and process them one by one.
 */
parse arg sArgs
sArgs = strip(sArgs);
do while (sArgs <> '')
    sArg = NextArg();
    if (left(sArg, 1) = '-') then
    do
        ch = translate(substr(sArg, 2, 1));
        select
            when (ch = 'E') then
                sExcludes = sExcludes || ArgValue(sArg) || ';';

            otherwise
            do
                say 'syntax error: Invalid argument '''sArg'''.';
                exit(8);
            end
        end
    end
    else
    do
        if (sDefFile = '') then
            sDefFile = translate(sArg, '\', '/');
        else if (sLegacyFile = '') then
            sLegacyFile = translate(sArg, '\', '/');
        else
        do
            say 'syntax error: Too many filenames.';
            exit(8);
        end
    end
end


/*
 * Did we get two files?
 */
if (sLegacyFile = '') then
do
    say 'syntax error: Two filenames are required!';
    exit(8);
end


/*
 * Read the legacy file first (skip to exports).
 * ASSUMES that EXPORTS is the LAST section in the file!!!!!
 */
aResult.0 = 0;
fInExports = 0;
iLastOrdinal = 0;
do while (lines(sLegacyFile) > 0)
    sLine = strip(linein(sLegacyFile));

    /* strip of comments and such. */
    iComment = pos(';', sLine);
    if (iComment > 0) then
        sLine = strip(substr(sLine, 1, iComment - 1));
    if (sLine = '') then
        iterate;

    /* process the line */
    if (fInExports) then
    do
        sWordLine = sLine;

        i = aResult.0 + 1;
        aResult.i.mfFound = 0;
        aResult.i.msSymbol = NextWord();
        aResult.i.msIntSymbol = '';
        s = NextWord();
        if (s = '=') then
        do
            aResult.i.msIntSymbol = NextWord();
            s = NextWord();
        end

        aResult.i.miOrdinal = 0;
        if (left(s, 1) = '@') then
        do
            aResult.i.miOrdinal = substr(s, 2);
            if (aResult.i.miOrdinal > iLastOrdinal) then
                iLastOrdinal = aResult.i.miOrdinal;
            s = NextWord();
        end
        else
        do
            iLastOrdinal = iLastOrdinal + 1;
            iOrdinal = iLastOrdinal;
        end
        aResult.i.msRest = s;
        if (sLine <> '') then
            aResult.i.msRest = aResult.i.msRest || ' ' || sLine;

        if (pos(';'aResult.i.msSymbol';', sExcludes) <= 0) then
            aResult.0 = i;
    end
    else if (translate(word(sLine, 1)) = 'EXPORTS') then
        fInExports = 1;
end
call stream sLegacyFile, 'c', 'close';
cLegacy = aResult.0;



/*
 * Now we must read the def file.
 */
asHeader.0 = 0;
fInExports = 0;
do while (lines(sDefFile) > 0)
    sLine = strip(linein(sDefFile));

    /* strip of comments and such. */
    iComment = pos(';', sLine);
    if (iComment > 0) then
        sLine = strip(substr(sLine, 1, iComment - 1));


    /* process the line */
    if (fInExports) then
    do
        if (sLine = '' | left(sLine, 1) = ';') then
            iterate;

        sWordLine = sLine;
        sSymbol = NextWord();
        sIntSymbol = '';
        s = NextWord();
        if (s = '=') then
        do
            sIntSymbol = NextWord();
            s = NextWord();
        end
        iOrdinal = 0;
        if (left(s, 1) = '@') then
        do
            iOrdinal = substr(s, 2);
            /*if (iOrdinal > iLastOrdinal) then
                iLastOrdinal = iOrdinal; */
            s = NextWord();
        end
        sRest = s;
        if (sLine <> '') then
            sRest = sRest || ' ' || sLine;

        /* Excluded? */
        if (pos(';'sSymbol';', sExcludes) > 0) then
            iterate;

        /*
         * Now we must look for this export in the legacy
         * If not found we must add it to the legacy file.
         */
        fFound = 0;
        do j = 1 to cLegacy
            if (aResult.j.msSymbol = sSymbol) then
            do
                aResult.j.mfFound = 1;
                fFound = 1;
                leave;
            end
        end
        if (\fFound) then
        do
            /*if (iOrdinal = 0) then */
            do
                iLastOrdinal = iLastOrdinal + 1;
                iOrdinal = iLastOrdinal;
            end
            j = aResult.0 + 1;
            aResult.j.mfFound = 1;
            aResult.j.msSymbol = sSymbol;
            aResult.j.msIntSymbol = sIntSymbol;
            aResult.j.miOrdinal = iOrdinal;
            aResult.j.msRest = sRest;
            aResult.0 = j;
        end
    end
    else
    do
        /* header line */
        if (translate(word(sLine, 1)) = 'EXPORTS') then
            fInExports = 1;
        i = asHeader.0 + 1;
        asHeader.i = sLine;
        asHeader.0 = i
    end
end
call stream sDefFile, 'c', 'close';


/*
 * Now update the legacy file if required.
 */
if (cLegacy < aResult.0) then
do
    if (stream(sLegacyFile, 'c', 'query exist') = '') then
        call lineout sLegacyFile, 'EXPORTS';
    do i = cLegacy + 1 to aResult.0
        sLine = '    "'aResult.i.msSymbol'"';
        if (aResult.i.msIntSymbol <> '') then
            sLine = sLine' = "'aResult.i.msIntSymbol'"';
        if (aResult.i.miOrdinal > 0) then
            sLine = sLine' @'aResult.i.miOrdinal;
        if (aResult.i.msRest > 0) then
            sLine = sLine' 'aResult.i.msRest;
        call lineout sLegacyFile, sLine;
    end
    call stream sLegacyFile, 'c', 'close';
end


/*
 * Rewrite the def file.
 */
call SysFileDelete sDefFile;
do i = 1 to asHeader.0
    call lineout sDefFile, asHeader.i;
end
do i = 1 to aResult.0
    sLine = '    "'aResult.i.msSymbol'"';
    if (aResult.i.msIntSymbol <> '') then
        sLine = sLine' = "'aResult.i.msIntSymbol'"';
    if (aResult.i.miOrdinal > 0) then
        sLine = sLine' @'aResult.i.miOrdinal;
    if (aResult.i.msRest > 0) then
        sLine = sLine' 'aResult.i.msRest;
    call lineout sDefFile, sLine;
end
call stream sDefFile, 'c', 'close';

exit 0;



/**
 * Gets the next argument from sArgs.
 * @returns Next argument.
 */
NextArg: procedure expose sArgs
    /*
     * Exctract the first word.
     */
    ch = left(sArgs, 1);
    if (pos(ch, '"''`') > 0) then
    do  /* quoted */
        iEndQuote = pos(ch, sArgs, 2);
        if (iEndQuote <= 0) then
        do
            say 'error: invalid quoting!'
            exit(8);
        end
        if (iEndQuote > 2) then
            sArg = substr(sArgs, 2, iEndQuote - 2);
        else
            sArg = '';
        sArgs = strip(substr(sArgs, iEndQuote + 1));
    end
    else
    do
        sArg = word(sArgs, 1);
        sArgs = strip(substr(sArgs, length(sArg) + 1));
    end
return sArg;


/**
 * Gets the argument value, which is either following in third position
 * or as next argument.
 * @returns Value of current argument.
 */
ArgValue: procedure expose sArgs
parse arg sArg
    if (length(sArg) > 2) then
        return substr(sArg, 3);
    sValue = '';
    if (sArgs <> '') then
        sValue = NextArg();
    if (sValue = '') then
    do
        say 'syntax error: missing argument value for '''sArg'''.';
        exit(8);
    end
return sValue;


/**
 * Gets the word from sWordLine. (respect quoting)
 * @returns Next word.
 */
NextWord: procedure expose sWordLine
    if (sWordLine = '') then
        return '';
    /*
     * Exctract the first word.
     */
    ch = left(sWordLine, 1);
    if (pos(ch, '"''`') > 0) then
    do  /* quoted */
        iEndQuote = pos(ch, sWordLine, 2);
        if (iEndQuote <= 0) then
        do
            say 'error: invalid quoting!'
            exit(8);
        end
        if (iEndQuote > 2) then
            sWord = substr(sWordLine, 2, iEndQuote - 2);
        else
            sWord = '';
        sWordLine = strip(substr(sWordLine, iEndQuote + 1));
    end
    else
    do
        sWord = word(sWordLine, 1);
        sWordLine = strip(substr(sWordLine, length(sWord) + 1));
    end
return sWord;


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


/**
 * No value handler
 */
NoValueHandler:
    say 'NoValueHandler: line 'SIGL;
    if (sDir <> '') then
        call directory sSavedDir;
exit(16);

