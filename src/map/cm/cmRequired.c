 /**CFile****************************************************************

  FileName    [cmRequired.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cone mapping based on priority cuts.]

  Synopsis    [Required time calculation functions]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: cmRequired.c,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/


#include "cm.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Calculates the required times from CO to CI of 
               nodes visible for mapping.]

  Description []
               
  SideEffects [Node mark is reset and CM_MARK_VISIBLE is recalculated.]

  SeeAlso     []

***********************************************************************/
void Cm_ManCalcVisibleRequired(Cm_Man_t *p)
{
    float * AicDelay = p->pPars->AicDelay;
    int enumerator;
    Cm_Obj_t *pObj;
    Cm_ManForEachObj(p, pObj, enumerator)
    {
        pObj->fMark = 0;
        if ( pObj->Type == CM_CO )
        {
            pObj->fMark = CM_MARK_VISIBLE;
            pObj->pFanin0->fMark = CM_MARK_VISIBLE;
            pObj->pFanin0->Required = pObj->Required;
        }
        else 
        {
            pObj->Required = CM_FLOAT_LARGE;
        }
       
    }
    Cm_ManForEachObjReverse(p, pObj, enumerator)
    {
        if ( !(pObj->fMark & CM_MARK_VISIBLE) || pObj->Type != CM_AND )
            continue;
        Cm_Obj_t * pRepr = Cm_ObjGetRepr(pObj);
        float req = pObj->Required - AicDelay[pRepr->BestCut.Depth];
        for(int i=0; i<pRepr->BestCut.nFanins; i++){
            Cm_Obj_t *l = pRepr->BestCut.Leafs[i];
            l->fMark |= CM_MARK_VISIBLE;
            if ( l->Required > req)
                l->Required = req;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Sets the required times for invisible nodes.]

  Description [The required time is set for each node to the minimum
               value of all visible cuts it is contained in. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_ManSetInvisibleRequired(Cm_Man_t *p)
{
    int enumerator;
    Cm_Obj_t * pObj;
    Cm_ManForEachObj(p, pObj, enumerator)
    {
        if( pObj->Type != CM_AND)
            continue;
        if ( !(pObj->fMark&CM_MARK_VISIBLE) )
            continue;
        for(int i=0; i<pObj->BestCut.nFanins; i++)
            pObj->BestCut.Leafs[i]->fMark |= CM_MARK_LEAF;
 
        Cm_Obj_t * pQueue[CM_MAX_FA_SIZE];
        pQueue[0] = pObj->pFanin0;
        pQueue[1] = pObj->pFanin1;
        int front = 0, back = 2;
        while(front != back)
        {
            // discard outside / invisible nodes
            Cm_Obj_t * pFront = pQueue[front++];
            // found deepest leaf? 
            if ( (pFront->fMark & CM_MARK_LEAF)  )
                continue;
            float req = pObj->Required; 

            if ( !(pFront->fMark & CM_MARK_VISIBLE) && pFront->Required > req)
                pFront->Required = req;

            if ( pFront->pFanin0 )
                pQueue[back] = pFront->pFanin0;
            if ( pFront->pFanin1 )
                pQueue[back] = pFront->pFanin1;
       }
       for(int i=0; i<pObj->BestCut.nFanins; i++)
           pObj->BestCut.Leafs[i]->fMark &= ~CM_MARK_LEAF;
    }
}

/**Function*************************************************************

  Synopsis    [Sets the optimal required times for all nodes of an
               circuit without timing constraint.]

  Description [The required times for each node depends only on its
               height, depth and the circuit height; as for each cone
               its depth (forward traversal) equals its height (backward
               traversal).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_ManCalcRequiredStructural(Cm_Man_t * p)
{
    float * AicDelay = p->pPars->AicDelay;
    int enumerator;
    Cm_Obj_t * pObj;
    // calc optimum arrival time for each level
    float * pArrival = ABC_ALLOC(float, p->nLevelMax + 1);
    pArrival[0] = 0;
    for(int i=1; i<=p->nLevelMax; i++)
    {
        pArrival[i] = CM_FLOAT_LARGE;
        for(int k=1; k<=p->pPars->nConeDepth && k<=i; k++)
        {
            float ar = pArrival[i-k] + AicDelay[k];
            if ( ar < pArrival[i])
                pArrival[i] = ar;
        }
    }
    float circuitArrival = pArrival[p->nLevelMax] * p->pPars->ArrivalRelaxFactor;
    // set required time to at least arrival time
    // this ensures that every slack can be respected, even if some nodes may not be usefull on
    // critical path
    Cm_ManForEachObj(p, pObj, enumerator)
    {
        pObj->Required = pArrival[pObj->Level];
        pObj->iTemp = 0;
    }
    // calc depth and update required time
    Cm_ManForEachObjReverse(p, pObj, enumerator)
    {
        
        float req = circuitArrival - pArrival[pObj->iTemp];
        if ( pObj->Required < req )
            pObj->Required = req;
        if ( pObj->Type == CM_AND)
        {

            if ( pObj->pFanin0->iTemp <= pObj->iTemp )
                pObj->pFanin0->iTemp = pObj->iTemp + 1;
            if ( pObj->pFanin1->iTemp <= pObj->iTemp )
                pObj->pFanin1->iTemp = pObj->iTemp + 1;
        }
    }
    ABC_FREE ( pArrival );
}

ABC_NAMESPACE_IMPL_END
