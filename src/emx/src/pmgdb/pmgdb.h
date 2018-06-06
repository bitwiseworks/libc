/* pmgdb.h -*- C++ -*-
   Copyright (c) 1996 Eberhard Mattes

This file is part of pmgdb.

pmgdb is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

pmgdb is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pmgdb; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#define ID_COMMAND_WINDOW       1
#define ID_SOURCE_WINDOW        2
#define ID_BREAKPOINTS_WINDOW   3
#define ID_DISPLAY_WINDOW       4
#define ID_THREADS_WINDOW       5
#define ID_SOURCE_FILES_WINDOW  6
#define ID_REGISTER_WINDOW      7

#define IDM_FILEMENU    1000
#define IDM_EXIT        1001
#define IDM_OPEN_EXEC   1003
#define IDM_OPEN_CORE   1004
#define IDM_OPEN_SOURCE 1005
#define IDM_STARTUP     1006
#define IDM_WHERE       1007
#define IDM_RESTART     1009
#define IDM_RUNMENU     1100
#define IDM_GO          1101
#define IDM_STEPINTO    1102
#define IDM_STEPOVER    1103
#define IDM_ISTEPINTO   1104
#define IDM_ISTEPOVER   1105
#define IDM_JUMP        1106
#define IDM_UNTIL       1107
#define IDM_FINISH      1108
#define IDM_BRKPTMENU   1200
#define IDM_BRKPT_LIST  1201
#define IDM_BRKPT_LINE  1202
#define IDM_OPTIONSMENU 1300
#define IDM_PMDEBUGMODE 1301
#define IDM_ANNOTATE    1302
#define IDM_FONT        1303
#define IDM_EDITMENU    1400
#define IDM_ENABLE      1401
#define IDM_DISABLE     1402
#define IDM_MODIFY      1403
#define IDM_DELETE      1404
#define IDM_ENABLE_ALL  1405
#define IDM_DISABLE_ALL 1406
#define IDM_DELETE_ALL  1407
#define IDM_SWITCH      1408
#define IDM_ADD         1409
#define IDM_DEREFERENCE 1410
#define IDM_COMPLETE    1411
#define IDM_HISTORY     1412
#define IDM_WINMENU     1500
#define IDM_WIN_CMD     1501
#define IDM_WIN_THR     1502
#define IDM_WIN_BRK     1503
#define IDM_WIN_DSP     1504
#define IDM_WIN_SRCS    1505
#define IDM_WIN_REG     1506
#define IDM_REPMENU     1600
#define IDM_REP_DEC_S   1601
#define IDM_REP_DEC_U   1602
#define IDM_REP_HEX     1603
#define IDM_REP_OCT     1604
#define IDM_REP_BIN     1605
#define IDM_REP_CHR     1606
#define IDM_REP_ADR     1607
#define IDM_REP_FLT     1608
#define IDM_REP_STR     1609
#define IDM_REP_INS     1610
#define IDM_DSPMENU     1700
#define IDM_DSP_SHOW    1701
#define IDM_DSP_ADD     1702
#define IDM_HELPMENU    1800
#define IDM_HELP_EXT    1801
#define IDM_HELP_KEYS   1802
#define IDM_HELP_INDEX  1803
#define IDM_HELP_CONTENTS 1804
#define IDM_HELP_HELP   1805
#define IDM_ABOUT       1806
#define IDM_TUTORIAL    1807
#define IDM_VIEWMENU    1900
#define IDM_GOTO        1901
#define IDM_FIND        1902
#define IDM_FINDNEXT    1903
#define IDM_FINDPREV    1904
#define IDM_REFRESH     1905
#define IDM_WIN_SRC     2000    // ...and following IDs
#define IDM_COMPLETIONS 3000    // ...and following IDs

#define IDD_PMDEBUGMODE 1000
#define IDD_BRKPT_LINE  1001
#define IDD_STARTUP     1002
#define IDD_DISPLAY     1003
#define IDD_GOTO        1004
#define IDD_ABOUT       1005
#define IDD_FIND        1006
#define IDD_HISTORY     1007

#define IDC_HELP        1000
#define IDC_SYNC        1001
#define IDC_ENABLE      1002
#define IDC_CLOSE_WIN   1003
#define IDC_ARGS        1004
#define IDC_EXPR        1005
#define IDC_FORMAT      1006
#define IDC_COUNT       1007
#define IDC_SOURCE      1008
#define IDC_LINENO      1009
#define IDC_RUN         1010
#define IDC_CONDITION   1011
#define IDC_DISPOSITION 1012
#define IDC_STRING      1013
#define IDC_LIST        1014
#define IDC_IGNORE      1015

#define UWM_STATE               (WM_USER+0)
#define UWM_SOURCE              (WM_USER+1)
#define UWM_CLOSE_SRC           (WM_USER+2)
#define UWM_PMDBG_START         (WM_USER+3)
#define UWM_PMDBG_STOP          (WM_USER+4)
#define UWM_PMDBG_TERM          (WM_USER+5)
#define UWM_MENU                (WM_USER+6)
#define UWM_FATAL               (WM_USER+7)

#define HELP_TABLE              1
