/* $Id: statshdr.cmd 660 2003-09-07 20:59:28Z bird $
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

parse arg sInclude sOther
sFile = '';
if (sInclude = '') then
do
    say 'syntax: statshdrs <includedir>';
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
asExcludes.0 = i - 1;


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
 * Do parsing.
 */
sOut = 'STDOUT';
call lineout sOut, 'Header Statistics'
call lineout sOut, '-----------------'
call lineout sOut, ''
call lineout sOut, left('File', 24)||X2C('09')||left('Level', 16);
call lineout sOut, left('-', 24, '-')||X2C('09')||left('-', 16,'-');
asTodos.0 = 0;
do i = 1 to asFiles.0
    call statfile sOut, sInclude, asFiles.i;
end
call lineout sOut;

/*
 * Print todos.
 */
say '';
say 'TODOs:'
do i = 1 to asTodos.0
    call lineout sOut, '-'||asTodos.i;
end

exit(asFailed.0);



/**
 * Test one file.
 * @returns 0
 * @param   sDir    Include directory (for the -I option and for basing sFile).
 * @param   sFile   The include file to test, full path.
 */
statfile: procedure expose asExcludes. asTodos.
parse arg sOut, sDir, sFile
    sName = substr(sFile, length(sDir) + 2);
    sLevel = '--';
    asRemarks.0 = 0;
    asChanges.0 = 0;
    if (isExcluded(sName)) then
    do
        say 'info: skipping 'sName'...';
        return 0;
    end

    iLine = 0;
    fComment = 0;
    do while (lines(sFile) > 0)
        sLine = strip(linein(sFile));
        iLine = iLine + 1;

        if (pos('/'||'*', sLine) > 0) then
            fComment = 1;

        if (left(sLine, 3) = '/'||'*'||'*' & pos('@file', sLine) > 0) then
        do
            sLine = strip(linein(sFile));
            sLevel = strip(strip(sLine, 'L', '*'));
        end
        else
        do
            do while (fComment &,
                        (   (left(space(sLine), 3) = '* @'),
                         |  (left(space(sLine), 5) = '/'||'*'||'*'||' @')) )
                iPos = pos(' @', sLine);
                sWord = word(substr(sLine, iPos + 2), 1);
                sText = strip(substr(sLine, pos('@'sWord, sLine) + length(sWord) + 1));

                /* read following lines */
                do forever
                    if (pos('*'||'/', sLine) > 0) then
                    do
                        fComment = 0;
                        leave
                    end
                    sLine = strip(linein(sFile));
                    iLine = iLine + 1;
                    if (strip(sLine) = '*'||'/') then
                    do
                        fComment = 0;
                        leave
                    end
                    if (left(sLine, 1) = '*' & pos(' @', sLine) > 0) then
                        leave;

                    sText = sText || ' ' || strip(strip(sLine, 'L', '*'));
                end
                if (right(sText, 2) = '*/') then
                    sText = strip(substr(sText, 1, length(sText) - 2));

                if (sWord = 'remark') then
                do
                    i = asRemarks.0 + 1;
                    asRemarks.i = sText
                    asRemarks.0 = i;
                end
                else if (sWord = 'changed') then
                do
                    i = asChanges.0 + 1;
                    asChanges.i = sText
                    asChanges.0 = i;
                end
                else if (sWord = 'todo') then
                do
                    i = asTodos.0 + 1;
                    asTodos.i = sText;
                    asTodos.0 = i;
                end
            end
        end
    end

    /*
     * Produce output.
     */
    call lineout sOut, left(sName, 24)||X2C('09')||left(sLevel, 16);
    do i = 1 to asRemarks.0
        call lineout sOut, X2C('09')'remark: '||asRemarks.i;
    end

    do i = 1 to asChanges.0
        if (pos(':', word(asChanges.i, 1)) > 0) then
            call lineout sOut, X2C('09')'changes by '||asChanges.i
        else
            call lineout sOut, X2C('09')'changes: '||asChanges.i
    end
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

