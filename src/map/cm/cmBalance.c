  /**CFile****************************************************************

  FileName    [cmArea.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cone mapping based on priority cuts.]

  Synopsis    [Functions balance cuts.]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: cmArea.c,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/


#include "cm.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates Hop AIG from cut.]

  Description [iTemp has to be cut leaf number, leafs must be marked
               with CM_MARK_LEAF_CUT]

  SideEffects []

  SeeAlso     []

***********************************************************************/

Hop_Obj_t * Cm_ManCreateCutHop_rec(Hop_Man_t * pHm, Cm_Obj_t * pObj)
{
    if( (pObj->fMark & CM_MARK_LEAF_CUT) )
        return Hop_IthVar(pHm, pObj->iTemp);
    return Hop_And(pHm, Hop_NotCond(Cm_ManCreateCutHop_rec(pHm, pObj->pFanin0), pObj->fCompl0),
                        Hop_NotCond(Cm_ManCreateCutHop_rec(pHm, pObj->pFanin1), pObj->fCompl1));
}

/**Function*************************************************************

  Synopsis    [Creates equivalent nodes from (balanced) Hop AIG]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Obj_t * Cm_ManCreateEqCut_rec(Cm_Man_t *p, Hop_Obj_t * pHopObj, Cm_Obj_t * pCmRoot )
{
    if ( Hop_ObjIsPi(pHopObj) )
    {
        Cm_Obj_t * pL = pCmRoot->BestCut.Leafs[pHopObj->PioNum];
        pL->fMark |= CM_MARK_SEEN;
        return pL;
    }
    Hop_Obj_t *pH0 = Hop_ObjFanin0(pHopObj);
    Hop_Obj_t *pH1 = Hop_ObjFanin1(pHopObj);
    Cm_Obj_t * pRes = Cm_ManCreateAndEq(p, Cm_ManCreateEqCut_rec(p, pH0, pCmRoot),
                                           Cm_ManCreateEqCut_rec(p, pH1, pCmRoot));
    pRes->fCompl0 = Hop_ObjFaninC0(pHopObj);
    pRes->fCompl1 = Hop_ObjFaninC1(pHopObj);
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Tries do reduce depth of cut via balancing.]

  Description [If possible, a new equivalent node is created
               and returned. Returns otherwise NULL.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Obj_t * Cm_ManBalanceCut(Cm_Man_t *p, Cm_Obj_t * pObj)
{
    Cm_Obj_t * pEqCut = NULL;
    int d = pObj->BestCut.Depth;
    for(int i=0; i<pObj->BestCut.nFanins; i++)
    {
        pObj->BestCut.Leafs[i]->iTemp = i;
        pObj->BestCut.Leafs[i]->fMark |= CM_MARK_LEAF_CUT;
        pObj->BestCut.Leafs[i]->fMark &= ~CM_MARK_SEEN;
    }
    Hop_Man_t * pHm = Hop_ManStart();
    Hop_IthVar( pHm, pObj->BestCut.nFanins- 1 );
    Hop_Obj_t *pHopRoot = Cm_ManCreateCutHop_rec(pHm, pObj);
    Hop_ObjCreatePo(pHm, pHopRoot );
    for(int i=0; i<pObj->BestCut.nFanins; i++) 
        pObj->BestCut.Leafs[i]->fMark &= ~CM_MARK_LEAF_CUT;
    Hop_Man_t *pN = Hop_ManBalance(pHm, 1);
    int balancedDepth = Hop_ManCountLevels(pN);
    Hop_ManStop(pHm);
    if ( balancedDepth < d && d >= 2 && balancedDepth > 0)
    {
        Cm_PrintBestCut(pObj);
        pHopRoot = Hop_ObjFanin0(Hop_ManPo(pN, 0));
        // get last element in equivalence list
        Cm_Obj_t * pPre = pObj;
        while( pPre->pEquiv )
            pPre = pPre->pEquiv;
        
        Cm_Obj_t * pEq = Cm_ManCreateEqCut_rec(p, pHopRoot, pObj);

        // transfer only used leafs
        pEq->BestCut.nFanins = 0;
        for(int i=0; i<pObj->BestCut.nFanins; i++)
            if ( (pObj->BestCut.Leafs[i]->fMark & CM_MARK_SEEN) )
                pEq->BestCut.Leafs[pEq->BestCut.nFanins++] = pObj->BestCut.Leafs[i];
        pEq->BestCut.Depth = balancedDepth;
        pEq->BestCut.SoOfCutAt = NULL;
        Cm_Obj_t *pNodes[128];
        pNodes[1] = pEq;
        Cm_FaBuildWithMaximumDepth(pNodes, balancedDepth);
        Cm_PrintFa(pNodes, balancedDepth); 
        Cm_PrintBestCut(pEq);
        // add node
        pPre->pEquiv = pEq;
        pEqCut = pEq;
    }
    Hop_ManStop(pN);
    return pEqCut;
}

ABC_NAMESPACE_IMPL_END
