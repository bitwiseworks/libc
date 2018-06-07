/* $Id: externalvar.c 725 2003-09-24 18:49:24Z bird $
 *
 * Testing if we can monitor a global external variable.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */

extern const char *gpszExt;
extern const char *gpszUnused;
const char *pszLocal = "local";

int main()
{
    return *gpszExt + *pszLocal;
}

