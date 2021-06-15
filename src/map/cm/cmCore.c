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
    pPars->Epsilon = (float)0.005;
    // This is only temporary, until the delay is inferred from the MiMoLib-Gates
    float * AicDelay = &pPars->AicDelay[0];
    AicDelay[0] = 0; AicDelay[1] = 184; AicDelay[2] = 184; AicDelay[3] = 252; AicDelay[4] = 318;
    AicDelay[5] = 388; AicDelay[6] = 450;
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
    }
    return 0;
}



ABC_NAMESPACE_IMPL_END
