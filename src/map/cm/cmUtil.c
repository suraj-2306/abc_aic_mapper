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
float Cm_CutLatestLeafMoArrival(Cm_Cut_t * pCut)
{
    float arrival = -CM_FLOAT_LARGE;
    for(int i=0; i<pCut->nFanins; i++)
    {
        Cm_Obj_t * pL = pCut->Leafs[i];
        if ( arrival < pL->BestCut.Arrival)
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
float Cm_CutLatestLeafArrival(Cm_Cut_t * pCut)
{
    float arrival = -CM_FLOAT_LARGE;
    for(int i=0; i<pCut->nFanins; i++)
    {
        Cm_Obj_t *pL = pCut->Leafs[i];
        float ca = pL->BestCut.SoOfCutAt ? pL->BestCut.SoArrival : pL->BestCut.Arrival;
        if ( ca > arrival )
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
void Cm_ManSetCoRequired(Cm_Man_t *p, float required)
{
    int i;
    Cm_Obj_t * pObj;
    float *pReq = p->pPars->pCoRequired;
    if ( !pReq ){
        Cm_ManForEachCo(p, pObj, i)
           pObj->Required = required;
    } else {
       Cm_ManForEachCo(p, pObj, i)
           pObj->Required = CM_MIN(pReq[i], required);
    }
}

/**Function*************************************************************

  Synopsis    [Initializes the CI arrival time.]

  Description [To 0 If no external arrival time is given]

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_ManSetCiArrival(Cm_Man_t *p)
{
    int i;
    Cm_Obj_t * pObj;
    float *pArr = p->pPars->pCiArrival;
    if ( !pArr ){
        Cm_ManForEachCi(p, pObj, i)
           pObj->BestCut.Arrival = 0;
    } else {
       Cm_ManForEachCi(p, pObj, i)
           pObj->BestCut.Arrival = pArr[i];
    }
}

/**Function*************************************************************

  Synopsis    [Calculates the latest CO arrival]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
float Cm_ManLatestCoArrival(Cm_Man_t *p)
{
    int i;
    Cm_Obj_t * pObj;
    float circuitArrival = -CM_FLOAT_LARGE;
    Cm_ManForEachCo(p, pObj, i)
        circuitArrival = CM_MAX(circuitArrival, pObj->pFanin0->BestCut.Arrival);
    return circuitArrival;
}

/**Function*************************************************************

  Synopsis    [Calculates areaflow sum of the cut leafs.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
float Cm_CutLeafAreaFlowSum(Cm_Cut_t * pCut)
{
    float af = 0;
    for(int i=0; i<pCut->nFanins; i++)
        af += pCut->Leafs[i]->BestCut.AreaFlow;
    return af;
}

/**Function*************************************************************

  Synopsis    [Calculates the areaflow of the cut.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
float Cm_ManCutAreaFlow(Cm_Man_t *p, Cm_Cut_t * pCut)
{
    float *AicArea = p->pPars->AicArea;
    float af = 0;
    for(int i=0; i<pCut->nFanins; i++)
        af += pCut->Leafs[i]->BestCut.AreaFlow;
    return af + AicArea[pCut->Depth];
}

/**Function*************************************************************

  Synopsis    [Copies the relevant content of one cut to another.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_CutCopy(Cm_Cut_t *pFrom, Cm_Cut_t *pTo)
{
    pTo->Depth = pFrom->Depth;
    pTo->Arrival = pFrom->Arrival;
    pTo->AreaFlow = pFrom->AreaFlow;
    pTo->nFanins = pFrom->nFanins;
    for(int i=0; i< pFrom->nFanins; i++)
        pTo->Leafs[i] = pFrom->Leafs[i];
}

/**Function*************************************************************

  Synopsis    [Calculates the minimal arrival time of an side output]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cm_ObjLatestLeafArrival_rec(Cm_Obj_t *pObj)
{
    if ( (pObj->fMark & CM_MARK_LEAF) )
    {
        return pObj->BestCut.SoOfCutAt ? pObj->BestCut.SoArrival : pObj->BestCut.Arrival;
    }
    return CM_MAX(Cm_ObjLatestLeafArrival_rec(pObj->pFanin0), Cm_ObjLatestLeafArrival_rec(pObj->pFanin1));
}
int Cm_ObjMaxLeafDepth_rec(Cm_Obj_t *pObj)
{
    if ( (pObj->fMark & CM_MARK_LEAF) )
        return 0;
    return 1 + CM_MAX(Cm_ObjMaxLeafDepth_rec(pObj->pFanin0), Cm_ObjMaxLeafDepth_rec(pObj->pFanin1));
}
float Cm_ObjSoArrival(Cm_Obj_t * pObj, float *coneDelay)
{
    Cm_Obj_t *pSoRoot = pObj->BestCut.SoOfCutAt;
    Cm_CutMarkLeafs(&pSoRoot->BestCut, CM_MARK_LEAF);
    int maxDepth = Cm_ObjMaxLeafDepth_rec(pObj);
    float latestArrival = Cm_ObjLatestLeafArrival_rec(pObj);
    Cm_CutClearMarkLeafs(&pSoRoot->BestCut, CM_MARK_LEAF);
    return latestArrival + coneDelay[maxDepth]; 
}


ABC_NAMESPACE_IMPL_END
