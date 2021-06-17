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

ABC_NAMESPACE_IMPL_END
