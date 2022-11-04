/**CFile****************************************************************

  FileName    [cmUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cone mapping based on priority cuts.]

  Synopsis    [Utility functions]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: cmUtil.c,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/

#include "cm.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Calculates the latest arrival time of all cut leafs by
               considering always the main output arrival times.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
float Cm_CutLatestLeafMoArrival(Cm_Cut_t* pCut) {
    float arrival = -CM_FLOAT_LARGE;
    for (int i = 0; i < pCut->nFanins; i++) {
        Cm_Obj_t* pL = Cm_ObjGetRepr(pCut->Leafs[i]);
        if (arrival < pL->BestCut.Arrival)
            arrival = pL->BestCut.Arrival;
    }
    return arrival;
}

/**Function*************************************************************

  Synopsis    [Calculates the latest arrival time of all cut leafs by
               considering the main and side outputs arrival times. ]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
float Cm_CutLatestLeafArrival(Cm_Cut_t* pCut) {
    float arrival = -CM_FLOAT_LARGE;
    for (int i = 0; i < pCut->nFanins; i++) {
        Cm_Obj_t* pL = Cm_ObjGetRepr(pCut->Leafs[i]);
        float ca = pL->BestCut.SoOfCutAt ? pL->BestCut.SoArrival : pL->BestCut.Arrival;
        if (ca > arrival)
            arrival = ca;
    }
    return arrival;
}

/**Function*************************************************************

  Synopsis    [Sets the required times of the COs.]

  Description [Takes the minimum of the direct and external provided
               required time.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_ManSetCoRequired(Cm_Man_t* p, float required) {
    int i;
    Cm_Obj_t* pObj;
    float* pReq = p->pPars->pCoRequired;
    if (!pReq) {
        Cm_ManForEachCo(p, pObj, i)
            pObj->Required
            = required;
    } else {
        Cm_ManForEachCo(p, pObj, i)
            pObj->Required
            = CM_MIN(pReq[i], required);
    }
}

/**Function*************************************************************

  Synopsis    [Initializes the CI arrival time.]

  Description [To 0 If no external arrival time is given]

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_ManSetCiArrival(Cm_Man_t* p) {
    int i;
    Cm_Obj_t* pObj;
    float* pArr = p->pPars->pCiArrival;
    if (!pArr) {
        Cm_ManForEachCi(p, pObj, i)
            pObj->BestCut.Arrival
            = 0;
    } else {
        Cm_ManForEachCi(p, pObj, i)
            pObj->BestCut.Arrival
            = pArr[i];
    }
    p->pConst1->BestCut.Arrival = 0;
}

/**Function*************************************************************

  Synopsis    [Calculates the latest CO arrival]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
float Cm_ManLatestCoArrival(Cm_Man_t* p) {
    int i;
    Cm_Obj_t* pObj;
    float circuitArrival = -CM_FLOAT_LARGE;
    Cm_ManForEachCo(p, pObj, i)
        circuitArrival
        = CM_MAX(circuitArrival, pObj->pFanin0->BestCut.Arrival);
    return circuitArrival;
}

/**Function*************************************************************

  Synopsis    [Calculates areaflow sum of the cut leafs.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
float Cm_CutLeafAreaFlowSum(Cm_Cut_t* pCut) {
    float af = 0;
    for (int i = 0; i < pCut->nFanins; i++)
        af += pCut->Leafs[i]->BestCut.AreaFlow;
    return af;
}

/**Function*************************************************************

  Synopsis    [Calculates the areaflow of the cut.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
float Cm_ManCutAreaFlow(Cm_Man_t* p, Cm_Cut_t* pCut) {
    float* AicArea = p->pPars->AicArea;
    float af = 0;
    for (int i = 0; i < pCut->nFanins; i++)
        af += pCut->Leafs[i]->BestCut.AreaFlow;
    return af + AicArea[pCut->Depth];
}

/**Function*************************************************************

  Synopsis    [Copies the relevant content of one cut to another.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_CutCopy(Cm_Cut_t* pFrom, Cm_Cut_t* pTo) {
    pTo->Depth = pFrom->Depth;
    pTo->Arrival = pFrom->Arrival;
    pTo->AreaFlow = pFrom->AreaFlow;
    pTo->nFanins = pFrom->nFanins;
    for (int i = 0; i < pFrom->nFanins; i++)
        pTo->Leafs[i] = pFrom->Leafs[i];
}

/**Function*************************************************************

  Synopsis    [Calculates the minimal arrival time of an side output]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cm_ObjLatestLeafArrival_rec(Cm_Obj_t* pObj) {
    if ((pObj->fMark & CM_MARK_LEAF)) {
        return pObj->BestCut.SoOfCutAt ? pObj->BestCut.SoArrival : pObj->BestCut.Arrival;
    }
    return CM_MAX(Cm_ObjLatestLeafArrival_rec(pObj->pFanin0), Cm_ObjLatestLeafArrival_rec(pObj->pFanin1));
}
int Cm_ObjMaxLeafDepth_rec(Cm_Obj_t* pObj) {
    if ((pObj->fMark & CM_MARK_LEAF))
        return 0;
    return 1 + CM_MAX(Cm_ObjMaxLeafDepth_rec(pObj->pFanin0), Cm_ObjMaxLeafDepth_rec(pObj->pFanin1));
}
float Cm_ObjSoArrival(Cm_Obj_t* pObj, float* coneDelay) {
    Cm_Obj_t* pSoRoot = pObj->BestCut.SoOfCutAt;
    Cm_CutMarkLeafs(&pSoRoot->BestCut, CM_MARK_LEAF);
    int maxDepth = Cm_ObjMaxLeafDepth_rec(pObj);
    float latestArrival = Cm_ObjLatestLeafArrival_rec(pObj);
    Cm_CutClearMarkLeafs(&pSoRoot->BestCut, CM_MARK_LEAF);
    return latestArrival + coneDelay[maxDepth];
}

/**Function*************************************************************

  Synopsis    [Calculation of area metrics]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_ManGetAreaMetrics(Cm_Man_t* p) {
    int i;
    for (i = 0; i < CM_MAX_DEPTH; i++) {
        p->paAnal->CellCountAll += p->paAnal->CellCount[i];
        p->paAnal->CellAreaAll += p->paAnal->CellArea[i];
    }
}
/**Function*************************************************************

  Synopsis    [Calculates the number of nodes which are present in the given cone of depth d]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
float Cm_ManGetConeOccupancy(Cm_Man_t* p, Cm_Obj_t** pNodes, int depth) {
    int noOfNodes = 0;
    for (int i = 1; i < (2 << depth); i++)
        if (pNodes[i])
            noOfNodes++;
    double maxNoOfNodes = (2 << depth);
    float coneOccupancy = noOfNodes / maxNoOfNodes;
    return coneOccupancy;
}

/**Function*************************************************************

  Synopsis    [Procedure used for sorting the nodes in decreasing order of levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cm_NodeCompareLevelsDecrease(Cm_Obj_t** pp1, Cm_Obj_t** pp2) {
    int Diff = Cm_Regular(*pp1)->Level - Cm_Regular(*pp2)->Level;
    if (Diff > 0)
        return -1;
    if (Diff < 0)
        return 1;
    Diff = Cm_Regular(*pp1)->Id - Cm_Regular(*pp2)->Id;
    if (Diff > 0)
        return -1;
    if (Diff < 0)
        return 1;
    return 0;
}
/**Function*************************************************************

  Synopsis    [Inserts a new node in the order by levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t* Cm_VecObjPushUniqueOrderByLevel(Vec_Ptr_t* p, Cm_Obj_t* pObj) {
    Cm_Obj_t *pObj1, *pObj2;
    int i;
    if (Vec_PtrPushUnique(p, pObj))
        return NULL;
    // find the p of the node
    for (i = p->nSize - 1; i > 0; i--) {
        pObj1 = (Cm_Obj_t*)p->pArray[i];
        pObj2 = (Cm_Obj_t*)p->pArray[i - 1];
        if (pObj1->Level <= pObj2->Level)
            break;
        p->pArray[i] = pObj2;
        p->pArray[i - 1] = pObj1;
    }
    return p;
}
/**Function*************************************************************

  Synopsis    [Utility to copy the elements]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Obj_t* Cm_ObjCopy(Cm_Obj_t* pObj) {
    Cm_Obj_t* pObjCopy = ABC_ALLOC(Cm_Obj_t, 1);
    pObjCopy->Id = pObj->Id;
    pObjCopy->fMark = pObj->fMark;
    pObjCopy->Type = pObj->Type;
    pObjCopy->fCompl0 = pObj->fCompl0;
    pObjCopy->fCompl1 = pObj->fCompl1;
    pObjCopy->fCompl2 = pObj->fCompl2; // pObjCopy->complementedattribute  =pObj->attribute of FanIns
    pObjCopy->pFanin0 = pObj->pFanin0;
    pObjCopy->pFanin1 = pObj->pFanin1; // the Fanins
    pObjCopy->pFanin2 = pObj->pFanin2;
    pObjCopy->fPhase = pObj->fPhase; // phase of the node
    pObjCopy->Level = pObj->Level;   // level/depth of node in AIG
    pObjCopy->Id = pObj->Id;         // identifier (to vObjs)
    pObjCopy->IdPio = pObj->IdPio;   // identifier to PI/PO
    pObjCopy->nRefs = pObj->nRefs;
    pObjCopy->nSoRefs = pObj->nSoRefs; // counts how often this node is used as SO
    pObjCopy->nMoRefs = pObj->nMoRefs; // counts how often this node is used as MO
    pObjCopy->nVisits = pObj->nVisits;
    pObjCopy->nRefsEstimate = pObj->nRefsEstimate; // estimation how often node will be used as MO (area recovery)
    pObjCopy->Required = pObj->Required;           // time requirement on node
                                                   // used as temporary storage for pointers
    pObjCopy->pCopy = pObj->pCopy;
    pObjCopy->iCopy = pObj->iCopy;

    pObjCopy->fTemp = pObj->fTemp; // used as temporary storage for calculations
    pObjCopy->iTemp = pObj->iTemp;
    pObjCopy->fRepr = pObj->fRepr;   // representative node over all equivalent nodes
    pObjCopy->pEquiv = pObj->pEquiv; // choice nodes
    pObjCopy->fMark = pObj->fMark;   // used as temporary storage for marking/coloring
    pObjCopy->BestCut = pObj->BestCut;
    pObjCopy->pIfFanout = pObj->pIfFanout; // the next pointer in the hash table
                                           //Hash table entries
    pObjCopy->pNext = pObj->pNext;         // the next pointer in the hash table

    //Hash table entries
    pObjCopy->pNextBal = pObj->pNextBal; // the next pointer in the hash table
    return pObjCopy;
}
ABC_NAMESPACE_IMPL_END
