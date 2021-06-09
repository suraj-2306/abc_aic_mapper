/**CFile****************************************************************

  FileName    [miMoRead.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Multiple Input Output Gate Library]

  Synopsis    [Core functionality]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

***********************************************************************/

#include "miMo.h"

ABC_NAMESPACE_IMPL_START

/**Function*************************************************************

  Synopsis    [Starts a new (empty) mimolib with given name]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_Library_t * MiMo_LibStart(char *pName)
{
    MiMo_Library_t * pLib = ABC_ALLOC(MiMo_Library_t, 1);
    pLib->pName = Abc_UtilStrsav(pName);
    return pLib; 
}

/**Function*************************************************************

  Synopsis    [Frees the given mimolib]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_LibFree(MiMo_Library_t * pLib)
{
    ABC_FREE(pLib->pName);
    ABC_FREE(pLib);
}

ABC_NAMESPACE_IMPL_END
