/*
 * Copyright (c) 1983, 1988, 1993
 *      The Regents of the University of California.  All rights reserved.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)syslog.c	8.5 (Berkeley) 4/29/95";
#endif /* LIBC_SCCS and not lint */
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/lib/libc/gen/syslog.c,v 1.29 2003/02/10 08:31:28 alfred Exp $");

#include "namespace.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <netdb.h>
#ifdef __EMX__
# include <netinet/in.h>
#endif

#include <errno.h>
#include <fcntl.h>
#ifndef __EMX__
# include <paths.h>
#else
# define _PATH_CONSOLE "/dev/con"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <stdarg.h>
#include "un-namespace.h"

#include "libc_private.h"

static int      LogFile = -1;           /* fd for log */
static int      connected;              /* have done connect */
static int	opened;			/* have done openlog() */
static int      LogStat = 0;            /* status bits, set by openlog() */
static const char *LogTag = NULL;       /* string to tag the entry with */
static int      LogFacility = LOG_USER; /* default facility code */
static int      LogMask = 0xff;         /* mask of priorities to be logged */

static void	disconnectlog(void); /* disconnect from syslogd */
static void	connectlog(void);	/* (re)connect to syslogd */

#ifndef __EMX__
/*
 * Format of the magic cookie passed through the stdio hook
 */
struct bufcookie {
	char	*base;	/* start of buffer */
	int	left;
};

/*
 * stdio write hook for writing to a static string buffer
 * XXX: Maybe one day, dynamically allocate it so that the line length
 *      is `unlimited'.
 */
static
int writehook(cookie, buf, len)
	void	*cookie;	/* really [struct bufcookie *] */
	char	*buf;		/* characters to copy */
	int	len;		/* length to copy */
{
	struct bufcookie *h;	/* private `handle' */

	h = (struct bufcookie *)cookie;
	if (len > h->left) {
		/* clip in case of wraparound */
		len = h->left;
	}
	if (len > 0) {
		(void)memcpy(h->base, buf, len); /* `write' it. */
		h->base += len;
		h->left -= len;
	}
	return 0;
}
#endif

/*
 * syslog, vsyslog --
 *      print message on log file; output is intended for syslogd(8).
 */
void
syslog(int pri, const char *fmt, ...)
{
        va_list ap;

	va_start(ap, fmt);
	vsyslog(pri, fmt, ap);
	va_end(ap);
}

void
vsyslog(pri, fmt, ap)
	int pri;
	const char *fmt;
	va_list ap;
{
        int cnt;
        char ch, *p;
        time_t now;
        int fd, saved_errno;
	char *stdp = NULL, tbuf[2048], fmt_cpy[1024], timbuf[26];
#ifndef __EMX__
	FILE *fp, *fmt_fp;
	struct bufcookie tbuf_cookie;
	struct bufcookie fmt_cookie;
#endif

#define INTERNALLOG     LOG_ERR|LOG_CONS|LOG_PERROR|LOG_PID
        /* Check for invalid bits. */
        if (pri & ~(LOG_PRIMASK|LOG_FACMASK)) {
                syslog(INTERNALLOG,
                    "syslog: unknown facility/priority: %x", pri);
                pri &= LOG_PRIMASK|LOG_FACMASK;
        }

        /* Check priority against setlogmask values. */
        if (!(LOG_MASK(LOG_PRI(pri)) & LogMask))
                return;

        saved_errno = errno;

        /* Set default facility if none specified. */
        if ((pri & LOG_FACMASK) == 0)
                pri |= LogFacility;

#ifndef __EMX__
	/* Create the primary stdio hook */
	tbuf_cookie.base = tbuf;
	tbuf_cookie.left = sizeof(tbuf);
	fp = fwopen(&tbuf_cookie, writehook);
	if (fp == NULL)
		return;
#endif

        /* Build the message. */
        (void)time(&now);
#ifndef __EMX__
	(void)fprintf(fp, "<%d>", pri);
	(void)fprintf(fp, "%.15s ", ctime_r(&now, timbuf) + 4);
#else
        p = tbuf + sprintf(tbuf, "<%d>", pri);
        p += sprintf(p, "%.15s ", ctime_r(&now, timbuf) + 4);
#endif
	if (LogStat & LOG_PERROR) {
#ifndef __EMX__
		/* Transfer to string buffer */
		(void)fflush(fp);
		stdp = tbuf + (sizeof(tbuf) - tbuf_cookie.left);
#else
                stdp = p;
#endif
	}
        if (LogTag == NULL)
                LogTag = _getprogname();
        if (LogTag != NULL)
#ifndef __EMX__
            (void)fprintf(fp, "%s", LogTag);
#else
            p += sprintf(p, "%s", LogTag);
#endif
        if (LogStat & LOG_PID)
#ifndef __EMX__
            (void)fprintf(fp, "[%d]", getpid());
#else
            p += sprintf(p, "[%d]", getpid());
#endif
        if (LogTag != NULL) {
#ifndef __EMX__
            (void)fprintf(fp, ": ");
#else
                *p++ = ':';
                *p++ = ' ';
#endif
        }

	/* Check to see if we can skip expanding the %m */
	if (strstr(fmt, "%m")) {
#ifndef __EMX__
		/* Create the second stdio hook */
		fmt_cookie.base = fmt_cpy;
		fmt_cookie.left = sizeof(fmt_cpy) - 1;
		fmt_fp = fwopen(&fmt_cookie, writehook);
		if (fmt_fp == NULL) {
			fclose(fp);
			return;
		}
#else
            char *t = fmt_cpy;
#endif

            /*
             * Substitute error message for %m.  Be careful not to
             * molest an escaped percent "%%m".  We want to pass it
             * on untouched as the format is later parsed by vfprintf.
             */
            for ( ; (ch = *fmt); ++fmt) {
                    if (ch == '%' && fmt[1] == 'm') {
                            ++fmt;
#ifndef __EMX__
                            fputs(strerror(saved_errno), fmt_fp);
#else
                            t += sprintf(t, "%s", strerror(saved_errno));
#endif
                    } else if (ch == '%' && fmt[1] == '%') {
                            ++fmt;
#ifndef __EMX__
                            fputc(ch, fmt_fp);
                            fputc(ch, fmt_fp);
#else
                            *t++ = ch;
                            *t++ = ch;
#endif
                    } else {
#ifndef __EMX__
                        fputc(ch, fmt_fp);
#else
                            *t++ = ch;
#endif
                    }
            }

            /* Null terminate if room */
#ifndef __EMX__
            /* Null terminate if room */
            fputc(0, fmt_fp);
            fclose(fmt_fp);
#else
            *t = '\0';
#endif
            /* Guarantee null termination */
            fmt_cpy[sizeof(fmt_cpy) - 1] = '\0';

            fmt = fmt_cpy;
        }

#ifndef __EMX__
	(void)vfprintf(fp, fmt, ap);
	(void)fclose(fp);

	cnt = sizeof(tbuf) - tbuf_cookie.left;
#else
        cnt = p - tbuf;
        cnt += vsnprintf(p, sizeof(tbuf) - cnt - 1, fmt, ap);
#endif

        /* Output to stderr if requested. */
        if (LogStat & LOG_PERROR) {
                struct iovec iov[2];
		struct iovec *v = iov;

                v->iov_base = stdp;
                v->iov_len = cnt - (stdp - tbuf);
                ++v;
                v->iov_base = "\n";
                v->iov_len = 1;
		(void)_writev(STDERR_FILENO, iov, 2);
        }

	/* Get connected, output the message to the local logger. */
	if (!opened)
		openlog(LogTag, LogStat | LOG_NDELAY, 0);
	connectlog();
	if (send(LogFile, tbuf, cnt, 0) >= 0)
		return;

	/*
	 * If the send() failed, the odds are syslogd was restarted.
	 * Make one (only) attempt to reconnect to /dev/log.
	 */
	disconnectlog();
	connectlog();
	if (send(LogFile, tbuf, cnt, 0) >= 0)
		return;

        /*
	 * Output the message to the console; try not to block
	 * as a blocking console should not stop other processes.
	 * Make sure the error reported is the one from the syslogd failure.
         */
        if (LogStat & LOG_CONS &&
#ifndef __EMX__
	    (fd = _open(_PATH_CONSOLE, O_WRONLY|O_NONBLOCK, 0)) >= 0) {
#else
	    (fd = _open(_PATH_CONSOLE, O_WRONLY/*|O_NONBLOCK*/, 0)) >= 0) {
#endif

		struct iovec iov[2];
		struct iovec *v = iov;

                p = strchr(tbuf, '>') + 1;
		v->iov_base = p;
		v->iov_len = cnt - (p - tbuf);
		++v;
		v->iov_base = "\r\n";
		v->iov_len = 2;
		(void)_writev(fd, iov, 2);
		(void)_close(fd);
        }
}
static void
disconnectlog()
{
	/*
	 * If the user closed the FD and opened another in the same slot,
	 * that's their problem.  They should close it before calling on
	 * system services.
	 */
	if (LogFile != -1) {
		_close(LogFile);
		LogFile = -1;
	}
	connected = 0;			/* retry connect */
}

static void
connectlog()
{
#ifndef __EMX__
	struct sockaddr_un SyslogAddr;	/* AF_UNIX address of local logger */
#else
	struct sockaddr_in SyslogAddr;	/* AF_INET address of local logger */
#endif

	if (LogFile == -1) {
#ifndef __EMX__
		if ((LogFile = _socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
#else
		if ((LogFile = _socket(AF_INET, SOCK_DGRAM, 0)) == -1)
#endif
			return;
		(void)_fcntl(LogFile, F_SETFD, 1);
	}
	if (LogFile != -1 && !connected) {
#ifndef __EMX__
		SyslogAddr.sun_len = sizeof(SyslogAddr);
		SyslogAddr.sun_family = AF_UNIX;
		(void)strncpy(SyslogAddr.sun_path, _PATH_LOG,
		    sizeof SyslogAddr.sun_path);
#else
                memset(&SyslogAddr, 0, sizeof(SyslogAddr));
#ifndef TCPV40HDRS
                SyslogAddr.sin_len = sizeof(SyslogAddr);
#endif
                SyslogAddr.sin_family = AF_INET;
                SyslogAddr.sin_port = htons(514);
                SyslogAddr.sin_addr.s_addr = INADDR_ANY;
#endif
		connected = _connect(LogFile, (struct sockaddr *)&SyslogAddr,
			sizeof(SyslogAddr)) != -1;

#ifndef __EMX__
		if (!connected) {
			/*
			 * Try the old "/dev/log" path, for backward
			 * compatibility.
			 */
			(void)strncpy(SyslogAddr.sun_path, _PATH_OLDLOG,
			    sizeof SyslogAddr.sun_path);
			connected = _connect(LogFile,
				(struct sockaddr *)&SyslogAddr,
				sizeof(SyslogAddr)) != -1;
		}
#endif

		if (!connected) {
			(void)_close(LogFile);
			LogFile = -1;
		}
	}
}

void
openlog(ident, logstat, logfac)
        const char *ident;
        int logstat, logfac;
{
    if (ident != NULL)
            LogTag = ident;
    LogStat = logstat;
    if (logfac != 0 && (logfac &~ LOG_FACMASK) == 0)
            LogFacility = logfac;

    if (LogStat & LOG_NDELAY)	/* open immediately */
            connectlog();

    opened = 1;	/* ident and facility has been set */
}

void
closelog()
{
	(void)_close(LogFile);
        LogFile = -1;
        LogTag = NULL;
        connected = 0;
}

/* setlogmask -- set the log mask level */
int
setlogmask(pmask)
        int pmask;
{
        int omask;

        omask = LogMask;
        if (pmask != 0)
                LogMask = pmask;
        return (omask);
}
