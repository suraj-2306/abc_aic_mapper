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
MiMo_PinOut_t *MiMo_GateFindPinOut(MiMo_Gate_t *pGate, char *pName) {
  // for now slow check, that name is not already defined
  int i;
  MiMo_PinOut_t *pPinOut;
  MiMo_GateForEachPinOut(pGate, pPinOut,
                         i) if (!strcmp(pPinOut->pName, pName)) return pPinOut;
  return NULL;
}

/**Function*************************************************************

  Synopsis    [finds input pin of gate by given name]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_PinIn_t *MiMo_GateFindPinIn(MiMo_Gate_t *pGate, char *pName) {
  // for now slow check, that name is not already defined
  int i;
  MiMo_PinIn_t *pPinIn;
  MiMo_GateForEachPinIn(pGate, pPinIn,
                        i) if (!strcmp(pPinIn->pName, pName)) return pPinIn;
  return NULL;
}

/**Function*************************************************************

  Synopsis    [Sets delay of all delayList entries until given boundary]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_DelayListSetDelay(MiMo_PinOut_t *pPinOut, MiMo_PinDelay_t *pLast,
                            float delay) {
  MiMo_PinDelay_t *pDelayList = pPinOut->pDelayList;
  while (pDelayList && pDelayList != pLast) {
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
int MiMo_DelayListAdd(MiMo_Gate_t *pGate, MiMo_PinOut_t *pToPinOut,
                      char *pFromPinStr) {
  MiMo_PinIn_t *pPinIn = MiMo_GateFindPinIn(pGate, pFromPinStr);
  if (pPinIn) {
    MiMo_PinDelay_t *pPinDelay = ABC_ALLOC(MiMo_PinDelay_t, 1);
    pPinDelay->fFromPinOut = 0;
    pPinDelay->pFromPin = pPinIn;
    pPinDelay->pNext = pToPinOut->pDelayList;
    pToPinOut->pDelayList = pPinDelay;
    return 1;
  }
  MiMo_PinOut_t *pPinOut = MiMo_GateFindPinOut(pGate, pFromPinStr);
  if (pPinOut) {
    MiMo_PinDelay_t *pPinDelay = ABC_ALLOC(MiMo_PinDelay_t, 1);
    pPinDelay->fFromPinOut = 1;
    pPinDelay->pFromPin = pPinOut;
    pPinDelay->pNext = pToPinOut->pDelayList;
    pToPinOut->pDelayList = pPinDelay;
    return 1;
  }
  printf("No Input or output pin %s for delay specification not found in gate "
         "%s\n",
         pFromPinStr, pGate->pName);
  return 0;
}

/**Function*************************************************************

  Synopsis    [Performs some basic consistency checks for all gates in
               library. Returns 1 on sucess]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

int MiMo_LibCheck(MiMo_Library_t *pLib) {
  int fOk = 1;
  int i, k;
  MiMo_Gate_t *pGate;
  MiMo_PinOut_t *pPinOut;
  MiMo_LibForEachGate(pLib, pGate, i) {
    Vec_Int_t *vInSeen = Vec_IntStart(Vec_PtrSize(pGate->pPinIns));
    // check that every output has some input
    MiMo_GateForEachPinOut(pGate, pPinOut, k) {
      if (!pPinOut->pDelayList) {
        fOk = 0;
        printf("Output pin %s in gate %s has no input\n", pGate->pName,
               pPinOut->pName);
      }
      MiMo_PinDelay_t *pDelay = pPinOut->pDelayList;
      while (pDelay) {
        if (pDelay->Delay < 0) {
          fOk = 0;
          printf("Gate %s has from pin %s to pin %s a negative delay (%f)\n",
                 pGate->pName, pPinOut->pName, MiMo_PinDelayInName(pDelay),
                 pDelay->Delay);
        }
        if (!pDelay->fFromPinOut)
          Vec_IntWriteEntry(vInSeen, ((MiMo_PinIn_t *)pDelay->pFromPin)->Id, 1);

        pDelay = pDelay->pNext;
      }
    }
    // check that every input is connected to some output
    int seen;
    Vec_IntForEachEntry(vInSeen, seen, k) if (!seen) {
      fOk = 0;
      printf("Gate %s has unconnected input pin %s\n", pGate->pName,
             ((MiMo_PinIn_t *)Vec_PtrEntry(pGate->pPinIns, k))->pName);
    }
    Vec_IntFree(vInSeen);
    // TODO: check that there is no cyclic output ...
  }
  return fOk;
}

/**Function*************************************************************

  Synopsis    [Calculates recursively the maximum delay for all the
               input pins of the given output pin]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_PinCalcMaxDelay_rec(MiMo_PinOut_t *pPinOut) {
  if (pPinOut->MaxDelay >= 0)
    return;
  MiMo_PinDelay_t *pDelay = pPinOut->pDelayList;
  float maxDelay = -1;
  while (pDelay) {
    if (pDelay->fFromPinOut) {
      MiMo_PinOut_t *pPinOutIn = pDelay->pFromPin;
      MiMo_PinCalcMaxDelay_rec(pPinOutIn);
      float delay = pPinOutIn->MaxDelay + pDelay->Delay;
      if (maxDelay < delay)
        maxDelay = delay;
    } else {
      if (maxDelay < pDelay->Delay)
        maxDelay = pDelay->Delay;
    }
    pDelay = pDelay->pNext;
  }
  pPinOut->MaxDelay = maxDelay;
}

/**Function*************************************************************

  Synopsis    [Calculates all the maximum delays between all inputs and
               outputs]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_GateCalcMaxDelay(MiMo_Gate_t *pGate) {
  int i;
  MiMo_PinOut_t *pPinOut;
  MiMo_GateForEachPinOut(pGate, pPinOut, i) pPinOut->MaxDelay = -1;
  float maxDelay = -1;
  MiMo_GateForEachPinOut(pGate, pPinOut, i) {
    MiMo_PinCalcMaxDelay_rec(pPinOut);
    if (pPinOut->MaxDelay > maxDelay)
      maxDelay = pPinOut->MaxDelay;
  }
  pGate->MaxDelay = maxDelay;
}
/**Function*************************************************************

  Synopsis    [Renames the output pins such that the blif file can be compatible
with the architecture file of VTR]

  Description [Converting out_a to out[a]. Example: out0 is converted to out[0]]

  SideEffects []

  SeeAlso     []

***********************************************************************/
char *MiMo_GateOutRenamer(MiMo_PinOut_t *pPinOutOld) {
  MiMo_PinOut_t pPinOut = *pPinOutOld;
  char pNewPinName[7] = {};
  // Basically handling the case for two digit numbers. Caution: To work with 3
  // digit numbers, you would have to change this function
  assert(pPinOut.pName[5] != '\[');
  if (pPinOutOld->pName[4] == '\0')
    sprintf(pNewPinName, "out[%d]", pPinOutOld->pName[3] - 48);
  else
    sprintf(pNewPinName, "out[%d]",
            (pPinOutOld->pName[3] - 48) * 10 + (pPinOutOld->pName[4] - 48));
  pPinOut.pName = pNewPinName;
  return pPinOut.pName;
}
/**Function*************************************************************

  Synopsis    [Renames the input pins such that the blif file can be compatible
with the architecture file of VTR]

  Description [Converting ina[b] to in[a*4 + b]. Example: in1[3] is converted
to in[7]]

  SideEffects []

  SeeAlso     []

***********************************************************************/
char *MiMo_GateInRenamer(MiMo_PinIn_t *pPinInOld) {
  MiMo_PinIn_t pPinIn = *pPinInOld;
  char pNewPinName[7] = {};
  // TODO: Need to still implement a better mapping approach to handle NNC2 and
  // NNC3 together
  if (pPinInOld->pName[3] == '\[')
    sprintf(pNewPinName, "in[%d]",
            (pPinIn.pName[2] - 48) * 4 + (pPinIn.pName[4] - 48));
  else
    sprintf(pNewPinName, "in[%d]",
            ((pPinIn.pName[2] - 48) * 10 + (pPinIn.pName[3] - 48)) * 4 +
                (pPinIn.pName[5] - 48));

  pPinIn.pName = pNewPinName;
  return pPinIn.pName;
}
ABC_NAMESPACE_IMPL_END
