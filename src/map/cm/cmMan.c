/**CFile****************************************************************

  FileName    [cmMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cone mapping]

  Synopsis    [(Data) management functionality]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: ifCore.c,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/

#include "cm.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Cm_Obj_t* Cm_ManSetupObj(Cm_Man_t* p);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

unsigned Cm_HashKeyX(Vec_Ptr_t* vObjsCi, int TableSize) {
    unsigned Key = 0;
    int i;
    Vec_Ptr_t* pNode;
    //Need to update this to accomadate the actual ram cell configuration
    Vec_PtrForEachEntry(Vec_Ptr_t*, vObjsCi, pNode, i) {
        Key ^= Cm_Regular((Cm_Obj_t*)pNode)->Id * 7937;
        Key ^= Cm_IsComplement((Cm_Obj_t*)pNode) * 911;
    }
    return Key % TableSize;
}
/**Function*************************************************************

  Synopsis    [Add the Hash table for each output in terms of input]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static unsigned Cm_HashKey2(Cm_Obj_t* p0, Cm_Obj_t* p1, int TableSize) {
    unsigned Key = 0;
    Key ^= Cm_Regular(p0)->Id * 7937;
    Key ^= Cm_Regular(p1)->Id * 2971;
    Key ^= Cm_IsComplement(p0) * 911;
    Key ^= Cm_IsComplement(p1) * 353;
    return Key % TableSize;
}
/**Function*************************************************************

  Synopsis    [Prepares the memory for the next AIG node]

  Description []
               
  SideEffects [Stores reference to it in vObj]

  SeeAlso     []

***********************************************************************/
Cm_Obj_t* Cm_ManSetupObj(Cm_Man_t* p) {
    // get the memory for the object
    Cm_Obj_t* pObj = (Cm_Obj_t*)Mem_FixedEntryFetch(p->pMemObj);
    memset(pObj, 0, sizeof(Cm_Obj_t));
    pObj->Id = Vec_PtrSize(p->vObjs);
    Vec_PtrPush(p->vObjs, pObj);

    //For fanout pins
    pObj->pIfFanout = NULL;
    pObj->pIfFanout = Vec_PtrAlloc(1);

    return pObj;
}

/**Function*************************************************************

  Synopsis    [Creates a Cm_Man_t from an already existing instance ]

  Description [This function is called before the AND balancing stage
  and it clears all the output and intermediate nodes]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_ManStartFromCo(Cm_Man_t* p) {
    int i;
    Cm_Obj_t* pObjTemp;

    //Remove the output nodes from the objects array
    Vec_PtrForEachEntry(Cm_Obj_t*, p->vObjs, pObjTemp, i) {
        if (pObjTemp->Type == CM_CO)
            Vec_PtrRemove(p->vObjs, p->vObjs->pArray[i--]);
    }
    //Create a temporary array for copying all the ouputs
    p->vCosTemp = Vec_PtrAllocArrayCopy(p->vCos->pArray, p->vCos->nSize);
    //Free the current array to be populated after the balancing
    ABC_FREE(p->vCos);
    p->nObjs[CM_CO] = 0;
    p->vCos = Vec_PtrAlloc(10);
}

/**Function*************************************************************

  Synopsis    [Creates a new Cm_Man_t instance ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Man_t* Cm_ManStart(Cm_Par_t* pPars) {
    if (pPars->fVeryVerbose)
        Cm_PrintPars(pPars);
    Cm_Man_t* p = ABC_ALLOC(Cm_Man_t, 1);
    memset(p, 0, sizeof(Cm_Man_t));
    p->pPars = pPars;
    // allocate (initial) memory
    p->pMemObj = Mem_FixedStart(sizeof(Cm_Obj_t));
    p->vCis = Vec_PtrAlloc(100);
    p->vCos = Vec_PtrAlloc(100);
    p->vObjs = Vec_PtrAlloc(100);
    // create const1
    p->pConst1 = Cm_ManSetupObj(p);
    p->pConst1->Type = CM_CONST1;
    p->pConst1->fPhase = 1;
    // initialize counters
    p->nLevelMax = 0;
    p->nObjs[CM_CI] = p->nObjs[CM_CO] = p->nObjs[CM_AND] = 0;
    // additional init
    p->nObjBytes = sizeof(Cm_Obj_t);
    //Hash table entries
    p->nBins = Abc_PrimeCudd(10000);
    p->pBins = ABC_ALLOC(Cm_Obj_t*, p->nBins);
    memset(p->pBins, 0, sizeof(Cm_Obj_t*) * p->nBins);

    p->nBinsBal = Abc_PrimeCudd(10000);
    p->pBinsBal = ABC_ALLOC(Vec_Ptr_t*, p->nBinsBal);
    int i;
    for (i = 0; i < 10000; i++)
        p->pBinsBal[i] = Vec_PtrAlloc(1);
    memset(p->pBinsBal, 0, sizeof(Vec_Ptr_t*) * p->nBinsBal);
    p->vRefNodes = Vec_PtrAlloc(1);
    return p;
}

/**Function*************************************************************

  Synopsis    [Clean up and deallocating of given manager instance]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_ManStop(Cm_Man_t* p) {
    Vec_PtrFree(p->vCis);
    Vec_PtrFree(p->vCos);
    Vec_PtrFree(p->vObjs);
    Mem_FixedStop(p->pMemObj, 0);
    ABC_FREE(p->pName);
    // free all allocated memory
    ABC_FREE(p);
}

/**Function*************************************************************

  Synopsis    [Creates primary input.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Obj_t* Cm_ManCreateCi(Cm_Man_t* p) {
    Cm_Obj_t* pObj;
    pObj = Cm_ManSetupObj(p);
    pObj->Type = CM_CI;
    pObj->fRepr = 1;
    pObj->IdPio = Vec_PtrSize(p->vCis);
    Vec_PtrPush(p->vCis, pObj);
    p->nObjs[CM_CI]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Creates primary output with the given driver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Obj_t* Cm_ManCreateCo(Cm_Man_t* p, Cm_Obj_t* pDriver) {
    Cm_Obj_t* pObj;
    pObj = Cm_ManSetupObj(p);
    pObj->IdPio = Vec_PtrSize(p->vCos);
    Vec_PtrPush(p->vCos, pObj);
    pObj->Type = CM_CO;
    pObj->fRepr = 1;
    pObj->fCompl0 = Cm_IsComplement(pDriver);
    pDriver = Cm_Regular(pDriver);
    pObj->pFanin0 = pDriver;
    pDriver->nRefs++;
    pObj->fPhase = (pObj->fCompl0 ^ pDriver->fPhase);
    pObj->Level = pDriver->Level;
    if (p->nLevelMax < (int)pObj->Level)
        p->nLevelMax = (int)pObj->Level;
    p->nObjs[CM_CO]++;
    Vec_PtrPush(pObj->pFanin0->pIfFanout, pObj);
    return pObj;
}
/**Function*************************************************************

  Synopsis    [Create the new node assuming it does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Obj_t* Cm_ManCreateBalanceAnd(Cm_Man_t* p, Cm_Obj_t* pFan0, Cm_Obj_t* pFan1) {
    Cm_Obj_t* pObj;

    // get memory for the new object
    pObj = Cm_ManSetupObj(p);
    pObj->Type = CM_AND;
    pObj->fRepr = 1;
    pObj->fCompl0 = Cm_IsComplement(pFan0);
    pFan0 = Cm_Regular(pFan0);
    pObj->fCompl1 = Cm_IsComplement(pFan1);
    pFan1 = Cm_Regular(pFan1);
    pObj->pFanin0 = pFan0;
    pObj->pFanin1 = pFan1;
    pObj->pFanin2 = NULL;
    pObj->fPhase = (pObj->fCompl0 ^ pFan0->fPhase) & (pObj->fCompl1 ^ pFan1->fPhase);
    pObj->Level = 1 + CM_MAX(pFan0->Level, pFan1->Level);
    if (p->nLevelMax < (int)pObj->Level)
        p->nLevelMax = (int)pObj->Level;
    p->nObjs[CM_AND]++;
    return pObj;
}
/**Function*************************************************************

  Synopsis    [Create the new node assuming it does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Obj_t* Cm_ManCreateAnd(Cm_Man_t* p, Cm_Obj_t* pFan0, Cm_Obj_t* pFan1, int fFirst) {
    Cm_Obj_t* pObj;
    unsigned Key;
    //perform constant propagation only if it is the first time that the and node is being formed
    if (fFirst) {
        if (pFan0 == pFan1)
            return pFan0;
        if (pFan0 == Cm_Not(pFan1))
            return Cm_Not(p->pConst1);
        if (Cm_Regular(pFan0) == p->pConst1)
            return pFan0 == p->pConst1 ? pFan1 : Cm_Not(p->pConst1);
        if (Cm_Regular(pFan1) == p->pConst1)
            return pFan1 == p->pConst1 ? pFan0 : Cm_Not(p->pConst1);
    }
    pObj = Cm_ManSetupObj(p);
    // if (Cm_Regular(pFan0)->Id > Cm_Regular(pFan1)->Id)
    //     pObj = pFan0, pFan0 = pFan1, pFan1 = pObj;
    pObj->Type = CM_AND;
    pObj->fRepr = 1;
    pObj->fCompl0 = Cm_IsComplement(pFan0);
    pFan0 = Cm_Regular(pFan0);
    pObj->fCompl1 = Cm_IsComplement(pFan1);
    pFan1 = Cm_Regular(pFan1);
    pObj->pFanin0 = pFan0;
    pFan0->nRefs++;
    pFan0->nVisits++;
    pObj->pFanin1 = pFan1;
    pFan1->nRefs++;
    pFan1->nVisits++;
    pObj->pFanin2 = NULL;
    pObj->fPhase = (pObj->fCompl0 ^ pFan0->fPhase) & (pObj->fCompl1 ^ pFan1->fPhase);
    pObj->Level = 1 + CM_MAX(pFan0->Level, pFan1->Level);
    if (p->nLevelMax < (int)pObj->Level)
        p->nLevelMax = (int)pObj->Level;
    p->nObjs[CM_AND]++;

    //Adding fanout information
    Vec_PtrPush(pObj->pFanin0->pIfFanout, pObj);
    Vec_PtrPush(pObj->pFanin1->pIfFanout, pObj);

    // add the node to the corresponding linked list in the table
    Key = Cm_HashKey2(pFan0, pFan1, p->nBins);
    pObj->pNext = p->pBins[Key];
    p->pBins[Key] = pObj;
    p->nEntries++;
    pObj->pCopy = NULL;

    //To indicate that the node is formed newly only the time it has been called by the balancing
    if (!fFirst)
        pObj->fMark |= CM_MARK_CO;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Performs canonicization step.]

  Description [The argument nodes can be complemented.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Obj_t* Cm_ManNodeLookup(Cm_Man_t* p, Cm_Obj_t* pFan0, Cm_Obj_t* pFan1) {
    Cm_Obj_t* pObj;
    unsigned Key;
    // perform constant propagation
    if (pFan0 == pFan1) {
        //This is not the best way but since we are only calculating
        //the total number of references this should be fine for now we can change it later if requried
        return pFan0;
    }
    if (pFan0 == Cm_Not(pFan1))
        return Cm_Not(p->pConst1);
    if (Cm_Regular(pFan0) == p->pConst1)
        return pFan0 == p->pConst1 ? pFan1 : Cm_Not(p->pConst1);
    if (Cm_Regular(pFan1) == p->pConst1)
        return pFan1 == p->pConst1 ? pFan0 : Cm_Not(p->pConst1);
    {
        int nFans0 = pFan0->pIfFanout->nSize;
        int nFans1 = pFan1->pIfFanout->nSize;
        if (nFans0 == 0 || nFans1 == 0)
            return NULL;
    }

    // order the arguments
    if (Cm_Regular(pFan0)->Id > Cm_Regular(pFan1)->Id)
        pObj = pFan0, pFan0 = pFan1, pFan1 = pObj;
    // get the hash key for these two nodes
    Key = Cm_HashKey2(pFan0, pFan1, p->nBins);
    // find the matching node in the table
    Cm_ManBinForEachEntry(Cm_Obj_t*, p->pBins[Key], pObj) {
        if (pFan0 == pObj->pFanin0 && pFan1 == pObj->pFanin1) {
            // Vec_PtrPush(pFan1->pIfFanout, pObj);
            // Vec_PtrPush(pFan0->pIfFanout, pObj);
            return pObj;
        }
    }
    return NULL;
}
/**Function*************************************************************

  Synopsis    [Performs canonicization step.]

  Description [The argument nodes can be complemented.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Obj_t* Cm_ManAnd(Cm_Man_t* p, Cm_Obj_t* pFan0, Cm_Obj_t* pFan1) {
    Cm_Obj_t* pObj;
    if ((pObj = Cm_ManNodeLookup(p, pFan0, pFan1))) {
        // This is to indicate that this could be a co created after balancing
        pObj->fMark |= CM_MARK_COBAL;
        return pObj;
    }

    pObj = Cm_ManCreateAnd(p, pFan0, pFan1, 0);
    return pObj;
}
/**Function*************************************************************

  Synopsis    [Create a new 3-input node assuming it does not exist.]

  Description [Performs no constant propagation]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Obj_t* Cm_ManCreateAnd3(Cm_Man_t* p, Cm_Obj_t* pFan0, Cm_Obj_t* pFan1, Cm_Obj_t* pFan2) {
    Cm_Obj_t* pObj;
    // get memory for the new object
    pObj = Cm_ManSetupObj(p);
    pObj->Type = CM_AND;
    pObj->fRepr = 1;
    pObj->fCompl0 = Cm_IsComplement(pFan0);
    pFan0 = Cm_Regular(pFan0);
    pObj->fCompl1 = Cm_IsComplement(pFan1);
    pFan1 = Cm_Regular(pFan1);
    pObj->fCompl2 = Cm_IsComplement(pFan2);
    pFan2 = Cm_Regular(pFan2);
    pObj->pFanin0 = pFan0;
    pFan0->nRefs++;
    pFan0->nVisits++;
    pObj->pFanin1 = pFan1;
    pFan1->nRefs++;
    pFan1->nVisits++;
    pObj->pFanin2 = pFan2;
    pFan2->nRefs++;
    pFan2->nVisits++;
    pObj->fPhase = (pObj->fCompl0 ^ pFan0->fPhase) & (pObj->fCompl1 ^ pFan1->fPhase) & (pObj->fCompl2 ^ pFan2->fPhase);
    pObj->Level = 1 + CM_MAX(CM_MAX(pFan0->Level, pFan1->Level), pFan2->Level);
    if (p->nLevelMax < (int)pObj->Level)
        p->nLevelMax = (int)pObj->Level;
    p->nObjs[CM_AND]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Create the new node for equivalent cut representation
               assuming it does not exist.]

  Description [It is initially not representive]

  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Obj_t* Cm_ManCreateAndEq(Cm_Man_t* p, Cm_Obj_t* pFan0, Cm_Obj_t* pFan1) {
    Cm_Obj_t* pObj;
    // perform constant propagation
    if (pFan0 == pFan1)
        return pFan0;
    if (pFan0 == Cm_Not(pFan1))
        return Cm_Not(p->pConst1);
    if (Cm_Regular(pFan0) == p->pConst1)
        return pFan0 == p->pConst1 ? pFan1 : Cm_Not(p->pConst1);
    if (Cm_Regular(pFan1) == p->pConst1)
        return pFan1 == p->pConst1 ? pFan0 : Cm_Not(p->pConst1);
    // get memory for the new object
    pObj = Cm_ManSetupObj(p);
    pObj->Type = CM_AND_EQ;
    pObj->fRepr = 0;
    pObj->fCompl0 = Cm_IsComplement(pFan0);
    pFan0 = Cm_Regular(pFan0);
    pObj->fCompl1 = Cm_IsComplement(pFan1);
    pFan1 = Cm_Regular(pFan1);
    pObj->pFanin0 = pFan0;
    pObj->pFanin1 = pFan1;
    pObj->pFanin2 = NULL;
    p->nObjs[CM_AND_EQ]++;
    return pObj;
}

ABC_NAMESPACE_IMPL_END
