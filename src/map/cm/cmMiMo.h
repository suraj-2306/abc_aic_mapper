/**CFile****************************************************************

  FileName    [cmMiMo.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cone mapping]

  Synopsis    [Addtitional functionality for miMo library]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: if.h,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/
 
#ifndef ABC__map__cm__mimo_h
#define ABC__map__cm__mimo_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <assert.h>
#include "map/mimo/miMo.h"
#include "cm.h"
#include "aig/hop/hop.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////


extern int Cm_Cone2ReadOrderedConeGates(MiMo_Library_t *pLib, MiMo_Gate_t **ppGates, int minDepth, int maxDepth);
extern Vec_Ptr_t * Cm_Cone2ReadOrderedConeInputPins(MiMo_Gate_t ** ppGates, int minDepth, int maxDepth);
extern Vec_Ptr_t * Cm_Cone2ReadOrderedConeOutputPins(MiMo_Gate_t **ppGates, int minDepth, int maxDepth);

extern MiMo_Cell_t * MiMo_CmCreateInvertingCell( MiMo_Gate_t * pGate );
extern void MiMo_CmInvertMo( MiMo_Cell_t * pCell );
extern int MiMo_CmIsClassNN( MiMo_Cell_t * pCell );
extern int MiMo_CmMoInverted( MiMo_Cell_t * pCell );
extern int MiMo_CmSoInverted( MiMo_Cell_t * pCell, int soPos );
extern int MiMo_CmMoSoInverted( MiMo_Cell_t * pCell, int soPos );
extern void MiMo_CmInvertInput( MiMo_Cell_t * pCell, int faninId);

extern MiMo_Cell_t * MiMo_CmCellFromFa( MiMo_Gate_t * pGate, void **pPredArray, int fMoCompl );
extern Hop_Obj_t * MiMo_CmToAig( MiMo_Cell_t * pCell, Hop_Man_t * p, MiMo_PinOut_t * pPinOut );


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

