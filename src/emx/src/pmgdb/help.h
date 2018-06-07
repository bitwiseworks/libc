/* help.h -*- C++ -*-
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


#define HELP_GENERAL            1
#define HELP_CMD_WINDOW         2
#define HELP_CMD_KEYS           3
#define HELP_SRC_WINDOW         4
#define HELP_SRC_KEYS           5
#define HELP_SRCS_WINDOW        6
#define HELP_SRCS_KEYS          7
#define HELP_BRK_WINDOW         8
#define HELP_BRK_KEYS           9
#define HELP_DSP_WINDOW        10
#define HELP_DSP_KEYS          11
#define HELP_THR_WINDOW        12
#define HELP_THR_KEYS          13
#define HELP_REG_WINDOW        14
#define HELP_REG_KEYS          15
#define HELP_TUTORIAL         100

#define HPM_CMD_FILEMENU     1000
#define HPM_CMD_OPEN_EXEC    1001
#define HPM_CMD_OPEN_CORE    1002
#define HPM_OPEN_SOURCE      1003
#define HPM_CMD_STARTUP      1004
#define HPM_RESTART          1005
#define HPM_WHERE            1006
#define HPM_EXIT             1007
#define HPM_CMD_RUNMENU      1008
#define HPM_GO               1009
#define HPM_STEPOVER         1010
#define HPM_STEPINTO         1011
#define HPM_FINISH           1012
#define HPM_ISTEPOVER        1013
#define HPM_ISTEPINTO        1014
#define HPM_CMD_BRKPTMENU    1015
#define HPM_BRKPT_LINE       1016
#define HPM_BRKPT_LIST       1017
#define HPM_CMD_OPTIONSMENU  1018
#define HPM_PMDEBUGMODE      1019
#define HPM_CMD_ANNOTATE     1020
#define HPM_CMD_WINMENU      1021
#define HPM_WIN_BRK          1022
#define HPM_WIN_CMD          1023
#define HPM_WIN_DSP          1024
#define HPM_WIN_THR          1025
#define HPM_WIN_SRCS         1026
#define HPM_CMD_HELPMENU     1027
#define HPM_HELP_HELP        1028
#define HPM_HELP_EXT         1029
#define HPM_HELP_KEYS        1030
#define HPM_HELP_INDEX       1031
#define HPM_CMD_ABOUT        1032
#define HPM_SRC_FILEMENU     1033
#define HPM_SRC_GOTO         1034
#define HPM_SRC_RUNMENU      1035
#define HPM_SRC_UNTIL        1036
#define HPM_SRC_JUMP         1037
#define HPM_SRC_BRKPTMENU    1038
#define HPM_SRC_DSPMENU      1039
#define HPM_SRC_DSP_SHOW     1041
#define HPM_SRC_OPTIONSMENU  1042
#define HPM_SRC_WINMENU      1043
#define HPM_SRC_HELPMENU     1044
#define HPM_SRCS_FILEMENU    1045
#define HPM_SRCS_WINMENU     1046
#define HPM_SRCS_HELPMENU    1047
#define HPM_HELP_CONTENTS    1048
#define HPM_BRK_FILEMENU     1049
#define HPM_BRK_EDITMENU     1050
#define HPM_BRK_MODIFY       1051
#define HPM_BRK_ENABLE       1052
#define HPM_BRK_DISABLE      1053
#define HPM_BRK_DELETE       1054
#define HPM_BRK_BRKPTMENU    1055
#define HPM_BRK_WINMENU      1056
#define HPM_BRK_HELPMENU     1057
#define HPM_DSP_FILEMENU     1058
#define HPM_DSP_EDITMENU     1059
#define HPM_DSP_WINMENU      1060
#define HPM_DSP_HELPMENU     1061
#define HPM_DSP_ADD          1062
#define HPM_DSP_MODIFY       1063
#define HPM_DSP_ENABLE       1064
#define HPM_DSP_DISABLE      1065
#define HPM_DSP_DELETE       1066
#define HPM_DSP_FORMAT       1067
#define HPM_DSP_DEREFERENCE  1068
#define HPM_DSP_ENABLE_ALL   1069
#define HPM_DSP_DISABLE_ALL  1070
#define HPM_DSP_DELETE_ALL   1071
#define HPM_THR_FILEMENU     1072
#define HPM_THR_EDITMENU     1073
#define HPM_THR_WINMENU      1074
#define HPM_THR_ENABLE       1075
#define HPM_THR_DISABLE      1076
#define HPM_THR_SWITCH       1077
#define HPM_SRC_VIEWMENU     1078
#define HPM_SRC_FIND         1079
#define HPM_SRC_FINDNEXT     1080
#define HPM_SRC_FINDPREV     1081
#define HPM_CMD_EDITMENU     1082
#define HPM_CMD_COMPLETE     1083
#define HPM_CMD_HISTORY      1084
#define HPM_CMD_FONT         1085
#define HPM_REG_FILEMENU     1086
#define HPM_REG_WINMENU      1087
#define HPM_WIN_REG          1088
#define HPM_REG_VIEWMENU     1089
#define HPM_REG_REFRESH      1090
#define HPM_BRK_REFRESH      1091
#define HPM_TUTORIAL         1092

#define HPD_PMDEBUGMODE      2000
#define HPD_BRKPT_LINE       2001
#define HPD_STARTUP          2002
#define HPD_DISPLAY          2003
#define HPD_GOTO             2004
#define HPD_FIND             2006
#define HPD_HISTORY          2007
