/**CFile****************************************************************

  FileName    [miMo.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Multiple Input Output Gate Library]

  Synopsis    [External declarations]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: if.h,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/
 
#ifndef ABC__map__mimo_h
#define ABC__map__mimo_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "misc/vec/vec.h"
#include "misc/mem/mem.h"
#include "misc/util/utilNam.h"
#include "misc/util/abc_global.h"
#include "base/io/ioAbc.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

typedef enum { MIMO_GENERIC, MIMO_SPECIAL, MIMO_AIC2, MIMO_AIC3, MIMO_NNC2, MIMO_NNC3} MiMo_GateType_t;

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct MiMo_Library_t_ MiMo_Library_t;
typedef struct MiMo_Gate_t_ MiMo_Gate_t;
typedef struct MiMo_PinIn_t_ MiMo_PinIn_t;
typedef struct MiMo_PinOut_t_ MiMo_PinOut_t;
typedef struct MiMo_PinDelay_t_ MiMo_PinDelay_t;
typedef struct MiMo_CellPinIn_t_ MiMo_CellPinIn_t;
typedef struct MiMo_CellFanout_t_ MiMo_CellFanout_t;
typedef struct MiMo_CellPinOut_t_ MiMo_CellPinOut_t;
typedef struct MiMo_Cell_t_ MiMo_Cell_t;

struct MiMo_Library_t_
{
    char * pName;  // name of the library
    Vec_Ptr_t * pGates; // of type MiMo_Gate_t
    MiMo_Gate_t * pGate0; // constant0
    MiMo_Gate_t * pGate1; // constant1
    MiMo_Gate_t * pGateBuf; // buffer
    MiMo_Cell_t * pCellList; // list of all allocated cells
};

struct MiMo_Gate_t_ {
    char * pName; // name of the gate
    float Area; // area of the gate
    float MaxDelay; // maximum delay of all inputs to outputs
    int Depth; // maximum depth of the gate
    int GateCount; // number of internal logic gates 
    MiMo_Library_t * pMiMoLib; // reference to library
    Vec_Ptr_t * pPinIns; // input pins of type MiMoPinIn_t
    Vec_Ptr_t * pPinOuts; // output pins of type MiMoPinOut_t
    MiMo_GateType_t Type; // gate type
};

struct MiMo_PinIn_t_
{
    char * pName; // name of the input pin
    int Id; // its id
};

struct MiMo_PinOut_t_
{
    char * pName; // name 
    int Id; // its id
    float MaxDelay; // maximum delay of all inputs
    int Pos; // position in external reference array 
    MiMo_PinDelay_t * pDelayList; // combinational input path specification
};

struct MiMo_PinDelay_t_ {
    float Delay;  // the delay from input to pin in MiMo_PinOut
    int fFromPinOut; // is input a gate input (or intermediate output) ?
    void *pFromPin; // input pin:  MiMo_PinIn_t or MiMo_PinOut_t
    MiMo_PinDelay_t *pNext; // next specification
};

struct MiMo_CellPinIn_t_ {
    MiMo_PinIn_t * pPinIn; // input pin
    int FaninId; // fanin Id in Abc_Obj_t
    int FaninFanoutNetId; // fanin index in successor Abc_Obj_t 
    MiMo_CellPinIn_t * pNext; // next pinIn
};

struct MiMo_CellFanout_t_ {
    int FanoutId; // fanout index in Abc_Obj_t
    MiMo_CellFanout_t * pNext; // next pinOut
};

struct MiMo_CellPinOut_t_ {
    MiMo_PinOut_t *pPinOut; // output pin
    int FanoutNetId; // fanout index in Abc_Obj_t
    MiMo_CellFanout_t * pFanoutList; // all fanouts of this (pin, fanoutNetId) combination
    MiMo_CellPinOut_t * pNext; // next pinOut
};

struct MiMo_Cell_t_
{
    MiMo_Gate_t * pGate; // corresponding gate
    MiMo_CellPinIn_t * pPinInList; // input pins
    MiMo_CellPinOut_t * pPinOutList; // output pins
    Vec_Bit_t * vBitConfig; // configuration bits
    MiMo_Cell_t * pNext; // next cell in MiMoLibrary
};

////////////////////////////////////////////////////////////////////////
///                       INLINE FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////


static inline char * MiMo_PinDelayInName (MiMo_PinDelay_t * pDelay)  { return (pDelay->fFromPinOut ? ((MiMo_PinOut_t*)pDelay->pFromPin)->pName : ((MiMo_PinIn_t*)pDelay->pFromPin)->pName); }
static inline MiMo_PinOut_t * MiMo_PinOutAt(MiMo_Gate_t *pGate, int id) { return Vec_PtrEntry(pGate->pPinOuts, id); }
static inline MiMo_PinIn_t * MiMo_PinInAt(MiMo_Gate_t *pGate, int id) { return Vec_PtrEntry(pGate->pPinIns, id); }

static inline int MiMo_GateIsConst1 (MiMo_Gate_t * pGate)  { return (pGate ? pGate->pMiMoLib->pGate1 == pGate : 0); }
static inline int MiMo_GateIsConst0 (MiMo_Gate_t * pGate)  { return (pGate ? pGate->pMiMoLib->pGate0 == pGate : 0); }
static inline int MiMo_GateIsConst (MiMo_Gate_t * pGate) { return MiMo_GateIsConst1(pGate) || MiMo_GateIsConst0(pGate); }
static inline int MiMo_GateIsBuf (MiMo_Gate_t *pGate)     { return (pGate ? pGate->pMiMoLib->pGateBuf == pGate : 0); }
static inline int MiMo_GateIsSpecial(MiMo_Gate_t *pGate)  { return MiMo_GateIsConst(pGate) || MiMo_GateIsBuf(pGate); }


////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define MiMo_LibForEachGate( pLib, pGate, i )                                          \
    Vec_PtrForEachEntry( MiMo_Gate_t *, pLib->pGates, pGate, i )
#define MiMo_GateForEachPinIn( pGate, pPinIn, i )                                      \
    Vec_PtrForEachEntry( MiMo_PinIn_t *, pGate->pPinIns, pPinIn, i )
#define MiMo_GateForEachPinOut( pGate, pPinOut, i )                                    \
    Vec_PtrForEachEntry( MiMo_PinOut_t *, pGate->pPinOuts, pPinOut, i )


////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== miMoCell.c ===*/
extern MiMo_Cell_t * MiMo_CellCreate(MiMo_Gate_t * pGate);
extern void MiMo_CellFree(MiMo_Cell_t *pCell);
extern void MiMo_CellAddPinIn(MiMo_Cell_t *pCell, MiMo_PinIn_t * pPinIn, int faninId);
extern int MiMo_CellAddPinOut(MiMo_Cell_t *pCell, MiMo_PinOut_t *pPinOut, int fanoutId);
extern void MiMo_CellAddBufOut(MiMo_Cell_t * pCell, int fanoutId);
extern void MiMo_CellAddConstOut(MiMo_Cell_t * pCell, int fanoutId);
extern int MiMo_CellFanoutNetId(MiMo_Cell_t * pCell, int fanoutId);
extern void MiMo_CellSortFanoutNets(MiMo_Cell_t * pCell);
extern void MiMo_CellSetPinInNet(MiMo_Cell_t * pCell, int faninId, int netId);
extern int MiMo_CellGetPinInNet(MiMo_Cell_t * pCell, int faninId);
/*=== miMoCore.c ===*/
extern MiMo_Library_t * MiMo_LibStart( char * pName );
extern void MiMo_LibFree( MiMo_Library_t * pLib );
extern MiMo_Gate_t * MiMo_LibCreateGate(MiMo_Library_t *pLib, char *pName);
extern void MiMo_LibAddStandardGates(MiMo_Library_t *pLib);
extern void MiMo_GateFree( MiMo_Gate_t * pGate );
extern MiMo_PinIn_t * MiMo_GateCreatePinIn(MiMo_Gate_t * pGate, char *pName);
extern MiMo_PinOut_t * MiMo_GateCreatePinOut(MiMo_Gate_t * pGate, char *pName);
/*=== miMoPrint.c ===*/
extern void MiMo_PrintLibStatistics( MiMo_Library_t * pLib );
extern void MiMo_PrintLibrary( MiMo_Library_t * pLib, int fVerbose );
extern void MiMo_PrintCell( MiMo_Cell_t * pCell );
/*=== miMoRead.c ===*/
extern MiMo_Library_t * MiMo_ReadLibrary( char *pFileName, int fVerbose );
/*=== miMoUtil.c ===*/
extern MiMo_PinIn_t * MiMo_GateFindPinIn(MiMo_Gate_t * pGate, char *pName);
extern MiMo_PinOut_t * MiMo_GateFindPinOut(MiMo_Gate_t * pGate, char *pName);
extern void MiMo_DelayListSetDelay(MiMo_PinOut_t *pPinOut, MiMo_PinDelay_t * pLast, float delay);
extern int MiMo_DelayListAdd(MiMo_Gate_t *pGate, MiMo_PinOut_t * pToPinOut, char *pFromPinStr);
extern int MiMo_LibCheck(MiMo_Library_t *pLib);
extern void MiMo_GateCalcMaxDelay(MiMo_Gate_t * pGate);

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
