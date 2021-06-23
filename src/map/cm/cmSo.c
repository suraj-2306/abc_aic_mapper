 /**CFile****************************************************************

  FileName    [cmSo.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cone mapping based on priority cuts.]

  Synopsis    [Side output processing functions]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: cmSo.c,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/


#include "cm.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Returns 1 iff there is a path from pObj to node with cmpId
               or a path of length >= maxDepth starting at pObj exists ]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cm_ManSoInducesCycle_rec(Cm_Obj_t * pObj, int cmpId, int maxDepth)
{
    if (!maxDepth)
        return 1;

    if ( pObj->Id < cmpId || pObj->iCopy == cmpId )
        return 0;
    if ( pObj->Id == cmpId )
        return 1; 

    for(int i=0; i<pObj->BestCut.nFanins; i++)
    {
        Cm_Obj_t * pLeaf = pObj->BestCut.Leafs[i];
        if ( pLeaf->Id == cmpId )
            return 1;
    }
    for(int i=0; i<pObj->BestCut.nFanins; i++)
    {
        Cm_Obj_t * pLeaf = pObj->BestCut.Leafs[i];
        if ( pLeaf->Id > cmpId && ! pLeaf->BestCut.SoOfCutAt )
            if ( Cm_ManSoInducesCycle_rec(pLeaf, cmpId, maxDepth-1) )
                return 1;
    }
    for(int i=0; i<pObj->BestCut.nFanins; i++)
    {
        Cm_Obj_t * pLeaf = pObj->BestCut.Leafs[i];
        if ( pLeaf->Id > cmpId && pLeaf->BestCut.SoOfCutAt )
            if ( Cm_ManSoInducesCycle_rec(pLeaf->BestCut.SoOfCutAt, cmpId, maxDepth-1) )
                return 1;
    }
    pObj->iCopy = cmpId;
    return 0;
}


/**Function*************************************************************

  Synopsis    [Calculates all possible side outputs of the best cut rooted
               at pObj.]

  Description [Returns number of side outputs. The parameter arrays
               contain afterwards: pointer to the SOs, the
               position in the cone, the arrival time of the SO.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cm_ManCalcSo(Cm_Man_t *p , Cm_Obj_t * pObj, Cm_Obj_t ** pSo, int *pSoPos, float * pSoArrival)
{
    float *AicDelay = p->pPars->AicDelay;
    const int minSoHeight = p->pPars->MinSoHeight;

    if ( pObj->BestCut.Depth <= minSoHeight)
        return 0;
    Cm_Obj_t * pNodes[CM_MAX_FA_SIZE/2];
    int pHeight[CM_MAX_FA_SIZE/2];
    int pFaPos[CM_MAX_FA_SIZE/2];
    float pFaninArrival[CM_MAX_FA_SIZE/2];
    Cm_CutMarkLeafs(&pObj->BestCut, CM_MARK_LEAF);
    int back = 0;
    if ( !(pObj->pFanin0->fMark & CM_MARK_LEAF))
    {
        pHeight[back] = pObj->BestCut.Depth - 1;
        pFaPos[back] = 2;
        pNodes[back++] = pObj->pFanin0;
    }
    if ( !(pObj->pFanin1->fMark & CM_MARK_LEAF))
    {
        pHeight[back] = pObj->BestCut.Depth - 1;
        pFaPos[back] = 3;
        pNodes[back++] = pObj->pFanin1;
    }
    // BFS - collect nodes between leafs and nodes
    int front = 0;
    while(front != back)
    {
        Cm_Obj_t * pFront = pNodes[front];
        if ( !(pFront->pFanin0->fMark & CM_MARK_LEAF) )
        {
            pHeight[back] = pHeight[front] - 1;
            pFaPos[back] = 2 * pFaPos[front];
            pNodes[back++] = pFront->pFanin0;
        }
        if ( !(pFront->pFanin1->fMark & CM_MARK_LEAF) )
        {
            pHeight[back] = pHeight[front] - 1;
            pFaPos[back] = 2 * pFaPos[front] + 1;
            pNodes[back++] = pFront->pFanin1;
        }
        front++;
    }
    // calculate latest fanin arrival time of SO inputs
    for(int i=back-1; i>=0; --i)
    {
        int pos = pFaPos[i];
        Cm_Obj_t * pIns[2] = {pNodes[i]->pFanin0, pNodes[i]->pFanin1};
        float ar[2] = {0, 0};
        for(int k=0; k<2; k++)
            if( (pIns[k]->fMark & CM_MARK_LEAF) )
            {
                if ( pIns[k]->BestCut.SoOfCutAt )
                    ar[k] = CM_MAX(pIns[k]->BestCut.SoArrival, pIns[k]->BestCut.Arrival);
                else
                    ar[k] = pIns[k]->BestCut.Arrival;
            }
            else
            {
                ar[k] = pFaninArrival[2*pos+k];
            }
        pFaninArrival[pos] = CM_MAX(ar[0], ar[1]);
    }
    // count number of occurences of each node in iTemp
    for(int i=0; i<back; i++)
        pNodes[i]->iTemp = 0;
    for(int i=0;i<back; i++)
        pNodes[i]->iTemp++;
    // populate the Sos
    int nSo = 0;
    for(int i=back-1; i>= 0; --i)
    {
        // add as SO only once, with minimum allowed height and if not all fanouts are used in cone
        if ( !(pNodes[i]->fMark & (CM_MARK_FIXED|CM_MARK_LEAF)) &&
                  pHeight[i] >= minSoHeight && pNodes[i]->nRefs > pNodes[i]->iTemp)
        {
            int pos = pFaPos[i];
            pNodes[i]->fMark |= CM_MARK_FIXED;
            pSo[nSo] = pNodes[i];
            pSoPos[nSo] = pos;
            pSoArrival[nSo++] = AicDelay[pHeight[i]] + pFaninArrival[pos];
        }
    }
    // cleanup marking
    for(int i=0; i<back; i++)
        pNodes[i]->fMark &= ~CM_MARK_FIXED;
    Cm_CutClearMarkLeafs(&pObj->BestCut, CM_MARK_LEAF);
    return nSo; 
}
   
/**Function*************************************************************

  Synopsis    [Removes Side outputs to ensure, that
                - all side outputs come only from visible cones
                - the resulting graph contains no cycles or long side
                  output chains (cone level).
               Returns the number of disabled side outputs.]
  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cm_ManPostProcessSoAssignment(Cm_Man_t *p)
{
    int nMaxRecDepth = p->pPars->nMaxCycleDetectionRecDepth;
    int disabledSos = 0;
    Cm_Obj_t * pObj;
    int enumerator;
    Cm_ManForEachNode(p, pObj, enumerator)
    {
        pObj->nMoRefs = 0;
        pObj->nSoRefs = 0;
        pObj->fMark &= ~ CM_MARK_VISIBLE;
    }
    Cm_ManForEachObjReverse(p, pObj, enumerator)
    {
        if ( pObj->Type == CM_CO )
            pObj->pFanin0->fMark |= CM_MARK_VISIBLE;
        if ( pObj->Type != CM_AND || !(pObj->fMark & CM_MARK_VISIBLE))
        {
            if ( pObj->BestCut.SoOfCutAt )
            {
                disabledSos++;
                pObj->BestCut.SoOfCutAt = NULL;
            }
            continue;
        }
        Cm_Obj_t * pSoRoot = pObj->BestCut.SoOfCutAt;
        if ( pSoRoot && ( pSoRoot->nMoRefs == 0 || Cm_ManSoInducesCycle_rec(pSoRoot, pObj->Id, nMaxRecDepth)))
        {
            disabledSos++;
            pObj->BestCut.SoOfCutAt = NULL;
        }
        if ( pObj->BestCut.SoOfCutAt )
        {
            pSoRoot->nSoRefs++;
        } 
        else
        {
            pObj->nMoRefs++;
            for(int i=0; i<pObj->BestCut.nFanins; i++)
                pObj->BestCut.Leafs[i]->fMark |= CM_MARK_VISIBLE;
        } 
    }
    return disabledSos;
}

/**Function*************************************************************

  Synopsis    [Inserts side outputs into the mapping]

  Description [Considers only nodes marked VISIBLE.]

  SideEffects [May increase arrival times of nodes. Either unconstrained
               or limited by required times (fRespectSoSlack).]

  SeeAlso     []

***********************************************************************/
void Cm_ManInsertSos(Cm_Man_t *p)
{
    Cm_Obj_t *pObj;
    int enumerator;
    float eps = p->pPars->Epsilon;
    float *AicDelay = p->pPars->AicDelay;
    int fRespectSlack = p->pPars->fRespectSoSlack;
 
    Cm_Obj_t *pSo[CM_MAX_NLEAFS];
    int soPos[CM_MAX_NLEAFS];
    float soArrival[CM_MAX_NLEAFS];
    int nPossibleSos = 0;
    Cm_ManForEachNode(p, pObj, enumerator)
    {
        if(! (pObj->fMark&CM_MARK_VISIBLE) )
            continue;
        int nSo = Cm_ManCalcSo(p, pObj, pSo, soPos, soArrival);
        for(int i=0; i<nSo; i++)
        {
            if ( !(pSo[i]->fMark & CM_MARK_VISIBLE) )
                continue;
            // if desired: enable only Side outputs that don't violate the slack
            if ( !fRespectSlack || soArrival[i] < pSo[i]->Required + eps ||
                  (pSo[i]->BestCut.SoOfCutAt && soArrival[i] < pSo[i]->BestCut.SoArrival) )
            {
                if ( !pSo[i]->BestCut.SoOfCutAt )
                    nPossibleSos++;
                pSo[i]->BestCut.SoOfCutAt = pObj;
                pSo[i]->BestCut.SoPos = soPos[i];
                pSo[i]->BestCut.SoArrival = soArrival[i];
            }
        }
    }
    Cm_ManForEachNode(p, pObj, enumerator)
    {
        if ( pObj->BestCut.SoOfCutAt )
        {
            // update Mo arrival  as SO might be invalid
            pObj->BestCut.Arrival = Cm_CutLatestLeafArrival(&pObj->BestCut) + AicDelay[pObj->BestCut.Depth];
            Cm_Obj_t * pSoRoot = pObj->BestCut.SoOfCutAt;
            float arrivalSo = Cm_CutLatestLeafArrival(&pSoRoot->BestCut) + AicDelay[pSoRoot->BestCut.Depth];
          
            // ignore arrival reduction by SO, as it might be invalid
            float arrival = CM_MAX(arrivalSo, pObj->BestCut.Arrival);
            if ( !fRespectSlack || arrival < pObj->Required + eps  )
                pObj->BestCut.SoArrival = arrival;
            else
            {
                pObj->BestCut.SoOfCutAt = NULL;
                nPossibleSos--;
            }
        }
        if ( ! pObj->BestCut.SoOfCutAt )
        {
            pObj->BestCut.Arrival = Cm_CutLatestLeafArrival(&pObj->BestCut) + AicDelay[pObj->BestCut.Depth];
        }
    }
    int disabledSos = Cm_ManPostProcessSoAssignment(p);
    if ( p->pPars->fVerbose )
        printf("Enabled %d/%d side outputs\n", nPossibleSos - disabledSos, nPossibleSos);
}


ABC_NAMESPACE_IMPL_END
