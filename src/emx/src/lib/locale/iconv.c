/* $Id: iconv.c 3759 2012-03-14 10:24:02Z bird $ */
/** @file
 *
 * iconv wrapper based on OS/2 Unicode API.
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */



/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include <InnoTekLIBC/locale.h>
#define INCL_FSMACROS
#include <os2emx.h>
#include <uconv.h>

#include <386/builtin.h>
#include <sys/smutex.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <alloca.h>
#include <InnoTekLIBC/fork.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_ICONV
#include <InnoTekLIBC/logstrict.h>

typedef struct __libc_iconv_s
{
  UconvObject from;		/* "From" conversion handle */
  UconvObject to;		/* "To" conversion handle */
  UniChar * ucp_to;
  UniChar * ucp_from;
  struct __libc_iconv_s *pNext;
  struct __libc_iconv_s *pPrev;
} *iconv_t;

/* Tell "iconv.h" to not define iconv_t by itself.  */
#define _ICONV_T
#include "iconv.h"


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** List of open Iconv structures.
 * The purpose is to enable fork() to recreate the conversion objects. */
static iconv_t	gIconvHead = NULL;
/** Mutex protecting the list. */
static _smutex  gsmtxIconv = {0};

/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int iconvForkChild1(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);


iconv_t
_STD(iconv_open) (const char *cp_to, const char *cp_from)
{
    iconv_t conv = (iconv_t) calloc (sizeof (struct __libc_iconv_s), 1);
    if (conv)
    {
        /* add a good bit of room for cp conversion. */
        size_t cb = (strlen(cp_from) + 7) * sizeof(UniChar);
        if (cb < 48)
            cb = 48;
        conv->ucp_from = (UniChar *)malloc(cb);
        if (conv->ucp_from)
        {
            FS_VAR();
            FS_SAVE_LOAD();
            __libc_TranslateCodepage(cp_from, conv->ucp_from);
            if (!UniCreateUconvObject(conv->ucp_from, &conv->from))
            {
                /* add a good bit of room for cp conversion. */
                cb = (strlen(cp_to) + 7) * sizeof(UniChar);
                if (cb < 48)
                    cb = 48;
                conv->ucp_to = (UniChar *)malloc(cb);
                if (conv->ucp_to)
                {
                    __libc_TranslateCodepage(cp_to, conv->ucp_to);
                    if (!UniCreateUconvObject(conv->ucp_to, &conv->to))
                    {
                        /* Do not treat 0x7f as a control character
                           (don't understand what it exactly means but without it MBCS prefix
                           character detection sometimes could fail (when 0x7f is a prefix)).
                           
                           And don't treat the string as a path (the docs also don't explain
                           what it exactly means, but I'm pretty sure converted texts will
                           mostly not be paths). 
                           Ticket #182: This breaks samba with Korean CP, WON and backward 
                           slash seems to be mapped to the same ASCII character.  Better 
                           assume the input is PATHs. */
                        uconv_attribute_t   attr;
                        UniQueryUconvObject(conv->from, &attr, sizeof (attr), NULL, NULL, NULL);
                        attr.converttype &= ~(CVTTYPE_CTRL7F/* | CVTTYPE_PATH*/);
                        UniSetUconvObject(conv->from, &attr);

                        _smutex_request(&gsmtxIconv);
                        conv->pPrev = NULL;
                        conv->pNext = gIconvHead;
                        if (conv->pNext)
                            conv->pNext->pPrev = conv;
                        gIconvHead = conv;
                        _smutex_release(&gsmtxIconv);

                        FS_RESTORE();
                        return conv;
                    }
                    else
                        errno = EINVAL;
                    free(conv->ucp_to);
                }
                else
                    errno = ENOMEM;
                UniFreeUconvObject(conv->from);
            }
            else
                errno = EINVAL;
            FS_RESTORE();
            free(conv->ucp_from);
        }
        else
            errno = ENOMEM;
        free(conv);
    }
    else
        errno = ENOMEM;

    return (iconv_t)(-1);
}

size_t
_STD(iconv) (iconv_t conv,
             const char **in, size_t *in_left,
             char **out, size_t *out_left)
{
  int       rc;
  size_t    sl;
  size_t    nonid;
  UniChar  *ucs;
  UniChar  *pucsFree = NULL;
  UniChar  *orig_ucs;
  size_t    retval = 0;
  FS_VAR();

  /* just in case. */
  if (!conv || conv == (iconv_t)-1)
    {
      errno = EINVAL;
      return -1;
    }

  /* The caller wish to initate the conversion state and/or write initial
     shift prefix (or something like that) to the output buffer. */
  if (!in || !*in)
    {
      if (!out || !*out || !out_left || !*out_left)
        /* do nothing since we don't have any shift state in the iconv_t. */
        return 0;

      /** @todo We don't have any shift state I or so, so what to do now?
       *        Let's do nothing till we have anyone complaining about his DBCS
       *        stuff not working 100%, and accept patches for that guy.
       *        Perhaps try call UniUconvFromUcs(conv->to, and some empty input buffer or so...
       */
      return 0;
    }

  sl = *in_left;
  if (sl <= 1024)
    ucs = (UniChar *) alloca (sl * sizeof (UniChar));
  else
    {
      pucsFree = ucs = (UniChar *) malloc (sl * sizeof (UniChar));
      if (!ucs)
        {
          errno = ENOMEM;
          return -1;
        }
    }
  orig_ucs = ucs;

  FS_SAVE_LOAD();
  rc = UniUconvToUcs (conv->from, (void **)in, in_left, &ucs, &sl, &retval);
  if (rc)
    goto error;
  sl = ucs - orig_ucs;
  ucs = orig_ucs;
  /* UniUconvFromUcs will stop at first null byte (huh? indeed?)
     while we want ALL the bytes converted.  */
#if 1
  rc = UniUconvFromUcs (conv->to, &ucs, &sl, (void **)out, out_left, &nonid);
  if (rc)
    goto error;
  retval += nonid;
#else
  while (sl)
    {
      size_t usl = 0;
      while (sl && (ucs[usl] != 0))
        usl++, sl--;
      rc = UniUconvFromUcs (conv->to, &ucs, &usl, (void **)out, out_left, &nonid);
      if (rc)
        goto error;
      retval += nonid;
      if (sl && *out_left)
        {
          *(*out)++ = 0;
          (*out_left)--;
          ucs++; sl--;
        }
    }
#endif
  if (pucsFree)
      free(pucsFree);
  FS_RESTORE();
  return retval;

error:
  /* Convert OS/2 error code to errno.  */
  switch (rc)
  {
    case ULS_ILLEGALSEQUENCE:
      errno = EILSEQ;
      break;
    case ULS_INVALID:
      errno = EINVAL;
      break;
    case ULS_BUFFERFULL:
      errno = E2BIG;
      break;
    default:
      errno = EBADF;
      break;
  }
  if (pucsFree)
      free(pucsFree);
  FS_RESTORE();
  return (size_t)(-1);
}

int
_STD(iconv_close) (iconv_t conv)
{
  if (conv && conv != (iconv_t)(-1))
    {
      FS_VAR();

      _smutex_request(&gsmtxIconv);
      if (conv->pNext)
          conv->pNext->pPrev = conv->pPrev;
      if (conv->pPrev)
          conv->pPrev->pNext = conv->pNext;
      else
          gIconvHead = conv->pNext;
      _smutex_release(&gsmtxIconv);

      free (conv->ucp_to);
      free (conv->ucp_from);
      FS_SAVE_LOAD();
      if (conv->to) UniFreeUconvObject (conv->to);
      if (conv->from) UniFreeUconvObject (conv->from);
      FS_RESTORE();
      free (conv);
    }
  return 0;
}


#undef  __LIBC_LOG_GROUP
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_FORK
_FORK_CHILD1(0xffff0000, iconvForkChild1)

/**
 * Walks the chain of iconv_t structures.
 *
 * @returns 0 on success.
 * @returns positive errno on warning.
 * @returns negative errno on failure. Fork will be aborted.
 * @param   pForkHandle     Pointer to fork handle.
 * @param   enmOperation    Callback operation.
 */
static int iconvForkChild1(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("pForkHandle=%p enmOperation=%d\n", (void *)pForkHandle, enmOperation);
    int rc = 0;

    switch (enmOperation)
    {
        case __LIBC_FORK_OP_FORK_CHILD:
        {
            struct __libc_iconv_s *pCur;
            for (rc = 0, pCur = gIconvHead; !rc && pCur; pCur = pCur->pNext)
            {
                pCur->from = NULL;
                pCur->to   = NULL;

                rc = UniCreateUconvObject(pCur->ucp_from, &pCur->from);
                if (!rc)
                {
                    rc = UniCreateUconvObject(pCur->ucp_to, &pCur->to);
                    if (!rc)
                    {
                        uconv_attribute_t   Attr;
                        UniQueryUconvObject(pCur->from, &Attr, sizeof(Attr), NULL, NULL, NULL);
                        Attr.converttype &= ~(CVTTYPE_CTRL7F | CVTTYPE_PATH);
                        UniSetUconvObject(pCur->from, &Attr);
                    }
                    else
                        LIBC_ASSERTM_FAILED("Failed to create the 'to' object. rc=%d spec=%ls\n", rc, pCur->ucp_to);
                }
                else
                    LIBC_ASSERTM_FAILED("Failed to create the 'from' object. rc=%d spec=%ls\n", rc, pCur->ucp_from);
            }
            if (rc)
                rc = -EACCES; /* bad, fixme */
            break;
        }

        default:
            rc = 0;
            break;
    }
    LIBCLOG_RETURN_INT(rc);
}

