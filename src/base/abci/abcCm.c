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
#include "map/mio/mio.h"
#include "aig/aig/aig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Cm_Man_t *  Abc_Ntk2ToCm( Abc_Ntk_t * pNtk, Cm_Par_t * pPars );
static Cm_Man_t *  Abc_Ntk3ToCm( Abc_Ntk_t * pNtk, Cm_Par_t * pPars );
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
    assert ( (Abc_NtkIsStrash(pNtk) && !pPars->fThreeInputGates)
             || (Abc_NtkHasMapping(pNtk) && pPars->fThreeInputGates) );
    
    // read gates and pins from mimolib
    int minSoHeight = pPars->MinSoHeight;
    int maxDepth = pPars->nConeDepth;
    MiMo_Gate_t * pConeGates[CM_MAX_DEPTH+1];
    Vec_Ptr_t * pOrderedInputPins;
    Vec_Ptr_t * pOrderedOutputPins;
    if ( pPars->fThreeInputGates )
    {
        if ( !Cm_Cone3ReadOrderedConeGates(pPars->pMiMoLib, pConeGates, minSoHeight, maxDepth))
            return NULL;
        pOrderedInputPins = Cm_Cone3ReadOrderedConeInputPins(pConeGates, minSoHeight, maxDepth);
        if ( !pOrderedInputPins )
            return NULL;
        pOrderedOutputPins = Cm_Cone3ReadOrderedConeOutputPins(pConeGates, minSoHeight, maxDepth);
        if ( !pOrderedOutputPins )
        {
            Vec_PtrFree(pOrderedInputPins);
            return NULL;
        }
    } 
    else
    {
        if ( !Cm_Cone2ReadOrderedConeGates(pPars->pMiMoLib, pConeGates, minSoHeight, maxDepth))
            return NULL;
        pOrderedInputPins = Cm_Cone2ReadOrderedConeInputPins(pConeGates, minSoHeight, maxDepth);
        if ( !pOrderedInputPins )
            return NULL;
        pOrderedOutputPins = Cm_Cone2ReadOrderedConeOutputPins(pConeGates, minSoHeight, maxDepth);
        if ( !pOrderedOutputPins )
        {
            Vec_PtrFree(pOrderedInputPins);
            return NULL;
        }
    }
    MiMo_LibAddStandardGates(pPars->pMiMoLib);
    // transfer delay and area
    for(int i=1;i<minSoHeight; i++)
    {
        pPars->AicDelay[i] = pConeGates[minSoHeight]->MaxDelay + pPars->WireDelay;
        pPars->AicArea[i] = pConeGates[minSoHeight]->Area;
    }
    for(int i=minSoHeight; i<=pPars->nConeDepth; i++)
    {
        pPars->AicDelay[i] = pConeGates[i]->MaxDelay + pPars->WireDelay;
        pPars->AicArea[i] = pConeGates[i]->Area;
    }
    pPars->pCiArrival = Abc_NtkGetCiArrivalFloats( pNtk );
    pPars->pCoRequired = Abc_NtkGetCoRequiredFloats( pNtk );

    Cm_Man_t * pCmMan = pPars->fThreeInputGates ? Abc_Ntk3ToCm( pNtk, pPars ) : Abc_Ntk2ToCm( pNtk, pPars );
    // transfer gates and pins to mapping manager
    for(int i=0; i<=CM_MAX_DEPTH; i++)
        pCmMan->pConeGates[i] = pConeGates[i];
    pCmMan->pOrderedInputPins = pOrderedInputPins;
    pCmMan->pOrderedOutputPins = pOrderedOutputPins;
    // perform macpping 
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
Cm_Man_t * Abc_Ntk2ToCm( Abc_Ntk_t * pNtk, Cm_Par_t * pPars )
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
        // note that the choices nodes are currently ignored
    }
    Extra_ProgressBarStop ( pProgress );
    Vec_PtrFree( vNodes );
    // set the primary outputs without copying the phase
    Abc_NtkForEachCo( pNtk, pNode, i )
        pNode->pCopy = (Abc_Obj_t *)Cm_ManCreateCo( pCmMan, Cm_NotCond( Abc_ObjCmCopy(Abc_ObjFanin0(pNode)), pNode->fCompl0 ) );
    return pCmMan;
}

/**Function*************************************************************

  Synopsis    [Interface with the cone mapping package.]

  Description [Setup of a (copied) AIG-feasible network into a Cm.-Man]
               
  SideEffects [The network copy references to the AIG afterwars.]

  SeeAlso     []

***********************************************************************/
Cm_Man_t * Abc_Ntk3ToCm( Abc_Ntk_t * pNtk, Cm_Par_t * pPars )
{
    assert( Abc_NtkHasMapping(pNtk) );
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
    // Abc_AigConst1(pNtk)->pCopy = (Abc_Obj_t *)(pCmMan->pConst1);
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        Cm_Obj_t *pCi = Cm_ManCreateCi( pCmMan );
        pNode->pCopy = (Abc_Obj_t *) pCi;
        pCi->Level = pNode->Level;
        pCi->fMark = 0;
    }
    // add internal nodes -- currently quite slow as comparing to strings instead of gate pointers ...
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkObjNumMax(pNtk) );
    Vec_Ptr_t * vNodes = Abc_NtkDfs( pNtk, 0 );
    pCmMan->pConst1->fMark = 0;
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, "Initial" );
        // 
         Cm_Obj_t * pFanins[3] = { NULL, NULL, NULL};
        for(int k=0; k<Abc_ObjFaninNum(pNode); k++)
            pFanins[k] = Abc_ObjCmCopy(Abc_ObjFanin(pNode, k));
        // create nodes -- fMark = 1 means that output is complemented
        Mio_Gate_t * pGate = (Mio_Gate_t*)pNode->pData;
        char * pName = Mio_GateReadName(pGate);
        if ( ! strcmp(pName, CM_GENLIB_NAND2))
        {
            Cm_Obj_t * pCmNode = Cm_ManCreateAnd3(pCmMan, Cm_NotCond(pFanins[0], pFanins[0]->fMark), 
                                                          Cm_NotCond(pFanins[1], pFanins[1]->fMark),
                                                          pCmMan->pConst1);
            pCmNode->fMark = 1;
            pNode->pCopy = (Abc_Obj_t*) pCmNode;
        }
        if ( ! strcmp(pName, CM_GENLIB_NOR2))
        {
            Cm_Obj_t * pCmNode = Cm_ManCreateAnd3(pCmMan, Cm_NotCond(pFanins[0], 1^pFanins[0]->fMark),
                                                          Cm_NotCond(pFanins[1], 1^pFanins[1]->fMark),
                                                          Cm_Not(pCmMan->pConst1));
            pCmNode->fMark = 0;
            pNode->pCopy = (Abc_Obj_t*) pCmNode;
        }
        if ( ! strcmp(pName, CM_GENLIB_NAND3))
        {
            Cm_Obj_t * pCmNode = Cm_ManCreateAnd3(pCmMan, Cm_NotCond(pFanins[0], pFanins[0]->fMark), 
                                                          Cm_NotCond(pFanins[1], pFanins[1]->fMark),
                                                          Cm_NotCond(pFanins[2], pFanins[2]->fMark));
            pCmNode->fMark = 1;
            pNode->pCopy = (Abc_Obj_t*) pCmNode;
        }
        if ( ! strcmp(pName, CM_GENLIB_NOR3))
        {
            Cm_Obj_t * pCmNode = Cm_ManCreateAnd3(pCmMan, Cm_NotCond(pFanins[0], 1^pFanins[0]->fMark), 
                                                          Cm_NotCond(pFanins[1], 1^pFanins[1]->fMark),
                                                          Cm_NotCond(pFanins[2], 1^pFanins[2]->fMark));
            pCmNode->fMark = 0;
            pNode->pCopy = (Abc_Obj_t*) pCmNode;
        }
        if ( ! strcmp(pName, CM_GENLIB_INV))
        {
            Cm_Obj_t * pCmNode = Cm_ManCreateAnd3( pCmMan, Cm_NotCond(pFanins[0], pFanins[0]->fMark),
                                                                     pCmMan->pConst1, pCmMan->pConst1);
            pCmNode->fMark = 1;
            pNode->pCopy = (Abc_Obj_t*) pCmNode;
        }
        if ( ! strcmp(pName, CM_GENLIB_C0))
        {
            // this extra And gate might be avoided ...       
            Cm_Obj_t * pCmNode = Cm_ManCreateAnd3( pCmMan, pCmMan->pConst1,
                                                                     pCmMan->pConst1, pCmMan->pConst1);
            pCmNode->fMark = 1;
            pNode->pCopy = (Abc_Obj_t*) pCmMan->pConst1;
            printf("0\n");
        }
        if ( ! strcmp(pName, CM_GENLIB_BUF) )
        {
            // buffers are only inserted to connect CIs to COs
            assert( Abc_ObjIsCi(Abc_ObjFanin0(pNode)) && Abc_ObjIsCo(Abc_ObjFanout0(pNode)) );
            pNode->pCopy = Abc_ObjFanin0(pNode)->pCopy;

        }
        if ( ! strcmp(pName, CM_GENLIB_C1))
        {
            pNode->pCopy = (Abc_Obj_t *)(pCmMan->pConst1);
            printf("1\n");
        }
    }
    Extra_ProgressBarStop ( pProgress );
    Vec_PtrFree( vNodes );
    // set the primary outputs without copying the phase
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        Cm_Obj_t * pFanin = Abc_ObjCmCopy(Abc_ObjFanin0(pNode));
        pNode->pCopy = (Abc_Obj_t *)Cm_ManCreateCo( pCmMan, Cm_NotCond(pFanin, pFanin->fMark ) );
    }
    return pCmMan;
}


/**Function*************************************************************

  Synopsis    [Inserts buffers to make all circuit outputs simple]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_MakeSimpleCos(Abc_Ntk_t * pNtk)
{
    Abc_Obj_t * pNode, *pNodeOrig, *pDriver;
    int i;
    Abc_NtkForEachCo(pNtk, pNode, i)
        Abc_ObjFanin0(pNode)->pCopy = NULL;
    Abc_NtkForEachCo(pNtk, pNode, i)
    {
        pDriver = Abc_ObjFanin0(pNode);
        if ( pDriver->pCopy )
        {
            assert( !Abc_ObjIsCi(pDriver) );

            pNodeOrig = pDriver->pCopy;
            MiMo_Cell_t * pCell = pDriver->pData;
            int fanoutOrigNum =  Abc_ObjFaninFanoutNum(pDriver, pNodeOrig);
            int fanoutNewNum = Abc_ObjFaninFanoutNum(pDriver, pNode);
            MiMo_CellPinOut_t *pPinOut = pCell->pPinOutList;

            int fOrigFound = 0;
            int fNewFound = 0;
            while ( pPinOut && !fOrigFound && !fNewFound )
            {
                MiMo_CellFanout_t * pFanout = pPinOut->pFanoutList;
                while(pFanout && (!fNewFound || !fOrigFound))
                {
                    if(pFanout->FanoutId == fanoutOrigNum)
                        fOrigFound = 1;
                    if(pFanout->FanoutId == fanoutNewNum)
                        fNewFound = 1;
                    pFanout = pFanout->pNext;
                }
                if ( !fOrigFound && !fNewFound)
                    pPinOut = pPinOut->pNext;
            }
            if ( fOrigFound && fNewFound )
            {
                Abc_Obj_t * pNodeNew = Abc_ObjInsertBetween( pDriver, pNode, ABC_OBJ_NODE);
                pNodeNew->pData = MiMo_CellCreate(((MiMo_Library_t*)(pNtk->pMiMoLib))->pGateBuf);
                MiMo_CellAddBufOut(pNodeNew->pData, 0);
            }
        }
        else
            pDriver->pCopy = pNode;
    }
}

/**Function*************************************************************

  Synopsis    [Adds a new fanin of a Abc_Obj at the given pin position
               and updates the MiMoCell pin reference accordingly.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cm_AbcObjAddSoFanin(Cm_Man_t *pCmMan, Abc_Obj_t * pAbcObj, Abc_Obj_t *pFanin, int outputPos)
{
    MiMo_Cell_t * pCell = pFanin->pData;
    int fanoutNum = Abc_ObjFanoutNum(pFanin);
    Abc_ObjAddFanin(pAbcObj, pFanin);
    if (pCell)
    {
        int d = pCell->pGate->Depth;
        return MiMo_CellAddPinOut(pCell, Cm_ManGetOutputPin(pCmMan, d, outputPos), fanoutNum);
    }
    return -1;
}

/**Function*************************************************************

  Synopsis    [Adds a new fanin of a Abc_Obj at the Main output and 
               updates the MiMoCell pin reference accordingly.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cm_AbcObjAddFanin(Cm_Man_t *pCmMan, Abc_Obj_t * pAbcObj, Abc_Obj_t *pFanin)
{
    return Cm_AbcObjAddSoFanin(pCmMan, pAbcObj, pFanin, 1);
}

/**Function*************************************************************

  Synopsis    [Creates a new MiMoCell from best cut of cm node]

  Description [The output is inverted iff fMoCompl == 1]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_Cell_t * Cm_ManBuildCellWithInputs(Cm_Man_t * pCmMan, Cm_Obj_t *pCmObj, int fMoCompl)
{
    Cm_Obj_t * pFaninCone[CM_MAX_FA_SIZE];
    int depth = pCmObj->BestCut.Depth;
    int minDepth = pCmMan->pPars->MinSoHeight;
    if ( pCmMan->pPars->fThreeInputGates )
    {
        // generate cone and config information from given leafs
        if (depth < minDepth) // can't map to AIC of depth smaller MinSoHeight
        {
            depth = minDepth;
            Cm_Fa3Clear(pFaninCone, depth);
        }
        pFaninCone[1] = pCmObj;
        Cm_Fa3BuildWithMaximumDepth(pFaninCone, depth);
        // mark as leaf and store faninId in iTemp
        // skip constants
        Cm_ObjClearMarkFa3(pFaninCone, depth, CM_MARK_LEAF);
        int faninId = 0;
        for(int i=0; i<pCmObj->BestCut.nFanins; i++)
        {
            Cm_Obj_t * pLeaf = pCmObj->BestCut.Leafs[i];
            pLeaf->fMark |= CM_MARK_LEAF;
            if ( pLeaf->Type != CM_CONST1 )
                pCmObj->BestCut.Leafs[i]->iTemp = faninId++;
        }
        Cm_Fa3ShiftDownLeafs(pFaninCone, depth);
        MiMo_Gate_t * pGate = pCmMan->pConeGates[depth];
        MiMo_Cell_t * pCell = (MiMo_Cell_t*)MiMo_CmCellFromFa(pGate, (void**)pFaninCone, fMoCompl);
        for(int i=Cm_Fa3LayerStart(depth); i<Cm_Fa3LayerStart(depth+1); i++)
        {
            int inputPinPos = i - 1;
            if (pFaninCone[i] && (pFaninCone[i]->fMark&CM_MARK_LEAF) && pFaninCone[i]->Type != CM_CONST1)
                MiMo_CellAddPinIn(pCell, Cm_ManGetInputPin(pCmMan, inputPinPos), pFaninCone[i]->iTemp);
        }
        return pCell; 
    }
    // generate cone and config information from given leafs
    if (depth < minDepth) // can't map to AIC of depth smaller MinSoHeight
    {
        depth = minDepth;
        Cm_FaClear(pFaninCone, depth);
    }
    pFaninCone[1] = pCmObj;
    Cm_FaBuildWithMaximumDepth(pFaninCone, depth);
    // mark as leaf and store faninId in iTemp
    Cm_ObjClearMarkFa(pFaninCone, depth, CM_MARK_LEAF);
    for(int i=0; i<pCmObj->BestCut.nFanins; i++)
    {
        pCmObj->BestCut.Leafs[i]->fMark |= CM_MARK_LEAF;
        pCmObj->BestCut.Leafs[i]->iTemp = i;
    }
    Cm_FaShiftDownLeafs(pFaninCone, depth);
    MiMo_Gate_t * pGate = pCmMan->pConeGates[depth];
    MiMo_Cell_t * pCell = (MiMo_Cell_t*)MiMo_CmCellFromFa(pGate, (void**)pFaninCone, fMoCompl);

    for(int i=(1<<depth); i<(2<<depth); i++)
    {
        if (pFaninCone[i] && (pFaninCone[i]->fMark&CM_MARK_LEAF))
            MiMo_CellAddPinIn(pCell, Cm_ManGetInputPin(pCmMan, i), pFaninCone[i]->iTemp);
    }
    return pCell;
}

/**Function*************************************************************

  Synopsis    [Inverts the main output of a cell.]

  Description [Toggles the configuration bits the successive fanout cells
               for all inverted output signals]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_CmInvertMo(Cm_Man_t * pCmMan, Cm_Obj_t * pCmObj)
{
    Abc_Obj_t * pNode = pCmObj->pCopy;
    MiMo_Cell_t * pCell = pNode->pData;
    MiMo_CmInvertMo(pCell);
    MiMo_PinOut_t * pMoPinOut = Cm_ManGetOutputPin(pCmMan, pCmObj->BestCut.Depth, 1);
    MiMo_CellPinOut_t * pPinOut = pCell->pPinOutList;
    while ( pPinOut )
    {
        if ( !MiMo_CmIsClassNN(pCell) && pPinOut->pPinOut != pMoPinOut)
        {
            pPinOut = pPinOut->pNext;
            continue;
        }
        MiMo_CellFanout_t * pCellFanout = pPinOut->pFanoutList;
        while ( pCellFanout )
        {
            Abc_Obj_t * pFanout = Abc_ObjFanout(pNode, pCellFanout->FanoutId);
            MiMo_Cell_t * pFanoutCell = pFanout->pData;
            if ( pFanoutCell )
            {
                int d = pFanoutCell->pGate->Depth;
                MiMo_CellPinIn_t * pFanoutIn = pFanoutCell->pPinInList;
                while ( pFanoutIn )
                {
                    if ( Abc_ObjFanin(pFanout, pFanoutIn->FaninId) == pNode
                         && pFanoutIn->FaninFanoutNetId == pPinOut->FanoutNetId )
                    {
                        if ( pCmMan->pPars->fThreeInputGates )
                            printf("Adapt Abc_CmInvertMo!\n");
                        int configPos = (1<<d) + pFanoutIn->pPinIn->Id;
                        Vec_BitInvertEntry(pFanoutCell->vBitConfig, configPos);
                    }
                    pFanoutIn = pFanoutIn->pNext;
                }
            }
            pCellFanout = pCellFanout->pNext;
        }
        pPinOut = pPinOut->pNext;
    }
}

/**Function*************************************************************

  Synopsis    [Recursively creates Abc_Obj's with MiMo_Cells from cm graph]

  Description [The phase of the mainoutput is positive, negative or 
               don't care (dc). For dc the positive phase is implemented.
               The phase of an main output is inverted if benefitial.
               Meaning of marks:
                 - A == 1: phase is fixed
                 - B fcompl]
  SideEffects []

  SeeAlso     []

***********************************************************************/
typedef enum { CM_POSITIVE_MO, CM_NEGATIVE_MO, CM_DC_MO } Cm_AbcMainoutPhase_t;
Abc_Obj_t * Abc_NodeFromCm_rec( Abc_Ntk_t * pNtkNew, Cm_Man_t * pCmMan, Cm_Obj_t * pCmObj, Cm_AbcMainoutPhase_t moPhase)
{
    int fMoCompl = (moPhase == CM_NEGATIVE_MO) ? 1 : 0;
    // return node if already implemented and MO phase is matchable
    if ( pCmObj->pCopy )
    {
        Abc_Obj_t * pObj = pCmObj->pCopy;
        if ( moPhase == CM_DC_MO )
            return pObj;
        if (pObj->fMarkB == fMoCompl)
        {
            pObj->fMarkA = 1;
            return pObj;
        }
        if ( !pObj->fMarkA )
        {
            Abc_CmInvertMo(pCmMan, pCmObj);
            pObj->fMarkB ^= 1; // invert phase
            pObj->fMarkA = 1;
            return pObj;
        }
        // second phase is already implemented.
        if ( pObj->pCopy )
            return pObj->pCopy;
        else
            moPhase = fMoCompl ? CM_NEGATIVE_MO : CM_POSITIVE_MO;
    }

    Abc_Obj_t * pNodeNew = Abc_NtkCreateNode ( pNtkNew );
    pNodeNew->pCopy = NULL;
    Cm_Obj_t * pCmRep = Cm_ObjGetRepr(pCmObj);
    MiMo_Cell_t * pCell = Cm_ManBuildCellWithInputs( pCmMan, pCmRep, fMoCompl );
    pNodeNew->pData = pCell;
    pNodeNew->fMarkA = ((moPhase == CM_POSITIVE_MO) || (moPhase == CM_NEGATIVE_MO)) ?  1 : 0;
    pNodeNew->fMarkB = fMoCompl;
    for(int i=0; i<pCmRep->BestCut.nFanins; i++)
    {
        Cm_Obj_t * pLeaf = pCmRep->BestCut.Leafs[i];
        // skip constant inputs
        if ( pLeaf->Type == CM_CONST1)
            continue;
        // create and add nodes for side and main outputs
        if ( pLeaf->BestCut.SoOfCutAt )
        {
            Abc_Obj_t * pFanin = Abc_NodeFromCm_rec( pNtkNew, pCmMan, pLeaf->BestCut.SoOfCutAt, CM_DC_MO );
            int netId = Cm_AbcObjAddSoFanin(pCmMan, pNodeNew, pFanin, pLeaf->BestCut.SoPos );
            MiMo_CellSetPinInNet(pCell, i, netId);
            if (pFanin->pData && MiMo_CmSoInverted(pFanin->pData, pLeaf->BestCut.SoPos) ) 
                MiMo_CmInvertInput(pNodeNew->pData, i);
        }
        else 
        {
            Abc_Obj_t * pFanin = Abc_NodeFromCm_rec( pNtkNew, pCmMan, pLeaf, CM_DC_MO );
            int netId = Cm_AbcObjAddFanin(pCmMan, pNodeNew, pFanin);
            MiMo_CellSetPinInNet(pCell, i, netId);
            if ( pFanin->pData && MiMo_CmMoInverted(pFanin->pData) )
                MiMo_CmInvertInput(pNodeNew->pData, i);
        } 
    }
    if ( pCmObj->pCopy )
        ((Abc_Obj_t*)pCmObj->pCopy)->pCopy = pNodeNew;
    else
        pCmObj->pCopy = pNodeNew;
    return pNodeNew;
}


/**Function*************************************************************

  Synopsis    [Creates all the predecessor Abc nodes for the cm node]

  Description []

  SideEffects []

  SeeAlso     [Abc_NodeFromCm_rec]

***********************************************************************/
static inline MiMo_Cell_t * Cm_ManCreateInvertingConeCell(Cm_Man_t * pCmMan) {  return MiMo_CmCreateInvertingCell(pCmMan->pConeGates[pCmMan->pPars->MinSoHeight]); }
Abc_Obj_t * Abc_PhaseNodeFromCm(Abc_Ntk_t * pNtkNew, Cm_Man_t * pCmMan, Cm_Obj_t * pCmObj, int fCompl)
{
    MiMo_Library_t * pLib = pNtkNew->pMiMoLib;
    if ( pCmObj->Type == CM_CI)
    {
        if ( fCompl )
        {
            Abc_Obj_t * pAbc = pCmObj->pCopy;
            if ( pAbc->pCopy )
                return pAbc->pCopy;
            Abc_Obj_t * pNodeNew = Abc_NtkCreateNode ( pNtkNew );
            pNodeNew->pData = Cm_ManCreateInvertingConeCell( pCmMan );
            Cm_AbcObjAddFanin(pCmMan, pNodeNew, pAbc );
            pAbc->pCopy = pNodeNew;
            return pNodeNew;
        }
        else
        {
            Abc_Obj_t * pNodeNew = Abc_NtkCreateNode( pNtkNew );
            Abc_ObjAddFanin( pNodeNew, pCmObj->pCopy);
            pNodeNew->pData = MiMo_CellCreate(pLib->pGateBuf);
            MiMo_CellAddBufOut(pNodeNew->pData, 0);
            return pNodeNew;
        }
    }
    if ( pCmObj->Type == CM_CONST1 )
    {
        Abc_Obj_t * pNode;
        if (fCompl)
            pNode = Abc_NtkCreateNodeConst0(pNtkNew);
        else
            pNode = Abc_NtkCreateNodeConst1(pNtkNew);
        MiMo_CellAddConstOut(pNode->pData, Abc_ObjFanoutNum(pNode) );
        return pNode;
    }
    if ( pCmObj->BestCut.SoOfCutAt )
    {
        Abc_Obj_t * pAbcRoot = pCmObj->BestCut.SoOfCutAt->pCopy;
        if ( pAbcRoot )
        {
            int fSoInverted = MiMo_CmSoInverted(pAbcRoot->pData, pCmObj->BestCut.SoPos);
            if ( MiMo_CmIsClassNN(pAbcRoot->pData) )
            {
                if ( !pAbcRoot->fMarkA || fCompl == fSoInverted )
                {
                    fCompl ^= MiMo_CmMoSoInverted(pAbcRoot->pData, pCmObj->BestCut.SoPos);
                    return Abc_NodeFromCm_rec(pNtkNew, pCmMan, pCmObj->BestCut.SoOfCutAt,
                                               fCompl ? CM_NEGATIVE_MO : CM_POSITIVE_MO);
                }
                pCmObj->BestCut.SoOfCutAt = NULL;
            }
            else
            {
                if ( fSoInverted == fCompl )
                    return pAbcRoot;
                pCmObj->BestCut.SoOfCutAt = NULL;
            }
        }
        else
        {
            // to avoid the generation of extra nodes, probably something like the following line might be usefull: 
            // return Abc_NodeFromCm_rec(pNtkNew, pCmMan, pCmObj->BestCut.SoOfCutAt, fCompl ? CM_NEGATIVE_MO : CM_POSITIVE_MO);
            pCmObj->BestCut.SoOfCutAt = NULL;
        }
    }
    return Abc_NodeFromCm_rec(pNtkNew, pCmMan, pCmObj, fCompl ? CM_NEGATIVE_MO : CM_POSITIVE_MO );
}

/**Function*************************************************************

  Synopsis    [Interface with the cone mapping package.]

  Description [Generates a new Netlist with from output of cone mapping
               manager ]

  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFromCm( Cm_Man_t * pCmMan, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i;
    // clean copy data
    Cm_Obj_t *pCmObj;
    Cm_ManForEachObj(pCmMan, pCmObj, i)
       pCmObj->pCopy = NULL;
    Abc_Ntk_t * pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_MAP_MO );
    pNtkNew->pMiMoLib = pCmMan->pPars->pMiMoLib;
    Abc_NtkForEachCi ( pNtk, pNode, i )
    {
        Abc_Obj_t *pNodeNew = pNode->pCopy;
        Cm_ManCi(pCmMan, i)->pCopy = pNodeNew;
        Cm_ManCi(pCmMan, i)->fMark |= CM_MARK_VALID;
        pNodeNew->fMarkA = 0;
        pNodeNew->fMarkB = 0;
        pNodeNew->pData = NULL;
    }
    printf("%d Cis created\n", Cm_ManCiNum(pCmMan));
    // create network structure
    Abc_NtkForEachCo(pNtk, pNode, i)
    {
        Abc_Obj_t *pFanin = Abc_ObjFanin0(pNode);
        // directly connect CI/CO pair with identical names
        if ( Abc_ObjIsCi(pFanin) && !strcmp( Abc_ObjName(pFanin), Abc_ObjName(pNode)) )
        {
            Abc_ObjAddFanin(pNode->pCopy, pFanin->pCopy);
            continue;
        }

        Cm_ManCo(pCmMan, i)->pCopy = pNode->pCopy;
        pCmObj = Cm_ManCo(pCmMan, i)->pFanin0;
        int fCompl = Cm_ManCo(pCmMan, i)->fCompl0;
        Abc_Obj_t * pNodeNew = Abc_PhaseNodeFromCm( pNtkNew, pCmMan, pCmObj, fCompl );
        if ( MiMo_GateIsSpecial(((MiMo_Cell_t*)pNodeNew->pData)->pGate) )
            Abc_ObjAddFanin(pNode->pCopy, pNodeNew);
        else
        {
            Cm_AbcObjAddSoFanin(pCmMan, pNode->pCopy, pNodeNew, pCmObj->BestCut.SoOfCutAt ? pCmObj->BestCut.SoPos : 1 );
        }
    }
    printf("Cos created\n");
    Cm_MakeSimpleCos(pNtkNew);
    // reset temporary markings
    Abc_NtkForEachObj(pNtkNew, pNode, i)
    {
        pNode->fMarkA = 0;
        pNode->fMarkB = 0;
    }
    return pNtkNew;
}
