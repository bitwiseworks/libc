/* $Id: 572c.c 1232 2004-02-12 15:21:45Z bird $ */
/** @file
 *
 * _Optlink declaration and definition testcases.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */
typedef struct some_struct_pointer * PTYPE;

/*
 * Function and method declarations.
 * Checks mangling.
 */
/* No underscore, No mangling. */
void    _Optlink            CVoid(int a, int b, int c, int d);
void *  _Optlink            CPVoid(int a, int b, int c, int d);
int     _Optlink            CInt(int a, int b, int c, int d);
PTYPE   _Optlink            CPType(int a, int b, int c, int d);

/*
 * Typedefs.
 * Checks that there is not warnings on these.
 */
typedef void    _Optlink    Typedef1Void(int a, int b, int c, int d);
typedef void *  _Optlink    Typedef1PVoid(int a, int b, int c, int d);
typedef int     _Optlink    Typedef1Int(int a, int b, int c, int d);
typedef PTYPE   _Optlink    Typedef1PType(int a, int b, int c, int d);

typedef void    (_Optlink   Typedef2Void)(int a, int b, int c, int d);
typedef void *  (_Optlink   Typedef2PVoid)(int a, int b, int c, int d);
typedef int     (_Optlink   Typedef2Int)(int a, int b, int c, int d);
typedef PTYPE   (_Optlink   Typedef2PType)(int a, int b, int c, int d);
#ifdef __EMX__
typedef void    _Optlink   (Typedef3Void)(int a, int b, int c, int d);
typedef void *  _Optlink   (Typedef3PVoid)(int a, int b, int c, int d);
typedef int     _Optlink   (Typedef3Int)(int a, int b, int c, int d);
typedef PTYPE   _Optlink   (Typedef3PType)(int a, int b, int c, int d);
#endif

typedef void    (* _Optlink PTypedef1Void)(int a, int b, int c, int d);
typedef void *  (* _Optlink PTypedef1PVoid)(int a, int b, int c, int d);
typedef int     (* _Optlink PTypedef1Int)(int a, int b, int c, int d);
typedef PTYPE   (* _Optlink PTypedef1PType)(int a, int b, int c, int d);

/* Alternate writing which should have the same effect I think... */
typedef void    (_Optlink * PTypedef2Void)(int a, int b, int c, int d);
typedef void *  (_Optlink * PTypedef2PVoid)(int a, int b, int c, int d);
typedef int     (_Optlink * PTypedef2Int)(int a, int b, int c, int d);
typedef PTYPE   (_Optlink * PTypedef2PType)(int a, int b, int c, int d);
#ifdef __EMX__
/* Alternate writing which should have the same effect I think... */
typedef void    _Optlink (* PTypedef3Void)(int a, int b, int c, int d);
typedef void *  _Optlink (* PTypedef3PVoid)(int a, int b, int c, int d);
typedef int     _Optlink (* PTypedef3Int)(int a, int b, int c, int d);
typedef PTYPE   _Optlink (* PTypedef3PType)(int a, int b, int c, int d);
#endif

/*
 * Structures.
 */
typedef struct VFT
{
    void    (_Optlink * PStructMemberVoid)(int a, int b, int c, int d);
    void *  (_Optlink * PStructMemberPVoid)(int a, int b, int c, int d);
    int     (_Optlink * PStructMemberInt)(int a, int b, int c, int d);
    PTYPE   (_Optlink * PStructMemberPType)(int a, int b, int c, int d);

    /* Alternate writing which should have the same effect I think... */
    void    (_Optlink * PStructMember2Void)(int a, int b, int c, int d);
    void *  (_Optlink * PStructMember2PVoid)(int a, int b, int c, int d);
    int     (_Optlink * PStructMember2Int)(int a, int b, int c, int d);
    PTYPE   (_Optlink * PStructMember2PType)(int a, int b, int c, int d);

#ifdef __EMX__
    /* Alternate writing which should have the same effect I think... */
    void    _Optlink (* PStructMember3Void)(int a, int b, int c, int d);
    void *  _Optlink (* PStructMember3PVoid)(int a, int b, int c, int d);
    int     _Optlink (* PStructMember3Int)(int a, int b, int c, int d);
    PTYPE   _Optlink (* PStructMember3PType)(int a, int b, int c, int d);
#endif
} VFT, *PVFT;


/*
 * Variables
 */
void    (* _Optlink PVar1Void)(int a, int b, int c, int d);
void *  (* _Optlink PVar1PVoid)(int a, int b, int c, int d);
int     (* _Optlink PVar1Int)(int a, int b, int c, int d);
PTYPE   (* _Optlink PVar1PType)(int a, int b, int c, int d);

/* Alternate writing which should have the same effect I think... */
void    (_Optlink * PVar2Void)(int a, int b, int c, int d);
void *  (_Optlink * PVar2PVoid)(int a, int b, int c, int d);
int     (_Optlink * PVar2Int)(int a, int b, int c, int d);
PTYPE   (_Optlink * PVar2PType)(int a, int b, int c, int d);

#ifdef __EMX__
void    _Optlink (* PVar3Void)(int a, int b, int c, int d);
void *  _Optlink (* PVar3PVoid)(int a, int b, int c, int d);
int     _Optlink (* PVar3Int)(int a, int b, int c, int d);
PTYPE   _Optlink (* PVar3PType)(int a, int b, int c, int d);
#endif


/*
 * Parameters.
 */
int ParamArgs(
    void    (* _Optlink pfn1Void)(int a, int b, int c, int d),
    void *  (* _Optlink pfn1PVoid)(int a, int b, int c, int d),
    int     (* _Optlink pfn1Int)(int a, int b, int c, int d),
    PTYPE   (* _Optlink pfn1PType)(int a, int b, int c, int d),
    void    (_Optlink * pfn2Void)(int a, int b, int c, int d),
    void *  (_Optlink * pfn2PVoid)(int a, int b, int c, int d),
    int     (_Optlink * pfn2Int)(int a, int b, int c, int d),
    PTYPE   (_Optlink * pfn2PType)(int a, int b, int c, int d)
#ifdef __EMX__
    ,
    void    _Optlink (* pfn3Void)(int a, int b, int c, int d),
    void *  _Optlink (* pfn3PVoid)(int a, int b, int c, int d),
    int     _Optlink (* pfn3Int)(int a, int b, int c, int d),
    PTYPE   _Optlink (* pfn3PType)(int a, int b, int c, int d)
#endif
    )
{
    pfn1Void(1,2,3,4);
    pfn1PVoid(1,2,3,4);
    pfn1Int(1,2,3,4);
    pfn1PType(1,2,3,4);

    pfn2Void(1,2,3,4);
    pfn2PVoid(1,2,3,4);
    pfn2Int(1,2,3,4);
    pfn2PType(1,2,3,4);

#ifdef __EMX__
    pfn3Void(1,2,3,4);
    pfn3PVoid(1,2,3,4);
    pfn3Int(1,2,3,4);
    pfn3PType(1,2,3,4);
#endif
    return 0;
}

/* @todo: extend and fix this */
extern void ParamArgs_1(void (_Optlink *)(void));
extern void ParamArgs_2(void (* _Optlink fixme) (void));
#ifdef __EMX__
extern void ParamArgs_3(void _Optlink (*)(void));
#endif


int DoC(int a, int b, int c, int d)
{
    static VFT vft = {CVoid, CPVoid, CInt, CPType,
#ifdef __EMX__
                      CVoid, CPVoid, CInt, CPType,
#endif
                      CVoid, CPVoid, CInt, CPType};

    static Typedef1Void       * pfnTypedef1Void   = CVoid;
    static Typedef1PVoid      * pfnTypedef1PVoid  = CPVoid;
    static Typedef1Int        * pfnTypedef1Int    = CInt;
    static Typedef1PType      * pfnTypedef1PType  = CPType;

    static Typedef2Void       * pfnTypedef2Void   = CVoid;
    static Typedef2PVoid      * pfnTypedef2PVoid  = CPVoid;
    static Typedef2Int        * pfnTypedef2Int    = CInt;
    static Typedef2PType      * pfnTypedef2PType  = CPType;
#ifdef __EMX__
    static Typedef3Void       * pfnTypedef3Void   = CVoid;
    static Typedef3PVoid      * pfnTypedef3PVoid  = CPVoid;
    static Typedef3Int        * pfnTypedef3Int    = CInt;
    static Typedef3PType      * pfnTypedef3PType  = CPType;
#endif

    static PTypedef1Void        pfnPTypedef1Void  = CVoid;
    static PTypedef1PVoid       pfnPTypedef1PVoid = CPVoid;
    static PTypedef1Int         pfnPTypedef1Int   = CInt;
    static PTypedef1PType       pfnPTypedef1PType = CPType;

    static PTypedef2Void        pfnPTypedef2Void  = CVoid;
    static PTypedef2PVoid       pfnPTypedef2PVoid = CPVoid;
    static PTypedef2Int         pfnPTypedef2Int   = CInt;
    static PTypedef2PType       pfnPTypedef2PType = CPType;

#ifdef __EMX__
    static PTypedef3Void        pfnPTypedef3Void  = CVoid;
    static PTypedef3PVoid       pfnPTypedef3PVoid = CPVoid;
    static PTypedef3Int         pfnPTypedef3Int   = CInt;
    static PTypedef3PType       pfnPTypedef3PType = CPType;
#endif

    PVar1Void   = CVoid;
    PVar1PVoid  = CPVoid;
    PVar1Int    = CInt;
    PVar1PType  = CPType;

    PVar2Void   = CVoid;
    PVar2PVoid  = CPVoid;
    PVar2Int    = CInt;
    PVar2PType  = CPType;

#ifdef __EMX__
    PVar3Void   = CVoid;
    PVar3PVoid  = CPVoid;
    PVar3Int    = CInt;
    PVar3PType  = CPType;
#endif

    /* extern functions */
    CVoid(1,2,3,4);
    CPVoid(1,2,3,4);
    CInt(1,2,3,4);
    CPType(1,2,3,4);

    /* typedefs */
    pfnTypedef1Void(1,2,3,4);
    pfnTypedef1PVoid(1,2,3,4);
    pfnTypedef1Int(1,2,3,4);
    pfnTypedef1PType(1,2,3,4);

    pfnTypedef2Void(1,2,3,4);
    pfnTypedef2PVoid(1,2,3,4);
    pfnTypedef2Int(1,2,3,4);
    pfnTypedef2PType(1,2,3,4);

#ifdef __EMX__
    pfnTypedef3Void(1,2,3,4);
    pfnTypedef3PVoid(1,2,3,4);
    pfnTypedef3Int(1,2,3,4);
    pfnTypedef3PType(1,2,3,4);
#endif

    pfnPTypedef1Void(1,2,3,4);
    pfnPTypedef1PVoid(1,2,3,4);
    pfnPTypedef1Int(1,2,3,4);
    pfnPTypedef1PType(1,2,3,4);

    pfnPTypedef2Void(1,2,3,4);
    pfnPTypedef2PVoid(1,2,3,4);
    pfnPTypedef2Int(1,2,3,4);
    pfnPTypedef2PType(1,2,3,4);

#ifdef __EMX__
    pfnPTypedef3Void(1,2,3,4);
    pfnPTypedef3PVoid(1,2,3,4);
    pfnPTypedef3Int(1,2,3,4);
    pfnPTypedef3PType(1,2,3,4);
#endif

    /* structs */
    vft.PStructMemberVoid(1,2,3,4);
    vft.PStructMemberPVoid(1,2,3,4);
    vft.PStructMemberInt(1,2,3,4);
    vft.PStructMemberPType(1,2,3,4);

    vft.PStructMember2Void(1,2,3,4);
    vft.PStructMember2PVoid(1,2,3,4);
    vft.PStructMember2Int(1,2,3,4);
    vft.PStructMember2PType(1,2,3,4);

#ifdef __EMX__
    vft.PStructMember3Void(1,2,3,4);
    vft.PStructMember3PVoid(1,2,3,4);
    vft.PStructMember3Int(1,2,3,4);
    vft.PStructMember3PType(1,2,3,4);
#endif

    /* variables */
    PVar1Void(1,2,3,4);
    PVar1PVoid(1,2,3,4);
    PVar1Int(1,2,3,4);
    PVar1PType(1,2,3,4);

    PVar2Void(1,2,3,4);
    PVar2PVoid(1,2,3,4);
    PVar2Int(1,2,3,4);
    PVar2PType(1,2,3,4);

    /* parameters */
    ParamArgs(CVoid, CPVoid, CInt, CPType,
#ifdef __EMX__
              CVoid, CPVoid, CInt, CPType,
#endif
              CVoid, CPVoid, CInt, CPType);

    return 0;
}

