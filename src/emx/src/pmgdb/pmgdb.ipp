.* pmgdb.ipp
.*
.* Copyright (c) 1996 Eberhard Mattes
.*
.* This file is part of pmgdb.
.*
.* pmgdb is free software; you can redistribute it and/or modify it
.* under the terms of the GNU General Public License as published by
.* the Free Software Foundation; either version 2, or (at your option)
.* any later version.
.*
.* pmgdb is distributed in the hope that it will be useful,
.* but WITHOUT ANY WARRANTY; without even the implied warranty of
.* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
.* GNU General Public License for more details.
.*
.* You should have received a copy of the GNU General Public License
.* along with pmgdb; see the file COPYING.  If not, write to the
.* the Free Software Foundation, 59 Temple Place - Suite 330,
.* Boston, MA 02111-1307, USA.
.*
:userdoc.
:prolog.
:docprof toc=1234.
:title.Help for pmgdb
:eprolog.
:body.

.imd help.h

.nameit symbol=tt text=':font facename=Courier size=16x9.'
.nameit symbol=ett text=':font facename=default size=0x0.'

.************************************************************************
.* General help, nothing selected
.************************************************************************
:h1 res=&HELP_GENERAL..About pmgdb
:i1 id='pmgdb'.pmgdb
:lm margin=1.
:p.
:hp2.pmgdb:ehp2. is a Presentation Manager frontend for GDB, the
GNU debugger.
:p.
To start :hp2.pmgdb:ehp2. from the command line, type
:lm margin=5.:xmp.pmgdb app.exe arg1 arg2:exmp.:lm margin=1.
:p.
where &tt.app.exe&ett. is the application
to be debugged and &tt.arg1&ett. and &tt.arg2&ett. are command
line arguments for that application.
:p.
Alternatively, you can select the application to debug and provide
command line arguments with :hp2.pmgdb:ehp2.'s menus.
:p.
In addition to a :link reftype=hd res=&HELP_TUTORIAL..tutorial:elink.,
help is available on the following topics&colon.
:p.
:ul.
:li.:link reftype=hd res=&HELP_CMD_WINDOW..The command window:elink.
:li.:link reftype=hd res=&HELP_SRC_WINDOW..Source windows:elink.
:li.:link reftype=hd res=&HELP_SRCS_WINDOW..Source files window:elink.
:li.:link reftype=hd res=&HELP_BRK_WINDOW..Breakpoints window:elink.
:li.:link reftype=hd res=&HELP_DSP_WINDOW..Display window:elink.
:li.:link reftype=hd res=&HELP_THR_WINDOW..Threads window:elink.
:li.:link reftype=hd res=&HELP_REG_WINDOW..Register window:elink.
:eul.

.************************************************************************
.* Command window
.************************************************************************
:h1 res=&HELP_CMD_WINDOW..Help for command window
:i1 id='cmd'.pmgdb command window
:lm margin=1.
:p.
The command window of :hp2.pmgdb:ehp2. shows GDB's output. You can
type GDB commands in the command window. Select
:link reftype=hd res=&HPM_CMD_HISTORY..History:elink. from the
:link reftype=hd res=&HPM_CMD_FILEMENU..File:elink. menu to
edit the command. If you close the command
window, :hp2.pmgdb:ehp2. will be terminated.
:p.
In addition to a :link reftype=hd res=&HELP_TUTORIAL..tutorial:elink.,
help is available on the following topics&colon.
:p.
:ul.
:li.:link reftype=hd res=&HPM_CMD_FILEMENU..File menu:elink.
:li.:link reftype=hd res=&HPM_CMD_EDITMENU..Edit menu:elink.
:li.:link reftype=hd res=&HPM_CMD_RUNMENU..Run menu:elink.
:li.:link reftype=hd res=&HPM_CMD_BRKPTMENU..Breakpoints menu:elink.
:li.:link reftype=hd res=&HPM_CMD_OPTIONSMENU..Options menu:elink.
:li.:link reftype=hd res=&HPM_CMD_WINMENU..Windows menu:elink.
:li.:link reftype=hd res=&HPM_CMD_HELPMENU..Help menu:elink.
:li.:link reftype=hd res=&HELP_CMD_KEYS..Keys in the command window:elink.
:eul.

.************************************************************************
.* File
.************************************************************************
:h2 res=&HPM_CMD_FILEMENU..Help for File
:i2 refid='cmd'.File menu
:i1 id='cmd.file'.File menu choices (command window)
:lm margin=1.
:p.
The File menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_CMD_OPEN_EXEC..Open program file:elink.
:pd.Open a program file.
:pt.:link reftype=hd res=&HPM_CMD_OPEN_CORE..Open core file:elink.
:pd.Open a core dump file.
:pt.:link reftype=hd res=&HPM_OPEN_SOURCE..Open a source file:elink.
:pd.Open a source file.
:pt.:link reftype=hd res=&HPM_CMD_STARTUP..Startup:elink.
:pd.Set command line arguments and other startup options.
:pt.:link reftype=hd res=&HPM_RESTART..Restart:elink.
:pd.Restart the program.
:pt.:link reftype=hd res=&HPM_WHERE..Where:elink.
:pd.Show current location of execution.
:pt.:link reftype=hd res=&HPM_EXIT..Exit:elink.
:pd.Terminate :hp2.pmgdb:ehp2..
:eparml.

.************************************************************************
.* Open program file
.************************************************************************
:h3 res=&HPM_CMD_OPEN_EXEC..Help for Open program file
:i2 refid='cmd.file'.Open program file
:i2 refid='pmgdb'.Open program file
:i2 refid='pmgdb'.Program file
:lm margin=1.
:p.
Use this choice to open a program file, that is, to specify the program
to be debugged (debuggee).

.************************************************************************
.* Open core file
.************************************************************************
:h3 res=&HPM_CMD_OPEN_CORE..Help for Open core file
:i2 refid='cmd.file'.Open core file
:i2 refid='pmgdb'.Open core file
:i2 refid='pmgdb'.core file
:lm margin=1.
:p.
Use this choice to open a core dump file.

.************************************************************************
.* Open source file
.************************************************************************
:h3 res=&HPM_OPEN_SOURCE..Help for Open source file
:i2 refid='cmd.file'.Open source file
:i2 refid='pmgdb'.Open source file
:i2 refid='pmgdb'.Source file
:lm margin=1.
:p.
Use this choice to open a source file.  If the source file is already
loaded, the window for that source file will be displayed.  Otherwise,
the source file will be loaded and displayed in a new window.

.************************************************************************
.* Startup
.************************************************************************
:h3 res=&HPM_CMD_STARTUP..Help for Startup
:i2 refid='cmd.file'.Startup
:i2 refid='pmgdb'.Startup
:i2 refid='pmgdb'.Command line arguments for debuggee
:lm margin=1.
:p.
Use this choice to specify startup options. 
This choice opens the
:link reftype=hd res=&HPD_STARTUP..Startup:elink. dialog box.
You can enter the command line options and select whether the
VIO window of the debuggee should be closed after termination of the
debuggee.

.************************************************************************
.* Restart
.************************************************************************
:h3 res=&HPM_RESTART..Help for Restart
:i2 refid='cmd.file'.Restart
:i2 refid='pmgdb'.Restart
:lm margin=1.
:p.
Use this choice to restart the debuggee.  If the debuggee is running,
it will be stopped and restarted.  If the debuggee is not running, it
will be started.  The debugger keeps any breakpoints.  If no
breakpoints are set (or all breakpoints are disabled), a breakpoint
will be set on &tt.main&ett.

.************************************************************************
.* Where
.************************************************************************
:h3 res=&HPM_WHERE..Help for Where
:i2 refid='cmd.file'.Where
:i2 refid='pmgdb'.Where
:lm margin=1.
:p.
Use this choice to show the current location of execution.  If source
code is available for that location, the appropriate source file will
be displayed with the current line highlighted.

.************************************************************************
.* Exit
.************************************************************************
:h3 res=&HPM_EXIT..Help for Exit
:i2 refid='cmd.file'.Exit
:i2 refid='pmgdb'.Exit
:lm margin=1.
:p.
Use this choice to terminate :hp2.pmgdb:ehp2..  If a debuggee is running,
it will be terminated.  :hp2.GDB:ehp2. will also be terminated.

.************************************************************************
.* Edit
.************************************************************************
:h2 res=&HPM_CMD_EDITMENU..Help for Edit
:i2 refid='cmd'.Edit menu
:i1 id='cmd.edit'.Edit menu choices (command window)
:lm margin=1.
:p.
The Edit menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_CMD_COMPLETE..Complete:elink.
:pd.Command completion.
:pt.:link reftype=hd res=&HPM_CMD_HISTORY..History:elink.
:pd.Select, edit, and repeat a previously issued command.
:eparml.

.************************************************************************
.* Complete
.************************************************************************
:h3 res=&HPM_CMD_COMPLETE..Help for Complete
:i2 refid='cmd.edit'.Complete
:i2 refid='pmgdb'.Complete
:lm margin=1.
:p.
Use this choice to complete the command typed in the command window.
If there is more than one possible completion, a popup window will
show all available completions.  Select one of those choices to
complete the command.

.************************************************************************
.* History
.************************************************************************
:h3 res=&HPM_CMD_HISTORY..Help for History
:i2 refid='cmd.edit'.History
:i2 refid='pmgdb'.History
:lm margin=1.
:p.
Use this choice to select and edit a command from a list of all previously
issued commands or to edit the current command.  This choice opens the
:link reftype=hd res=&HPD_HISTORY..History:elink. dialog box.

.************************************************************************
.* Run
.************************************************************************
:h2 res=&HPM_CMD_RUNMENU..Help for Run
:i2 refid='pmgdb'.Run menu (command window)
:i1 id='cmd.run'.Run menu choices
:lm margin=1.
:p.
The Run menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_GO..Go:elink.
:pd.Continue (or start) program.
:pt.:link reftype=hd res=&HPM_STEPOVER..Step over:elink.
:pd.Step program, proceeding through subroutine calls.
:pt.:link reftype=hd res=&HPM_STEPINTO..Step into:elink.
:pd.Step program until it reaches a different source line.
:pt.:link reftype=hd res=&HPM_FINISH..Finish:elink.
:pd.Execute until selected stack frame returns.
:pt.:link reftype=hd res=&HPM_ISTEPOVER..Insn step over:elink.
:pd.Step one instruction, proceeding through subroutine calls.
:pt.:link reftype=hd res=&HPM_ISTEPINTO..Insn step into:elink.
:pd.Step one instruction.
:eparml.

.************************************************************************
.* Go
.************************************************************************
:h3 res=&HPM_GO..Help for Go
:i2 refid='cmd.run'.Go
:i2 refid='pmgdb'.Go
:i2 refid='pmgdb'.Continue program
:i2 refid='pmgdb'.Start program
:lm margin=1.
:p.
Use this choice to let the program run until a breakpoint is hit or a
signal occurs.  If the program has not yet been started, it will be
started and a breakpoint will be set on &tt.main&ett.  if no
breakpoints are set (or all breakpoints are disabled).

.************************************************************************
.* Step over
.************************************************************************
:h3 res=&HPM_STEPOVER..Help for Step over
:i2 refid='cmd.run'.Step over
:i2 refid='pmgdb'.Step over
:lm margin=1.
:p.
Use this choice to step the program, proceeding through subroutine calls.
Subroutine calls are treated as one instruction.  If there are no subroutine
calls, this choice is equivalent to
:link reftype=hd res=&HPM_STEPINTO..Step into:elink..

.************************************************************************
.* Step into
.************************************************************************
:h3 res=&HPM_STEPINTO..Help for Step into
:i2 refid='cmd.run'.Step into
:i2 refid='pmgdb'.Step into
:lm margin=1.
:p.
Use this choice to step the program until it reaches a different source
line.  See also
:link reftype=hd res=&HPM_STEPOVER..Step over:elink..

.************************************************************************
.* Finish
.************************************************************************
:h3 res=&HPM_FINISH..Help for Finish
:i2 refid='cmd.run'.Finish
:i2 refid='pmgdb'.Finish
:lm margin=1.
:p.
Use this choice to execute the program until the selected stack frame
returns.  That is, this choice finishes a subroutine call, stopping
at the location from which the subroutine was called.  The returned value
is printed in the command window.

.************************************************************************
.* Insn step over
.************************************************************************
:h3 res=&HPM_ISTEPOVER..Help for Insn step over
:i2 refid='cmd.run'.Insn step over
:i2 refid='pmgdb'.Insn step over
:lm margin=1.
:p.
Use this choice to step one instruction of the program. Subroutine
calls are treated as one instruction. If the instruction is not
a subroutine call, this choice is equivalent to
:link reftype=hd res=&HPM_ISTEPINTO..Insn step into:elink..

.************************************************************************
.* Insn step into
.************************************************************************
:h3 res=&HPM_ISTEPINTO..Help for Insn step into
:i2 refid='cmd.run'.Insn step into
:i2 refid='pmgdb'.Insn step into
:lm margin=1.
:p.
Use this choice to step one instruction of the program.  See also
:link reftype=hd res=&HPM_ISTEPOVER..Insn step over:elink..

.************************************************************************
.* Breakpoints
.************************************************************************
:h2 res=&HPM_CMD_BRKPTMENU..Help for Breakpoints
:i2 refid='cmd'.Breakpoints menu
:i1 id='cmd.brkpt'.Breakpoints menu choices (command window)
:lm margin=1.
:p.
The Breakpoints menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_BRKPT_LINE..On line:elink.
:pd.Set a breakpoint on a line.
:pt.:link reftype=hd res=&HPM_BRKPT_LIST..List:elink.
:pd.Open the breakpoints window.
:eparml.

.************************************************************************
.* On line
.************************************************************************
:h3 res=&HPM_BRKPT_LINE..Help for On line
:i2 refid='cmd.brkpt'.On line
:i2 refid='pmgdb'.On line
:i2 refid='pmgdb'.Breakpoint, on line
:lm margin=1.
:p.
Use this choice to set a breakpoint on a line of the source code.
This choice opens the
:link reftype=hd res=&HPD_BRKPT_LINE..Line Breakpoint:elink. dialog box.
:p.
You can also set or clear a breakpoint by double clicking with the
first mouse button on the line number in a source window.

.************************************************************************
.* List
.************************************************************************
:h3 res=&HPM_BRKPT_LIST..Help for List
:i2 refid='cmd.brkpt'.List
:i2 refid='pmgdb'.List
:i2 refid='pmgdb'.Breakpoint, list
:lm margin=1.
:p.
Use this choice to open the breakpoints window which shows all the
breakpoints.

.************************************************************************
.* Options
.************************************************************************
:h2 res=&HPM_CMD_OPTIONSMENU..Help for Options
:i2 refid='cmd'.Options menu
:i1 id='cmd.options'.Options menu choices
:lm margin=1.
:p.
The Options menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_PMDEBUGMODE..PM debugging mode:elink.
:pd.Select the mode for debugging PM applications.
:pt.:link reftype=hd res=&HPM_CMD_ANNOTATE..Annotate:elink.
:pd.Print annotations.
:pt.:link reftype=hd res=&HPM_CMD_FONT..Font:elink.
:pd.Select font.
:eparml.

.************************************************************************
.* PM debugging mode
.************************************************************************
:h3 res=&HPM_PMDEBUGMODE..Help for PM debugging mode
:i2 refid='cmd.options'.PM debugging mode
:i2 refid='pmgdb'.PM debugging mode
:lm margin=1.
:p.
Use this choice to select the mode for debugging PM applications.
Synchronous mode should be selected before starting a Presentation
Manager application, otherwise the Presentation Manager will hang,
requiring a reboot to recover.  In Synchronous mode, only windows of
:hp2.pmgdb:ehp2. and of the program being debugged can be made active;
all other windows cannot be used.
:hp2.pmgdb:ehp2. attempts to set Synchronous mode automatically when
a PM program is loaded.  This choice opens the
:link reftype=hd res=&HPD_PMDEBUGMODE..PM debugging mode:elink. dialog box.

.************************************************************************
.* Annotate
.************************************************************************
:h3 res=&HPM_CMD_ANNOTATE..Help for Annotate
:i2 refid='cmd.options'.Annotate
:i2 refid='pmgdb'.Annotate
:lm margin=1.
:p.
Use this choice to turn on printing of GDB's annotations.  This us
used for debugging :hp2.pmgdb:ehp2..

.************************************************************************
.* Font
.************************************************************************
:h3 res=&HPM_CMD_FONT..Help for Font
:i2 refid='cmd.options'.Font
:i2 refid='pmgdb'.Font
:lm margin=1.
:p.
Use this choice to select a font for all windows of :hp2.pmgdb:ehp2..
You can change the font of a single window by dragging a font from the
Font Palette and dropping it on a :hp2.pmgdb:ehp2. window.

.************************************************************************
.* Windows
.************************************************************************
:h2 res=&HPM_CMD_WINMENU..Help for Windows
:i2 refid='cmd'.Windows menu
:i1 id='cmd.win'.Windows menu choices
:lm margin=1.
:p.
The Windows menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_WIN_SRCS..Source files:elink.
:pd.Show the source files window.
:pt.:link reftype=hd res=&HPM_WIN_BRK..Breakpoints:elink.
:pd.Show the breakpoints window.
:pt.:link reftype=hd res=&HPM_WIN_DSP..Display:elink.
:pd.Show the display window.
:pt.:link reftype=hd res=&HPM_WIN_THR..Threads:elink.
:pd.Show the threads window.
:pt.:link reftype=hd res=&HPM_WIN_REG..Register:elink.
:pd.Show the register window.
:eparml.

.************************************************************************
.* Source files
.************************************************************************
:h3 res=&HPM_WIN_SRCS..Help for Source files
:i2 refid='cmd.win'.Source files
:i2 refid='pmgdb'.Source files
:lm margin=1.
:p.
Use this choice to show the source files window.

.************************************************************************
.* Breakpoints
.************************************************************************
:h3 res=&HPM_WIN_BRK..Help for Breakpoints
:i2 refid='cmd.win'.Breakpoints
:i2 refid='pmgdb'.Breakpoints
:lm margin=1.
:p.
Use this choice to show the breakpoints window.

.************************************************************************
.* Display
.************************************************************************
:h3 res=&HPM_WIN_DSP..Help for Display
:i2 refid='cmd.win'.Display
:i2 refid='pmgdb'.Display
:lm margin=1.
:p.
Use this choice to show the display window.

.************************************************************************
.* Threads
.************************************************************************
:h3 res=&HPM_WIN_THR..Help for Threads
:i2 refid='cmd.win'.Threads
:i2 refid='pmgdb'.Threads
:lm margin=1.
:p.
Use this choice to show the threads window.

.************************************************************************
.* Register
.************************************************************************
:h3 res=&HPM_WIN_REG..Help for Register
:i2 refid='cmd.win'.Register
:i2 refid='pmgdb'.Register
:lm margin=1.
:p.
Use this choice to show the register window.

.************************************************************************
.* Help
.************************************************************************
:h2 res=&HPM_CMD_HELPMENU..Help for Help
:i2 refid='cmd'.Help menu
:i1 id='cmd.help'.Help menu choices
:lm margin=1.
:p.
The Help menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_HELP_HELP..Help for help:elink.
:pd.Display help for help.
:pt.:link reftype=hd res=&HPM_HELP_EXT..Extended help:elink.
:pd.Display extended help.
:pt.:link reftype=hd res=&HPM_HELP_KEYS..Keys help:elink.
:pd.Display help about keys.
:pt.:link reftype=hd res=&HPM_HELP_INDEX..Help index:elink.
:pd.Display the help index.
:pt.:link reftype=hd res=&HPM_HELP_CONTENTS..Help index:elink.
:pd.Display the help contents.
:pt.:link reftype=hd res=&HPM_TUTORIAL..Tutorial:elink.
:pd.Display tutorial.
:pt.:link reftype=hd res=&HPM_CMD_ABOUT..About:elink.
:pd.Display copyright information.
:eparml.

.************************************************************************
.* Help for Help
.************************************************************************
:h3 res=&HPM_HELP_HELP..Help for Help
:i2 refid='cmd.help'.Help for Help
:lm margin=1.
:p.
Use this choice to get online help for help.

.************************************************************************
.* Extended help
.************************************************************************
:h3 res=&HPM_HELP_EXT..Help for Extended help
:i2 refid='cmd.help'.Extended help
:lm margin=1.
:p.
Use this choice to get online help
for :hp2.pmgdb:ehp2.. You can also press the :hp2.F1:ehp2. key
to get online help.

.************************************************************************
.* Keys help
.************************************************************************
:h3 res=&HPM_HELP_KEYS..Help for Keys help
:i2 refid='cmd.help'.Keys help
:lm margin=1.
:p.
Use this choice to get online help for :hp2.pmgdb:ehp2. keys.

.************************************************************************
.* Help index
.************************************************************************
:h3 res=&HPM_HELP_INDEX..Help for Help index
:i2 refid='cmd.help'.Help for Help index
:lm margin=1.
:p.
Use this choice to display the help index.

.************************************************************************
.* Help contents
.************************************************************************
:h3 res=&HPM_HELP_CONTENTS..Help for Help contents
:i2 refid='cmd.help'.Help for Help contents
:lm margin=1.
:p.
Use this choice to display the table of contents for online help.

.************************************************************************
.* Tutorial
.************************************************************************
:h3 res=&HPM_TUTORIAL..Help for Tutorial
:i2 refid='cmd.help'.Tutorial
:lm margin=1.
:p.
Use this choice to display the :hp2.pmgdb:ehp2.
:link reftype=hd res=&HELP_TUTORIAL..tutorial:elink..

.************************************************************************
.* About
.************************************************************************
:h3 res=&HPM_CMD_ABOUT..Help for About
:i2 refid='cmd.help'.About
:i2 refid='pmgdb'.Author
:i2 refid='pmgdb'.Copyright
:lm margin=1.
:p.
Use this choice to display copyright information
about :hp2.pmgdb:ehp2..

.************************************************************************
.* Keys help (command window)
.************************************************************************
:h1 res=&HELP_CMD_KEYS..Keys help (command window)
:i1 id='cmd.keys'.Keys (command window)
:lm margin=1.
:p.
You can use the following keys in the command window&colon.
:dl tsize=20 compact.
:dt.Backspace:dd.Delete one character to the left.
:dt.Up arrow:dd.Fetch the previous command from the command history.
:dt.Down arrow:dd.Fetch the next command from the command history.
:dt.Page Up:dd.Scroll down by one window.
:dt.Page Down:dd.Scroll up by one window.
:dt.Ctrl+Page Up:dd.Scroll down right by one window.
:dt.Ctrl+Page Down:dd.Scroll left by one window.
:edl.
:p.
Accelerator keys are missed out in the list above.

.************************************************************************
.* Source window
.************************************************************************
:h1 res=&HELP_SRC_WINDOW..Help for source window
:i1 id='src'.pmgdb source window
:lm margin=1.
:p.
The source windows of :hp2.pmgdb:ehp2. show the source code of the
application being debugged.  There may be multiple source windows,
one for each source file.  Closing a source window doesn't terminate
:hp2.pmgdb:ehp2..
:p.
Help is available on the following topics&colon.
:p.
:ul.
:li.:link reftype=hd res=&HPM_SRC_FILEMENU..File menu:elink.
:li.:link reftype=hd res=&HPM_SRC_VIEWMENU..View menu:elink.
:li.:link reftype=hd res=&HPM_SRC_RUNMENU..Run menu:elink.
:li.:link reftype=hd res=&HPM_SRC_BRKPTMENU..Breakpoints menu:elink.
:li.:link reftype=hd res=&HPM_SRC_DSPMENU..Display menu:elink.
:li.:link reftype=hd res=&HPM_SRC_OPTIONSMENU..Options menu:elink.
:li.:link reftype=hd res=&HPM_SRC_WINMENU..Windows menu:elink.
:li.:link reftype=hd res=&HPM_SRC_HELPMENU..Help menu:elink.
:li.:link reftype=hd res=&HELP_SRC_KEYS..Keys in a source window:elink.
:eul.

.************************************************************************
.* File
.************************************************************************
:h2 res=&HPM_SRC_FILEMENU..Help for File
:i2 refid='src'.File menu
:i1 id='src.file'.File menu choices (source window)
:lm margin=1.
:p.
The File menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_OPEN_SOURCE..Open a source file:elink.
:pd.Open a source file.
:pt.:link reftype=hd res=&HPM_RESTART..Restart:elink.
:pd.Restart the program.
:pt.:link reftype=hd res=&HPM_WHERE..Where:elink.
:pd.Show current location of execution.
:pt.:link reftype=hd res=&HPM_EXIT..Exit:elink.
:pd.Terminate :hp2.pmgdb:ehp2..
:eparml.

.************************************************************************
.* View
.************************************************************************
:h2 res=&HPM_SRC_VIEWMENU..Help for View
:i2 refid='src'.View menu
:i1 id='src.view'.View menu choices (source window)
:lm margin=1.
:p.
The View menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_SRC_FIND..Find:elink.
:pd.Search for a regular expression.
:pt.:link reftype=hd res=&HPM_SRC_FINDNEXT..Find next:elink.
:pd.Search for next occurrence.
:pt.:link reftype=hd res=&HPM_SRC_FINDPREV..Find previous:elink.
:pd.Search for previous occurrence.
:pt.:link reftype=hd res=&HPM_SRC_GOTO..Go to line:elink.
:pd.Show a line specified by line number.
:eparml.

.************************************************************************
.* Find
.************************************************************************
:h3 res=&HPM_SRC_FIND..Help for Find
:i2 refid='src.view'.Find
:i2 refid='pmgdb'.Find
:lm margin=1.
:p.
Use this choice to search the source file for a regular expression.
This choice opens the
:link reftype=hd res=&HPD_FIND..Find:elink. dialog box.
The search starts at the beginning of the file.
See also :link reftype=hd res=&HPM_SRC_FINDNEXT..Find next:elink.
and :link reftype=hd res=&HPM_SRC_FINDPREV..Find previous:elink..

.************************************************************************
.* Find next
.************************************************************************
:h3 res=&HPM_SRC_FINDNEXT..Help for Find next
:i2 refid='src.view'.Find next
:i2 refid='pmgdb'.Find next
:lm margin=1.
:p.
Use this choice to search for the next occurrence of a regular expression.
The search continues from the line following the line containing the
previously found match.
See also :link reftype=hd res=&HPM_SRC_FIND..Find:elink.
and :link reftype=hd res=&HPM_SRC_FINDPREV..Find previous:elink..

.************************************************************************
.* Find previous
.************************************************************************
:h3 res=&HPM_SRC_FINDPREV..Help for Find previous
:i2 refid='src.view'.Find previous
:i2 refid='pmgdb'.Find previous
:lm margin=1.
:p.
Use this choice to search for the previous occurrence of a regular expression.
The search continues from the line preceding the line containing the
previously found match.
See also :link reftype=hd res=&HPM_SRC_FIND..Find:elink.
and :link reftype=hd res=&HPM_SRC_FINDNEXT..Find next:elink..

.************************************************************************
.* Go to line
.************************************************************************
:h3 res=&HPM_SRC_GOTO..Help for Go to line
:i2 refid='src.view'.Go to line
:i2 refid='pmgdb'.Go to line
:lm margin=1.
:p.
Use this choice to show a line of the current source file
specified by line number.  This choice opens the
:link reftype=hd res=&HPD_GOTO..Go to line:elink. dialog box.

.************************************************************************
.* Run
.************************************************************************
:h2 res=&HPM_SRC_RUNMENU..Help for Run
:i2 refid='pmgdb'.Run menu (source window)
:i1 id='src.run'.Run menu choices
:lm margin=1.
:p.
The Run menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_GO..Go:elink.
:pd.Continue (or start) program.
:pt.:link reftype=hd res=&HPM_STEPOVER..Step over:elink.
:pd.Step program, proceeding through subroutine calls.
:pt.:link reftype=hd res=&HPM_STEPINTO..Step into:elink.
:pd.Step program until it reaches a different source line.
:pt.:link reftype=hd res=&HPM_FINISH..Finish:elink.
:pd.Execute until selected stack frame returns.
:pt.:link reftype=hd res=&HPM_ISTEPOVER..Insn step over:elink.
:pd.Step one instruction, proceeding through subroutine calls.
:pt.:link reftype=hd res=&HPM_ISTEPINTO..Insn step into:elink.
:pd.Step one instruction.
:pt.:link reftype=hd res=&HPM_SRC_UNTIL..Until:elink.
:pd.Execute until the program reaches the selected line.
:pt.:link reftype=hd res=&HPM_SRC_JUMP..Jump:elink.
:pd.Jump to the selected line.
:eparml.

.************************************************************************
.* Until
.************************************************************************
:h3 res=&HPM_SRC_UNTIL..Help for Until
:i2 refid='src.run'.Until
:i2 refid='pmgdb'.Until
:lm margin=1.
:p.
Use this choice to let the program run until it reaches the selected line.
Click with the first mouse button on a line number in a source window to
select a line.

.************************************************************************
.* Jump
.************************************************************************
:h3 res=&HPM_SRC_JUMP..Help for Jump
:i2 refid='src.run'.Jump
:i2 refid='pmgdb'.Jump
:lm margin=1.
:p.
Use this choice to jump to the selected line.  Execution will continue
from that line.  Click with the first mouse button on a line number in
a source window to select a line.

.************************************************************************
.* Breakpoints
.************************************************************************
:h2 res=&HPM_SRC_BRKPTMENU..Help for Breakpoints
:i2 refid='src'.Breakpoints menu
:i1 id='src.brkpt'.Breakpoints menu choices (source window)
:lm margin=1.
:p.
The Breakpoints menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_BRKPT_LINE..On line:elink.
:pd.Set a breakpoint on a line.
:pt.:link reftype=hd res=&HPM_BRKPT_LIST..List:elink.
:pd.Open the breakpoints window.
:eparml.

.************************************************************************
.* Display
.************************************************************************
:h2 res=&HPM_SRC_DSPMENU..Help for Display
:i2 refid='src'.Display menu
:i1 id='src.dsp'.Display menu choices
:lm margin=1.
:p.
The Display menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_DSP_ADD..Add:elink.
:pd.Add a display.
:pt.:link reftype=hd res=&HPM_SRC_DSP_SHOW..Show:elink.
:pd.Show the display window.
:eparml.

.************************************************************************
.* Show
.************************************************************************
:h3 res=&HPM_SRC_DSP_SHOW..Help for Show
:i2 refid='src.dsp'.Show
:i2 refid='pmgdb'.Show
:lm margin=1.
:p.
Use this choice to show the display window.

.************************************************************************
.* Options
.************************************************************************
:h2 res=&HPM_SRC_OPTIONSMENU..Help for Options
:i2 refid='src'.Options menu
:i1 id='src.options'.Options menu choices
:lm margin=1.
:p.
The Options menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_PMDEBUGMODE..PM debugging mode:elink.
:pd.Select the mode for debugging PM applications.
:eparml.

.************************************************************************
.* Windows
.************************************************************************
:h2 res=&HPM_SRC_WINMENU..Help for Windows
:i2 refid='src'.Windows menu
:i1 id='src.win'.Windows menu choices (source window)
:lm margin=1.
:p.
The Windows menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_WIN_CMD..Command:elink.
:pd.Show the command window.
:pt.:link reftype=hd res=&HPM_WIN_SRCS..Source files:elink.
:pd.Show the source files window.
:pt.:link reftype=hd res=&HPM_WIN_BRK..Breakpoints:elink.
:pd.Show the breakpoints window.
:pt.:link reftype=hd res=&HPM_WIN_DSP..Display:elink.
:pd.Show the display window.
:pt.:link reftype=hd res=&HPM_WIN_THR..Threads:elink.
:pd.Show the threads window.
:pt.:link reftype=hd res=&HPM_WIN_REG..Register:elink.
:pd.Show the register window.
:eparml.

.************************************************************************
.* Command
.************************************************************************
:h3 res=&HPM_WIN_CMD..Help for Command
:i2 refid='src.win'.Command
:i2 refid='pmgdb'.Command
:lm margin=1.
:p.
Use this choice to show the command window.

.************************************************************************
.* Help
.************************************************************************
:h2 res=&HPM_SRC_HELPMENU..Help for Help
:i2 refid='src'.Help menu
:i1 id='src.help'.Help menu choices
:lm margin=1.
:p.
The Help menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_HELP_HELP..Help for help:elink.
:pd.Display help for help.
:pt.:link reftype=hd res=&HPM_HELP_EXT..Extended help:elink.
:pd.Display extended help.
:pt.:link reftype=hd res=&HPM_HELP_KEYS..Keys help:elink.
:pd.Display help about keys.
:pt.:link reftype=hd res=&HPM_HELP_INDEX..Help index:elink.
:pd.Display the help index.
:pt.:link reftype=hd res=&HPM_HELP_CONTENTS..Help index:elink.
:pd.Display the help contents.
:eparml.

.************************************************************************
.* Keys help (source window)
.************************************************************************
:h1 res=&HELP_SRC_KEYS..Keys help (source window)
:i1 id='src.keys'.Keys (source window)
:lm margin=1.
:p.
You can use the following keys in a source window&colon.
:dl tsize=20 compact.
:dt.Up arrow:dd.Scroll down by one line.
:dt.Down arrow:dd.Scroll up by one line.
:dt.Page Up:dd.Scroll down by one window.
:dt.Page Down:dd.Scroll up by one window.
:dt.Left:dd.Scroll left by one character (average character width).
:dt.Right:dd.Scroll right by one character (average character width).
:dt.Ctrl+Page Up:dd.Scroll down right by one window.
:dt.Ctrl+Page Down:dd.Scroll left by one window.
:edl.
:p.
Accelerator keys are missed out in the list above.

.************************************************************************
.* Source files window
.************************************************************************
:h1 res=&HELP_SRCS_WINDOW..Help for source files window
:i1 id='srcs'.pmgdb source files window
:lm margin=1.
:p.
The source files window of :hp2.pmgdb:ehp2. lists the names of
all source files for modules compiled with debugging enabled.
Double clicking with the first mouse button on a file name will show
an existing source window or open a new source window for that file.
Closing the source files window doesn't terminate :hp2.pmgdb:ehp2..
:p.
Help is available on the following topics&colon.
:p.
:ul.
:li.:link reftype=hd res=&HPM_SRCS_FILEMENU..File menu:elink.
:li.:link reftype=hd res=&HPM_SRCS_WINMENU..Windows menu:elink.
:li.:link reftype=hd res=&HPM_SRCS_HELPMENU..Help menu:elink.
:li.:link reftype=hd res=&HELP_SRCS_KEYS..Keys in the source files
window:elink.
:eul.

.************************************************************************
.* File
.************************************************************************
:h2 res=&HPM_SRCS_FILEMENU..Help for File
:i2 refid='srcs'.File menu
:i1 id='srcs.file'.File menu choices (source files window)
:lm margin=1.
:p.
The File menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_EXIT..Exit:elink.
:pd.Terminate :hp2.pmgdb:ehp2..
:eparml.

.************************************************************************
.* Windows
.************************************************************************
:h2 res=&HPM_SRCS_WINMENU..Help for Windows
:i2 refid='srcs'.Windows menu
:i1 id='srcs.win'.Windows menu choices (source files window)
:lm margin=1.
:p.
The Windows menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_WIN_CMD..Command:elink.
:pd.Show the command window.
:pt.:link reftype=hd res=&HPM_WIN_BRK..Breakpoints:elink.
:pd.Show the breakpoints window.
:pt.:link reftype=hd res=&HPM_WIN_DSP..Display:elink.
:pd.Show the display window.
:pt.:link reftype=hd res=&HPM_WIN_THR..Threads:elink.
:pd.Show the threads window.
:pt.:link reftype=hd res=&HPM_WIN_REG..Register:elink.
:pd.Show the register window.
:eparml.

.************************************************************************
.* Help
.************************************************************************
:h2 res=&HPM_SRCS_HELPMENU..Help for Help
:i2 refid='srcs'.Help menu
:i1 id='srcs.help'.Help menu choices
:lm margin=1.
:p.
The Help menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_HELP_HELP..Help for help:elink.
:pd.Display help for help.
:pt.:link reftype=hd res=&HPM_HELP_EXT..Extended help:elink.
:pd.Display extended help.
:pt.:link reftype=hd res=&HPM_HELP_KEYS..Keys help:elink.
:pd.Display help about keys.
:pt.:link reftype=hd res=&HPM_HELP_INDEX..Help index:elink.
:pd.Display the help index.
:pt.:link reftype=hd res=&HPM_HELP_CONTENTS..Help index:elink.
:pd.Display the help contents.
:eparml.

.************************************************************************
.* Keys help (source files window)
.************************************************************************
:h1 res=&HELP_SRCS_KEYS..Keys help (source files window)
:i1 id='srcs.keys'.Keys (source files window)
:lm margin=1.
:p.
You can use the following keys in the source files window&colon.
:dl tsize=20 compact.
:edl.
:p.
Accelerator keys are missed out in the list above.

.************************************************************************
.* Breakpoints window
.************************************************************************
:h1 res=&HELP_BRK_WINDOW..Help for breakpoints window
:i1 id='brk'.pmgdb breakpoints window
:lm margin=1.
:p.
The breakpoints window of :hp2.pmgdb:ehp2. shows all the breakpoints.
For each breakpoint, the following values are displayed&colon.
:dl tsize=12 compact.
:dt.No:dd.The number of the breakpoint as maintained by GDB.  Modifying
a breakpoint may change its number
:dt.Ena:dd.y if the breakpoint is enabled, n if the breakpoint is disabled
:dt.Address:dd.Address of the breakpoint
:dt.Source:dd.Source file for the breakpoint address
:dt.Line:dd.Line number for the breakpoint address
:dt.Disp:dd.Disposition of the breakpoint: `keep' means that the breakpoint
remains enabled when hit, `dis' means that the breakpoint will be disabled
when hit, `del' means that the breakpoint will be deleted when hit.
:dt.Ign:dd.Ignore count.  The breakpoint will be ignored this many
times.
:dt.Cond:dd.Condition.  This expression is evaluated whenever the
breakpoint is hit; the breakpoint breaks only if the expression is true.
:edl.
:p.
You can toggle the enabled state of a breakpoint by double clicking
with the first mouse button on the value in the `Ena' column.  You can
view the source code at the breakpoint by double clicking on the value
in the `Source' or `Line' column.
Closing the breakpoints window doesn't terminate :hp2.pmgdb:ehp2..
:p.
Help is available on the following topics&colon.
:p.
:ul.
:li.:link reftype=hd res=&HPM_BRK_FILEMENU..File menu:elink.
:li.:link reftype=hd res=&HPM_BRK_EDITMENU..Edit menu:elink.
:li.:link reftype=hd res=&HPM_BRK_BRKPTMENU..Breakpoints menu:elink.
:li.:link reftype=hd res=&HPM_BRK_WINMENU..Windows menu:elink.
:li.:link reftype=hd res=&HPM_BRK_HELPMENU..Help menu:elink.
:li.:link reftype=hd res=&HELP_BRK_KEYS..Keys in the breakpoints window:elink.
:eul.

.************************************************************************
.* File
.************************************************************************
:h2 res=&HPM_BRK_FILEMENU..Help for File
:i2 refid='brk'.File menu
:i1 id='brk.file'.File menu choices (breakpoints window)
:lm margin=1.
:p.
The File menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_EXIT..Exit:elink.
:pd.Terminate :hp2.pmgdb:ehp2..
:eparml.

.************************************************************************
.* Edit
.************************************************************************
:h2 res=&HPM_BRK_EDITMENU..Help for Edit
:i2 refid='brk'.Edit menu
:i1 id='brk.edit'.Edit menu choices (breakpoints window)
:lm margin=1.
:p.
The Edit menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_BRK_MODIFY..Modify:elink.
:pd.Modify the selected breakpoint.
:pt.:link reftype=hd res=&HPM_BRK_ENABLE..Enable:elink.
:pd.Enable the selected breakpoint.
:pt.:link reftype=hd res=&HPM_BRK_DISABLE..Disable:elink.
:pd.Disable the selected breakpoint.
:pt.:link reftype=hd res=&HPM_BRK_DELETE..Delete:elink.
:pd.Delete the selected breakpoint.
:eparml.

.************************************************************************
.* Modify
.************************************************************************
:h3 res=&HPM_BRK_MODIFY..Help for Modify
:i2 refid='brk.edit'.Modify
:i2 refid='pmgdb'.Modify breakpoint
:lm margin=1.
:p.
Use this choice to modify the selected breakpoint.  Click with the
first mouse button select a breakpoint.

.************************************************************************
.* Enable
.************************************************************************
:h3 res=&HPM_BRK_ENABLE..Help for Enable
:i2 refid='brk.edit'.Enable
:i2 refid='pmgdb'.Enable
:lm margin=1.
:p.
Use this choice to enable the selected breakpoint.  Click with the
first mouse button select a breakpoint.

.************************************************************************
.* Disable
.************************************************************************
:h3 res=&HPM_BRK_DISABLE..Help for Disable
:i2 refid='brk.edit'.Disable
:i2 refid='pmgdb'.Disable breakpoint
:lm margin=1.
:p.
Use this choice to disable the selected breakpoint.  Click with the
first mouse button select a breakpoint.

.************************************************************************
.* Delete
.************************************************************************
:h3 res=&HPM_BRK_DELETE..Help for Delete
:i2 refid='brk.edit'.Delete
:i2 refid='pmgdb'.Delete breakpoint
:lm margin=1.
:p.
Use this choice to delete the selected breakpoint.  Click with the
first mouse button select a breakpoint.

.************************************************************************
.* Breakpoints
.************************************************************************
:h2 res=&HPM_BRK_BRKPTMENU..Help for Breakpoints
:i2 refid='brk'.Breakpoints menu
:i1 id='brk.brkpt'.Breakpoints menu choices (breakpoints window)
:lm margin=1.
:p.
The Breakpoints menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_BRKPT_LINE..On line:elink.
:pd.Set a breakpoint on a line.
:pt.:link reftype=hd res=&HPM_BRK_REFRESH..Refresh:elink.
:pd.Refresh the window contents.
:eparml.

.************************************************************************
.* Refresh
.************************************************************************
:h3 res=&HPM_BRK_REFRESH..Help for Refresh
:i2 refid='brk.brkpt'.Refresh
:i2 refid='pmgdb'.Refresh (breakpoints window)
:lm margin=1.
:p.
Use this choice to refresh the contents of the breakpoints window.
Due to shortcomings in GDB, :hp2.pmgdb:ehp2. does not always automatically
update the breakpoints window to reflect changes of the breakpoints.
For instance, the ignore count is not updated when the debuggee stops.

.************************************************************************
.* Windows
.************************************************************************
:h2 res=&HPM_BRK_WINMENU..Help for Windows
:i2 refid='brk'.Windows menu
:i1 id='brk.win'.Windows menu choices (breakpoints window)
:lm margin=1.
:p.
The Windows menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_WIN_CMD..Command:elink.
:pd.Show the command window.
:pt.:link reftype=hd res=&HPM_WIN_SRCS..Source files:elink.
:pd.Show the source files window.
:pt.:link reftype=hd res=&HPM_WIN_DSP..Display:elink.
:pd.Show the display window.
:pt.:link reftype=hd res=&HPM_WIN_THR..Threads:elink.
:pd.Show the threads window.
:pt.:link reftype=hd res=&HPM_WIN_REG..Register:elink.
:pd.Show the register window.
:eparml.

.************************************************************************
.* Help
.************************************************************************
:h2 res=&HPM_BRK_HELPMENU..Help for Help
:i2 refid='brk'.Help menu
:i1 id='brk.help'.Help menu choices
:lm margin=1.
:p.
The Help menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_HELP_HELP..Help for help:elink.
:pd.Display help for help.
:pt.:link reftype=hd res=&HPM_HELP_EXT..Extended help:elink.
:pd.Display extended help.
:pt.:link reftype=hd res=&HPM_HELP_KEYS..Keys help:elink.
:pd.Display help about keys.
:pt.:link reftype=hd res=&HPM_HELP_INDEX..Help index:elink.
:pd.Display the help index.
:pt.:link reftype=hd res=&HPM_HELP_CONTENTS..Help index:elink.
:pd.Display the help contents.
:eparml.

.************************************************************************
.* Keys help (breakpoints window)
.************************************************************************
:h1 res=&HELP_BRK_KEYS..Keys help (breakpoints window)
:i1 id='brk.keys'.Keys (breakpoints window)
:lm margin=1.
:p.
You can use the following keys in the breakpoints window&colon.
:dl tsize=20 compact.
:edl.
:p.
Accelerator keys are missed out in the list above.

.************************************************************************
.* Display window
.************************************************************************
:h1 res=&HELP_DSP_WINDOW..Help for display window
:i1 id='dsp'.pmgdb display window
:lm margin=1.
:p.
The display window of :hp2.pmgdb:ehp2. shows all the displays.
For each display, the following values are displayed&colon.
:dl tsize=12 compact.
:dt.No:dd.The number of the display as maintained by GDB.  Modifying
a display may change its number
:dt.Ena:dd.y if the display is enabled, n if the display is disabled
:dt.Fmt:dd.Format to be used for displaying the value
:dt.Expr:dd.Expression to be evaluated
:dt.Value:dd.Value of the expression (if the display is enabled)
:edl.
:p.
You can toggle the enabled state of a display by double clicking
with the first mouse button on the value in the `Ena' column.  You can
modify a display by double clicking on the value
in the `Fmt' or `Expr' column.
Closing the display window doesn't terminate :hp2.pmgdb:ehp2..
:p.
Help is available on the following topics&colon.
:p.
:ul.
:li.:link reftype=hd res=&HPM_DSP_FILEMENU..File menu:elink.
:li.:link reftype=hd res=&HPM_DSP_EDITMENU..Edit menu:elink.
:li.:link reftype=hd res=&HPM_DSP_WINMENU..Windows menu:elink.
:li.:link reftype=hd res=&HPM_DSP_HELPMENU..Help menu:elink.
:li.:link reftype=hd res=&HELP_DSP_KEYS..Keys in the display window:elink.
:eul.

.************************************************************************
.* File
.************************************************************************
:h2 res=&HPM_DSP_FILEMENU..Help for File
:i2 refid='dsp'.File menu
:i1 id='dsp.file'.File menu choices (display window)
:lm margin=1.
:p.
The File menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_EXIT..Exit:elink.
:pd.Terminate :hp2.pmgdb:ehp2..
:eparml.

.************************************************************************
.* Edit
.************************************************************************
:h2 res=&HPM_DSP_EDITMENU..Help for Edit
:i2 refid='dsp'.Edit menu
:i1 id='dsp.edit'.Edit menu choices (display window)
:lm margin=1.
:p.
The Edit menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_DSP_ADD..Add:elink.
:pd.Add a display.
:pt.:link reftype=hd res=&HPM_DSP_MODIFY..Modify:elink.
:pd.Modify the selected display.
:pt.:link reftype=hd res=&HPM_DSP_ENABLE..Enable:elink.
:pd.Enable the selected display.
:pt.:link reftype=hd res=&HPM_DSP_DISABLE..Disable:elink.
:pd.Disable the selected display.
:pt.:link reftype=hd res=&HPM_DSP_FORMAT..Format:elink.
:pd.Change format for selected display.
:pt.:link reftype=hd res=&HPM_DSP_DELETE..Delete:elink.
:pd.Delete the selected display.
:pt.:link reftype=hd res=&HPM_DSP_DEREFERENCE..Dereference:elink.
:pd.Display the value pointed to by a pointer.
:pt.:link reftype=hd res=&HPM_DSP_ENABLE_ALL..Enable all:elink.
:pd.Enable all displays.
:pt.:link reftype=hd res=&HPM_DSP_DISABLE_ALL..Disable all:elink.
:pd.Disable all displays.
:pt.:link reftype=hd res=&HPM_DSP_DELETE_ALL..Delete all:elink.
:pd.Delete all displays.
:eparml.

.************************************************************************
.* Add
.************************************************************************
:h3 res=&HPM_DSP_ADD..Help for Add
:i2 refid='dsp.dsp'.Add
:i2 refid='pmgdb'.Add display
:lm margin=1.
:p.
Use this choice to add a display.  This choice opens the
:link reftype=hd res=&HPD_DISPLAY..Display:elink. dialog box.
You can also double click with the first mouse button on a variable
in a source window to add it to the display window.

.************************************************************************
.* Modify
.************************************************************************
:h3 res=&HPM_DSP_MODIFY..Help for Modify
:i2 refid='dsp.edit'.Modify
:i2 refid='pmgdb'.Modify display
:lm margin=1.
:p.
Use this choice to modify the selected display.  This choice opens the
:link reftype=hd res=&HPD_DISPLAY..Display:elink. dialog box.  Click with the
first mouse button select a display.

.************************************************************************
.* Enable
.************************************************************************
:h3 res=&HPM_DSP_ENABLE..Help for Enable
:i2 refid='dsp.edit'.Enable
:i2 refid='pmgdb'.Enable display
:lm margin=1.
:p.
Use this choice to enable the selected display.  Click with the
first mouse button select a display.

.************************************************************************
.* Disable
.************************************************************************
:h3 res=&HPM_DSP_DISABLE..Help for Disable
:i2 refid='dsp.edit'.Disable
:i2 refid='pmgdb'.Disable display
:lm margin=1.
:p.
Use this choice to disable the selected display.  Click with the
first mouse button select a display.

.************************************************************************
.* Format
.************************************************************************
:h3 res=&HPM_DSP_FORMAT..Help for Format
:i2 refid='dsp.edit'.Format
:i2 refid='pmgdb'.Format
:lm margin=1.
:p.
Use this choice to change the format of the selected display.  Click with the
first mouse button select a display.

.************************************************************************
.* Delete
.************************************************************************
:h3 res=&HPM_DSP_DELETE..Help for Delete
:i2 refid='dsp.edit'.Delete
:i2 refid='pmgdb'.Delete display
:lm margin=1.
:p.
Use this choice to delete the selected display.  Click with the
first mouse button select a display.

.************************************************************************
.* Dereference
.************************************************************************
:h3 res=&HPM_DSP_DEREFERENCE..Help for Dereference
:i2 refid='dsp.edit'.Dereference
:i2 refid='pmgdb'.Dereference
:lm margin=1.
:p.
Use this choice to replace the selected display with one which shows
the object pointed to by the display's value.  This can be done only
if the selected display shows a pointer.  Click with the first mouse
button select a display.
:note.The value is dereferenced in the current context, not in the
context which was active when the display was created.

.************************************************************************
.* Enable all
.************************************************************************
:h3 res=&HPM_DSP_ENABLE_ALL..Help for Enable all
:i2 refid='dsp.edit'.Enable all
:i2 refid='pmgdb'.Enable all
:lm margin=1.
:p.
Use this choice to enable all displays.

.************************************************************************
.* Disable all
.************************************************************************
:h3 res=&HPM_DSP_DISABLE_ALL..Help for Disable all
:i2 refid='dsp.edit'.Disable all
:i2 refid='pmgdb'.Disable all
:lm margin=1.
:p.
Use this choice to disable all displays.

.************************************************************************
.* Delete all
.************************************************************************
:h3 res=&HPM_DSP_DELETE_ALL..Help for Delete all
:i2 refid='dsp.edit'.Delete all
:i2 refid='pmgdb'.Delete all
:lm margin=1.
:p.
Use this choice to delete all displays.

.************************************************************************
.* Windows
.************************************************************************
:h2 res=&HPM_DSP_WINMENU..Help for Windows
:i2 refid='dsp'.Windows menu
:i1 id='dsp.win'.Windows menu choices (display window)
:lm margin=1.
:p.
The Windows menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_WIN_CMD..Command:elink.
:pd.Show the command window.
:pt.:link reftype=hd res=&HPM_WIN_SRCS..Source files:elink.
:pd.Show the source files window.
:pt.:link reftype=hd res=&HPM_WIN_BRK..Breakpoints:elink.
:pd.Show the breakpoints window.
:pt.:link reftype=hd res=&HPM_WIN_THR..Threads:elink.
:pd.Show the threads window.
:pt.:link reftype=hd res=&HPM_WIN_REG..Register:elink.
:pd.Show the register window.
:eparml.

.************************************************************************
.* Help
.************************************************************************
:h2 res=&HPM_DSP_HELPMENU..Help for Help
:i2 refid='dsp'.Help menu
:i1 id='dsp.help'.Help menu choices
:lm margin=1.
:p.
The Help menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_HELP_HELP..Help for help:elink.
:pd.Display help for help.
:pt.:link reftype=hd res=&HPM_HELP_EXT..Extended help:elink.
:pd.Display extended help.
:pt.:link reftype=hd res=&HPM_HELP_KEYS..Keys help:elink.
:pd.Display help about keys.
:pt.:link reftype=hd res=&HPM_HELP_INDEX..Help index:elink.
:pd.Display the help index.
:pt.:link reftype=hd res=&HPM_HELP_CONTENTS..Help index:elink.
:pd.Display the help contents.
:eparml.

.************************************************************************
.* Keys help (display window)
.************************************************************************
:h1 res=&HELP_DSP_KEYS..Keys help (display window)
:i1 id='dsp.keys'.Keys (display window)
:lm margin=1.
:p.
You can use the following keys in the display window&colon.
:dl tsize=20 compact.
:edl.
:p.
Accelerator keys are missed out in the list above.

.************************************************************************
.* Threads window
.************************************************************************
:h1 res=&HELP_THR_WINDOW..Help for threads window
:i1 id='thr'.pmgdb threads window
:lm margin=1.
:p.
The threads window of :hp2.pmgdb:ehp2. shows all threads.
For each thread, the following values are displayed&colon.
:dl tsize=12 compact.
:dt.TID:dd.The thread ID
:dt.Ena:dd.y if the thread is enabled, n if the thread is disabled
:edl.
:p.
You can toggle the enabled state of a thread by double clicking
with the first mouse button on the value in the `Ena' column.
Closing the threads window doesn't terminate :hp2.pmgdb:ehp2..
:p.
Help is available on the following topics&colon.
:p.
:ul.
:li.:link reftype=hd res=&HPM_THR_FILEMENU..File menu:elink.
:li.:link reftype=hd res=&HPM_THR_EDITMENU..Edit menu:elink.
:li.:link reftype=hd res=&HPM_THR_WINMENU..Windows menu:elink.
:li.:link reftype=hd res=&HELP_THR_KEYS..Keys in the threads window:elink.
:eul.

.************************************************************************
.* File
.************************************************************************
:h2 res=&HPM_THR_FILEMENU..Help for File
:i2 refid='thr'.File menu
:i1 id='thr.file'.File menu choices (threads window)
:lm margin=1.
:p.
The File menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_EXIT..Exit:elink.
:pd.Terminate :hp2.pmgdb:ehp2..
:eparml.

.************************************************************************
.* Edit
.************************************************************************
:h2 res=&HPM_THR_EDITMENU..Help for Edit
:i2 refid='thr'.Edit menu
:i1 id='thr.edit'.Edit menu choices (threads window)
:lm margin=1.
:p.
The Edit menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_THR_ENABLE..Enable (thaw):elink.
:pd.Enable (thaw) the selected thread.
:pt.:link reftype=hd res=&HPM_THR_DISABLE..Disable (freeze):elink.
:pd.Disable (freeze) the selected thread.
:pt.:link reftype=hd res=&HPM_THR_SWITCH..Switch to:elink.
:pd.Switch to the selected thread.
:eparml.

.************************************************************************
.* Enable
.************************************************************************
:h3 res=&HPM_THR_ENABLE..Help for Enable
:i2 refid='thr.edit'.Enable
:i2 refid='pmgdb'.Enable thread
:lm margin=1.
:p.
Use this choice to enable the selected thread.  Click with the
first mouse button select a thread.

.************************************************************************
.* Disable
.************************************************************************
:h3 res=&HPM_THR_DISABLE..Help for Disable
:i2 refid='thr.edit'.Disable
:i2 refid='pmgdb'.Disable thread
:lm margin=1.
:p.
Use this choice to disable the selected thread.  Click with the
first mouse button select a thread.

.************************************************************************
.* Switch to
.************************************************************************
:h3 res=&HPM_THR_SWITCH..Help for Switch to
:i2 refid='thr.edit'.Switch to
:i2 refid='pmgdb'.Switch to thread
:lm margin=1.
:p.
Use this choice to switch to the selected thread.  Click with the
first mouse button select a thread.  You can also switch to a thread
by double clicking on the thread ID (TID).

.************************************************************************
.* Windows
.************************************************************************
:h2 res=&HPM_THR_WINMENU..Help for Windows
:i2 refid='thr'.Windows menu
:i1 id='thr.win'.Windows menu choices (threads window)
:lm margin=1.
:p.
The Windows menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_WIN_CMD..Command:elink.
:pd.Show the command window.
:pt.:link reftype=hd res=&HPM_WIN_SRCS..Source files:elink.
:pd.Show the source files window.
:pt.:link reftype=hd res=&HPM_WIN_BRK..Breakpoints:elink.
:pd.Show the breakpoints window.
:pt.:link reftype=hd res=&HPM_WIN_DSP..Display:elink.
:pd.Show the display window.
:pt.:link reftype=hd res=&HPM_WIN_REG..Register:elink.
:pd.Show the register window.
:eparml.

.************************************************************************
.* Keys help (threads window)
.************************************************************************
:h1 res=&HELP_THR_KEYS..Keys help (threads window)
:i1 id='thr.keys'.Keys (threads window)
:lm margin=1.
:p.
You can use the following keys in the threads window&colon.
:dl tsize=20 compact.
:edl.
:p.
Accelerator keys are missed out in the list above.

.************************************************************************
.* Register window
.************************************************************************
:h1 res=&HELP_REG_WINDOW..Help for register window
:i1 id='reg'.pmgdb register window
:lm margin=1.
:p.
The register window of :hp2.pmgdb:ehp2. shows the values of the
CPU registers. Closing the register window doesn't terminate
:hp2.pmgdb:ehp2..
:p.
Help is available on the following topics&colon.
:p.
:ul.
:li.:link reftype=hd res=&HPM_REG_FILEMENU..File menu:elink.
:li.:link reftype=hd res=&HPM_REG_VIEWMENU..View menu:elink.
:li.:link reftype=hd res=&HPM_REG_WINMENU..Windows menu:elink.
:li.:link reftype=hd res=&HELP_REG_KEYS..Keys in the register window:elink.
:eul.

.************************************************************************
.* File
.************************************************************************
:h2 res=&HPM_REG_FILEMENU..Help for File
:i2 refid='reg'.File menu
:i1 id='reg.file'.File menu choices (register window)
:lm margin=1.
:p.
The File menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_EXIT..Exit:elink.
:pd.Terminate :hp2.pmgdb:ehp2..
:eparml.

.************************************************************************
.* View
.************************************************************************
:h2 res=&HPM_REG_VIEWMENU..Help for View
:i2 refid='reg'.View menu
:i1 id='reg.view'.View menu choices (register window)
:lm margin=1.
:p.
The View menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_REG_REFRESH..Refresh:elink.
:pd.Refresh the window contents.
:eparml.

.************************************************************************
.* Refresh
.************************************************************************
:h3 res=&HPM_REG_REFRESH..Help for Refresh
:i2 refid='reg.view'.Refresh
:i2 refid='pmgdb'.Refresh (register window)
:lm margin=1.
:p.
Use this choice to refresh the contents of the register window.
:hp2.pmgdb:ehp2. does not automatically update the register window
to reflect modifications of the register values done with GDB commands.

.************************************************************************
.* Windows
.************************************************************************
:h2 res=&HPM_REG_WINMENU..Help for Windows
:i2 refid='reg'.Windows menu
:i1 id='reg.win'.Windows menu choices (register window)
:lm margin=1.
:p.
The Windows menu contains the following choices&colon.
:parml compact.
:pt.:link reftype=hd res=&HPM_WIN_CMD..Command:elink.
:pd.Show the command window.
:pt.:link reftype=hd res=&HPM_WIN_SRCS..Source files:elink.
:pd.Show the source files window.
:pt.:link reftype=hd res=&HPM_WIN_BRK..Breakpoints:elink.
:pd.Show the breakpoints window.
:pt.:link reftype=hd res=&HPM_WIN_DSP..Display:elink.
:pd.Show the display window.
:pt.:link reftype=hd res=&HPM_WIN_THR..Threads:elink.
:pd.Show the threads window.
:eparml.

.************************************************************************
.* Keys help (register window)
.************************************************************************
:h1 res=&HELP_REG_KEYS..Keys help (register window)
:i1 id='reg.keys'.Keys (register window)
:lm margin=1.
:p.
You can use the following keys in the register window&colon.
:dl tsize=20 compact.
:edl.
:p.
Accelerator keys are missed out in the list above.

.************************************************************************
.* PM debugging mode
.************************************************************************
:h1 res=&HPD_PMDEBUGMODE..Help for PM debugging mode dialog box
:i2 refid='pmgdb'.PM debugging mode dialog box
:lm margin=1.
:p.
Select the :hp8.Synchronous mode:ehp8. checkbox to enable
Synchronous mode.  Synchronous mode is required for debugging PM
applications and must be selected before starting a PM application,
otherwise the Presentation Manager will hang, requiring a reboot to
recover.  In Synchronous mode, only windows of :hp2.pmgdb:ehp2. and
of the program being debugged can be made active; all other windows
cannot be used.  :hp2.pmgdb:ehp2. attempts to set Synchronous mode
automatically when a PM program is loaded.

.************************************************************************
.* Line breakpoint
.************************************************************************
:h1 res=&HPD_BRKPT_LINE..Help for Line Breakpoint dialog box
:i2 refid='pmgdb'.Line Breakpoint dialog box
:lm margin=1.
:p.
This dialog box is used for adding or modifying a line breakpoint.
A line breakpoint is a breakpoint in a source line of the program.
:p.
Enter the name of the source file and the line number in that source file
in the :hp8.Source file:ehp8. and :hp8.Line:ehp8. fields, respectively.
:p.
A non-zero :hp8.Ignore count:ehp8. causes the breakpoint to not stop
the program until it has been reached this many times.  If this
field is empty or contains zero, the program is stopped the first time
it reaches the breakpoint.  Each time a breakpoint is reached, the ignore
count is decremented by one (unless it's already zero); if the ignore
count becomes zero (or was zero before), the breakpoint stops the program.
:p.
You can enter an expression in the :hp8.Condition:ehp8. field; that
expression will be evaluated each time the breakpoint is hit (unless
its ignore count is still non-zero); if it evaluates to false, the
breakpoint is ignored.  That is, the breakpoint stops the program only
if the expression evaluates to true at the time the breakpoint is hit.
See the :hp2.GDB:ehp2. manual for details on expressions.
:p.
Breakpoints can be disabled by unchecking the :hp8.Enable:ehp8.
checkbox.  Disabled breakpoints are ignored.  Instead of deleting
breakpoints which you may need again later, you should disable them.
:p.
In the :hp8.Disposition:ehp8. field, you can select what to do with a
breakpoint when it stops the program&colon.
:dl tsize=20 compact.
:dt.&tt.del&ett.:dd.delete the breakpoint
:dt.&tt.dis&ett.:dd.disable the breakpoint
:dt.&tt.keep&ett.:dd.keep the breakpoint (default)
:edl.
:p.
This can be used to define temporary breakpoints.

.************************************************************************
.* Startup
.************************************************************************
:h1 res=&HPD_STARTUP..Help for Startup dialog box
:i2 refid='pmgdb'.Startup dialog box
:lm margin=1.
:p.
This dialog box is used for setting startup options.
You can enter command line arguments for the debuggee
in the :hp8.Arguments:ehp8. field.
:p.
By default, the VIO window of the debuggee stays open after termination
of the debuggee, so that you can view the debuggee's output.  You
have to close that window manually.  If :hp8.Close window after exit:ehp8.
is checked, the VIO window will be closed automatically on termination
of the debuggee.
:p.
Use the :hp8.OK:ehp8. button to accept the settings, use the
:hp8.Run:ehp8. button to accept the settings and start the program.
If no breakpoints are set (or all breakpoints are disabled), a
breakpoint will be set on &tt.main&ett. if the :hp8.Run:ehp8. button
is selected.

.************************************************************************
.* Display
.************************************************************************
:h1 res=&HPD_DISPLAY..Help for Display dialog box
:i2 refid='pmgdb'.Display dialog box
:lm margin=1.
:p.
This dialog box is used to add or modify a display.
:p.
Enter the expression to be displayed in the :hp8.Expression:ehp8.
field.  See the :hp2.GDB:ehp2. manual for details on expressions.
:p.
Select a display format in the :hp8.Format:ehp8. field.
:p.
For some formats, the number of items to be displayed can be entered
in the :hp8.Count:ehp8. field.
:p.
Displays can be disabled by unchecking the :hp8.Enable:ehp8.
checkbox.  The values of disabled displays are not displayed.
Instead of deleting displays which you may need again later, you should
disable them.

.************************************************************************
.* Go to line
.************************************************************************
:h1 res=&HPD_GOTO..Help for Go to line dialog box
:i2 refid='pmgdb'.Go to line dialog box
:lm margin=1.
:p.
Enter a line number in the :hp8.Line:ehp8. field.

.************************************************************************
.* Find
.************************************************************************
:h1 res=&HPD_FIND..Help for Find dialog box
:i2 refid='pmgdb'.Find dialog box
:lm margin=1.
:p.
Enter the regular expression to be searched for in the :hp8.Regexp:ehp8.
field.

.************************************************************************
.* History
.************************************************************************
:h1 res=&HPD_HISTORY..Help for History dialog box
:i2 refid='pmgdb'.History dialog box
:lm margin=1.
:p.
In this dialog box, you can select and/or edit one of the commands
previously entered.

.************************************************************************
.* Tutorial
.************************************************************************
:h1 res=&HELP_TUTORIAL..pmgdb tutorial
:i2 refid='pmgdb'.Tutorial
:i1 id='tut'.Tutorial
:lm margin=1.
:p.
:ol.
:li.If not already done, compile the &tt.sieve&ett. sample program
for debugging&colon.
:cgraphic.
cd \emx\samples
gcc -g sieve.c:ecgraphic.
:li.Start :hp2.pmgdb:ehp2. in the directory containing the source code
of the program to debug (debuggee).  Specify the program and its command
line arguments on the command line of :hp2.pmgdb:ehp2.&colon.
:cgraphic.
cd \emx\samples
pmgdb sieve 100
:ecgraphic.
:p.
That is, just prepend &tt.pmgdb&ett. to the command line.
:li.:hp2.pmgdb:ehp2. will come up with two windows&colon.
the :link reftype=hd res=&HELP_CMD_WINDOW..command window:elink.,
titled `&tt.pmgdb - sieve&ett.', and the 
:link reftype=hd res=&HELP_SRCS_WINDOW..Source files window:elink..
The command window shows :hp2.GDB:ehp2.'s welcome message and the
:hp2.GDB:ehp2. prompt.  The Source Files window lists all the source
files of the debuggee.  For this sample session, that's just
&tt.sieve.c&ett..  The source file which contains the &tt.main&ett.
function is highlighted (grey background).  You can view a source
file by double clicking on a file name in the Source files window
(click on the file name, not on the blank space after the file name).
However we don't need to do this now.
:li.Run the program by selecting :link reftype=hd res=&HPM_GO..Go:elink.
from the :link reftype=hd res=&HPM_CMD_RUNMENU..Run menu:elink..
Note that the command window shows that a breakpoint has been hit&colon.
:cgraphic.
Breakpoint 1, main (argc=2, argv=0x282ffcc) at sieve.c&colon.76
:ecgraphic.
:p.
That breakpoint has been set automatically by :hp2.pmgdb:ehp2..
Moreover, a
:link reftype=hd res=&HELP_SRC_WINDOW..Source window:elink.
has popped up.  The first executable line of the &tt.main&ett. function
has black background.  The line where the debuggee has been stopped
is marked this way.  Moreover, the line number has a red background; this
means that a breakpoint is set on that line.
:li.Now let's set a breakpoint on the &tt.isqrt&ett. function.
Either type
:cgraphic.
b isqrt
:ecgraphic.
:p.
in the command window or double click on the line number of line 45
(the first executable line of the function).
Let the program continue execution.  (Remember?  Go...)
:li.While stepping through &tt.isqrt&ett. we want to watch the
values of the variables &tt.l&ett., &tt.r&ett., and &tt.x&ett.
in :hp2.pmgdb:ehp2.'s
:link reftype=hd res=&HELP_DSP_WINDOW..Display window:elink..
This is done by double clicking on any instance of &tt.l&ett.,
&tt.r&ett., and &tt.x&ett. in the source code (:hp2.GDB:ehp2. uses
dynamic scoping, that is, it uses those variables which are visible at
the time the display is defined.  Other debuggers use static scoping,
that is, they use the source code context in which the display
is defined.)  The Display window should become visible automatically.
Move and/or resize the windows so that you can see both the Display
window and the source window.  The two most important columns of
the Display window are :hp8.Expr:ehp8. (the expression being displayed,
for instance, a variable) and :hp8.Value:ehp8. (the value of the
expression).  Note that the Display window
shows garbage for the &tt.l&ett. and &tt.r&ett. variables.  This
is because these variables have not yet been initialized, so they
indeed contain garbage.
:li.Now let's execute some lines of C code (single step).
There are two operations for stepping lines of source code&colon.
:link reftype=hd res=&HPM_STEPOVER..Step over:elink.
and :link reftype=hd res=&HPM_STEPINTO..Step into:elink.,
both available in the
:link reftype=hd res=&HPM_CMD_RUNMENU..Run menu:elink..
The first one does not step into function calls, treating function
calls as one statement.  The second one does step into function calls.
As &tt.isqrt&ett. does not call any functions, there's no difference
between the two here.  Step over five lines of source code by hitting
the &tt.o&ett. key or &tt.i&ett. key five times while the source window
has the focus.  Note how the values of &tt.l&ett. and &tt.r&ett. become valid.
:li.In addition to the three variables, we want to watch the value
of the expression `&tt.m*m&ett.'.  Select
:link reftype=hd res=&HPM_DSP_ADD..Add:elink. from the
:link reftype=hd res=&HPM_SRC_DSPMENU..Display menu:elink.
and type `&tt.m*m&ett.' in the :hp8.Expression:ehp8. field
of the dialog box and hit Return or click the :hp8.OK:ehp8. button.
Note that the expression and its value have been added to the Display
window.  You can also add displays by using :hp2.GDB:ehp2.'s &tt.display&ett.
command&colon.
:cgraphic.
disp m*m
:ecgraphic.
:p.
Continue stepping some (say, 20) source lines and watch the Display window.
:li.To let &tt.isqrt&ett. run to its end and stop at the place where
it has been called, select
:link reftype=hd res=&HPM_FINISH..Finish:elink. from the
:link reftype=hd res=&HPM_CMD_RUNMENU..Run menu:elink.
or hit the &tt.f&ett. key.  The return value is displayed in
the command window.  Note that the program is stopped in
the line containing the call to &tt.isqrt&ett., not on the line after.
This is because there's still some code of that line not yet executed.
As examining &tt.sqrt_size&ett. shows, the return value of &tt.isqrt&ett.
has not yet been assigned to that variable.  If you don't want to
clutter the Display window with values which you want to examine only
once, use :hp2.GDB:ehp2.'s &tt.print&ett. command in the
command window&colon.
:cgraphic.
p sqrt_size
:ecgraphic.
:p.
Hit the &tt.o&ett. key to execute the rest of the code of that line.
:li.Now we want to trace through the code of &tt.main&ett. which
prints the result.  That code starts at line 113.  This time, we
don't set a breakpoint.  Instead, select line 113 by clicking once
on the line number of that line.  (You can scroll to an arbitrary line by
selecting :link reftype=hd res=&HPM_SRC_GOTO..Go to line:elink.
from the :link reftype=hd res=&HPM_SRC_VIEWMENU..View menu:elink.
and typing the line number.)  Then, either select
:link reftype=hd res=&HPM_SRC_UNTIL..Until:elink. from the
:link reftype=hd res=&HPM_CMD_RUNMENU..Run menu:elink.
or hit the &tt.u&ett. key.  This operations works by setting
a temporary breakpoint, which will vanish after completion of
the operation.  Therefore, the line number does not have a red
background.  This time, we do not want to step into functions
(such as &tt.printf&ett.), therefore use the &tt.o&ett. key
to step over the lines.  If you're tired, hit the &tt.g&ett. key.
After stepping the last line, :hp2.GDB:ehp2. will say
:cgraphic.
Program exited normally.
:ecgraphic.
:p.
in the command window.  Note that the window in which &tt.sieve.exe&ett.
was running won't go away automatically; the title changed
to
:cgraphic.
Completed&colon. sieve.exe
:ecgraphic.
:p.
and you have to close it manually.  The application window will
be closed automatically if you select :hp8.Close window after exit:ehp8..
in the dialog box popped up by selecting 
:link reftype=hd res=&HPM_CMD_STARTUP..Startup:elink.
from the :link reftype=hd res=&HPM_CMD_FILEMENU..File menu:elink.
before starting the program.
:eol.

:euserdoc.
