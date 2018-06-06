/* $Id: 631asmc.c 675 2003-09-09 18:27:08Z bird $
 *
 * The code which VAC generate the 631asm.asm file from.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */
 
/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
struct ret4bytes
{
    unsigned int au[1];
};

struct ret8bytes
{
    unsigned int au[2];
};

struct ret12bytes
{
    unsigned int au[3];
};

struct ret16bytes
{
    unsigned int au[4];
};
 

struct ret4bytes _System    asmfoosys4(void)
{
    struct ret4bytes ret = {1};
    return ret;
}

struct ret8bytes _System    asmfoosys8(void)
{
    struct ret8bytes ret = {1,2};
    return ret;
}

struct ret12bytes _System   asmfoosys12(void)
{
    struct ret12bytes ret = {1,2,3};
    return ret;
}

struct ret16bytes _System   asmfoosys16(void)
{
    struct ret16bytes ret = {1,2,3,4};
    return ret;
}

/* optlink */
struct ret4bytes _Optlink   asmfooopt4(void)
{
    struct ret4bytes ret = {1};
    return ret;
}

struct ret8bytes _Optlink   asmfooopt8(void)
{
    struct ret8bytes ret = {1,2};
    return ret;
}

struct ret12bytes _Optlink  asmfooopt12(void)
{
    struct ret12bytes ret = {1,2,3};
    return ret;
}

struct ret16bytes _Optlink  asmfooopt16(void)
{
    struct ret16bytes ret = {1,2,3,4};
    return ret;
}


/* stdcall */
struct ret4bytes __stdcall  asmfoostd4(void)
{
    struct ret4bytes ret = {1};
    return ret;
}

struct ret8bytes __stdcall  asmfoostd8(void)
{
    struct ret8bytes ret = {1,2};
    return ret;
}

struct ret12bytes __stdcall asmfoostd12(void)
{
    struct ret12bytes ret = {1,2,3};
    return ret;
}

struct ret16bytes __stdcall asmfoostd16(void)
{
    struct ret16bytes ret = {1,2,3,4};
    return ret;
}

