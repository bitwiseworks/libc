/*
 * drem() wrapper for remainder().
 *
 * Written by J.T. Conklin, <jtc@wimsey.com>
 * Placed into the Public Domain, 1994.
 */

#include "namespace.h"
#include <math.h>

double
drem(x, y)
	double x, y;
{
	return remainder(x, y);
}
