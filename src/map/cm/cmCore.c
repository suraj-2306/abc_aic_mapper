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
}

/**Function*************************************************************

  Synopsis    [Performs the cone mapping]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cm_ManPerformMapping( Cm_Man_t * p )
{
    if ( p->pPars->fVeryVerbose)
    {
        Cm_PrintPars(p->pPars);
        Cm_ManPrintAigStructure(p, 100);
    }
    return 0;
}



