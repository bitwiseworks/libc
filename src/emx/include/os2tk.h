/* os2tk.h,v 1.2 2004/09/14 22:27:35 bird Exp */
/** @file
 * EMX
 */

#ifndef __OS2_H__
#define __OS2_H__

#define OS2_INCLUDED

#include <os2def.h>

#if !defined (INCL_NOBASEAPI)

#if defined (INCL_BASE) || defined (INCL_SUB) || defined (INCL_KBD)

#define KBD16CHARIN             KbdCharIn
#define KBD16CLOSE              KbdClose
#define KBD16DEREGISTER         KbdDeRegister
#define KBD16FLUSHBUFFER        KbdFlushBuffer
#define KBD16FREEFOCUS          KbdFreeFocus
#define KBD16GETCP              KbdGetCp
#define KBD16GETFOCUS           KbdGetFocus
#define KBD16GETHWID            KbdGetHWID
#define KBD16GETSTATUS          KbdGetStatus
#define KBD16OPEN               KbdOpen
#define KBD16PEEK               KbdPeek
#define KBD16REGISTER           KbdRegister
#define KBD16SETCP              KbdSetCp
#define KBD16SETCUSTXT          KbdSetCustXt
#define KBD16SETFGND            KbdSetFgnd
#define KBD16SETHWID            KbdSetHWID
#define KBD16SETSTATUS          KbdSetStatus
#define KBD16STRINGIN           KbdStringIn
#define KBD16SYNCH              KbdSynch
#define KBD16XLATE              KbdXlate

#endif

#if defined (INCL_BASE) || defined (INCL_SUB) || defined (INCL_VIO)

#define VIO16CHECKCHARTYPE      VioCheckCharType
#define VIO16DEREGISTER         VioDeRegister
#define VIO16ENDPOPUP           VioEndPopUp
#define VIO16GETANSI            VioGetAnsi
#define VIO16GETBUF             VioGetBuf
#define VIO16GETCONFIG          VioGetConfig
#define VIO16GETCP              VioGetCp
#define VIO16GETCURPOS          VioGetCurPos
#define VIO16GETCURTYPE         VioGetCurType
#define VIO16GETFONT            VioGetFont
#define VIO16GETMODE            VioGetMode
#define VIO16GETPHYSBUF         VioGetPhysBuf
#define VIO16GETSTATE           VioGetState
#define VIO16MODEUNDO           VioModeUndo
#define VIO16MODEWAIT           VioModeWait
#define VIO16POPUP              VioPopUp
#define VIO16PRTSC              VioPrtSc
#define VIO16PRTSCTOGGLE        VioPrtScToggle
#define VIO16READCELLSTR        VioReadCellStr
#define VIO16READCHARSTR        VioReadCharStr
#define VIO16REDRAWSIZE         VioRedrawSize
#define VIO16REGISTER           VioRegister
#define VIO16SAVREDRAWUNDO      VioSavRedrawUndo
#define VIO16SAVREDRAWWAIT      VioSavRedrawWait
#define VIO16SCRLOCK            VioScrLock
#define VIO16SCRUNLOCK          VioScrUnLock
#define VIO16SCROLLDN           VioScrollDn
#define VIO16SCROLLLF           VioScrollLf
#define VIO16SCROLLRT           VioScrollRt
#define VIO16SCROLLUP           VioScrollUp
#define VIO16SETANSI            VioSetAnsi
#define VIO16SETCP              VioSetCp
#define VIO16SETCURPOS          VioSetCurPos
#define VIO16SETCURTYPE         VioSetCurType
#define VIO16SETFONT            VioSetFont
#define VIO16SETMODE            VioSetMode
#define VIO16SETSTATE           VioSetState
#define VIO16SHOWBUF            VioShowBuf
#define VIO16WRTCELLSTR         VioWrtCellStr
#define VIO16WRTCHARSTR         VioWrtCharStr
#define VIO16WRTCHARSTRATT      VioWrtCharStrAtt
#define VIO16WRTNATTR           VioWrtNAttr
#define VIO16WRTNCELL           VioWrtNCell
#define VIO16WRTNCHAR           VioWrtNChar
#define VIO16WRTTTY             VioWrtTTY

#endif

#if defined (INCL_BASE) || defined (INCL_SUB) || defined (INCL_MOU)

#define MOU16CLOSE              MouClose
#define MOU16DEREGISTER         MouDeRegister
#define MOU16DRAWPTR            MouDrawPtr
#define MOU16FLUSHQUE           MouFlushQue
#define MOU16GETDEVSTATUS       MouGetDevStatus
#define MOU16GETEVENTMASK       MouGetEventMask
#define MOU16GETNUMBUTTONS      MouGetNumButtons
#define MOU16GETNUMMICKEYS      MouGetNumMickeys
#define MOU16GETNUMQUEEL        MouGetNumQueEl
#define MOU16GETPTRPOS          MouGetPtrPos
#define MOU16GETPTRSHAPE        MouGetPtrShape
#define MOU16GETSCALEFACT       MouGetScaleFact
#define MOU16GETTHRESHOLD       MouGetThreshold
#define MOU16INITREAL           MouInitReal
#define MOU16OPEN               MouOpen
#define MOU16READEVENTQUE       MouReadEventQue
#define MOU16REGISTER           MouRegister
#define MOU16REMOVEPTR          MouRemovePtr
#define MOU16SETDEVSTATUS       MouSetDevStatus
#define MOU16SETEVENTMASK       MouSetEventMask
#define MOU16SETPTRPOS          MouSetPtrPos
#define MOU16SETPTRSHAPE        MouSetPtrShape
#define MOU16SETSCALEFACT       MouSetScaleFact
#define MOU16SETTHRESHOLD       MouSetThreshold
#define MOU16SYNCH              MouSynch

#endif

#include <bse.h>
#endif

#if !defined (INCL_NOPMAPI)

#if defined (INCL_PM) || defined (INCL_AVIO)

#define VIO16ASSOCIATE          VioAssociate
#define VIO16CREATELOGFONT      VioCreateLogFont
#define VIO16CREATEPS           VioCreatePS
#define VIO16DELETESETID        VioDeleteSetId
#define VIO16DESTROYPS          VioDestroyPS
#define VIO16GETDEVICECELLSIZE  VioGetDeviceCellSize
#define VIO16GETORG             VioGetOrg
#define VIO16QUERYFONTS         VioQueryFonts
#define VIO16QUERYSETIDS        VioQuerySetIds
#define VIO16SETDEVICECELLSIZE  VioSetDeviceCellSize
#define VIO16SETORG             VioSetOrg
#define VIO16SHOWPS             VioShowPS
#define WIN16DEFAVIOWINDOWPROC  WinDefAVioWindowProc

#endif

#include <pm.h>
#endif

#undef MAKE16P
#undef MAKEP
#undef SELECTOROF
#undef OFFSETOF

#endif /* not __OS2_H__ */
