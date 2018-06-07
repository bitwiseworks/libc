/* $Id: 631main.c 675 2003-09-09 18:27:08Z bird $
 *
 * Testcase #631 - return structs from _System and _Optlink.
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
*   Defined Constants And Macros                                               *
*******************************************************************************/
#if defined(__IBMC__) || defined(__IBMCPP__)
#define DEF(a) a##_vac
#else
#define DEF(a) a##_gcc
#endif 

       
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


/*******************************************************************************
*   External Functions                                                         *
*******************************************************************************/
extern struct ret4bytes _System    asmfoosys4(void);
extern struct ret8bytes _System    asmfoosys8(void);
extern struct ret12bytes _System   asmfoosys12(void);
extern struct ret16bytes _System   asmfoosys16(void);
extern struct ret4bytes _Optlink   asmfooopt4(void);
extern struct ret8bytes _Optlink   asmfooopt8(void);
extern struct ret12bytes _Optlink  asmfooopt12(void);
extern struct ret16bytes _Optlink  asmfooopt16(void);
extern struct ret4bytes __stdcall  asmfoostd4(void);
extern struct ret8bytes __stdcall  asmfoostd8(void);
extern struct ret12bytes __stdcall asmfoostd12(void);
extern struct ret16bytes __stdcall asmfoostd16(void);
extern struct ret4bytes            DEF(asmfoodef4)(void);
extern struct ret8bytes            DEF(asmfoodef8)(void);
extern struct ret12bytes           DEF(asmfoodef12)(void);
extern struct ret16bytes           DEF(asmfoodef16)(void);

struct ret4bytes _System    foosys4(void)
{
    struct ret4bytes ret = {1};
    return ret;
}

struct ret8bytes _System    foosys8(void)
{
    struct ret8bytes ret = {1,2};
    return ret;
}

struct ret12bytes _System   foosys12(void)
{
    struct ret12bytes ret = {1,2,3};
    return ret;
}

struct ret16bytes _System   foosys16(void)
{
    struct ret16bytes ret = {1,2,3,4};
    return ret;
}

/* optlink */
struct ret4bytes _Optlink   fooopt4(void)
{
    struct ret4bytes ret = {1};
    return ret;
}

struct ret8bytes _Optlink   fooopt8(void)
{
    struct ret8bytes ret = {1,2};
    return ret;
}

struct ret12bytes _Optlink  fooopt12(void)
{
    struct ret12bytes ret = {1,2,3};
    return ret;
}

struct ret16bytes _Optlink  fooopt16(void)
{
    struct ret16bytes ret = {1,2,3,4};
    return ret;
}


/* stdcall */
struct ret4bytes __stdcall  foostd4(void)
{
    struct ret4bytes ret = {1};
    return ret;
}

struct ret8bytes __stdcall  foostd8(void)
{
    struct ret8bytes ret = {1,2};
    return ret;
}

struct ret12bytes __stdcall foostd12(void)
{
    struct ret12bytes ret = {1,2,3};
    return ret;
}

struct ret16bytes __stdcall foostd16(void)
{
    struct ret16bytes ret = {1,2,3,4};
    return ret;
}



/* default */
struct ret4bytes    foodef4(void)
{
    struct ret4bytes ret = {1};
    return ret;
}

struct ret8bytes    foodef8(void)
{
    struct ret8bytes ret = {1,2};
    return ret;
}

struct ret12bytes   foodef12(void)
{
    struct ret12bytes ret = {1,2,3};
    return ret;
}

struct ret16bytes   foodef16(void)
{
    struct ret16bytes ret = {1,2,3,4};
    return ret;
}




int main(void)
{
    int                 rcRet = 0;
    struct ret4bytes    rc4;
    struct ret8bytes    rc8;
    struct ret12bytes   rc12;
    struct ret16bytes   rc16;


    /* gcc */
    memset(&rc4, 0, sizeof(rc4));
    rc4 = foosys4();
    if (rc4.au[0] != 1)
    {
        printf("631main: foosys4 failed\n");
        rcRet++;
    }

    memset(&rc8, 0, sizeof(rc8));
    rc8 = foosys8();
    if (rc8.au[0] != 1 && rc8.au[1] != 2)
    {
        printf("631main: foosys8 failed\n");
        rcRet++;
    }

    memset(&rc12, 0, sizeof(rc12));
    rc12 = foosys12();
    if (rc12.au[0] != 1 && rc12.au[1] != 2 && rc16.au[2] != 3)
    {
        printf("631main: foosys12 failed\n");
        rcRet++;
    }

    memset(&rc16, 0, sizeof(rc16));
    rc16 = foosys16();
    if (rc16.au[0] != 1 && rc16.au[1] != 2 && rc16.au[2] != 3 && rc16.au[3] != 4)
    {
        printf("631main: foosys12 failed\n");
        rcRet++;
    }

    /* asm */
    memset(&rc4, 0, sizeof(rc4));
    rc4 = asmfoosys4();
    if (rc4.au[0] != 1)
    {
        printf("631main: asmfoosys4 failed\n");
        rcRet++;
    }

    memset(&rc8, 0, sizeof(rc8));
    rc8 = asmfoosys8();
    if (rc8.au[0] != 1 && rc8.au[1] != 2)
    {
        printf("631main: asmfoosys8 failed\n");
        rcRet++;
    }

    memset(&rc12, 0, sizeof(rc12));
    rc12 = asmfoosys12();
    if (rc12.au[0] != 1 && rc12.au[1] != 2 && rc16.au[2] != 3)
    {
        printf("631main: asmfoosys12 failed\n");
        rcRet++;
    }

    memset(&rc16, 0, sizeof(rc16));
    rc16 = asmfoosys16();
    if (rc16.au[0] != 1 && rc16.au[1] != 2 && rc16.au[2] != 3 && rc16.au[3] != 4)
    {
        printf("631main: asmfoosys12 failed\n");
        rcRet++;
    }


    /*
     * _Optlink
     */
    /* gcc */
    memset(&rc4, 0, sizeof(rc4));
    rc4 = fooopt4();
    if (rc4.au[0] != 1)
    {
        printf("631main: fooopt4 failed\n");
        rcRet++;
    }

    memset(&rc8, 0, sizeof(rc8));
    rc8 = fooopt8();
    if (rc8.au[0] != 1 && rc8.au[1] != 2)
    {
        printf("631main: fooopt8 failed\n");
        rcRet++;
    }

    memset(&rc12, 0, sizeof(rc12));
    rc12 = fooopt12();
    if (rc12.au[0] != 1 && rc12.au[1] != 2 && rc16.au[2] != 3)
    {
        printf("631main: fooopt12 failed\n");
        rcRet++;
    }

    memset(&rc16, 0, sizeof(rc16));
    rc16 = fooopt16();
    if (rc16.au[0] != 1 && rc16.au[1] != 2 && rc16.au[2] != 3 && rc16.au[3] != 4)
    {
        printf("631main: fooopt12 failed\n");
        rcRet++;
    }

    /* asm */
    memset(&rc4, 0, sizeof(rc4));
    rc4 = asmfooopt4();
    if (rc4.au[0] != 1)
    {
        printf("631main: asmfooopt4 failed\n");
        rcRet++;
    }

    memset(&rc8, 0, sizeof(rc8));
    rc8 = asmfooopt8();
    if (rc8.au[0] != 1 && rc8.au[1] != 2)
    {
        printf("631main: asmfooopt8 failed\n");
        rcRet++;
    }

    memset(&rc12, 0, sizeof(rc12));
    rc12 = asmfooopt12();
    if (rc12.au[0] != 1 && rc12.au[1] != 2 && rc16.au[2] != 3)
    {
        printf("631main: asmfooopt12 failed\n");
        rcRet++;
    }

    memset(&rc16, 0, sizeof(rc16));
    rc16 = asmfooopt16();
    if (rc16.au[0] != 1 && rc16.au[1] != 2 && rc16.au[2] != 3 && rc16.au[3] != 4)
    {
        printf("631main: asmfooopt12 failed\n");
        rcRet++;
    }


    /*
     * __stdcall
     */
    /* gcc */
    memset(&rc4, 0, sizeof(rc4));
    rc4 = foostd4();
    if (rc4.au[0] != 1)
    {
        printf("631main: foostd4 failed\n");
        rcRet++;
    }

    memset(&rc8, 0, sizeof(rc8));
    rc8 = foostd8();
    if (rc8.au[0] != 1 && rc8.au[1] != 2)
    {
        printf("631main: foostd8 failed\n");
        rcRet++;
    }

    memset(&rc12, 0, sizeof(rc12));
    rc12 = foostd12();
    if (rc12.au[0] != 1 && rc12.au[1] != 2 && rc16.au[2] != 3)
    {
        printf("631main: foostd12 failed\n");
        rcRet++;
    }

    memset(&rc16, 0, sizeof(rc16));
    rc16 = foostd16();
    if (rc16.au[0] != 1 && rc16.au[1] != 2 && rc16.au[2] != 3 && rc16.au[3] != 4)
    {
        printf("631main: foostd12 failed\n");
        rcRet++;
    }

    /* asm */
    memset(&rc4, 0, sizeof(rc4));
    rc4 = asmfoostd4();
    if (rc4.au[0] != 1)
    {
        printf("631main: asmfoostd4 failed\n");
        rcRet++;
    }

    memset(&rc8, 0, sizeof(rc8));
    rc8 = asmfoostd8();
    if (rc8.au[0] != 1 && rc8.au[1] != 2)
    {
        printf("631main: asmfoostd8 failed\n");
        rcRet++;
    }

    memset(&rc12, 0, sizeof(rc12));
    rc12 = asmfoostd12();
    if (rc12.au[0] != 1 && rc12.au[1] != 2 && rc16.au[2] != 3)
    {
        printf("631main: asmfoostd12 failed\n");
        rcRet++;
    }

    memset(&rc16, 0, sizeof(rc16));
    rc16 = asmfoostd16();
    if (rc16.au[0] != 1 && rc16.au[1] != 2 && rc16.au[2] != 3 && rc16.au[3] != 4)
    {
        printf("631main: asmfoostd12 failed\n");
        rcRet++;
    }


    /*
     * Default
     */
    /* gcc */
    memset(&rc4, 0, sizeof(rc4));
    rc4 = foodef4();
    if (rc4.au[0] != 1)
    {
        printf("631main: foodef4 failed\n");
        rcRet++;
    }

    memset(&rc8, 0, sizeof(rc8));
    rc8 = foodef8();
    if (rc8.au[0] != 1 && rc8.au[1] != 2)
    {
        printf("631main: foodef8 failed\n");
        rcRet++;
    }

    memset(&rc12, 0, sizeof(rc12));
    rc12 = foodef12();
    if (rc12.au[0] != 1 && rc12.au[1] != 2 && rc16.au[2] != 3)
    {
        printf("631main: foodef12 failed\n");
        rcRet++;
    }

    memset(&rc16, 0, sizeof(rc16));
    rc16 = foodef16();
    if (rc16.au[0] != 1 && rc16.au[1] != 2 && rc16.au[2] != 3 && rc16.au[3] != 4)
    {
        printf("631main: foodef12 failed\n");
        rcRet++;
    }

    /* asm */
    memset(&rc4, 0, sizeof(rc4));
    rc4 = DEF(asmfoodef4)();
    if (rc4.au[0] != 1)
    {
        printf("631main: asmfoodef4 failed\n");
        rcRet++;
    }

    memset(&rc8, 0, sizeof(rc8));
    rc8 = DEF(asmfoodef8)();
    if (rc8.au[0] != 1 && rc8.au[1] != 2)
    {
        printf("631main: asmfoodef8 failed\n");
        rcRet++;
    }

    memset(&rc12, 0, sizeof(rc12));
    rc12 = DEF(asmfoodef12)();
    if (rc12.au[0] != 1 && rc12.au[1] != 2 && rc16.au[2] != 3)
    {
        printf("631main: asmfoodef12 failed\n");
        rcRet++;
    }

    memset(&rc16, 0, sizeof(rc16));
    rc16 = DEF(asmfoodef16)();
    if (rc16.au[0] != 1 && rc16.au[1] != 2 && rc16.au[2] != 3 && rc16.au[3] != 4)
    {
        printf("631main: asmfoodef12 failed\n");
        rcRet++;
    }


    /* results */
    if (!rcRet)
        printf("Successfully executed return struct testcase (#631).\n");
    else
        printf("631main: %d failures.\n", rcRet);
    return rcRet;
}

