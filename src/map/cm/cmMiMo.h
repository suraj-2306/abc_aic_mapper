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

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////


extern int Cm_Cone2ReadOrderedConeGates(MiMo_Library_t *pLib, MiMo_Gate_t **ppGates, int minDepth, int maxDepth);
extern Vec_Ptr_t * Cm_Cone2ReadOrderedConeInputPins(MiMo_Gate_t ** ppGates, int minDepth, int maxDepth);
extern Vec_Ptr_t * Cm_Cone2ReadOrderedConeOutputPins(MiMo_Gate_t **ppGates, int minDepth, int maxDepth);
ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

