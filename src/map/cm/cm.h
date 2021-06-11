/**CFile****************************************************************

  FileName    [cm.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cone mapping]

  Synopsis    [External declarations.]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: if.h,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/
 
#ifndef ABC__map__cm__cm_h
#define ABC__map__cm__cm_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <assert.h>

#include "misc/vec/vec.h"
#include "misc/mem/mem.h"
#include "misc/util/utilNam.h"
#include "misc/util/abc_global.h"
#include "aig/hop/hop.h"


ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////


#define CM_MAX_DEPTH (6)
#define CM_MAX_NLEAFS (1<<CM_MAX_DEPTH)
#define CM_MAX_FA_SIZE (2<<CM_MAX_DEPTH)
#define CM_CUT_SIZE_LIMIT (10)


#define CM_MARK_VALID (1)
#define CM_MARK_LEAF_CUT (2)

/* Defines the type of the objects in the AIG  */
typedef enum {
    CM_NONE,   // 0: non-existent object
    CM_CONST1, // 1: constant 1 
    CM_CI,     // 2: combinational input
    CM_CO,     // 3: combinational output
    CM_AND,    // 4: And node
    CM_VOID    // 5: unused object
} Cm_Type_t;


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Cm_Par_t_ Cm_Par_t;
typedef struct Cm_Man_t_ Cm_Man_t;
typedef struct Cm_Obj_t_ Cm_Obj_t;
typedef struct Cm_Cut_t_ Cm_Cut_t;

struct Cm_Par_t_ {
    int nConeDepth; // cone depth to map for
    int fVerbose; // be verbose
    int fVeryVerbose; // be very verbose
    int fExtraValidityChecks; // run more checks -- mainly for debugging 
    
    float AicDelay[CM_MAX_DEPTH + 1]; // delay of the cones for each depth
    float Epsilon; // used for comparisons
};

struct Cm_Man_t_
{
    char * pName;
    Cm_Par_t * pPars;
    // nodes
    Cm_Obj_t * pConst1; // constant 1 node in AIG
    Vec_Ptr_t * vObjs; // all objects
    Vec_Ptr_t * vCis; // primary inputs
    Vec_Ptr_t * vCos; // primary outputs
    int nObjs[CM_VOID]; // number of object by type
    // additional
    int nLevelMax;
    int nObjBytes;
    // memory management
    Mem_Fixed_t * pMemObj; // memory manager for AIG nodes
};


struct Cm_Cut_t_
{
    float Arrival; // arrival time of the root at cut
    short Depth; // depth of the cut
    short nFanins; // number of leafs
    Cm_Obj_t * Leafs[CM_MAX_NLEAFS]; // pointer to leafs
};

struct Cm_Obj_t_
{
    unsigned Type;
    unsigned fCompl0;
    unsigned fCompl1;   // complemented attribute of FanIns
    Cm_Obj_t * pFanin0;
    Cm_Obj_t * pFanin1; // the Fanins
    unsigned fPhase; // phase of the node
    unsigned Level; // level/depth of node in AIG
    int Id; // identifier (to vObjs)
    int IdPio; // identifier to PI/PO
    int nRefs;
    int nVisits;
    float Required; // time requirement on node
    union {
        float fTemp; // used as temporary storage for calculations
        int iTemp;
    };
    union {          // used as temporary storage for pointers
        void * pCopy;
        int iCopy;
    };
    unsigned fMark; // used as temporary storage for marking/coloring
    Cm_Cut_t BestCut;
};


static inline Cm_Obj_t * Cm_Regular( Cm_Obj_t * p )                          { return (Cm_Obj_t *)((ABC_PTRUINT_T)(p) & ~01);  }
static inline Cm_Obj_t * Cm_Not( Cm_Obj_t * p )                              { return (Cm_Obj_t *)((ABC_PTRUINT_T)(p) ^  01);  }
static inline Cm_Obj_t * Cm_NotCond( Cm_Obj_t * p, int c )                   { return (Cm_Obj_t *)((ABC_PTRUINT_T)(p) ^ (c));  }
static inline int        Cm_IsComplement( Cm_Obj_t * p )                     { return (int )(((ABC_PTRUINT_T)p) & 01);         }

static inline int        Cm_ManCiNum( Cm_Man_t * p )                         { return p->nObjs[CM_CI];               }
static inline int        Cm_ManCoNum( Cm_Man_t * p )                         { return p->nObjs[CM_CO];               }
static inline int        Cm_ManAndNum( Cm_Man_t * p )                        { return p->nObjs[CM_AND];              }
static inline int        Cm_ManObjNum( Cm_Man_t * p )                        { return Vec_PtrSize(p->vObjs);         }

static inline int Cm_ObjIsAnd( Cm_Obj_t * pObj)                              { return pObj ? pObj->Type == CM_AND : 0;        }
static inline Cm_Obj_t * Cm_ManCi ( Cm_Man_t *p, int i)                      { return (Cm_Obj_t*)(Vec_PtrEntry( p->vCis, i)); }
static inline Cm_Obj_t * Cm_ManCo ( Cm_Man_t *p, int i)                      { return (Cm_Obj_t*)(Vec_PtrEntry( p->vCos, i)); }
static inline void Cm_ObjSetCopy( Cm_Obj_t * pObj, void * pCopy)             { pObj->pCopy = pCopy; }

static inline void Cm_ObjClearMarkFa(Cm_Obj_t **pFa, int depth, unsigned flag) { for(int i=1; i<(2<<depth); i++) if(pFa[i]) pFa[i]->fMark &= ~flag; }
////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////


#define CM_MIN(a,b)      (((a) < (b))? (a) : (b))
#define CM_MAX(a,b)      (((a) > (b))? (a) : (b))

// the small and large numbers (min/max float are 1.17e-38/3.40e+38)
#define CM_FLOAT_LARGE   ((float)1.0e+20)
#define CM_FLOAT_SMALL   ((float)1.0e-20)
#define CM_INT_LARGE     (10000000)

// iterator over the primary inputs
#define Cm_ManForEachCi( p, pObj, i )                                          \
    Vec_PtrForEachEntry( Cm_Obj_t *, p->vCis, pObj, i )
// iterator over the primary outputs
#define Cm_ManForEachCo( p, pObj, i )                                          \
    Vec_PtrForEachEntry( Cm_Obj_t *, p->vCos, pObj, i )
#define Cm_ManForEachObj( p, pObj, i )                                         \
    Vec_PtrForEachEntry (Cm_Obj_t *, p->vObjs, pObj, i)
// iterator over all objects in reverse topological order
#define Cm_ManForEachObjReverse( p, pObj, i )                                  \
    Vec_PtrForEachEntryReverse( Cm_Obj_t *, p->vObjs, pObj, i )
// iterator over logic nodes 
#define Cm_ManForEachNode( p, pObj, i )                                        \
    Cm_ManForEachObj( p, pObj, i ) if ( pObj->Type != CM_AND ) {} else
////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== cmCore.c =======================================================*/
extern void Cm_ManSetDefaultPars( Cm_Par_t * pPars );
extern int Cm_ManPerformMapping( Cm_Man_t * p );
/*=== cmFa.c =========================================================*/
extern float Cm_FaBuildDepthOptimal(Cm_Obj_t **pNodes, Cm_Par_t * pPars);
extern int Cm_FaBuildWithMaximumDepth(Cm_Obj_t **pFaninArray, int maxDepth);
extern void Cm_FaExtractLeafs(Cm_Obj_t **pNodes, Cm_Cut_t *pCut);
/*=== cmMan.c ========================================================*/
extern Cm_Man_t * Cm_ManStart( Cm_Par_t *pPars );
extern void Cm_ManStop( Cm_Man_t * p );
extern Cm_Obj_t * Cm_ManCreateCi( Cm_Man_t * p );
extern Cm_Obj_t * Cm_ManCreateCo( Cm_Man_t * p, Cm_Obj_t * pDriver );
extern Cm_Obj_t * Cm_ManCreateAnd( Cm_Man_t * p, Cm_Obj_t * pFan0, Cm_Obj_t * pFan1 );
/*=== cmPrint.c ======================================================*/
extern void Cm_PrintPars( Cm_Par_t * pPars );
extern void Cm_PrintFa(Cm_Obj_t ** pFaninArray, int depth);
extern void Cm_ManPrintAigStructure(Cm_Man_t * pMan, int lineLimit);
extern void Cm_PrintBestCut(Cm_Obj_t * pObj);


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
