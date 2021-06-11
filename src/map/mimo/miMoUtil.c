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

/**Function*************************************************************

  Synopsis    [Performs some basic consistency checks for all gates in
               library. Returns 1 on sucess]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

int MiMo_LibCheck(MiMo_Library_t *pLib)
{
    int fOk = 1;
    int i, k;
    MiMo_Gate_t * pGate;
    MiMo_PinOut_t *pPinOut;
    MiMo_LibForEachGate(pLib, pGate, i)
    {
        Vec_Int_t * vInSeen = Vec_IntStart ( Vec_PtrSize(pGate->pPinIns) );
        // check that every output has some input 
        MiMo_GateForEachPinOut(pGate, pPinOut, k)
        {
            if ( !pPinOut->pDelayList )
            {
                fOk = 0;
                printf("Output pin %s in gate %s has no input\n", pGate->pName, pPinOut->pName);
            }
            MiMo_PinDelay_t * pDelay = pPinOut->pDelayList;
            while (pDelay)
            {
                if (pDelay->Delay < 0)
                {
                    fOk = 0;
                    printf("Gate %s has from pin %s to pin %s a negative delay (%f)\n",
                            pGate->pName, pPinOut->pName, MiMo_PinDelayInName(pDelay), pDelay->Delay);
                }
                if ( ! pDelay->fFromPinOut )
                    Vec_IntWriteEntry( vInSeen, ((MiMo_PinIn_t*)pDelay->pFromPin)->Id, 1);

                pDelay = pDelay->pNext;
            }
        }
        // check that every input is connected to some output
        int seen;
        Vec_IntForEachEntry(vInSeen, seen, k)
            if ( !seen )
            {
                fOk = 0;
                printf("Gate %s has unconnected input pin %s\n", pGate->pName,
                         ((MiMo_PinIn_t*)Vec_PtrEntry(pGate->pPinIns, k))->pName);
            }
        Vec_IntFree(vInSeen);
        // TODO: check that there is no cyclic output ...
    }
    return fOk;
}


ABC_NAMESPACE_IMPL_END
