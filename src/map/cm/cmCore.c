/**CFile****************************************************************

  FileName    [cmCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cone mapping based on priority cuts.]

  Synopsis    [The central part of the mapper.]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: ifCore.c,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/

#include "cm.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Sets default parameter for Cone Mapping]

  Description []
               
  SideEffects []

  SeeAlso     [CommandCm in base/abc.c ]

***********************************************************************/
void Cm_ManSetDefaultPars( Cm_Par_t * pPars )
{
    memset( pPars, 0, sizeof(Cm_Par_t) );
    pPars->nConeDepth = 6;
    pPars->fVerbose = 0;
    pPars->fVeryVerbose = 0;
    pPars->fExtraValidityChecks = 0;
    pPars->fDirectCuts = 1;
    pPars->fPriorityCuts = 0;
    pPars->MaxCutSize = 10;
    pPars->MinSoHeight = 2;
    pPars->fRespectSoSlack = 1;
    pPars->Epsilon = (float)0.005;
    pPars->nMaxCycleDetectionRecDepth = 5;
}


/**Function*************************************************************

  Synopsis    [Selects the required cuts for the circuit covering]

  Description [The root nodes of the selected cuts are marked VISIBLE.
               No side outputs are enabled.]
               
  SideEffects []

  SeeAlso     [CommandCm in base/abc.c ]

***********************************************************************/
void Cm_ManAssignCones( Cm_Man_t * p )
{
    Cm_Obj_t * pObj;
    int enumerator;
    Cm_ManForEachObj(p, pObj, enumerator)
    {
        pObj->fMark = 0;
        pObj->nMoRefs = 0;
        pObj->nSoRefs = 0;
        pObj->BestCut.SoOfCutAt = NULL;
    }
    Cm_ManForEachObjReverse(p, pObj, enumerator)
    {
        if ( pObj->Type == CM_CO )
        {
            pObj->pFanin0->fMark |= CM_MARK_VISIBLE;
            continue;
        }
        if ( pObj->Type == CM_AND )
        {
            if ( !(pObj->fMark&CM_MARK_VISIBLE) )
                continue;
            for(int i=0; i<pObj->BestCut.nFanins; i++)
            {
                pObj->BestCut.Leafs[i]->fMark |= CM_MARK_VISIBLE;
                pObj->BestCut.Leafs[i]->nMoRefs++;
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    [Performs one round of area recovery]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_ManRecoverArea( Cm_Man_t * p )
{
    float * AicDelay = p->pPars->AicDelay;
    float eps = p->pPars->Epsilon;
    const int minDepth = p->pPars->MinSoHeight;
    const int maxDepth = p->pPars->nConeDepth;
    int enumerator;
    Cm_Obj_t * pNodes[(2<<maxDepth)];
    Cm_Obj_t * pObj;
    Cm_Cut_t tCut;
    Cm_ManForEachNode(p, pObj, enumerator)
    {
        int fUpdate = 0;

        float bestAreaFlow = CM_FLOAT_LARGE;
        for(int d=minDepth; d<=maxDepth; d++)
        {
            pNodes[1] = pObj;
            int cdepth = Cm_FaBuildWithMaximumDepth(pNodes, d);
            if ( cdepth < d )
                break;
            float latestInputArrival = Cm_FaLatestMoInputArrival(pNodes, d);
            float requiredInputArrival = pObj->Required - AicDelay[d];
            if ( latestInputArrival > requiredInputArrival + eps )
                continue;
            tCut.Depth = d;
            float areaFlow = Cm_ManMinimizeCutAreaFlow(p, pNodes, requiredInputArrival, &tCut);
            if ( areaFlow + eps < bestAreaFlow )
            {
                fUpdate = 1;
                Cm_CutCopy(&tCut, &pObj->BestCut);
                bestAreaFlow = areaFlow;
            }
        }
        if ( fUpdate )
            pObj->BestCut.AreaFlow = bestAreaFlow / (pObj->nRefs);
        else
            pObj->BestCut.AreaFlow = Cm_ManCutAreaFlow(p, &pObj->BestCut) / pObj->nRefs;
        pObj->BestCut.Arrival = Cm_CutLatestLeafMoArrival(&pObj->BestCut) + AicDelay[pObj->BestCut.Depth];
    }
}

/**Function*************************************************************

  Synopsis    [Performs the cone mapping]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cm_ManPerformMapping( Cm_Man_t * p )
{
    Cm_Obj_t * pObj;
    Cm_Obj_t * pNodes[CM_MAX_FA_SIZE];
    int enumerator;
    float * AicDelay = p->pPars->AicDelay;
    Cm_ManSetCiArrival(p);
    Cm_ManForEachCi(p, pObj, enumerator)
        pObj->BestCut.AreaFlow = 0;

    Cm_ManForEachNode(p, pObj, enumerator)
    {
        pNodes[1] = pObj;
        float arr = Cm_FaBuildDepthOptimal(pNodes, p->pPars);
        Cm_FaExtractLeafs(pNodes, &pObj->BestCut);
        pObj->BestCut.Arrival = arr + AicDelay[pObj->BestCut.Depth];
    }
    if ( p->pPars->fExtraValidityChecks)
        Cm_TestMonotonicArrival(p);

    float arrival = Cm_ManLatestCoArrival(p);
    Cm_ManSetCoRequired(p, arrival);
    Cm_ManCalcVisibleRequired(p);
    Cm_ManSetInvisibleRequired(p);
    if ( p->pPars->fVerbose )
        Cm_PrintBestCutStats(p);
    Cm_ManRecoverArea(p);

    Cm_ManCalcVisibleRequired(p);
    if ( p->pPars->fVerbose )
        Cm_PrintBestCutStats(p);
 
    if ( p->pPars->fVeryVerbose)
    {
        Cm_PrintCoArrival(p);
        Cm_PrintCiRequired(p);
    }
    if ( p->pPars->fExtraValidityChecks )
    {
        Cm_TestBestCutLeafsStructure(p);
        Cm_TestArrivalConsistency(p);
        Cm_TestPositiveSlacks(p, 1);
    }
    Cm_ManAssignCones(p);
    Cm_ManInsertSos(p);
    if ( p->pPars->fExtraValidityChecks )
    {
        Cm_TestArrivalConsistency(p);
        Cm_TestPositiveSlacks(p, 1);
    }
    return 0;
}


ABC_NAMESPACE_IMPL_END

