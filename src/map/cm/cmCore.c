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
    pPars->MinSoHeight = 2;
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

  Synopsis    [Performs the cone mapping]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cm_ManPerformMapping( Cm_Man_t * p )
{
    Cm_PrintConeDelays(p);
    Cm_Obj_t * pObj;
    Cm_Obj_t * pNodes[CM_MAX_FA_SIZE];
    int enumerator;
    float *AicDelay = p->pPars->AicDelay;
    Cm_ManForEachCi(p, pObj, enumerator)
        pObj->BestCut.Arrival = 0;
    Cm_ManForEachNode(p, pObj, enumerator)
    {
        pNodes[1] = pObj;
        float arr = Cm_FaBuildDepthOptimal(pNodes, p->pPars);
        Cm_FaExtractLeafs(pNodes, &pObj->BestCut);
        pObj->BestCut.Arrival = arr + AicDelay[pObj->BestCut.Depth];
    }
    if ( p->pPars->fExtraValidityChecks )
    {
        Cm_TestBestCutLeafsStructure(p);
        Cm_TestMonotonicArrival(p);
        Cm_TestArrivalConsistency(p);
    }
    Cm_ManAssignCones(p);
    Cm_ManInsertSos(p);
    return 0;
}


ABC_NAMESPACE_IMPL_END

