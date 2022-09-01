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
#    define ABC__map__cm__cm_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#    include <stdio.h>
#    include <assert.h>

#    include "misc/vec/vec.h"
#    include "misc/mem/mem.h"
#    include "misc/util/utilNam.h"
#    include "misc/util/abc_global.h"
#    include "aig/hop/hop.h"
#    include "misc/vec/vecInt.h"

#    include "cmMiMo.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

// defines the mapping
#    define CM_GENLIB_INV "inv"
#    define CM_GENLIB_NAND2 "nand2"
#    define CM_GENLIB_NAND3 "nand3"
#    define CM_GENLIB_NOR2 "nor2"
#    define CM_GENLIB_NOR3 "nor3"
#    define CM_GENLIB_BUF "buf"
#    define CM_GENLIB_C0 "zero"
#    define CM_GENLIB_C1 "one"

#    define CM_GENLIB_STR                                                              \
        "GATE inv     3   O=!a;             PIN * INV     1 1000000 1.0 0.0 1.0 0.0\n" \
        "GATE nand2   3   O=!(a*b);         PIN * INV     1 1000000 1.0 0.0 1.0 0.0\n" \
        "GATE nand3   3   O=!(a*b*c);       PIN * INV     1 1000000 1.0 0.0 1.0 0.0\n" \
        "GATE nor2    3   O=!(a+b);         PIN * INV     1 1000000 1.0 0.0 1.0 0.0\n" \
        "GATE nor3    3   O=!(a+b+c);       PIN * INV     1 1000000 1.0 0.0 1.0 0.0\n" \
        "GATE buf     1   O=a;              PIN * NONINV  1 1000000 1.0 0.0 1.0 0.0\n" \
        "GATE zero    0   O=CONST0;\n"                                                 \
        "GATE one     0   O=CONST1;\n"

#    define CM_MAX_DEPTH (6)
#    define CM_MAX_NLEAFS (1 << CM_MAX_DEPTH)
#    define CM_MAX_FA_SIZE (2 << CM_MAX_DEPTH)
#    define CM_CUT_SIZE_LIMIT (10)

#    define CM_MARK_VALID (1)
#    define CM_MARK_LEAF_CUT (2)
#    define CM_MARK_LEAF (4)
#    define CM_MARK_VISIBLE (8)
#    define CM_MARK_FIXED (16)
#    define CM_MARK_LEAF_SUB (32)
#    define CM_MARK_SEEN (64)
#    define CM_MARK_COBAL (128)
#    define CM_MARK_CO (256)

/* Defines the type of the objects in the AIG  */
typedef enum {
    CM_NONE,   // 0: non-existent object
    CM_CONST1, // 1: constant 1
    CM_CI,     // 2: combinational input
    CM_CO,     // 3: combinational output
    CM_AND,    // 4: And node
    CM_AND_EQ, // 5: equivalent and node
    CM_VOID    // 6: unused object
} Cm_Type_t;

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Cm_Par_t_ Cm_Par_t;
typedef struct Cm_Man_t_ Cm_Man_t;
typedef struct Cm_ManObjId_t_ Cm_ManObjId_t;
typedef struct Cm_Obj_t_ Cm_Obj_t;
typedef struct Cm_Cut_t_ Cm_Cut_t;
typedef struct Cm_ManAreaAnal_t_ Cm_ManAreaAnal_t;
typedef struct Cm_Strash_t_ Cm_Strash_t;

// the simple Strash manager for Cm
struct Cm_Strash_t_ {
    Cm_Man_t* p;        // the AIG network
    Cm_Obj_t** pBins;   // the table bins
    int nBins;          // the size of the table
    int nEntries;       // the total number of entries in the table
    Vec_Ptr_t* vNodes;  // the temporary array of nodes
    Vec_Vec_t* vLevels; // the nodes to be updated
};

struct Cm_Par_t_ {
    int nConeDepth;           // cone depth to map for
    int fVerbose;             // be verbose
    int fVeryVerbose;         // be very verbose
    int fExtraValidityChecks; // run more checks -- mainly for debugging
    int MinSoHeight;
    int fDirectCuts;                   // enable cut calculation based on priority cuts?
    int fPriorityCuts;                 // enable direct cut selection heuristic?
    int MaxCutSize;                    // maximum number of cuts in local priority cut list
    int nAreaRounds;                   // number of area recovery rounds to perform
    float AreaFlowAverageWeightFactor; // weighting factor to estimate exptected nRefs during area recovery
    int fCutBalancing;                 // enable balancing of cuts?
    int fEnableSo;                     // enable side outputs?
    int fRespectSoSlack;               // respect required time, when inserting side outputs ?
    int fStructuralRequired;           // enable direct required time calculation?
    float ArrivalRelaxFactor;
    float AicDelay[CM_MAX_DEPTH + 1]; // delay of the cones for each depth
    float AicArea[CM_MAX_DEPTH + 1];  // area of the cones for each depth
    float WireDelay;                  // wire delay of cones.
    float Epsilon;                    // used for comparisons
    int fThreeInputGates;
    int nMaxCycleDetectionRecDepth; // longest allowed side output chain path length
    MiMo_Library_t* pMiMoLib;
    float* pCiArrival;
    float* pCoRequired;
    double AreaFactor;
    int fVerboseCSV;
    int fAreaFlowHeuristic;
};

struct Cm_Man_t_ {
    char* pName;
    Cm_Par_t* pPars;
    // nodes
    Cm_Obj_t* pConst1;   // constant 1 node in AIG
    Vec_Ptr_t* vObjs;    // all objects
    Vec_Ptr_t* vCis;     // primary inputs
    Vec_Ptr_t* vCos;     // primary outputs
    Vec_Ptr_t* vCosTemp; // temporary primary outputs
    int nObjs[CM_VOID];  // number of object by type
    // additional
    int nLevelMax;
    int nObjBytes;
    // memory management
    Mem_Fixed_t* pMemObj; // memory manager for AIG nodes
    // data for cone mapping from MiMoLibrary (initialized during setup)
    MiMo_Gate_t* pConeGates[CM_MAX_DEPTH + 1];
    Vec_Ptr_t* pOrderedInputPins;
    Vec_Ptr_t* pOrderedOutputPins;
    Vec_Int_t* vTravIds;
    int nTravIds; // the unique traversal IDs of nodes
    double aTotalArea;
    double aTotalUsedGates;

    //Hash table entries
    int nBins;              // the size of the table
    int nEntries;           // the total number of entries in the table
    Cm_Obj_t** pBins;       // the table bins
    Vec_Ptr_t* vAddedCells; // the added nodes

    //Hash table for balancing
    int nBinsBal;         // the size of the table
    int nEntriesBal;      // the total number of entries in the table
    Vec_Ptr_t** pBinsBal; // the table bins
    //List to hold the updated nodes after balancing
    // These are the nodes refered during the and call in the balancing circuit
    Vec_Ptr_t* vRefNodes;

    //For the area metrics and calculation purposes
    Cm_ManAreaAnal_t* paAnal;

    //For area flow optimization
    double slackNodeMax;
    double slackNodeMean;
};

struct Cm_ManAreaAnal_t_ {
    int CellCount[CM_MAX_DEPTH];  //Array of number of used gates in the total mapping
    int CellCountAll;             //Sum of all used gates in the mapping
    float CellArea[CM_MAX_DEPTH]; //Array of area of used gates in the total mapping
    float CellAreaAll;            //Sum of areas of all used gates in the mapping
};

struct Cm_Cut_t_ {
    float Arrival;                  // arrival time of the root at cut
    float AreaFlow;                 // area flow
    short Depth;                    // depth of the cut
    short nFanins;                  // number of leafs
    Cm_Obj_t* Leafs[CM_MAX_NLEAFS]; // pointer to leafs
    int SoPos;                      // corresponding cone position of side output
    float SoArrival;                // arrival time of the side output
    Cm_Obj_t* SoOfCutAt;            // side output pointer: references to the root of the cut
};

struct Cm_Obj_t_ {
    unsigned Type;
    unsigned fCompl0;
    unsigned fCompl1;
    unsigned fCompl2; // complemented attribute of FanIns
    Cm_Obj_t* pFanin0;
    Cm_Obj_t* pFanin1; // the Fanins
    Cm_Obj_t* pFanin2;
    unsigned fPhase; // phase of the node
    unsigned Level;  // level/depth of node in AIG
    int Id;          // identifier (to vObjs)
    int IdPio;       // identifier to PI/PO
    int nRefs;
    int nSoRefs; // counts how often this node is used as SO
    int nMoRefs; // counts how often this node is used as MO
    int nVisits;
    float nRefsEstimate; // estimation how often node will be used as MO (area recovery)
    float Required;      // time requirement on node
    union {              // used as temporary storage for pointers
        void* pCopy;
        int iCopy;
    };
    union {
        float fTemp; // used as temporary storage for calculations
        int iTemp;
    };
    unsigned fRepr;   // representative node over all equivalent nodes
    Cm_Obj_t* pEquiv; // choice nodes
    unsigned fMark;   // used as temporary storage for marking/coloring
    Cm_Cut_t BestCut;
    Vec_Ptr_t* pIfFanout; // the next pointer in the hash table

    //Hash table entries
    Cm_Obj_t* pNext; // the next pointer in the hash table

    //Hash table entries
    Vec_Ptr_t* pNextBal; // the next pointer in the hash table
};

// working with the traversal ID
static inline int Cm_ManObjNumMax(Cm_Man_t* p) { return Vec_PtrSize(p->vObjs); }
static inline void Cm_ManIncrementTravId(Cm_Man_t* p) {
    if (!p->vTravIds) {
        p->vTravIds = Vec_IntAlloc(10);
        Vec_IntFill(p->vTravIds, Cm_ManObjNumMax(p) + 500, 0);
    }
    p->nTravIds++;
    assert(p->nTravIds < (1 << 30));
}
static inline unsigned Cm_ObjId(Cm_Obj_t* pObj) { return pObj->Id; }
static inline int Cm_ManTravId(Cm_Man_t* p, Cm_Obj_t* pObj) { return Vec_IntGetEntry(p->vTravIds, Cm_ObjId(pObj)); }
static inline void Cm_ObjSetTravId(Cm_Man_t* p, Cm_Obj_t* pObj, int TravId) { Vec_IntSetEntry(p->vTravIds, Cm_ObjId(pObj), TravId); }
static inline void Cm_ObjSetTravIdCurrent(Cm_Man_t* p, Cm_Obj_t* pObj) { Cm_ObjSetTravId(p, pObj, p->nTravIds); }
static inline int Cm_ManIsTravIdCurrent(Cm_Man_t* p, Cm_Obj_t* pObj) { return (Cm_ManTravId(p, pObj) == p->nTravIds); }

static inline Cm_Obj_t* Cm_Regular(Cm_Obj_t* p) { return (Cm_Obj_t*)((ABC_PTRUINT_T)(p) & ~(ABC_PTRUINT_T)01); }
static inline Cm_Obj_t* Cm_Not(Cm_Obj_t* p) { return (Cm_Obj_t*)((ABC_PTRUINT_T)(p) ^ (ABC_PTRUINT_T)01); }
static inline Cm_Obj_t* Cm_NotCond(Cm_Obj_t* p, int c) { return (Cm_Obj_t*)((ABC_PTRUINT_T)(p) ^ (ABC_PTRUINT_T)(c != 0)); }
static inline int Cm_IsComplement(Cm_Obj_t* p) { return (int)((ABC_PTRUINT_T)p & (ABC_PTRUINT_T)01); }

static inline int Cm_ManCiNum(Cm_Man_t* p) { return p->nObjs[CM_CI]; }
static inline int Cm_ManCoNum(Cm_Man_t* p) { return p->nObjs[CM_CO]; }
static inline int Cm_ManAndNum(Cm_Man_t* p) { return p->nObjs[CM_AND]; }
static inline int Cm_ManObjNum(Cm_Man_t* p) { return Vec_PtrSize(p->vObjs); }

static inline int Cm_ObjIsAnd(Cm_Obj_t* pObj) { return pObj ? pObj->Type == CM_AND : 0; }
static inline int Cm_ObjIsCi(Cm_Obj_t* pObj) { return pObj ? pObj->Type == CM_CI : 0; }
static inline int Cm_ObjIsCo(Cm_Obj_t* pObj) { return pObj ? pObj->Type == CM_CO : 0; }
static inline Cm_Obj_t* Cm_ManCi(Cm_Man_t* p, int i) { return (Cm_Obj_t*)(Vec_PtrEntry(p->vCis, i)); }
static inline Cm_Obj_t* Cm_ManCo(Cm_Man_t* p, int i) { return (Cm_Obj_t*)(Vec_PtrEntry(p->vCos, i)); }
static inline void Cm_ObjSetCopy(Cm_Obj_t* pObj, void* pCopy) { pObj->pCopy = pCopy; }
static inline Cm_Obj_t* Cm_ObjGetRepr(Cm_Obj_t* pObj) {
    Cm_Obj_t* pRepr = pObj;
    while (!pRepr->fRepr)
        pRepr = pRepr->pEquiv;
    return pRepr;
}

static inline int Cm_Pow3(int e) {
    int p = 1;
    for (int i = 0; i < e; i++)
        p *= 3;
    return p;
}
static inline int Cm_Fa3LayerStart(int depth) {
    int sp = 1;
    for (int i = 0; i < depth; i++)
        sp = 3 * sp - 1;
    return sp;
}
static inline int Cm_Fa3Size(int depth) { return (Cm_Pow3(depth) - 1) / 2; }
static inline int Cm_Fa3OutPinStartPos(int nr) { return Cm_Fa3Size(nr) / 2; }

static inline void Cm_ObjClearMarkFa(Cm_Obj_t** pFa, int depth, unsigned flag) {
    for (int i = 1; i < (2 << depth); i++)
        if (pFa[i]) pFa[i]->fMark &= ~flag;
}
static inline void Cm_ObjClearMarkFa3(Cm_Obj_t** pFa, int depth, unsigned flag) {
    for (int i = 1; i <= Cm_Fa3Size(depth); i++)
        if (pFa[i]) pFa[i]->fMark &= ~flag;
}
static inline void Cm_FaClear(Cm_Obj_t** pFa, int depth) {
    for (int i = 1; i < (2 << depth); i++)
        pFa[i] = NULL;
}
static inline void Cm_Fa3Clear(Cm_Obj_t** pFa, int depth) {
    for (int i = 1; i <= Cm_Fa3Size(depth); i++)
        pFa[i] = NULL;
}

static inline void Cm_CutClearMarkLeafs(Cm_Cut_t* pCut, unsigned flag) {
    for (int i = 0; i < pCut->nFanins; i++)
        pCut->Leafs[i]->fMark &= ~flag;
}
static inline void Cm_CutMarkLeafs(Cm_Cut_t* pCut, unsigned flag) {
    for (int i = 0; i < pCut->nFanins; i++)
        pCut->Leafs[i]->fMark |= flag;
}
static inline MiMo_PinIn_t* Cm_ManGetInputPin(Cm_Man_t* p, int pos) { return Vec_PtrEntry(p->pOrderedInputPins, pos); }
static inline MiMo_PinOut_t* Cm_ManGetOutputPin(Cm_Man_t* p, int coneDepth, int pos) {
    int sp = (p->pPars->fThreeInputGates ? Cm_Fa3OutPinStartPos(coneDepth) : (1 << (coneDepth - 1)));
    return Vec_PtrEntry(p->pOrderedOutputPins, sp + pos);
}

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#    define CM_MIN(a, b) (((a) < (b)) ? (a) : (b))
#    define CM_MAX(a, b) (((a) > (b)) ? (a) : (b))

// the small and large numbers (min/max float are 1.17e-38/3.40e+38)
#    define CM_FLOAT_LARGE ((float)1.0e+20)
#    define CM_FLOAT_SMALL ((float)1.0e-20)
#    define CM_INT_LARGE (10000000)

// iterator over the primary inputs
#    define Cm_ManForEachCi(p, pObj, i) \
        Vec_PtrForEachEntry(Cm_Obj_t*, p->vCis, pObj, i)
// iterator over the primary outputs
#    define Cm_ManForEachCo(p, pObj, i) \
        Vec_PtrForEachEntry(Cm_Obj_t*, p->vCos, pObj, i)
#    define Cm_ManForEachCoTemp(p, pObj, i) \
        Vec_PtrForEachEntry(Cm_Obj_t*, p->vCosTemp, pObj, i)
#    define Cm_ManForEachObj(p, pObj, i) \
        Vec_PtrForEachEntry(Cm_Obj_t*, p->vObjs, pObj, i)
// iterator over all objects in reverse topological order
#    define Cm_ManForEachObjReverse(p, pObj, i) \
        Vec_PtrForEachEntryReverse(Cm_Obj_t*, p->vObjs, pObj, i)
// iterator over logic nodes
#    define Cm_ManForEachNode(p, pObj, i)                         \
        Cm_ManForEachObj(p, pObj, i) if (pObj->Type != CM_AND) {} \
        else

// iterators through the entries in the linked lists of nodes
#    define Cm_ManBinForEachEntry(Type, pBin, pEnt) \
        for (pEnt = (Type)pBin;                     \
             pEnt;                                  \
             pEnt = (Type)pEnt->pNext)
////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== cmArea.c =======================================================*/
extern float Cm_ManMinimizeCutAreaFlow(Cm_Man_t* p, Cm_Obj_t** pNodes, float latestArrival, Cm_Cut_t* pCut);
extern float Cm_ManMinimizeCutAreaFlowPriority(Cm_Man_t* p, Cm_Obj_t** pNodes, float latestArrival, Cm_Cut_t* pCut);
extern float Cm_ManMinimizeCutAreaFlowPriority3(Cm_Man_t* p, Cm_Obj_t** pNodes, float latestArrival, Cm_Cut_t* pCut);
extern float Cm_ManMinimizeCutAreaFlowDirect(Cm_Man_t* p, Cm_Obj_t** pNodes, float latestArrival, Cm_Cut_t* pCut);
extern float Cm_ManMinimizeCutAreaFlowDirect3(Cm_Man_t* p, Cm_Obj_t** pNodes, float latestArrival, Cm_Cut_t* pCut);
/*=== cmBalance.c ====================================================*/
extern Cm_Obj_t* Cm_ManBalanceCut(Cm_Man_t* p, Cm_Obj_t* pObj);
extern Cm_Man_t* Cm_ManBalance(Cm_Man_t* p);
/*=== cmCore.c =======================================================*/
extern void Cm_ManSetDefaultPars(Cm_Par_t* pPars);
extern int Cm_ManPerformMapping(Cm_Man_t* p);
/*=== cmFa.c =========================================================*/
extern float Cm_FaBuildDepthOptimal(Cm_Obj_t** pNodes, Cm_Par_t* pPars);
extern int Cm_FaBuildWithMaximumDepth(Cm_Obj_t** pFaninArray, int maxDepth);
extern int Cm_Fa3BuildWithMaximumDepth(Cm_Obj_t** pFaninArray, int maxDepth);
extern void Cm_FaBuildSub(Cm_Obj_t** pFaninArray, int rootPos, Cm_Cut_t* pCut, int depth);
extern void Cm_Fa3BuildSub(Cm_Obj_t** pFaninArray, int rootPos, Cm_Cut_t* pCut, int depth);
extern void Cm_FaExtractLeafs(Cm_Obj_t** pNodes, Cm_Cut_t* pCut);
extern void Cm_Fa3ExtractLeafs(Cm_Obj_t** pNodes, Cm_Cut_t* pCut);
extern void Cm_FaShiftDownLeafs(Cm_Obj_t** pFaninArray, int depth);
extern void Cm_Fa3ShiftDownLeafs(Cm_Obj_t** pFaninArray, int depth);
extern void Cm_FaClearSub(Cm_Obj_t** pFa, int pos, int depth);
extern void Cm_Fa3ClearSub(Cm_Obj_t** pFa, int pos, int depth);
extern float Cm_FaLatestMoInputArrival(Cm_Obj_t** pFa, int depth);
extern float Cm_Fa3LatestMoInputArrival(Cm_Obj_t** pFa, int depth);
/*=== cmMan.c ========================================================*/
extern Cm_Man_t* Cm_ManStart(Cm_Par_t* pPars);
extern void Cm_ManStartFromCo(Cm_Man_t* p);
extern void Cm_ManStop(Cm_Man_t* p);
extern Cm_Obj_t* Cm_ManCreateCi(Cm_Man_t* p);
extern Cm_Obj_t* Cm_ManCreateCo(Cm_Man_t* p, Cm_Obj_t* pDriver);
extern Cm_Obj_t* Cm_ManAnd(Cm_Man_t* p, Cm_Obj_t* pFan0, Cm_Obj_t* pFan1);
extern Cm_Obj_t* Cm_ManCreateAnd(Cm_Man_t* p, Cm_Obj_t* pFan0, Cm_Obj_t* pFan1, int fFirst);
extern Cm_Obj_t* Cm_ManCreateAnd3(Cm_Man_t* p, Cm_Obj_t* pFan0, Cm_Obj_t* pFan1, Cm_Obj_t* pFan2);
extern Cm_Obj_t* Cm_ManCreateAndEq(Cm_Man_t* p, Cm_Obj_t* pFan0, Cm_Obj_t* pFan1);
extern unsigned Cm_HashKeyX(Vec_Ptr_t* vObjsCi, int TableSize);
// extern Vec_Ptr_t* Cm_ManDfs(Cm_Man_t* p, Vec_Ptr_t* vCoTemp);
extern void Cm_ManSortById(Cm_Man_t* p);
extern int Cm_ManNodeBalanceFindLeft(Vec_Ptr_t* vSuper);
extern Cm_Obj_t* Cm_ManNodeLookup(Cm_Man_t* p, Cm_Obj_t* pFan0, Cm_Obj_t* pFan1);
/*=== cmPrint.c ======================================================*/
extern void Cm_PrintPars(Cm_Par_t* pPars);
extern void Cm_PrintFa(Cm_Obj_t** pFaninArray, int depth);
extern void Cm_PrintFa3(Cm_Obj_t** pFaninArray, int depth);
extern void Cm_PrintAigStructure(Cm_Man_t* pMan, int lineLimit);
extern void Cm_PrintConeDelays(Cm_Man_t* p);
extern void Cm_PrintBestCut(Cm_Obj_t* pObj);
extern void Cm_PrintBestCutStats(Cm_Man_t* p);
extern void Cm_PrintCoArrival(Cm_Man_t* pObj);
extern void Cm_PrintCiRequired(Cm_Man_t* pObj);
extern void Cm_PrintAllRequired(Cm_Man_t* pObj);
extern void Cm_PrintAreaMetrics(Cm_Man_t* pObj);
extern void Cm_PrintAreaMetricsCSV(Cm_Man_t* pObj);
/*=== cmRequired.c ===================================================*/
extern void Cm_ManCalcVisibleRequired(Cm_Man_t* p);
extern void Cm_ManSetInvisibleRequired(Cm_Man_t* p);
extern void Cm_ManCalcRequiredStructural(Cm_Man_t* p);
extern void Cm_ManSetSlackTimes(Cm_Man_t* p);
/*=== cmSo.c =========================================================*/
extern void Cm_ManInsertSos(Cm_Man_t* p);
/*=== cmTest.c =======================================================*/
extern int Cm_TestBestCutLeafsStructure(Cm_Man_t* p);
extern int Cm_TestMonotonicArrival(Cm_Man_t* p);
extern int Cm_TestArrivalConsistency(Cm_Man_t* p);
extern int Cm_TestPositiveSlacks(Cm_Man_t* p, int fConservative);
/*=== cmUtil.c =======================================================*/
extern float Cm_CutLatestLeafMoArrival(Cm_Cut_t* pCut);
extern float Cm_CutLatestLeafArrival(Cm_Cut_t* pCut);
extern void Cm_ManSetCoRequired(Cm_Man_t* p, float required);
extern void Cm_ManSetCiArrival(Cm_Man_t* p);
extern float Cm_ManLatestCoArrival(Cm_Man_t* p);
extern float Cm_CutLeafAreaFlowSum(Cm_Cut_t* pCut);
extern float Cm_ManCutAreaFlow(Cm_Man_t* p, Cm_Cut_t* pCut);
extern void Cm_CutCopy(Cm_Cut_t* pFrom, Cm_Cut_t* pTo);
extern float Cm_ObjSoArrival(Cm_Obj_t* pObj, float* coneDelay);
extern ABC_DLL void Cm_ManGetAreaMetrics(Cm_Man_t* p);
extern int Cm_NodeCompareLevelsDecrease(Cm_Obj_t** pObj1, Cm_Obj_t** pObj2);
extern Vec_Ptr_t* Cm_VecObjPushUniqueOrderByLevel(Vec_Ptr_t* p, Cm_Obj_t* pObj);
extern Cm_Obj_t* Cm_ObjCopy(Cm_Obj_t* pObj);

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
