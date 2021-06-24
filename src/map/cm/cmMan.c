/**CFile****************************************************************

  FileName    [cmMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cone mapping]

  Synopsis    [(Data) management functionality]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: ifCore.c,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/

#include "cm.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////


static Cm_Obj_t * Cm_ManSetupObj( Cm_Man_t *p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Prepares the memory for the next AIG node]

  Description []
               
  SideEffects [Stores reference to it in vObj]

  SeeAlso     []

***********************************************************************/
Cm_Obj_t * Cm_ManSetupObj (Cm_Man_t *p )
{
    // get the memory for the object
    Cm_Obj_t *pObj = (Cm_Obj_t *)Mem_FixedEntryFetch( p->pMemObj ); 
    memset( pObj, 0, sizeof(Cm_Obj_t) );
    pObj->Id = Vec_PtrSize(p->vObjs);
    Vec_PtrPush( p->vObjs, pObj );
    return pObj;
}


/**Function*************************************************************

  Synopsis    [Creates a new Cm_Man_t instance ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Man_t * Cm_ManStart( Cm_Par_t *pPars )
{
    if ( pPars->fVeryVerbose )
        Cm_PrintPars(pPars);
    Cm_Man_t * p = ABC_ALLOC( Cm_Man_t, 1);
    memset(p, 0, sizeof(Cm_Man_t));
    p->pPars = pPars;
    // allocate (initial) memory
    p->pMemObj = Mem_FixedStart(sizeof(Cm_Obj_t));
    p->vCis = Vec_PtrAlloc( 100 );
    p->vCos = Vec_PtrAlloc( 100 );
    p->vObjs = Vec_PtrAlloc( 100 );
    // create const1
    p->pConst1 = Cm_ManSetupObj ( p );
    p->pConst1->Type = CM_CONST1;
    p->pConst1->fPhase = 1;
    // initialize counters
    p->nLevelMax = 0;
    p->nObjs[CM_CI] = p->nObjs[CM_CO] = p->nObjs[CM_AND] = 0;
    // additional init
    p->nObjBytes = sizeof(Cm_Obj_t);
    return p;
}

/**Function*************************************************************

  Synopsis    [Clean up and deallocating of given manager instance]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_ManStop( Cm_Man_t * p )
{
    Vec_PtrFree( p->vCis );
    Vec_PtrFree( p->vCos );
    Vec_PtrFree( p->vObjs );
    Mem_FixedStop( p->pMemObj, 0 );
    ABC_FREE ( p->pName );
    // free all allocated memory
    ABC_FREE ( p );
}

/**Function*************************************************************

  Synopsis    [Creates primary input.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Obj_t * Cm_ManCreateCi( Cm_Man_t * p )
{
    Cm_Obj_t * pObj;
    pObj = Cm_ManSetupObj( p );
    pObj->Type = CM_CI;
    pObj->fRepr = 1;
    pObj->IdPio = Vec_PtrSize( p->vCis );
    Vec_PtrPush( p->vCis, pObj );
    p->nObjs[CM_CI]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Creates primary output with the given driver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Obj_t * Cm_ManCreateCo( Cm_Man_t * p, Cm_Obj_t * pDriver )
{
    Cm_Obj_t * pObj;
    pObj = Cm_ManSetupObj( p );
    pObj->IdPio = Vec_PtrSize( p->vCos );
    Vec_PtrPush( p->vCos, pObj );
    pObj->Type = CM_CO;
    pObj->fRepr = 1;
    pObj->fCompl0 = Cm_IsComplement(pDriver); pDriver = Cm_Regular(pDriver);
    pObj->pFanin0 = pDriver; pDriver->nRefs++; 
    pObj->fPhase  = (pObj->fCompl0 ^ pDriver->fPhase);
    pObj->Level   = pDriver->Level;
    if ( p->nLevelMax < (int)pObj->Level )
        p->nLevelMax = (int)pObj->Level;
    p->nObjs[CM_CO]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Create the new node assuming it does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Obj_t * Cm_ManCreateAnd( Cm_Man_t * p, Cm_Obj_t * pFan0, Cm_Obj_t * pFan1 )
{
    Cm_Obj_t * pObj;
    // perform constant propagation
    if ( pFan0 == pFan1 )
        return pFan0;
    if ( pFan0 == Cm_Not(pFan1) )
        return Cm_Not(p->pConst1);
    if ( Cm_Regular(pFan0) == p->pConst1 )
        return pFan0 == p->pConst1 ? pFan1 : Cm_Not(p->pConst1);
    if ( Cm_Regular(pFan1) == p->pConst1 )
        return pFan1 == p->pConst1 ? pFan0 : Cm_Not(p->pConst1);
    // get memory for the new object
    pObj = Cm_ManSetupObj( p );
    pObj->Type    = CM_AND;
    pObj->fRepr = 1;
    pObj->fCompl0 = Cm_IsComplement(pFan0); pFan0 = Cm_Regular(pFan0);
    pObj->fCompl1 = Cm_IsComplement(pFan1); pFan1 = Cm_Regular(pFan1);
    pObj->pFanin0 = pFan0; pFan0->nRefs++; pFan0->nVisits++;
    pObj->pFanin1 = pFan1; pFan1->nRefs++; pFan1->nVisits++;
    pObj->fPhase  = (pObj->fCompl0 ^ pFan0->fPhase) & (pObj->fCompl1 ^ pFan1->fPhase);
    pObj->Level   = 1 + CM_MAX( pFan0->Level, pFan1->Level );
    if ( p->nLevelMax < (int)pObj->Level )
        p->nLevelMax = (int)pObj->Level;
    p->nObjs[CM_AND]++;
    return pObj;
}


/**Function*************************************************************

  Synopsis    [Create the new node for equivalent cut representation
               assuming it does not exist.]

  Description [It is initially not representive]

  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Obj_t * Cm_ManCreateAndEq( Cm_Man_t * p, Cm_Obj_t * pFan0, Cm_Obj_t * pFan1 )
{
    Cm_Obj_t * pObj;
    // perform constant propagation
    if ( pFan0 == pFan1 )
        return pFan0;
    if ( pFan0 == Cm_Not(pFan1) )
        return Cm_Not(p->pConst1);
    if ( Cm_Regular(pFan0) == p->pConst1 )
        return pFan0 == p->pConst1 ? pFan1 : Cm_Not(p->pConst1);
    if ( Cm_Regular(pFan1) == p->pConst1 )
        return pFan1 == p->pConst1 ? pFan0 : Cm_Not(p->pConst1);
    // get memory for the new object
    pObj = Cm_ManSetupObj( p );
    pObj->Type    = CM_AND_EQ;
    pObj->fRepr = 0;
    pObj->fCompl0 = Cm_IsComplement(pFan0); pFan0 = Cm_Regular(pFan0);
    pObj->fCompl1 = Cm_IsComplement(pFan1); pFan1 = Cm_Regular(pFan1);
    pObj->pFanin0 = pFan0;
    pObj->pFanin1 = pFan1;
    p->nObjs[CM_AND_EQ]++;
    return pObj;
}

ABC_NAMESPACE_IMPL_END
