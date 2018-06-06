/* dllar - a tool to build both a .dll and an .a file
 * from a set of object (.o) files for EMX/OS2.
 *
 * Written by Andrew Zabolotny, bit@freya.etu.ru
 *
 * This program will accept a set of files on the command line.
 * All the public symbols from the .o files will be exported into
 * a .DEF file, then linker will be run (through gcc) against them to
 * build a shared library consisting of all given .o files. All libraries
 * (.a) will be first decompressed into component .o files then act as
 * described above. You can optionally give a description (-d "description")
 * which will be put into .DLL. To see the list of accepted options (as well
 * as command-line format) simply run this program without options. The .DLL
 * is built to be imported by name (there is no guarantee that new versions
 * of the library you build will have same ordinals for same symbols).
 *
 * dllar is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * dllar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dllar; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

/* To successfuly run this program you will need:
 * - Current drive should have LFN support (HPFS, ext2, network, etc)
 *   (Sometimes dllar generates filenames which won't fit 8.3 scheme)
 * - gcc
 *   (used to build the .dll)
 * - emxexp
 *   (used to create .def file from .o files)
 * - emximp
 *   (used to create .a file from .def file)
 * - GNU text utilites (cat, sort, uniq)
 *   used to process emxexp output
 * - lxlite (optional, see flag below)
 *   (used for general .dll cleanup)
 */

    flag_USE_LXLITE = 1;

    /*
     * Load REXX Util Functions.
     */
    if (RxFuncQuery('SysLoadFuncs') = 1) then
    do
        call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs';
        call SysLoadFuncs;
    end

    /*
     * Parse commandline / setup globals.
     */
    parse arg cmdLine;
    cmdLine = cmdLine||" "||value('DLLAR_CMDLINE',,'OS2ENVIRONMENT');
    outFile = '';
    useDefFile = '';
    inputFiles.0 = 0;
    description = '';
    CC = 'gcc.exe';
    CFLAGS = '-Zcrtdll';
    EXTRA_CFLAGS = '';
    EXPORT_BY_ORDINALS = 0;
    exclude_symbols = '';
    library_flags = '';
    library_data = '';
    flag_omf = 0;
    AR='ar'
    curDir = directory();
    curDirS = curDir;
    if (right(curDirS, 1) \= '\') then
        curDirS = curDirS||'\';

    do I = 1 to words(cmdLine)
        tmp = word(cmdLine, I);
        if left(tmp, 1) = '-' then
        do
            select
            when abbrev('output', substr(tmp, 2), 1) then
            do
                i = i + 1;
                outFile = word(cmdLine, i);
            end
            when abbrev('def', substr(tmp, 2), 3) then
                useDefFile = GetLongArg();
            when abbrev('description', substr(tmp, 2), 1) then
                description = GetLongArg();
            when abbrev('flags', substr(tmp, 2), 1) then
                CFLAGS = GetLongArg();
            when abbrev('cc', substr(tmp, 2), 1) then
                CC = translate(GetLongArg(), '\', '/');
            when abbrev('help', substr(tmp, 2), 1) then
                call PrintHelp;
            when abbrev('ordinals', substr(tmp, 2), 3) then
                EXPORT_BY_ORDINALS = 1;
            when abbrev('exclude', substr(tmp, 2), 2) then
                exclude_symbols = exclude_symbols||GetLongArg()' ';
            when abbrev('libflags', substr(tmp, 2), 4) then
                library_flags = library_flags||GetLongArg()' ';
            when abbrev('libdata', substr(tmp, 2), 4) then
                library_data = library_data||GetLongArg()' ';
            when abbrev('nocrtdll', substr(tmp, 2), 5) then
                CFLAGS = '';
            when abbrev('nolxlite', substr(tmp, 2), 5) then
                flag_USE_LXLITE = 0;
            when abbrev('omf', substr(tmp, 2), 3) then
                flag_omf = 1;
            otherwise
                EXTRA_CFLAGS = EXTRA_CFLAGS' 'tmp;
            end /*select*/
        end
        else
        do
            rc = SysFileTree(tmp, "files", "FO");
            if (rc = 0) then
            do J = 1 to files.0
                inputFiles.0 = inputFiles.0 + 1;
                K = inputFiles.0;
                inputFiles.K = files.J;
            end
            else
                files.0 = 0;
            if (files.0 = 0) then
            do
                say 'ERROR: No file(s) found: "'tmp'"';
                exit 8;
            end
            drop files.;
        end
    end /* iterate cmdline words */

    if (flag_omf = 1) then
    do
        AR = 'emxomfar';
        CFLAGS = CFLAGS || ' -Zomf';
    end

    /* check arg sanity */
    if (inputFiles.0 = 0) then
    do
        say 'dllar: no input files'
        call PrintHelp;
    end

    /*
     * Now extract all .o files from .a files
     */
    do I = 1 to inputFiles.0
        if (right(inputFiles.I, 2) = '.a') then
        do
            fullname = inputFiles.I;
            inputFiles.I = '$_'filespec('NAME', fullname);
            inputFiles.I = left(inputFiles.I, length(inputFiles.I) - 2);
            Address CMD '@mkdir 'inputFiles.I;
            if (rc \= 0) then
            do
                say 'Failed to create subdirectory ./'inputFiles.I;
                call CleanUp;
                exit 8;
            end

            /* Prefix with '!' to indicate archive */
            inputFiles.I = '!'inputFiles.I;
            call doCommand('cd 'substr(inputFiles.I, 2)' && 'AR' x 'fullname);
            call directory(curDir);
            rc = SysFileTree(substr(inputFiles.I, 2)'\*.o*', 'files', 'FO');
            if (rc = 0) then
            do
                inputFiles.0 = inputFiles.0 + 1;
                K = inputFiles.0;
                inputFiles.K = substr(inputFiles.I, 2)'/*.o*';
                /* Remove all empty files from archive since emxexp will barf */
                do J = 1 to files.0
                    if (stream(files.J, 'C', 'QUERY SIZE') <= 32) then
                        call SysFileDelete(files.J);
                end
                drop files.;
            end
            else
                say 'WARNING: there are no files in archive "'substr(inputFiles.I, 2)'"';
        end
    end

    /*
     * Now remove extra directory prefixes
     */
    do I = 1 to inputFiles.0
        if (left(inputFiles.I, length(curDirS)) = curDirS) then
            inputFiles.I = substr(inputFiles.I, length(curDirS) + 1);
    end

    /*
     * Output filename(s).
     */
    do_backup = 0;
    if (outFile = '') then
    do
        do_backup = 1;
        outFile = inputFiles.1;
    end

    /* If its an archive, remove the '!' and the '$_' prefixes */
    if (left(outFile, 3) = '!$_') then
        outFile = substr(outFile, 4);
    dotpos = lastpos('.', outFile);
    if (dotpos > 0) then
    do
        ext = translate(substr(outFile, dotpos + 1));
        if ((ext = 'DLL') | (ext = 'O') | (ext = 'A')) then
            outFile = substr(outFile, 1, dotpos - 1);
    end

    EXTRA_CFLAGS = substr(EXTRA_CFLAGS, 2);

    defFile = outFile'.def';
    dllFile = outFile'.dll';
    arcFile = outFile'.a';
    arcFileOmf = outFile'.lib';

    if (do_backup & stream(arcFile, 'C', 'query exists') \= '') then
    do
        '@del 'outFile'_s.a > NUL';
        call doCommand('ren 'arcFile' 'outFile'_s.a');
    end

    if useDefFile = '' then
    do
        /*
         * Extract public symbols from all the object files.
         */
        tmpdefFile = '$_'filespec('NAME', defFile);
        call SysFileDelete(tmpdefFile);
        do I = 1 to inputFiles.0
            if (left(inputFiles.I, 1) \= '!') then
                call doCommand('emxexp -u' inputFiles.I' >>'tmpdefFile);
        end

        /*
         * Create the def file.
         */
        call SysFileDelete(defFile);
        call stream defFile, 'c', 'open write';
        call lineOut defFile, 'LIBRARY 'filespec('NAME', outFile)' 'library_flags;
        if (length(library_data) > 0) then
            call lineOut defFile, 'DATA 'library_data;
        if (length(description) > 0) then
            call lineOut defFile, 'DESCRIPTION "'description'"';
        call lineOut defFile, 'EXPORTS';

        queTmp = RxQueue('Create');
        queOld = RxQueue('Set', queTmp);
        call doCommand('cat 'tmpdefFile' | sort.exe | uniq.exe | rxqueue.exe' queTmp);

        ordinal = 1;
        do while queued() > 0
            parse pull line;
            if (length(line) > 0) & (word(line, 1) \= ';') & (export_ok(line)) then
            do
                if (EXPORT_BY_ORDINALS) then
                do
                    iPos = pos(';', line);
                    if (iPos > 1) then
                        line = strip(substr(line, 1, iPos - 1), 'T');
                    line = line||' @'||ordinal||' NONAME';
                    ordinal = ordinal + 1;
                end
                call lineOut defFile, line;
            end
        end
        call RxQueue 'Delete', RxQueue('Set', queOld);
        call stream defFile, 'C', 'CLOSE';
        call SysFileDelete(tmpdefFile);
        drop line ordinal tmpdefFile;          /* try prevent running out of memory... */
    end
    else
    do
        defFile = useDefFile;
    end


if 0 then
do
    /*
     * Do linking, create implib, and apply lxlite.
     *  We just apply long cmdline hack here to save us trouble with slashes and such.
     *  The hack is to make a shell script for we execute using sh.exe. (.exe is of vital importance!)
     *  OLD: call doCommand('gcc 'CFLAGS' -Zdll -o 'dllFile defFile||gccCmdl' 'EXTRA_CFLAGS);
     */
    sTmpFile = SysTempFileName('.\dllar-??.???');
    call lineout sTmpFile, '#!/bin/sh'
    call charout sTmpFile, CC CFLAGS' -Zdll -o 'translate(dllFile defFile, '/', '\');
    do I = 1 to inputFiles.0
        if (left(inputFiles.I, 1) \= '!') then
            call charout sTmpFile, ' 'translate(inputFiles.I, '/', '\');
    end
    call lineout sTmpFile, ' 'EXTRA_CFLAGS
    call stream dsTmpFile, 'c', 'close'
    call doCommand 'sh.exe 'sTmpFile;
    call SysFileDelete sTmpFile;
    drop sTmpFile;
end
else
do
    /*
     * Do linking, create implib, and apply lxlite.
     */
    gccCmdl = '';
    do I = 1 to inputFiles.0
        if (left(inputFiles.I, 1) \= '!') then
            gccCmdl = gccCmdl' 'inputFiles.I;
    end
    call doCommand(CC CFLAGS' -Zdll -o 'dllFile defFile||gccCmdl' 'EXTRA_CFLAGS);
end

    call doCommand('emximp -o 'arcFile defFile);
    call doCommand('emximp -o 'arcFileOmf defFile);
    if (flag_USE_LXLITE) then
    do
        add_flags = '';
        if (EXPORT_BY_ORDINALS) then
            add_flags = '-ynd';
        call doCommand('lxlite -cs -t: -mrn -ml1 'add_flags' 'dllFile);
    end

    /*
     * Successful exit.
     */
    call CleanUp 1;
exit 0;

/**
 * Print usage and exit script with rc=1.
 */
PrintHelp:
 say 'Usage: dllar [-o[utput] output_file] [-d[escription] "dll descrption"]'
 say '       [-cc "CC"] [-f[lags] "CFLAGS"] [-ord[inals]] -ex[clude] "symbol(s)"'
 say '       [-libf[lags] "{INIT|TERM}{GLOBAL|INSTANCE}"] [-nocrt[dll]]'
 say '       [-libd[ata] "DATA"] [-omf]'
 say '       [-nolxlite] [-def def_file]"'
 say '       [*.o] [*.a]'
 say '*> "output_file" should have no extension.'
 say '   If it has the .o, .a or .dll extension, it is automatically removed.'
 say '   The import library name is derived from this and is set to "name".a.'
 say '*> "cc" is used to use another GCC executable.   (default: gcc.exe)'
 say '*> "flags" should be any set of valid GCC flags. (default: -Zcrtdll)'
 say '   These flags will be put at the start of GCC command line.'
 say '*> -ord[inals] tells dllar to export entries by ordinals. Be careful.'
 say '*> -ex[clude] defines symbols which will not be exported. You can define'
 say '   multiple symbols, for example -ex "myfunc yourfunc _GLOBAL*".'
 say '   If the last character of a symbol is "*", all symbols beginning'
 say '   with the prefix before "*" will be exclude, (see _GLOBAL* above).'
 say '*> -libf[lags] can be used to add INITGLOBAL/INITINSTANCE and/or'
 say '   TERMGLOBAL/TERMINSTANCE flags to the dynamically-linked library.'
 say '*> -libd[ata] can be used to add data segment attributes flags to the '
 say '   dynamically-linked library.'
 say '*> -nocrtdll switch will disable linking the library against emx''s'
 say '   C runtime DLLs.'
 say '*> -nolxlite does not compress executable'
 say '*> -def def_file do not generate .def file, use def_file instead.'
 say '*> -omf will use OMF tools to extract the static library objects.'
 say '*> All other switches (for example -L./ or -lmylib) will be passed'
 say '   unchanged to GCC at the end of command line.'
 say '*> If you create a DLL from a library and you do not specify -o,'
 say '   the basename for DLL and import library will be set to library name,'
 say '   the initial library will be renamed to 'name'_s.a (_s for static)'
 say '   i.e. "dllar gcc.a" will create gcc.dll and gcc.a, and the initial'
 say '   library will be renamed into gcc_s.a.'
 say '--------'
 say 'Example:'
 say '   dllar -o gcc290.dll libgcc.a -d "GNU C runtime library" -ord'
 say '    -ex "__main __ctordtor*" -libf "INITINSTANCE TERMINSTANCE"'
 call CleanUp;
exit 1;


/**
 * Get long arg i + 1 from cmdline taking quoting into account.
 * @returns     Long argument.
 * @Uses        i, cmdline
 * @Modifies    i
 */
GetLongArg:
    i = i + 1;
    _tmp_ = word(cmdLine, i);
    if (left(_tmp_, 1) = '"') | (left(_tmp_, 1) = "'") then
    do
        do while (i < words(cmdLine) & (length(_tmp_) <= 1 | right(_tmp_, 1) \= left(_tmp_, 1)))
            i = i + 1;
            if (_tmp_ = '') then
                _tmp_ = word(cmdLine, i);
            else
                _tmp_ = _tmp_' 'word(cmdLine, i);
        end
        if (right(_tmp_, 1) = left(_tmp_, 1)) then
            _tmp_ = substr(_tmp_, 2, length(_tmp_) - 2);
    end
return _tmp_;

/**
 * Checks if the export is ok or not.
 * @returns 1 if ok.
 * @returns 0 if not ok.
 */
export_ok: procedure expose exclude_symbols;
    parse arg line;
    cWords = words(exclude_symbols)
    do i = 1 to cWords
        noexport = '"'word(exclude_symbols, i);
        if (right(noexport, 1) = '*') then
            noexport = left(noexport, length(noexport) - 1)
        else
            noexport = noexport'"';
        if (pos(noexport, line) > 0) then
            return 0;
    end
return 1;

/**
 * Execute a command.
 * If exit code of the commnad <> 0 CleanUp() is called and we'll exit the script.
 * @Uses    Whatever CleanUp() uses.
 */
doCommand: procedure expose inputFiles.
    parse arg _cmd_;
    say _cmd_;
    if (length(_cmd_) > 1023) then
    do
        /* Trick: use a different shell to launch the command since CMD.EXE has a
         *  1024 byte limit on the length of the command line ...
         * Note that .exe is important!
         */
        say 'INFO: doCommand: applying commandlength hack, cmdlen:' length(_cmd_) '. (1)'
        call value '__TMP__',_cmd_,'OS2ENVIRONMENT';
        Address CMD 'sh.exe -c "$__TMP__"';
        rcCmd = rc;
        call value '__TMP__','','OS2ENVIRONMENT';
    end
    else
    do
        Address CMD '@'_cmd_;
        rcCmd = rc;
    end

    if (rcCmd \= 0) then
    do
        say 'command failed, exit code='rcCmd;
        call CleanUp;
        exit rcCmd;
    end
    drop _cmd_;                            /* prevent running out of memory... */
return;

/*
 * Cleanup temporary files and output
 * @Uses    inputFiles.
 * @Uses    out
 */
CleanUp: procedure expose inputFiles. outFile
    parse arg fSuccess
    call directory(curDir);
    do i = inputFiles.0 to 1 by -1
        if (left(inputFiles.I, 1) = '!') then
            Address CMD 'rm -rf' substr(inputFiles.I, 2);
    end

    /*
     * Kill result in case of failure as there is just to many stupid make/nmake
     * things out there which doesn't do this.
     */
    if (fSuccess = '') then
        Address CMD 'rm -f' outFile||'.a' outFile||'.def' outFile||'.dll'
return;
