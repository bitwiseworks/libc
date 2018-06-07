/* $Id: 572main.cpp 1232 2004-02-12 15:21:45Z bird $ */
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


#include <stdio.h>
typedef struct some_struct_pointer * PTYPE;

/*
 * Function and method declarations.
 * Checks mangling.
 */

/* No underscore, No mangling. */
extern "C" void   _Optlink  ExternCVoid(int a, int b, int c, int d);
extern "C" void * _Optlink  ExternCPVoid(int a, int b, int c, int d);
extern "C" int    _Optlink  ExternCInt(int a, int b, int c, int d);
extern "C" PTYPE  _Optlink  ExternCPType(int a, int b, int c, int d);

/* No underscore, No mangling. */
void    _Optlink            Void(int a, int b, int c, int d);
void *  _Optlink            PVoid(int a, int b, int c, int d);
int     _Optlink            Int(int a, int b, int c, int d);
PTYPE   _Optlink            PType(int a, int b, int c, int d);


/* Members are mangled. */
class foo
{
public:
static void    _Optlink     StaticMemberVoid(int a, int b, int c, int d);
static void *  _Optlink     StaticMemberPVoid(int a, int b, int c, int d);
static int     _Optlink     StaticMemberInt(int a, int b, int c, int d);
static PTYPE   _Optlink     StaticMemberPType(int a, int b, int c, int d);

/* VAC365 allows this too, and actually mangles the
 * calling convention into the name.
 *      _System: MemberVoid__3fooF__l2_v
 *      default: MemberVoid__3fooFv
 * We don't need to support this, it's just a curiosity.
 */
void           _Optlink     MemberVoid(int a, int b, int c, int d);
void *         _Optlink     MemberPVoid(int a, int b, int c, int d);
int            _Optlink     MemberInt(int a, int b, int c, int d);
PTYPE          _Optlink     MemberPType(int a, int b, int c, int d);
};


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
 * Should not cause warnings.
 */
typedef struct VFT
{
    void    (* _Optlink PStructMemberVoid)(int a, int b, int c, int d);
    void *  (* _Optlink PStructMemberPVoid)(int a, int b, int c, int d);
    int     (* _Optlink PStructMemberInt)(int a, int b, int c, int d);
    PTYPE   (* _Optlink PStructMemberPType)(int a, int b, int c, int d);

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


extern "C" int DoC(void);
int main(void)
{
    static VFT vft = {Void, PVoid, Int, PType,
#ifdef __EMX__
                      Void, PVoid, Int, PType,
#endif
                      Void, PVoid, Int, PType};

    static Typedef1Void       * pfnTypedef1Void   = Void;
    static Typedef1PVoid      * pfnTypedef1PVoid  = PVoid;
    static Typedef1Int        * pfnTypedef1Int    = Int;
    static Typedef1PType      * pfnTypedef1PType  = PType;

    static Typedef2Void       * pfnTypedef2Void   = Void;
    static Typedef2PVoid      * pfnTypedef2PVoid  = PVoid;
    static Typedef2Int        * pfnTypedef2Int    = Int;
    static Typedef2PType      * pfnTypedef2PType  = PType;
#ifdef __EMX__
    static Typedef3Void       * pfnTypedef3Void   = Void;
    static Typedef3PVoid      * pfnTypedef3PVoid  = PVoid;
    static Typedef3Int        * pfnTypedef3Int    = Int;
    static Typedef3PType      * pfnTypedef3PType  = PType;
#endif

    static PTypedef1Void        pfnPTypedef1Void  = Void;
    static PTypedef1PVoid       pfnPTypedef1PVoid = PVoid;
    static PTypedef1Int         pfnPTypedef1Int   = Int;
    static PTypedef1PType       pfnPTypedef1PType = PType;

    static PTypedef2Void        pfnPTypedef2Void  = Void;
    static PTypedef2PVoid       pfnPTypedef2PVoid = PVoid;
    static PTypedef2Int         pfnPTypedef2Int   = Int;
    static PTypedef2PType       pfnPTypedef2PType = PType;
#ifdef __EMX__
    static PTypedef3Void        pfnPTypedef3Void  = Void;
    static PTypedef3PVoid       pfnPTypedef3PVoid = PVoid;
    static PTypedef3Int         pfnPTypedef3Int   = Int;
    static PTypedef3PType       pfnPTypedef3PType = PType;
#endif

    PVar1Void   = Void;
    PVar1PVoid  = PVoid;
    PVar1Int    = Int;
    PVar1PType  = PType;

    PVar2Void   = Void;
    PVar2PVoid  = PVoid;
    PVar2Int    = Int;
    PVar2PType  = PType;
#ifdef __EMX__
    PVar3Void   = Void;
    PVar3PVoid  = PVoid;
    PVar3Int    = Int;
    PVar3PType  = PType;
#endif

    /* extern functions */
    ExternCVoid(1,2,3,4);
    ExternCPVoid(1,2,3,4);
    ExternCInt(1,2,3,4);
    ExternCPType(1,2,3,4);

    Void(1,2,3,4);
    PVoid(1,2,3,4);
    Int(1,2,3,4);
    PType(1,2,3,4);

    /* class */
    foo::StaticMemberVoid(1,2,3,4);
    foo::StaticMemberPVoid(1,2,3,4);
    foo::StaticMemberInt(1,2,3,4);
    foo::StaticMemberPType(1,2,3,4);

    foo obj;
    obj.MemberVoid(1,2,3,4);
    obj.MemberPVoid(1,2,3,4);
    obj.MemberInt(1,2,3,4);
    obj.MemberPType(1,2,3,4);

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
    ParamArgs(Void, PVoid, Int, PType,
#ifdef __EMX__
              Void, PVoid, Int, PType,
#endif
              Void, PVoid, Int, PType);

    /* test C stuff */
    DoC();

    printf("Successfully executed _Optlink testcase (assumed).\n");
    return 0;
}

