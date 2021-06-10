/**CFile****************************************************************

  FileName    [miMoUtild.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Multiple Input Output Gate Library]

  Synopsis    [Utility functionality]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

***********************************************************************/

#include "miMo.h"

ABC_NAMESPACE_IMPL_START

/**Function*************************************************************

  Synopsis    [Finds output pin of gate by name]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_PinOut_t * MiMo_GateFindPinOut(MiMo_Gate_t *pGate, char *pName)
{
    // for now slow check, that name is not already defined
    int i; 
    MiMo_PinOut_t * pPinOut;
    MiMo_GateForEachPinOut(pGate, pPinOut, i)
        if( !strcmp(pPinOut->pName, pName) )
            return pPinOut;
    return NULL;

}
 

/**Function*************************************************************

  Synopsis    [finds input pin of gate by given name]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_PinIn_t * MiMo_GateFindPinIn(MiMo_Gate_t *pGate, char *pName)
{
    // for now slow check, that name is not already defined
    int i; 
    MiMo_PinIn_t * pPinIn;
    MiMo_GateForEachPinIn(pGate, pPinIn, i)
        if( !strcmp(pPinIn->pName, pName) )
            return pPinIn;
    return NULL;
}


/**Function*************************************************************

  Synopsis    [Sets delay of all delayList entries until given boundary]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_DelayListSetDelay(MiMo_PinOut_t *pPinOut, MiMo_PinDelay_t * pLast, float delay){
    MiMo_PinDelay_t * pDelayList = pPinOut->pDelayList;
    while ( pDelayList && pDelayList != pLast )
    {
        pDelayList->Delay = delay;
        pDelayList = pDelayList->pNext;
    }
}

/**Function*************************************************************

  Synopsis    [Creates a new delay list entry based on input pin string]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_DelayListAdd(MiMo_Gate_t *pGate, MiMo_PinOut_t * pToPinOut, char *pFromPinStr)
{
    MiMo_PinIn_t * pPinIn = MiMo_GateFindPinIn(pGate, pFromPinStr);
    if ( pPinIn )
    {
        MiMo_PinDelay_t * pPinDelay = ABC_ALLOC(MiMo_PinDelay_t, 1);
        pPinDelay->fFromPinOut = 0;
        pPinDelay->pFromPin = pPinIn;
        pPinDelay->pNext = pToPinOut->pDelayList;
        pToPinOut->pDelayList = pPinDelay;
        return 1;
    }
    MiMo_PinOut_t * pPinOut =  MiMo_GateFindPinOut(pGate, pFromPinStr);
    if ( pPinOut )
    {
        MiMo_PinDelay_t * pPinDelay = ABC_ALLOC(MiMo_PinDelay_t, 1);
        pPinDelay->fFromPinOut = 1;
        pPinDelay->pFromPin = pPinOut;
        pPinDelay->pNext = pToPinOut->pDelayList;
        pToPinOut->pDelayList = pPinDelay;
        return 1;
    }
    printf("No Input or output pin %s for delay specification not found in gate %s\n", pFromPinStr, pGate->pName);
    return 0;
}


ABC_NAMESPACE_IMPL_END
