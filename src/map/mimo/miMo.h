/**CFile****************************************************************

  FileName    [miMo.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Multiple Input Output Gate Library]

  Synopsis    [External declarations]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: if.h,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/
 
#ifndef ABC__map__mimo_h
#define ABC__map__mimo_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "misc/vec/vec.h"
#include "misc/mem/mem.h"
#include "misc/util/utilNam.h"
#include "misc/util/abc_global.h"
#include "base/io/ioAbc.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct MiMo_Library_t_ MiMo_Library_t;

struct MiMo_Library_t_
{
    char * pName;  // name of the library
};
////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== miMoCore.c ===*/
extern MiMo_Library_t * MiMo_LibStart( char * pName );
extern void MiMo_LibFree( MiMo_Library_t * pLib );
/*=== miMoPrint.c ===*/
extern void MiMo_PrintLibStatistics( MiMo_Library_t * pLib );
extern void MiMo_PrintLibrary( MiMo_Library_t * pLib, int fVerbose );
/*=== miMoRead.c ===*/
extern MiMo_Library_t * MiMo_ReadLibrary( char *pFileName, int fVerbose );

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
