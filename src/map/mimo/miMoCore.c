/**CFile****************************************************************

  FileName    [miMoRead.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Multiple Input Output Gate Library]

  Synopsis    [Core functionality]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

***********************************************************************/

#include "miMo.h"

ABC_NAMESPACE_IMPL_START

/**Function*************************************************************

  Synopsis    [Starts a new (empty) mimolib with given name]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_Library_t * MiMo_LibStart(char *pName)
{
    MiMo_Library_t * pLib = ABC_ALLOC(MiMo_Library_t, 1);
    pLib->pName = Abc_UtilStrsav(pName);
    pLib->pCellList = NULL;
    pLib->pGates = Vec_PtrAlloc(8);
    return pLib; 
}

/**Function*************************************************************

  Synopsis    [Frees the given mimolib]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_LibFree(MiMo_Library_t * pLib)
{
    ABC_FREE(pLib->pName);
    int i;
    MiMo_Gate_t * pGate;
    MiMo_LibForEachGate(pLib, pGate, i)
        MiMo_GateFree(pGate);
    MiMo_Cell_t * pCell = pLib->pCellList;
    while ( pCell )
    {
        MiMo_Cell_t * pCellNext = pCell->pNext;
        MiMo_CellFree(pCell);
        pCell = pCellNext;
    }
    ABC_FREE(pLib);
}

/**Function*************************************************************

  Synopsis    [Frees the given gate]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_GateFree(MiMo_Gate_t * pGate)
{
    int i;
    MiMo_PinIn_t * pPinIn;
    MiMo_PinOut_t * pPinOut;
 
    ABC_FREE(pGate->pName);
    MiMo_GateForEachPinIn(pGate, pPinIn, i)
    {
        ABC_FREE(pPinIn->pName);
        ABC_FREE(pPinIn);
    }
    MiMo_GateForEachPinOut(pGate, pPinOut, i)
    {
        ABC_FREE(pPinOut->pName);
        MiMo_PinDelay_t * pNext, * pPinDelay = pPinOut->pDelayList;
        while(pPinDelay)
        {
            pNext = pPinDelay->pNext;
            ABC_FREE(pPinDelay);
            pPinDelay = pNext;
        }
        ABC_FREE(pPinOut);
    }
    ABC_FREE(pGate);
}


/**Function*************************************************************

  Synopsis    [Creates a new gate of library with given name]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_Gate_t * MiMo_LibCreateGate(MiMo_Library_t *pLib, char *pName)
{
    MiMo_Gate_t * pGate = ABC_ALLOC(MiMo_Gate_t, 1);
    pGate->pName = Abc_UtilStrsav(pName);
    pGate->Type = MIMO_GENERIC;
    pGate->MaxDelay = -1;
    pGate->Depth = -1;
    pGate->pMiMoLib = pLib;
    pGate->pPinIns = Vec_PtrAlloc(8);
    pGate->pPinOuts = Vec_PtrAlloc(8);
    Vec_PtrPush(pLib->pGates, pGate);
    return pGate;
}

/**Function*************************************************************

  Synopsis    [Adds constant and buffer gates to the MiMoLib]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_LibAddStandardGates(MiMo_Library_t *pLib)
{
    pLib->pGate1 = MiMo_LibCreateGate(pLib, "gateConst1");
    pLib->pGate1->Type = MIMO_SPECIAL;
    MiMo_GateCreatePinOut(pLib->pGate1, "const1");
    pLib->pGate0 = MiMo_LibCreateGate(pLib, "gateConst1");
    pLib->pGate0->Type = MIMO_SPECIAL;
    MiMo_GateCreatePinOut(pLib->pGate0, "const0");
    pLib->pGateBuf = MiMo_LibCreateGate(pLib, "gateBuff");
    pLib->pGateBuf->Type = MIMO_SPECIAL;
    MiMo_GateCreatePinIn(pLib->pGateBuf, "in");
    MiMo_GateCreatePinOut(pLib->pGateBuf, "out");
}

/**Function*************************************************************

  Synopsis    [Creates a new input pin of gate with given name]

  Description [If pin of desired name already exists,  NULL is returned]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_PinIn_t * MiMo_GateCreatePinIn(MiMo_Gate_t *pGate, char *pName)
{
    if ( MiMo_GateFindPinIn(pGate, pName) )
        return NULL;
    MiMo_PinIn_t * pPinIn = ABC_ALLOC(MiMo_PinIn_t, 1);
    pPinIn->pName = Abc_UtilStrsav(pName);
    pPinIn->Id = Vec_PtrSize(pGate->pPinIns);
    Vec_PtrPush(pGate->pPinIns, pPinIn);
    return pPinIn;
}

/**Function*************************************************************

  Synopsis    [Creates a new output pin of gate with given name]

  Description [If pin of desired name already exists,  NULL is returned]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_PinOut_t * MiMo_GateCreatePinOut(MiMo_Gate_t *pGate, char *pName)
{
    if ( MiMo_GateFindPinOut(pGate, pName) )
        return NULL;
    MiMo_PinOut_t * pPinOut = ABC_ALLOC(MiMo_PinOut_t, 1);
    pPinOut->pName = Abc_UtilStrsav(pName);
    pPinOut->pDelayList = NULL;
    pPinOut->Id = Vec_PtrSize(pGate->pPinOuts);
    pPinOut->MaxDelay = -1;
    pPinOut->Pos = -1;
    Vec_PtrPush(pGate->pPinOuts, pPinOut);
    return pPinOut;
}

ABC_NAMESPACE_IMPL_END
