/*
 * Initial implementation:
 * Copyright (c) 2002 Robert Drehmel
 * All rights reserved.
 *
 * As long as the above copyright statement and this notice remain
 * unchanged, you can do what ever you want with this file.
 */
#include "libc-alias.h"
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/lib/libc/stdlib/remque.c,v 1.3 2003/01/04 07:34:41 tjr Exp $");

#define	_SEARCH_PRIVATE
#include <search.h>
#include <stdlib.h>	/* for NULL */

void
_STD(remque)(void *element)
{
	struct que_elem *prev, *next, *elem;

	elem = (struct que_elem *)element;

	prev = elem->prev;
	next = elem->next;

	if (prev != NULL)
		prev->next = next;
	if (next != NULL)
		next->prev = prev;
}
