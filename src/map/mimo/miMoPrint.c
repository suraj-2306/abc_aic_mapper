/**CFile****************************************************************

  FileName    [miMoRead.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Multiple Input Output Gate Library]

  Synopsis    [Printing ionality]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

***********************************************************************/

#include "miMo.h"

ABC_NAMESPACE_IMPL_START

/**Function*************************************************************

  Synopsis    [Prints basic statistics of the mimolibrary]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_PrintLibStatistics(MiMo_Library_t * pLib)
{
    printf("Library statistics\n");
}

/**Function*************************************************************

  Synopsis    [Prints content of mimolibary]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_PrintLibrary(MiMo_Library_t * pLib, int fVerbose)
{
    printf("MiMoLibary: %s\n", pLib->pName);
    if (fVerbose)
        MiMo_PrintLibStatistics(pLib);
}

ABC_NAMESPACE_IMPL_END
