/**CFile****************************************************************

  FileName    [cmCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cone mapping based on priority cuts.]

  Synopsis    [Printing functionality.]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: ifCore.c,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/

#include <stdio.h>

#include "cm.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Prints given parameter set to stdout]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_PrintPars(Cm_Par_t * pPars)
{
    int w = 35;
    printf("%-*s%d\n", w, "Cone mapping depth", pPars->nConeDepth );
    printf("%-*s%s\n", w, "Verbose", pPars->fVerbose ? "yes" : "no");
    printf("%-*s%s\n", w, "Very verbose", pPars->fVeryVerbose ? "yes" : "no");
    printf("%-*s%s\n", w, "Extra validity checks", pPars->fExtraValidityChecks ? "yes" : "no" );
    printf("\n");
}
/**Function*************************************************************

  Synopsis    [Prints AIG statistic and limited number of nodes with 
               fanin references to stdout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline char Cm_ManCompl0ToChar(Cm_Obj_t * pObj) { return (pObj->fCompl0 ? ' ' : '!'); }
static inline char Cm_ManCompl1ToChar(Cm_Obj_t * pObj) { return (pObj->fCompl1 ? ' ' : '!'); }
void Cm_ManPrintAigStructure(Cm_Man_t *pMan, int lineLimit)
{  
    printf( "Found: %d CIs, %d ANDs, and %d COs\n", pMan->nObjs[CM_CI],
            pMan->nObjs[CM_AND], pMan->nObjs[CM_CO]);
    printf( "Up to  %d first nodes of AIG\n", lineLimit );
    
    int i;
    Cm_Obj_t *pObj;
    Cm_ManForEachObj(pMan, pObj, i)
    {
        if ( i >= lineLimit )
            break;
        switch(pObj->Type){
            case CM_CONST1:
                printf("Constant 1: %d\n", pObj->Id);
                break;
            case CM_CI:
                printf("CI %d\n", pObj->Id);
                break;
            case CM_AND:
                printf("N %d: (%c%d,%c%d)\n", pObj->Id, Cm_ManCompl0ToChar(pObj), pObj->pFanin0->Id,
                                                         Cm_ManCompl1ToChar(pObj), pObj->pFanin1->Id );
                break;
            case CM_CO:
                printf("Co %d: (%c%d)\n", pObj->Id, Cm_ManCompl0ToChar(pObj), pObj->pFanin0->Id );
                break;
            default:
                printf( "Unrecognized type\n" );
        }
    }
}



ABC_NAMESPACE_IMPL_END
