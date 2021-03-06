/* $Id: 570c.c 1231 2004-02-12 15:00:11Z bird $ */
/** @file
 *
 * _System declaration and definition testcases.
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
void    _System             Void(int a, int b, int c, int d);
void *  _System             PVoid(int a, int b, int c, int d);
int     _System             Int(int a, int b, int c, int d);
PTYPE   _System             PType(int a, int b, int c, int d);

/*
 * Typedefs.
 * Checks that there is not warnings on these.
 */
typedef void    _System     Typedef1Void(int a, int b, int c, int d);
typedef void *  _System     Typedef1PVoid(int a, int b, int c, int d);
typedef int     _System     Typedef1Int(int a, int b, int c, int d);
typedef PTYPE   _System     Typedef1PType(int a, int b, int c, int d);

typedef void    (_System    Typedef2Void)(int a, int b, int c, int d);
typedef void *  (_System    Typedef2PVoid)(int a, int b, int c, int d);
typedef int     (_System    Typedef2Int)(int a, int b, int c, int d);
typedef PTYPE   (_System    Typedef2PType)(int a, int b, int c, int d);

typedef void    (* _System  PTypedef1Void)(int a, int b, int c, int d);
typedef void *  (* _System  PTypedef1PVoid)(int a, int b, int c, int d);
typedef int     (* _System  PTypedef1Int)(int a, int b, int c, int d);
typedef PTYPE   (* _System  PTypedef1PType)(int a, int b, int c, int d);

/* Alternate writing which should have the same effect I think... */
typedef void    (_System *  PTypedef2Void)(int a, int b, int c, int d);
typedef void *  (_System *  PTypedef2PVoid)(int a, int b, int c, int d);
typedef int     (_System *  PTypedef2Int)(int a, int b, int c, int d);
typedef PTYPE   (_System *  PTypedef2PType)(int a, int b, int c, int d);


/*
 * Structures.
 */
typedef struct VFT
{
    void    (_System *  PStructMemberVoid)(int a, int b, int c, int d);
    void *  (_System *  PStructMemberPVoid)(int a, int b, int c, int d);
    int     (_System *  PStructMemberInt)(int a, int b, int c, int d);
    PTYPE   (_System *  PStructMemberPType)(int a, int b, int c, int d);

    /* Alternate writing which should have the same effect I think... */
    void    ( _System * PStructMember2Void)(int a, int b, int c, int d);
    void *  ( _System * PStructMember2PVoid)(int a, int b, int c, int d);
    int     ( _System * PStructMember2Int)(int a, int b, int c, int d);
    PTYPE   ( _System * PStructMember2PType)(int a, int b, int c, int d);

} VFT, *PVFT;


/*
 * Variables
 */
void    (* _System PVar1Void)(int a, int b, int c, int d);
void *  (* _System PVar1PVoid)(int a, int b, int c, int d);
int     (* _System PVar1Int)(int a, int b, int c, int d);
PTYPE   (* _System PVar1PType)(int a, int b, int c, int d);

/* Alternate writing which should have the same effect I think... */
void    (_System * PVar2Void)(int a, int b, int c, int d);
void *  (_System * PVar2PVoid)(int a, int b, int c, int d);
int     (_System * PVar2Int)(int a, int b, int c, int d);
PTYPE   (_System * PVar2PType)(int a, int b, int c, int d);


extern void _System glutDisplayFunc1(void (* _System)(void));
extern void _System glutDisplayFunc2(void (_System *)(void));

/*
 * Parameters.
 */
int ParamArgs(
    void    (* _System pfn1Void)(int a, int b, int c, int d),
    void *  (* _System pfn1PVoid)(int a, int b, int c, int d),
    int     (* _System pfn1Int)(int a, int b, int c, int d),
    PTYPE   (* _System pfn1PType)(int a, int b, int c, int d),
    void    (_System * pfn2Void)(int a, int b, int c, int d),
    void *  (_System * pfn2PVoid)(int a, int b, int c, int d),
    int     (_System * pfn2Int)(int a, int b, int c, int d),
    PTYPE   (_System * pfn2PType)(int a, int b, int c, int d)
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
    return 0;
}



int DoC(int a, int b, int c, int d)
{
    static VFT vft = {Void, PVoid, Int, PType,
                      Void, PVoid, Int, PType};

    static Typedef1Void       * pfnTypedef1Void   = Void;
    static Typedef1PVoid      * pfnTypedef1PVoid  = PVoid;
    static Typedef1Int        * pfnTypedef1Int    = Int;
    static Typedef1PType      * pfnTypedef1PType  = PType;

    static Typedef2Void       * pfnTypedef2Void   = Void;
    static Typedef2PVoid      * pfnTypedef2PVoid  = PVoid;
    static Typedef2Int        * pfnTypedef2Int    = Int;
    static Typedef2PType      * pfnTypedef2PType  = PType;

    static PTypedef1Void        pfnPTypedef1Void  = Void;
    static PTypedef1PVoid       pfnPTypedef1PVoid = PVoid;
    static PTypedef1Int         pfnPTypedef1Int   = Int;
    static PTypedef1PType       pfnPTypedef1PType = PType;

    static PTypedef2Void        pfnPTypedef2Void  = Void;
    static PTypedef2PVoid       pfnPTypedef2PVoid = PVoid;
    static PTypedef2Int         pfnPTypedef2Int   = Int;
    static PTypedef2PType       pfnPTypedef2PType = PType;

    PVar1Void   = Void;
    PVar1PVoid  = PVoid;
    PVar1Int    = Int;
    PVar1PType  = PType;

    PVar2Void   = Void;
    PVar2PVoid  = PVoid;
    PVar2Int    = Int;
    PVar2PType  = PType;


    /* extern functions */
    Void(1,2,3,4);
    PVoid(1,2,3,4);
    Int(1,2,3,4);
    PType(1,2,3,4);

    /* typedefs */
    pfnTypedef1Void(1,2,3,4);
    pfnTypedef1PVoid(1,2,3,4);
    pfnTypedef1Int(1,2,3,4);
    pfnTypedef1PType(1,2,3,4);

    pfnTypedef2Void(1,2,3,4);
    pfnTypedef2PVoid(1,2,3,4);
    pfnTypedef2Int(1,2,3,4);
    pfnTypedef2PType(1,2,3,4);

    pfnPTypedef1Void(1,2,3,4);
    pfnPTypedef1PVoid(1,2,3,4);
    pfnPTypedef1Int(1,2,3,4);
    pfnPTypedef1PType(1,2,3,4);

    pfnPTypedef2Void(1,2,3,4);
    pfnPTypedef2PVoid(1,2,3,4);
    pfnPTypedef2Int(1,2,3,4);
    pfnPTypedef2PType(1,2,3,4);


    /* structs */
    vft.PStructMemberVoid(1,2,3,4);
    vft.PStructMemberPVoid(1,2,3,4);
    vft.PStructMemberInt(1,2,3,4);
    vft.PStructMemberPType(1,2,3,4);

    vft.PStructMember2Void(1,2,3,4);
    vft.PStructMember2PVoid(1,2,3,4);
    vft.PStructMember2Int(1,2,3,4);
    vft.PStructMember2PType(1,2,3,4);


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
    ParamArgs(Void, PVoid, Int, PType,
              Void, PVoid, Int, PType);


    return 0;
}

