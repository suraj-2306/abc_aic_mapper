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

  Synopsis    [Creates a new cell from given gate]

  Description []
               
  SideEffects [Stores reference in MiMoLib]

  SeeAlso     []

***********************************************************************/
MiMo_Cell_t * MiMo_CellCreate(MiMo_Gate_t * pGate)
{
    MiMo_Cell_t * pCell = ABC_ALLOC(MiMo_Cell_t, 1);
    pCell->pNext = pGate->pMiMoLib->pCellList;
    pGate->pMiMoLib->pCellList = pCell;
    pCell->pGate = pGate;
    pCell->pPinInList = NULL;
    pCell->pPinOutList = NULL;
    pCell->vBitConfig = NULL;
    return pCell;
}

/**Function*************************************************************

  Synopsis    [Frees memory of cell]

  Description []
               
  SideEffects [MiMolib reference is afterwards invalid]

  SeeAlso     []

***********************************************************************/
void MiMo_CellFree(MiMo_Cell_t *pCell)
{
    MiMo_CellPinIn_t *pCurrentIn = pCell->pPinInList;
    while(pCurrentIn)
    {
        MiMo_CellPinIn_t * pNext = pCurrentIn->pNext;
        ABC_FREE(pCurrentIn);
        pCurrentIn = pNext;
    }
    MiMo_CellPinOut_t *pCurrentOut = pCell->pPinOutList;
    while(pCurrentOut)
    {
        MiMo_CellPinOut_t * pNext = pCurrentOut->pNext;
        MiMo_CellFanout_t * pFanout = pCurrentOut->pFanoutList;
        while (pFanout)
        {
            MiMo_CellFanout_t * pNext = pFanout->pNext;
            ABC_FREE(pFanout);
            pFanout = pNext;
        } 
        ABC_FREE(pCurrentOut);
        pCurrentOut = pNext;
    }
    if (pCell->vBitConfig)
        Vec_BitFree(pCell->vBitConfig);
    ABC_FREE(pCell);

}

/**Function*************************************************************

  Synopsis    [Adds the given input pin with faninID to the cell ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_CellAddPinIn(MiMo_Cell_t *pCell, MiMo_PinIn_t * pPinIn, int faninId)
{
    MiMo_CellPinIn_t * pCellPinIn = ABC_ALLOC(MiMo_CellPinIn_t, 1);
    pCellPinIn->pPinIn = pPinIn;
    pCellPinIn->FaninId = faninId;
    pCellPinIn->FaninFanoutNetId = -1;
    pCellPinIn->pNext = pCell->pPinInList;
    pCell->pPinInList = pCellPinIn;
}

/**Function*************************************************************

  Synopsis    [Adds an output to a buffer cell with fanoutID]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_CellAddBufOut(MiMo_Cell_t *pCell, int fanoutId )
{
    assert( MiMo_GateIsBuf(pCell->pGate) );
    MiMo_CellAddPinOut(pCell, MiMo_PinOutAt(pCell->pGate, 0), fanoutId); // 0 = first (and only) output pin
}

/**Function*************************************************************

  Synopsis    [Adds an output to a constant cell with fanoutID]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_CellAddConstOut(MiMo_Cell_t *pCell, int fanoutId )
{
    assert( MiMo_GateIsConst(pCell->pGate) );
    MiMo_CellAddPinOut(pCell, MiMo_PinOutAt(pCell->pGate, 0), fanoutId); // 0 = first (and only) output pin
}

/**Function*************************************************************

  Synopsis    [Adds the given output pin with fanoutID to the cell and 
               returns the fanoutNetId]

  Description [The fanoutNetId of two cell pinouts is identical iff they
               share the same gate output pin]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_CellAddPinOut(MiMo_Cell_t *pCell, MiMo_PinOut_t * pPinOut, int fanoutId)
{
    MiMo_CellPinOut_t * pCellPinOut = pCell->pPinOutList;
    while ( pCellPinOut && pCellPinOut->pPinOut != pPinOut)
        pCellPinOut = pCellPinOut->pNext;
    // create new output if not used before
    if (!pCellPinOut)
    {
        pCellPinOut = ABC_ALLOC(MiMo_CellPinOut_t, 1);
        pCellPinOut->pPinOut = pPinOut;
        pCellPinOut->pFanoutList = NULL;
        pCellPinOut->pNext = pCell->pPinOutList;
        pCell->pPinOutList = pCellPinOut;
        pCellPinOut->FanoutNetId = (pCellPinOut->pNext ? pCellPinOut->pNext->FanoutNetId + 1 : 0);
    } 
    
    MiMo_CellFanout_t * pCellFanout = ABC_ALLOC(MiMo_CellFanout_t, 1);
    pCellFanout->FanoutId = fanoutId;
    pCellFanout->pNext = pCellPinOut->pFanoutList;
    pCellPinOut->pFanoutList = pCellFanout;
    return pCellPinOut->FanoutNetId;
}

/**Function*************************************************************

  Synopsis    [Returns the fanoutNetId ]

  Description []
               
  SideEffects []

  SeeAlso     [MiMo_CellAddPinOut]

***********************************************************************/
int MiMo_CellFanoutNetId(MiMo_Cell_t * pCell, int fanoutId)
{
    if ( !pCell || MiMo_GateIsSpecial(pCell->pGate))
        return 0;
    MiMo_CellPinOut_t * pPinOut = pCell->pPinOutList;
    while ( pPinOut )
    {
        MiMo_CellFanout_t * pCellFanout = pPinOut->pFanoutList;
        while ( pCellFanout )
        {
            if ( fanoutId == pCellFanout->FanoutId )
                return pPinOut->FanoutNetId;
            pCellFanout = pCellFanout->pNext;
        }
        pPinOut = pPinOut->pNext;
    }
    return -1;
}
/**Function*************************************************************

  Synopsis    [Sorts the cell pinouts according to its fanoutNetId]

  Description [Therefore the cell pinouts are then grouped according
               to the corresponding gate pinouts]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_CellSortFanoutNets(MiMo_Cell_t * pCell)
{
    //using bucket sort
    int k;
    Vec_Ptr_t * vBucket = Vec_PtrAlloc(16);
    MiMo_CellPinOut_t * pPinOut = pCell->pPinOutList;
    while ( pPinOut )
    {
        Vec_PtrSetEntry(vBucket, pPinOut->FanoutNetId, pPinOut);
        pPinOut = pPinOut->pNext;
    }
    pCell->pPinOutList = Vec_PtrEntry(vBucket, 0);
    MiMo_CellPinOut_t * pPrev = pCell->pPinOutList;
    Vec_PtrForEachEntryStart(MiMo_CellPinOut_t*, vBucket, pPinOut, k, 1)
    {
        pPrev->pNext = pPinOut;
        pPrev = pPrev->pNext;
    }
    pPrev->pNext = NULL;
}

/**Function*************************************************************

  Synopsis    [Sets the input netID of all cell input pins with matching
               faninId]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_CellSetPinInNet(MiMo_Cell_t * pCell, int faninId, int netId)
{
    MiMo_CellPinIn_t * pPinIn = pCell->pPinInList;
    while( pPinIn )
    {
        if ( pPinIn->FaninId == faninId )
            pPinIn->FaninFanoutNetId = netId;
        pPinIn = pPinIn->pNext;
    }
}
/**Function*************************************************************

  Synopsis    [Returns the for the first matching input pin the input
               netID]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_CellGetPinInNet(MiMo_Cell_t * pCell, int faninId)
{
    if ( !pCell)
        return -1;
    MiMo_CellPinIn_t * pPinIn = pCell->pPinInList;
    while( pPinIn )
    {
        if ( pPinIn->FaninId == faninId)
            return pPinIn->FaninFanoutNetId;
        pPinIn = pPinIn->pNext;
    }
    return -1;
}


ABC_NAMESPACE_IMPL_END
