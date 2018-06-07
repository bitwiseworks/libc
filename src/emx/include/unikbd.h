/* unikbd.h,v 1.2 2004/09/14 22:27:36 bird Exp */
/** @file
 * Legalesy-free Unicode API interface for OS/2
 * Interface definitions for basic Unicode API functions
 *
 * Written by bird
 *
 * This file is put into public domain. You are free to do
 * literally anything you wish with it: modify, print, sell,
 * rent, eat, throw out of window: in all (esp. in later)
 * cases I am not responsible for any damage it causes.
 */
#ifndef __UNIKBD_H__
#define __UNIKBD_H__


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <sys/cdefs.h>
#include <unidef.h>

#if !defined(OS2_INCLUDED) && !defined(_OS2EMX_H)
#warning Including os2.h for you.
#include <os2.h>
#endif


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** Linkage convetion for unikbd calls. */
#define KBDLINK                         APIENTRY

#ifndef INCL_WININPUT
#define INCL_WININPUT
/** @group Virtual Key defines - duplicates of os2emx.h
 * @{ */
#define VK_BUTTON1			0x0001
#define VK_BUTTON2			0x0002
#define VK_BUTTON3			0x0003
#define VK_BREAK			0x0004
#define VK_BACKSPACE			0x0005
#define VK_TAB				0x0006
#define VK_BACKTAB			0x0007
#define VK_NEWLINE			0x0008
#define VK_SHIFT			0x0009
#define VK_CTRL				0x000a
#define VK_ALT				0x000b
#define VK_ALTGRAF			0x000c
#define VK_PAUSE			0x000d
#define VK_CAPSLOCK			0x000e
#define VK_ESC				0x000f
#define VK_SPACE			0x0010
#define VK_PAGEUP			0x0011
#define VK_PAGEDOWN			0x0012
#define VK_END				0x0013
#define VK_HOME				0x0014
#define VK_LEFT				0x0015
#define VK_UP				0x0016
#define VK_RIGHT			0x0017
#define VK_DOWN				0x0018
#define VK_PRINTSCRN			0x0019
#define VK_INSERT			0x001a
#define VK_DELETE			0x001b
#define VK_SCRLLOCK			0x001c
#define VK_NUMLOCK			0x001d
#define VK_ENTER			0x001e
#define VK_SYSRQ			0x001f
#define VK_F1				0x0020
#define VK_F2				0x0021
#define VK_F3				0x0022
#define VK_F4				0x0023
#define VK_F5				0x0024
#define VK_F6				0x0025
#define VK_F7				0x0026
#define VK_F8				0x0027
#define VK_F9				0x0028
#define VK_F10				0x0029
#define VK_F11				0x002a
#define VK_F12				0x002b
#define VK_F13				0x002c
#define VK_F14				0x002d
#define VK_F15				0x002e
#define VK_F16				0x002f
#define VK_F17				0x0030
#define VK_F18				0x0031
#define VK_F19				0x0032
#define VK_F20				0x0033
#define VK_F21				0x0034
#define VK_F22				0x0035
#define VK_F23				0x0036
#define VK_F24				0x0037
#define VK_ENDDRAG			0x0038
#define VK_CLEAR			0x0039
#define VK_EREOF			0x003a
#define VK_PA1				0x003b
#define VK_ATTN				0x003c
#define VK_CRSEL			0x003d
#define VK_EXSEL			0x003e
#define VK_COPY				0x003f
#define VK_BLK1				0x0040
#define VK_BLK2				0x0041
#define VK_MENU				VK_F10
#if defined (INCL_NLS)
#define VK_DBCSFIRST			0x0080
#define VK_DBCSLAST			0x00ff
#define VK_BIDI_FIRST			0x00e0
#define VK_BIDI_LAST			0x00ff
#endif /* INCL_NLS */
#define VK_USERFIRST			0x0100
#define VK_USERLAST			0x01ff
/** @} */
#endif /* !INCL_WININPUT */


/** @group Virtual Keys defines (Unikbd specific)
 * @{ */
#define VK_PA2                          0x0050
#define VK_PA3                          0x0051
#define VK_GROUP                        0x0052
#define VK_GROUPLOCK                    0x0053
#define VK_APPL                         0x0054
#define VK_WINLEFT                      0x0055
#define VK_WINRIGHT                     0x0056
#define VK_M_DOWNLEFT                   0x0061
#define VK_M_DOWN                       0x0062
#define VK_M_DOWNRIGHT                  0x0063
#define VK_M_LEFT                       0x0064
#define VK_M_CENTER                     0x0065
#define VK_M_RIGHT                      0x0066
#define VK_M_UPLEFT                     0x0067
#define VK_M_UP                         0x0068
#define VK_M_UPRIGHT                    0x0069
#define VK_M_BUTTONLOCK                 0x006A
#define VK_M_BUTTONRELEASE              0x006B
#define VK_M_DOUBLECLICK                0x006C
/** @} */

/** @group Dead Key defines
 * @{ */
#define DK_MIN                          0x1000
#define DK_ACUTE                        0x1001
#define DK_GRAVE                        0x1002
#define DK_DIERESIS                     0x1003
#define DK_UMLAUT                       0x1003
#define DK_CIRCUMFLEX                   0x1004
#define DK_TILDE                        0x1005
#define DK_CEDILLA                      0x1006
#define DK_MACRON                       0x1007
#define DK_BREVE                        0x1008
#define DK_OGONEK                       0x1009
#define DK_DOT                          0x100a
#define DK_BAR                          0x100b
#define DK_RING                         0x100c
#define DK_CARON                        0x100d
#define DK_HACEK                        DK_CARON
#define DK_HUNGARUMLAUT                 0x100e
#define DK_ACUTEDIA                     0x100f
#define DK_PSILI                        0x1010
#define DK_DASIA                        0x1011
#define DK_OVERLINE                     0x1012
#define DK_UNDERDOT                     0x1013
#define DK_MAX                          0x1fff
/** @} */

/** @group Keyboard Shift and Effective flags.
 * @{ */
#define KBD_SHIFT                   0x00000001
#define KBD_CONTROL                 0x00000002
#define KBD_ALT                     0x00000004
#define KBD_ALTCTRLSHIFT            0x00000007
#define KBD_ALTGR                   0x00000008
#define KBD_NLS1                    0x00000010
#define KBD_NLS2                    0x00000020
#define KBD_NLS3                    0x00000040
#define KBD_NLS4                    0x00000080
/** @} */

/** @group Keyboard Shift and Effective flags - Japanese NLS usage
 * @{ */
#define KBD_WIDE                    KBD_NLS1
#define KBD_KATAKANA                KBD_NLS2
#define KBD_HIRAGANA                KBD_NLS3
#define KBD_ROMANJI                 KBD_NLS4
/** @} */

/** @group Keyboard Shift and Effective flags - Korean NLS usage
 * @{ */
#define KBD_JAMO                    KBD_NLS2
#define KBD_HANGEUL                 KBD_NLS3
#define KBD_HANJACSR                KBD_NLS4
/** @} */

/** @group Keyboard Shift and Effective flags - Taiwan NLS usage */
#define KBD_PHONETIC                KBD_NLS2
#define KBD_TSANGJYE                KBD_NLS3
/** @} */


/** @group Keyboard Lock States
 * @{ */
#define KBD_SCROLLLOCK              0x00000100
#define KBD_NUMLOCK                 0x00000200
#define KBD_CAPSLOCK                0x00000400
#define KBD_EXTRALOCK               0x00000800
#define KBD_APPL                    0x00001000
#define KBD_DBCS                    0x00008000
/** @} */

/** Bitmask for all the effective bits. */
#define KBD_EFFECTIVE               0x0000ffff

/** @group Keyboard Shift States Left/Right separation.
 * @{ */
#define KBD_LEFTSHIFT               0x00010000
#define KBD_RIGHTSHIFT              0x00020000
#define KBD_LEFTCONTROL             0x00040000
#define KBD_RIGHTCONTROL            0x00080000
#define KBD_LEFTALT                 0x00100000
#define KBD_RIGHTALT                0x00200000
#define KBD_LEFTWINDOWS             0x00400000
#define KBD_RIGHTWINDOWS            0x00800000
/** @} */

/** @group Keyboard LED and other status bits.
 * @{ */
#define KBD_NOROMANJI               0x04000000
#define KBD_KANJI                   0x08000000
#define KBD_DEADKEY                 0x10000000
#define KBD_WAIT                    0x20000000
#define KBD_HOLD                    0x40000000
#define KBD_LOCK                    0x80000000
/** @} */

/** @group Make/Break/Repeat defines.
 * @{ */
#define KEYEV_MAKEBREAK             0
#define KEYEV_MAKE                  1
#define KEYEV_BREAK                 2
#define KEYEV_REPEAT                3
/** @} */

/** @group Keyboard Query Flags.
 * @{ */
#define KBDF_DEFAULTVKEY            0x0001
#define KBDF_NOCTRLSHIFT            0x0002
#define KBDF_NOALTGR                0x0004
#define KBDF_SHIFTALTGR             0x0010
#define KBDF_DEADGOOD               0x0020
#define KBDF_DEADPRIVATE            0x0040
#define KBDF_SYSTEM                 0x8000
#define KBDF_INTERNATIONAL          0x4000
#define KBDF_DVORAK                 0x2000
#define KBDF_NATIONAL               0x1000
#define KBDF_LETTERKEYS             0x3000
#define KBDF_ISOKEYS                0x0800
/** @} */

/** @group Keyboard layout flags (advisory only)
 * @{ */
#define KBDF_LAYOUT101              0x0000
#define KBDF_LAYOUT102              0x0100
#define KBDF_LAYOUT106              0x0200
#define KBDF_LAYOUT103              0x0300
#define KBDF_LAYOUT100              0x0400
#define KBDF_LAYOUTS                0x0700
/** @} */

/** @group UniResetShiftState types.
 * @{ */
#define KEYEV_SET                   0
#define KEYEV_RELEASE               1
#define KEYEV_ZERO                  2
/** @} */




/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/** Translation table handle. */
typedef unsigned int        KHAND;
/** Virtual scan code. */
typedef unsigned char       VSCAN;
/** Virtual dead key. */
typedef unsigned short      VDKEY;
/** Keyboard name. */
typedef UniChar             KBDNAME;

/** Keyboard shift states. */
typedef struct _USHIFTSTATE
{
    /** Actual shift and lock state. */
    ULONG           Shift;
    /** Effective shift and lock state. */
    ULONG           Effective;
    /** Keyboard indicators. */
    ULONG           Led;
} USHIFTSTATE;

/** Virtual key event. */
typedef struct _INKEYEVENT
{
    /** Logical device (0=real). */
    USHORT          ldev;
    /** Make/break indicator. */
    BYTE            makebreak;
    /** Virtual scan code. */
    VSCAN           scan;
    /** Timestamp. */
    ULONG           time;
} INKEYEVENT;

/** Query keyboard structure. */
typedef struct  _KEYBOARDINFO
{
    /** Length of structure. */
    ULONG           len;
    /** Keyboard architecture id. */
    USHORT          kbid;
    /** Version number. */
    USHORT          version;
    /** Normal language. */
    BYTE            language[2];
    /** Normal country. */
    BYTE            country[2];
    /** Flags (KBDF_). */
    USHORT          flags;
    /** Reserved. */
    USHORT          resv;
    /** Description of keyboard. */
    UniChar description[32];
} KEYBOARDINFO;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
__BEGIN_DECLS
APIRET KBDLINK UniCreateKeyboard(KHAND * phKeyboard, const KBDNAME * puszName, ULONG ulMode);
APIRET KBDLINK UniDestroyKeyboard(KHAND hKeyboard);
APIRET KBDLINK UniQueryKeyboard(KHAND hKeyboard, KEYBOARDINFO * pKbdInfo);
APIRET KBDLINK UniResetShiftState(KHAND hKeyboard, USHIFTSTATE * pKbdStat, ULONG ulType);
APIRET KBDLINK UniUpdateShiftState(KHAND hKeyboard, USHIFTSTATE * pKbdStat, VSCAN uchScan, BYTE bMakeBreak);
APIRET KBDLINK UniTranslateKey(KHAND hKeyboard, ULONG ulEShift, VSCAN bScan, UniChar * puc, VDKEY * pusDeadKey, BYTE * pbScan);
APIRET KBDLINK UniTranslateDeadkey(KHAND hKeyboard, VDKEY usDead, UniChar ucIn, UniChar * pucOut, VDKEY * pusDeadKey);
APIRET KBDLINK UniUntranslateKey(KHAND hKeyboard, UniChar uc, VDKEY vdkey, VSCAN * pscan, ULONG * eshift);
__END_DECLS

#endif


