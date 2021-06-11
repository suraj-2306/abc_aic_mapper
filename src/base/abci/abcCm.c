 /**CFile****************************************************************

  FileName    [abcCm.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Interface with the Cone mapping package.]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "map/cm/cm.h"
#include "aig/aig/aig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Cm_Man_t *  Abc_NtkToCm( Abc_Ntk_t * pNtk, Cm_Par_t * pPars );
static Abc_Ntk_t * Abc_NtkFromCm( Cm_Man_t * pMan, Abc_Ntk_t * pNtk);
 
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Interface with the cone mapping package.]

  Description [Performs data copying/conversation to/from CM and runs 
               the cone mapping algorithm]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCm( Abc_Ntk_t * pNtk, Cm_Par_t * pPars)
{
    assert (Abc_NtkIsStrash(pNtk) );

    Cm_Man_t * pCmMan = Abc_NtkToCm( pNtk, pPars );
    Cm_ManPerformMapping( pCmMan );
    Abc_Ntk_t * pNtkNew = Abc_NtkFromCm( pCmMan, pNtk );
    Cm_ManStop( pCmMan );
    if ( pNtkNew == NULL )
        return NULL;
    if ( !Abc_NtkCheck ( pNtkNew ) )
    {
        printf( "Abc_NtkCm: The network check has failed.\n" );
        Abc_NtkDelete ( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Interface with the cone mapping package.]

  Description [Setup of a (copied) AIG-feasible network into a Cm.-Man]
               
  SideEffects [The network copy references to the AIG afterwars.]

  SeeAlso     []

***********************************************************************/
static inline Cm_Obj_t * Abc_ObjCmCopy ( Abc_Obj_t * pNode ) { return (Cm_Obj_t *)pNode->pCopy; }
Cm_Man_t * Abc_NtkToCm( Abc_Ntk_t * pNtk, Cm_Par_t * pPars )
{
    assert( Abc_NtkIsStrash(pNtk) );
    int i;
    Abc_Obj_t * pNode;
    ProgressBar * pProgress;
    // initialize the mapping manager
    Cm_Man_t * pCmMan = Cm_ManStart( pPars );
    pCmMan->pName = Abc_UtilStrsav( Abc_NtkName(pNtk) );
    float estimatedMemoryGB = (1.0 * Abc_NtkObjNum(pNtk) * pCmMan->nObjBytes / (1<<30));
    if ( pPars->fVerbose || estimatedMemoryGB > 1)
        printf("Going to allocate %1.1f GB of memory for %d AIG nodes\n",
                estimatedMemoryGB, Abc_NtkObjNum(pNtk) );
    // add Primary inputs
    Abc_NtkCleanCopy( pNtk );
    Abc_AigConst1(pNtk)->pCopy = (Abc_Obj_t *)(pCmMan->pConst1);
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        Cm_Obj_t *pCi = Cm_ManCreateCi( pCmMan );
        pNode->pCopy = (Abc_Obj_t *) pCi;
        pCi->Level = pNode->Level;
    }
    // add internal nodes
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkObjNumMax(pNtk) );
    Vec_Ptr_t * vNodes = Abc_AigDfs ( pNtk, 0, 0);
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, "Initial" );
        // add node
        pNode->pCopy = (Abc_Obj_t *)Cm_ManCreateAnd( pCmMan, 
            Cm_NotCond( Abc_ObjCmCopy(Abc_ObjFanin0(pNode)), pNode->fCompl0 ), 
            Cm_NotCond( Abc_ObjCmCopy(Abc_ObjFanin1(pNode)), pNode->fCompl1 ) );
        // node that the choices nodes are currently ignored
    }
    Extra_ProgressBarStop ( pProgress );
    Vec_PtrFree( vNodes );
    // set the primary outputs without copying the phase
    Abc_NtkForEachCo( pNtk, pNode, i )
        pNode->pCopy = (Abc_Obj_t *)Cm_ManCreateCo( pCmMan, Cm_NotCond( Abc_ObjCmCopy(Abc_ObjFanin0(pNode)), pNode->fCompl0 ) );
    return pCmMan;
}

/**Function*************************************************************

  Synopsis    [Interface with the cone mapping package. -- Currently only template!!!]

  Description [Generates a new Netlist with from output of cone mapping
               manager ] 
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFromCm( Cm_Man_t * pCmMan, Abc_Ntk_t * pNtk )
{
    return NULL;
}
