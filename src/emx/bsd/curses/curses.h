/*
 * Copyright (c) 1981 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)curses.h	5.9 (Berkeley) 7/1/90
 */

#if defined (__cplusplus)
extern "C" {
#endif

#ifndef WINDOW

#include	<stdio.h>
#include	<sys/ioctl.h>
#include	<sys/termio.h>
typedef	struct termio	SGTTY;

#define	bool	char
#define	reg	register

#define	TRUE	(1)
#define	FALSE	(0)
#define	ERR	(0)
#define	OK	(1)

#define	_ENDLINE	001
#define	_FULLWIN	002
#define	_SCROLLWIN	004
#define	_FLUSH		010
#define	_FULLLINE	020
#define	_IDLINE		040
#define	_STANDOUT	0200
#define	_NOCHANGE	-1

#define	_puts(s)	tputs(s, 0, _putchar)

/*
 * Capabilities from termcap
 */

extern bool     AM, BS, CA, DA, DB, EO, HC, HZ, IN, MI, MS, NC, NS, OS, UL,
		XB, XN, XT, XS, XX;
extern char	*AL, *BC, *BT, *CD, *CE, *CL, *CM, *CR, *CS, *DC, *DL,
		*DM, *DO, *ED, *EI, *K0, *K1, *K2, *K3, *K4, *K5, *K6,
		*K7, *K8, *K9, *HO, *IC, *IM, *IP, *KD, *KE, *KH, *KL,
		*KR, *KS, *KU, *LL, *MA, *ND, *NL, *RC, *SC, *SE, *SF,
		*SO, *SR, *TA, *TE, *TI, *UC, *UE, *UP, *US, *VB, *VS,
		*VE, *AL_PARM, *DL_PARM, *UP_PARM, *DOWN_PARM,
		*LEFT_PARM, *RIGHT_PARM;
extern char	PC;

/*
 * From the tty modes...
 */

extern bool	GT, NONL, UPPERCASE, normtty, _pfast;

struct _win_st {
	short		_cury, _curx;
	short		_maxy, _maxx;
	short		_begy, _begx;
	short		_flags;
	short		_ch_off;
	bool		_clear;
	bool		_leave;
	bool		_scroll;
	char		**_y;
	short		*_firstch;
	short		*_lastch;
	struct _win_st	*_nextp, *_orig;
};

#define	WINDOW	struct _win_st

extern bool	My_term, _echoit, _rawmode, _endwin;
extern char	*Def_term, ttytype[];
extern int	LINES, COLS, _tty_ch, _res_flg;
extern SGTTY	_tty;
extern WINDOW	*stdscr, *curscr;

#define	VOID(x)	(x)

/*
 * pseudo functions for standard screen
 */
#define	addch(ch)	VOID(waddch(stdscr, ch))
#define	getch()		VOID(wgetch(stdscr))
#define	addbytes(da,co)	VOID(waddbytes(stdscr, da,co))
#define	addstr(str)	VOID(waddbytes(stdscr, str, strlen(str)))
#define	getstr(str)	VOID(wgetstr(stdscr, str))
#define	move(y, x)	VOID(wmove(stdscr, y, x))
#define	clear()		VOID(wclear(stdscr))
#define	erase()		VOID(werase(stdscr))
#define	clrtobot()	VOID(wclrtobot(stdscr))
#define	clrtoeol()	VOID(wclrtoeol(stdscr))
#define	insertln()	VOID(winsertln(stdscr))
#define	deleteln()	VOID(wdeleteln(stdscr))
#define	refresh()	VOID(wrefresh(stdscr))
#define	inch()		VOID(winch(stdscr))
#define	insch(c)	VOID(winsch(stdscr,c))
#define	delch()		VOID(wdelch(stdscr))
#define	standout()	VOID(wstandout(stdscr))
#define	standend()	VOID(wstandend(stdscr))

/*
 * mv functions
 */
#define	mvwaddch(win,y,x,ch)	VOID(wmove(win,y,x)==ERR?ERR:waddch(win,ch))
#define	mvwgetch(win,y,x)	VOID(wmove(win,y,x)==ERR?ERR:wgetch(win))
#define	mvwaddbytes(win,y,x,da,co) \
		VOID(wmove(win,y,x)==ERR?ERR:waddbytes(win,da,co))
#define	mvwaddstr(win,y,x,str) \
		VOID(wmove(win,y,x)==ERR?ERR:waddbytes(win,str,strlen(str)))
#define mvwgetstr(win,y,x,str)  VOID(wmove(win,y,x)==ERR?ERR:wgetstr(win,str))
#define	mvwinch(win,y,x)	VOID(wmove(win,y,x) == ERR ? ERR : winch(win))
#define	mvwdelch(win,y,x)	VOID(wmove(win,y,x) == ERR ? ERR : wdelch(win))
#define	mvwinsch(win,y,x,c)	VOID(wmove(win,y,x) == ERR ? ERR:winsch(win,c))
#define	mvaddch(y,x,ch)		mvwaddch(stdscr,y,x,ch)
#define	mvgetch(y,x)		mvwgetch(stdscr,y,x)
#define	mvaddbytes(y,x,da,co)	mvwaddbytes(stdscr,y,x,da,co)
#define	mvaddstr(y,x,str)	mvwaddstr(stdscr,y,x,str)
#define mvgetstr(y,x,str)       mvwgetstr(stdscr,y,x,str)
#define	mvinch(y,x)		mvwinch(stdscr,y,x)
#define	mvdelch(y,x)		mvwdelch(stdscr,y,x)
#define	mvinsch(y,x,c)		mvwinsch(stdscr,y,x,c)

/*
 * pseudo functions
 */

#define	clearok(win,bf)	 (win->_clear = bf)
#define	leaveok(win,bf)	 (win->_leave = bf)
#define	scrollok(win,bf) (win->_scroll = bf)
#define flushok(win,bf)	 (bf ? (win->_flags |= _FLUSH):(win->_flags &= ~_FLUSH))
#define	getyx(win,y,x)	 y = win->_cury, x = win->_curx
#define	winch(win)	 (win->_y[win->_cury][win->_curx])

#define crmode() cbreak()	/* backwards compatability */
#define nocrmode() nocbreak()	/* backwards compatability */

#define noraw()         _setraw(0)
#define raw()           _setraw(1)
#define nocbreak()      _setraw(2)
#define cbreak()        _setraw(3)
#define echo()          _setecho(1)
#define noecho()        _setecho(0)
#define nl()            _setnl(1)
#define nonl()          _setnl(0)

/*
 * Used to be in unctrl.h.
 */
#define	unctrl(c)	_unctrl[(c) & 0377]
extern char *_unctrl[];

int baudrate (void);
int box (WINDOW *win, int vert, int hor);
int delwin (WINDOW *win);
int endwin (void);
int erasechar (void);
char *getcap (char *name);
int gettmode (void);
WINDOW *initscr (void);
int killchar (void);
char *longname (char *bp, char *def);
int mvcur (int ly, int lx, int y, int x);
int mvprintw (int y, int x, const char *fmt, ...);
int mvscanw (int y, int x, const char *fmt, ...);
int mvwin (WINDOW *win, int by, int bx);
int mvwprintw (WINDOW *win, int y, int x, const char *fmt, ...);
int mvwscanw (WINDOW *win, int y, int x, const char *fmt, ...);
WINDOW *newwin (int num_lines, int num_cols, int begy, int begx);
int overlay (WINDOW *win1, WINDOW *win2);
int overwrite (WINDOW *win1, WINDOW *win2);
int printw (const char *fmt, ...);
int resetty (void);
int savetty (void);
int scanw (const char *fmt, ...);
int scroll (WINDOW *win);
int setterm (char *type);
WINDOW *subwin (WINDOW *orig, int num_lines, int num_cols, int begy, int begx);
int touchline (WINDOW *win, int y, int sx, int ex);
int touchwin (WINDOW *win);
int waddbytes (reg WINDOW *win, reg char *bytes, reg int count);
int waddch (WINDOW *win, int c);
int waddstr (reg WINDOW *win, reg char *str);
int wclear (WINDOW *win);
int wclrtobot (WINDOW *win);
int wclrtoeol (WINDOW *win);
int wdelch (WINDOW *win);
int wdeleteln (WINDOW *win);
int werase (WINDOW *win);
int wgetch (WINDOW *win);
int wgetstr (WINDOW *win, char *str);
int winsch (WINDOW *win, int c);
int winsertln (WINDOW *win);
int wmove (WINDOW *win, int y, int x);
int wprintw (WINDOW *win, const char *fmt, ...);
int wrefresh (WINDOW *win);
int wscanw (WINDOW *win, const char *fmt, ...);
char *wstandend (WINDOW *win);
char *wstandout (WINDOW *win);
char _putchar (int c);
int _setecho (int on);
int _setnl (int on);
int _setraw (int on);

#if defined (__cplusplus)
}
#endif

#endif
