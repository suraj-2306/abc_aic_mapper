/**CFile****************************************************************

  FileName    [cmTest.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cone mapping based on priority cuts.]

  Synopsis    [Testing and validation functions.]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: ifCore.c,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/

#include <stdio.h>

#include "cm.h"

ABC_NAMESPACE_IMPL_START

/**Function*************************************************************

  Synopsis    [Tests that leafs of all cuts are valid.]

  Description [Returns 1 if ok. The leafs of a cut are valid if each path
               from from the CI to the node contains at least one leaf.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cm_TestBestCutLeafsStructure(Cm_Man_t *p)
{
    int lineLimit = p->pPars->fVeryVerbose ? 1000*1000*1000  : 10;
    int lineCount = 0;
    int i, fail = 0;
    Cm_Obj_t * pObj;
    Cm_ManForEachNode(p, pObj, i)
    {
        Cm_Obj_t * pNodes[CM_MAX_FA_SIZE];
        pNodes[1] = pObj;
        int depth = Cm_FaBuildWithMaximumDepth(pNodes, pObj->BestCut.Depth);
        // mark exactly leafs
        for(int i=1; i<(2<<depth); i++)
            if(pNodes[i])
                pNodes[i]->fMark = 0;
        for(int i=0; i<pObj->BestCut.nFanins; i++)
           pObj->BestCut.Leafs[i]->fMark = CM_MARK_LEAF; 
        
        int trIds[CM_MAX_DEPTH];
        for(int i=2; i<(2<<depth); i++)
        {
            if ( !pNodes[i] || (i<(1<<depth) && (pNodes[2*i] || pNodes[2*i+1])) )
                continue;
            int leafCount = 0;
            int l = i;
            while(l)
            {
                if(pNodes[l]->fMark)
                    trIds[leafCount++] = pNodes[l]->Id;
                l /= 2;
            }
            if(leafCount == 0)
            {
                fail = 1;
                if(lineCount < lineLimit)
                {
                    printf("Structure fail: Path from node %d to root %d (cone depth %d) traverses through no leafes", 
                            pNodes[i]->Id, pObj->Id, pObj->BestCut.Depth);
                    if (leafCount > 0)
                    {
                        printf(" (");
                        for(int k=0; k<leafCount-1; k++)
                            printf("%d, ", trIds[k]);
                        printf("%d)", trIds[leafCount-1]);
                    }
                    printf("\n");
                }
                lineCount++;
            }
        }
    }
    if(fail)
    {
        printf("----------------------- %d bestcuts have ill-formed leafs\n", lineCount);
    } else
    {
        if ( p->pPars->fVerbose )   
            printf("----------------------- All bestcuts have well formed leafs\n");
    }
    return !fail;
}

/**Function*************************************************************

  Synopsis    [Tests that arrival times obey same partial ordering as the
               AIG.]

  Description [Returns 1 if so.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cm_TestMonotonicArrival(Cm_Man_t *p)
{
    Cm_Obj_t *pObj;
    int i;
    int fail = 0;
    int lineCount = 0;
    int lineLimit = p->pPars->fVeryVerbose ? 1000*1000*1000  : 10;
    Cm_ManForEachNode(p, pObj, i)
    {
        float d = pObj->BestCut.Arrival + p->pPars->Epsilon;
        if(pObj->pFanin0->BestCut.Arrival > d)
        {
            if ( lineCount < lineLimit )
            {
                printf("Monotonic arrival fail  Id: (%d, %3.1f) -> F0: (%d, %3.1f)\n", pObj->Id, d, 
                    pObj->pFanin0->Id, pObj->pFanin0->BestCut.Arrival);
                lineCount++;
            }
            fail = 1;
        }
        if(pObj->pFanin1->BestCut.Arrival > d)
        {
            if(lineCount < lineLimit)
            {
                printf("Monotonic arrival fail  Id: (%d, %3.1f) -> F1: (%d, %3.1f)\n", pObj->Id, d, 
                        pObj->pFanin1->Id, pObj->pFanin1->BestCut.Arrival);
                lineCount++;
            }
            fail = 1;
        }
    }
    if(fail)
    {
        printf("----------------------- Monontic arrival property not given\n");
    } else
    {
        if ( p->pPars->fVerbose )
            printf("----------------------- Monontic arrival property OK\n");
    }
    return !fail;
}

/**Function*************************************************************

  Synopsis    [Tests that the arrival time of a node is at least the
               minimal arrival time of the cut rooted at the node.]

  Description [Returns 0 if so.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cm_TestMoArrivalConsistency(Cm_Man_t *p, Cm_Obj_t * pObj, int fVerbose)
{
    float * AicDelay = p->pPars->AicDelay;
    float eps = p->pPars->Epsilon;
    float latestAllowedArrival = pObj->BestCut.Arrival - AicDelay[pObj->BestCut.Depth];
    int fNodeFail = 0;
    for(int i=0; i<pObj->BestCut.nFanins; i++)
    {
        Cm_Obj_t * pL = pObj->BestCut.Leafs[i];
        float d = pL->BestCut.Arrival;
        if ( d > latestAllowedArrival + eps )
        {
            if (!fNodeFail)
            {
                if ( !fVerbose )
                    return 1;
                printf("Arrival fail at node %d (Given Ar: %3.1f, depth: %d, maxAllowedFaninAr: %3.1f) ->",
                    pObj->Id, pObj->BestCut.Arrival, pObj->BestCut.Depth, latestAllowedArrival);
                fNodeFail  = 1;
            }
            printf(" (Id: %d, Arr: %3.1f)", pL->Id, d);
        }
    }
    if ( fNodeFail )
        printf("\n");
    return fNodeFail;
}

/**Function*************************************************************

  Synopsis    [Tests that arrival times of each node is at least the
               minimal arrival time of the cut rooted at the node.]

  Description [Returns 1 if so.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cm_TestArrivalConsistency(Cm_Man_t * p)
{
    float eps = p->pPars->Epsilon;
    int enumerator;
    Cm_Obj_t * pObj;
    int lineLimit = p->pPars->fVeryVerbose ? 1000*1000*1000  : 10;
    int failCount = 0;
    Cm_ManForEachCi(p, pObj, enumerator)
    {
        if ( pObj->BestCut.Arrival + eps < 0)
        {
            if (failCount < lineLimit)
                printf("Ci %d has negative arrival %3.1f\n", pObj->Id, pObj->BestCut.Arrival);
            failCount++;
        }
    }
    Cm_ManForEachNode(p, pObj, enumerator)
    {
        failCount += Cm_TestMoArrivalConsistency(p, pObj, failCount < lineLimit);
    }
    if ( failCount )
        printf("----------------------- %d nodes have invalid arrival time\n", failCount);
    else
    {
        if ( p->pPars->fVerbose )
            printf("----------------------- Consistent arrival propagation\n");
    }
    return !failCount;
}

ABC_NAMESPACE_IMPL_END
