/**CFile****************************************************************

  FileName    [cmMiMo.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cone mapping]

  Synopsis    [Addtitional functionality for miMo library]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: cmMiMo.h,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/

#include "cmMiMo.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                 FUNCTIONS FOR LIBRARY PARSING                    ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the cone gates of pLib into ppGates,from minDepth to 
               maxDepth, with fixed expected names.
               Internal gates are assumed to have exactly 2 inputs.]

  Description [Returns 1 on success.]
               
  SideEffects [Updates the depth and gateCount of the gates.]

  SeeAlso     []

***********************************************************************/
int Cm_Cone2ReadOrderedConeGates(MiMo_Library_t *pLib, MiMo_Gate_t **ppGates, int minDepth, int maxDepth)
{
    MiMo_Gate_t * pGate = NULL;
    int i;
    char coneStr [20];
    for(int coneDepth=minDepth; coneDepth<=maxDepth; coneDepth++)
    {
        sprintf(coneStr, "cone_%d", coneDepth);
        // for now using slow direct string comparison search
        int fFound = 0;
        MiMo_LibForEachGate(pLib, pGate, i)
            if (!strcmp(coneStr, pGate->pName))
            {
                ppGates[coneDepth] = pGate;
                pGate->Depth = coneDepth;
                pGate->GateCount = (1<<coneDepth) - 1;
                fFound = 1;
            }
        if (!fFound){
            printf("%s not found in current MiMoLibrary\n", coneStr);
            return 0;
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Reads input pins of previously read gates in specific order]

  Description []
               
  SideEffects []

  SeeAlso     [Cm_Cone2ReadOrderedConeGates]

***********************************************************************/
Vec_Ptr_t * Cm_Cone2ReadOrderedConeInputPins(MiMo_Gate_t ** ppGates, int minDepth, int maxDepth)
{
    // add structural check instead of comparing to fixed strings...
    Vec_Ptr_t * pInVec = Vec_PtrAlloc(2<<maxDepth);
    for(int d=minDepth; d<=maxDepth; d++)
    {
        int startPos = (1<<d);
        int pinPos = 0;
        int i;
        MiMo_PinIn_t *pPinIn;
        char expectedPinName [32];
        MiMo_GateForEachPinIn(ppGates[d], pPinIn, i)
        {
            sprintf(expectedPinName, "in%d[%d]", pinPos/4, pinPos%4); 
            if ( !strcmp(expectedPinName, pPinIn->pName) )
            {
                Vec_PtrSetEntry(pInVec, startPos + pinPos, pPinIn);
                pinPos++;
            } else {
                printf("Expected input name %s, but got %s in gate %s while parsing MiMoLibrary\n",
                        expectedPinName, pPinIn->pName, ppGates[d]->pName);
                Vec_PtrFree(pInVec);
                return NULL;
            }
        }
    }
    return pInVec;
}

/**Function*************************************************************

  Synopsis    [Reads output pins of previously read gates in specific order]

  Description []
               
  SideEffects [Sets the position of the output pins.]

  SeeAlso     [Cm_Cone2ReadOrderedConeGates]

***********************************************************************/
Vec_Ptr_t * Cm_Cone2ReadOrderedConeOutputPins(MiMo_Gate_t **ppGates, int minDepth, int maxDepth)
{
    // add structural check instead of comparing to fixed strings...
    Vec_Ptr_t * pOutVec = Vec_PtrAlloc(1<<maxDepth);
    for(int d=minDepth; d<=maxDepth; d++)
    {
        int startPos = (1<<(d-1));
        int i;
        MiMo_PinOut_t *pPinOut;
        char expectedPinName[32];
        int pinOutNumber = 0;
        int layer = d-2;
        int pinLayerNumber = 0;
        MiMo_GateForEachPinOut(ppGates[d], pPinOut, i)
        {
            sprintf(expectedPinName, "out%d", pinOutNumber);
            if ( !strcmp(expectedPinName, pPinOut->pName) )
            {
                // push pin in correct place as it is seen in bfs order
                int predArrayPos = (1<<layer) + pinLayerNumber;
                pPinOut->Pos = predArrayPos;
                Vec_PtrSetEntry(pOutVec, startPos + predArrayPos, pPinOut );
            }
            else
            {
                printf("Expected output name %s, but got %s in gate %s while parsing MiMoLibrary\n",
                        expectedPinName, pPinOut->pName, ppGates[d]->pName);
                Vec_PtrFree(pOutVec);
                return NULL;
            }

            pinOutNumber++;
            pinLayerNumber++;
            if ( pinLayerNumber == (1<<layer) )
            {
                layer--;
                pinLayerNumber = 0;
            }
        }
    }
    return pOutVec;
}

/**Function*************************************************************

  Synopsis    [Reads the cone gates of pLib into ppGates,from minDepth to 
               maxDepth, with fixed expected names.
               Internal gates are assumed to have exactly 3 inputs.]

  Description [Returns 1 on success.]
               
  SideEffects [Updates the depth and gateCount of the gates.]

  SeeAlso     []

***********************************************************************/
int Cm_Cone3ReadOrderedConeGates(MiMo_Library_t *pLib, MiMo_Gate_t **ppGates, int minDepth, int maxDepth)
{
    MiMo_Gate_t * pGate = NULL;
    int i;
    char coneStr [20];
    for(int coneDepth=minDepth; coneDepth<=maxDepth; coneDepth++)
    {
        sprintf(coneStr, "cone_%d", coneDepth);
        // for now using slow direct string comparison search
        int fFound = 0;
        MiMo_LibForEachGate(pLib, pGate, i)
            if (!strcmp(coneStr, pGate->pName))
            {
                ppGates[coneDepth] = pGate;
                pGate->Depth = coneDepth;
                pGate->GateCount = (Cm_Pow3(coneDepth) - 1 ) / 2;
                fFound = 1;
            }
        if (!fFound)
        {
            printf("%s not found in current MiMoLibrary\n", coneStr);
            return 0;
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Reads input pins of previously read gates in specific order]

  Description []
               
  SideEffects []

  SeeAlso     [Cm_Cone3ReadOrderedConeGates]

***********************************************************************/
Vec_Ptr_t * Cm_Cone3ReadOrderedConeInputPins(MiMo_Gate_t ** ppGates, int minDepth, int maxDepth)
{
    // add structural check instead of comparing to fixed strings...
    Vec_Ptr_t * pInVec = Vec_PtrAlloc(Cm_Fa3Size(maxDepth+1));
    for(int d=minDepth; d<=maxDepth; d++)
    {
        int startPos = Cm_Fa3Size(d);
        int pinPos = 0;
        int i;
        MiMo_PinIn_t *pPinIn;
        char expectedPinName [32];
        MiMo_GateForEachPinIn(ppGates[d], pPinIn, i)
        {
            sprintf(expectedPinName, "in%d[%d]", pinPos/3, pinPos%3); 
            if ( !strcmp(expectedPinName, pPinIn->pName) )
            {
                Vec_PtrSetEntry(pInVec, startPos + pinPos, pPinIn);
                pinPos++;
            } else {
                printf("Expected input name %s, but got %s in gate %s while parsing MiMoLibrary\n",
                        expectedPinName, pPinIn->pName, ppGates[d]->pName);
                Vec_PtrFree(pInVec);
                return NULL;
            }
        }
    }
    return pInVec;
}

/**Function*************************************************************

  Synopsis    [Reads output pins of previously read gates in specific order]

  Description []
               
  SideEffects [Sets the position of the output pins.]

  SeeAlso     [Cm_Cone3ReadOrderedConeGates]

***********************************************************************/
Vec_Ptr_t * Cm_Cone3ReadOrderedConeOutputPins(MiMo_Gate_t **ppGates, int minDepth, int maxDepth)
{
    // add structural check instead of comparing to fixed strings...
    Vec_Ptr_t * pOutVec = Vec_PtrAlloc(Cm_Fa3Size(maxDepth));
    for(int d=minDepth; d<=maxDepth; d++)
    {
        int startPos = Cm_Fa3OutPinStartPos(d);
        int i;
        MiMo_PinOut_t *pPinOut;
        char expectedPinName[32];
        int pinOutNumber = 0;
        int layer = d-1;
        int pinLayerNumber = 0;
        MiMo_GateForEachPinOut(ppGates[d], pPinOut, i)
        {
            sprintf(expectedPinName, "out%d", pinOutNumber);
            if ( !strcmp(expectedPinName, pPinOut->pName) )
            {
                // push pin in correct place as it is seen in bfs order
                int predArrayPos = Cm_Fa3LayerStart(layer) + pinLayerNumber;
                pPinOut->Pos = predArrayPos;
                Vec_PtrSetEntry(pOutVec, startPos + predArrayPos, pPinOut );
            }
            else
            {
                printf("Expected output name %s, but got %s in gate %s while parsing MiMoLibrary\n",
                        expectedPinName, pPinOut->pName, ppGates[d]->pName);
                Vec_PtrFree(pOutVec);
                return NULL;
            }

            pinOutNumber++;
            pinLayerNumber++;
            if ( pinLayerNumber == Cm_Pow3(layer) )
            {
                layer--;
                pinLayerNumber = 0;
            }
        }
    }
    return pOutVec;
}

////////////////////////////////////////////////////////////////////////
///                        GENERIC FUNCTIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Calculates the faninIds of the cells according to the gate
               input pins]

  Description [Its a kind of bucket sort to map the cell inputs to the
               gate inputs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_CmCalcFaninIdArray(MiMo_Cell_t *pCell, int *pFaninId, int size)
{
    for(int i=0; i<size; i++)
        pFaninId[i] = -1;
    MiMo_CellPinIn_t * pCellPinIn = pCell->pPinInList;
    while ( pCellPinIn )
    {
        MiMo_PinIn_t * pPinIn = pCellPinIn->pPinIn;
        pFaninId[pPinIn->Id] = pCellPinIn->FaninId;
        pCellPinIn = pCellPinIn->pNext;
    }
}

/**Function*************************************************************

  Synopsis    [Creates for an cone of 2 input gates the AIG input layer
               nodes according to the configuration]

  Description [startPos and endPos are used to create only the required
               range of the input node layer (usefull for side outputs)]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_CmCreateInputLayer2(MiMo_Cell_t * pCell, Hop_Man_t *p, Hop_Obj_t ** pLayer, int startPos, int endPos)
{
    int d = pCell->pGate->Depth;
    int pFaninId[CM_MAX_NLEAFS];
    MiMo_CmCalcFaninIdArray(pCell, pFaninId, (1<<d));
    // create bottom layer with required inversions
    for(int i=startPos; i<endPos; i++)
    {
        int inId0 = pFaninId[2*i - (1<<d)];
        int inId1 = pFaninId[2*i+1 - (1<<d)];
        pLayer[2*i] = inId0 < 0 ? Hop_ManConst0( p ) : Hop_IthVar ( p, inId0 );
        if ( Vec_BitEntry(pCell->vBitConfig, 2*i ) )
            pLayer[2*i] = Hop_Not( pLayer[2*i] );
        pLayer[2*i+1] = inId1 < 0 ? Hop_ManConst0( p ) : Hop_IthVar ( p, inId1 );
        if ( Vec_BitEntry(pCell->vBitConfig, 2*i+1 ) )
            pLayer[2*i+1] = Hop_Not(pLayer[2*i+1] );
    }
}

/**Function*************************************************************

  Synopsis    [Creates for an cone of 3 input gates the AIG input layer
               nodes according to the configuration]

  Description [startPos and endPos are used to create only the required
               range of the input node layer (usefull for side outputs)]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_CmCreateInputLayer3(MiMo_Cell_t * pCell, Hop_Man_t *p, Hop_Obj_t ** pLayer, int startPos, int endPos)
{
    int d = pCell->pGate->Depth;
    int pFaninId[Cm_Pow3(d+1)];
    MiMo_CmCalcFaninIdArray(pCell, pFaninId, Cm_Pow3(d+1));
    int layerStart = Cm_Fa3LayerStart(d);
    // create bottom layer with required inversions
    for(int i=startPos; i<endPos; i++)
    {
        for(int k=-1; k<=1; k++)
        {
            int inId = pFaninId[3*i+k - layerStart];
            pLayer[3*i+k] = inId < 0 ? Hop_ManConst0( p ) : Hop_IthVar ( p, inId );
            if ( Vec_BitEntry(pCell->vBitConfig, 3*i+k ) )
                pLayer[3*i+k] = Hop_Not( pLayer[3*i+k] );
        }
    }
}


/**Function*************************************************************

  Synopsis    [Returns AIG for special cells (constants and buffers)]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * MiMo_SpecialToAig( MiMo_Cell_t * pCell, Hop_Man_t * p)
{
    MiMo_Gate_t * pGate = pCell->pGate;
    MiMo_Library_t * pLib = pGate->pMiMoLib;
    if ( pGate == pLib->pGate0)
        return Hop_ManConst0( p );
    if ( pGate == pLib->pGate1)
        return Hop_ManConst1( p );
    if ( pGate == pLib->pGateBuf )
        return Hop_IthVar(p, 0 );
    assert(0);
    return NULL;
}

////////////////////////////////////////////////////////////////////////
///                         AIC2 FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////
/*
 *  Meaning of config bits:
 *    0 -> select direct signal
 *    1 -> select inverted signal
 *    same meaning on input layer; if not connected assume const0
 */ 


/**Function*************************************************************

  Synopsis    [Creates a new cell from the given FaninArray and gate.]

  Description [if fMoCompl, the output will be inverted.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_Cell_t * MiMo_CmCellFromFaAIC2( MiMo_Gate_t * pGate, Cm_Obj_t ** pFa, int fMoCompl )
{
    MiMo_Cell_t * pCell = MiMo_CellCreate( pGate );
    int d = pGate->Depth;
    Vec_Bit_t * vConfig = Vec_BitStart((2<<d));
    Vec_BitWriteEntry(vConfig, 1, fMoCompl);
    for(int i=1; i<(1<<(d-1)); i++)
    {
        Vec_BitWriteEntry(vConfig, 2*i, pFa[i] ? pFa[i]->fCompl0 : 0 );
        Vec_BitWriteEntry(vConfig, 2*i+1, pFa[i] ? pFa[i]->fCompl1 : 0 );
    }
    for(int i=(1<<(d-1)); i<(1<<d); i++)
    {
        // on input layer assume implicit connection to const0
        int fBit0 = pFa[i] ? pFa[i]->fCompl0 : 0; 
        int fBit1 = pFa[i] ? pFa[i]->fCompl1 : 0;
        Vec_BitWriteEntry(vConfig, 2*i, pFa[2*i] ? fBit0 : 1 ^ fBit0 );
        Vec_BitWriteEntry(vConfig, 2*i+1, pFa[2*i+1] ? fBit1 : 1 ^ fBit1 );
    }
    pCell->vBitConfig = vConfig;
    return pCell;
}

/**Function*************************************************************

  Synopsis    [Creates an cell out of the given gate, which inverts the
               input signal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_Cell_t * MiMo_CmCreateInvertingCellAIC2( MiMo_Gate_t * pGate )
{
    int d = pGate->Depth;
    MiMo_Cell_t * pCell = MiMo_CellCreate( pGate );
    MiMo_CellAddPinIn( pCell, MiMo_PinInAt(pGate, 0), 0); // first pin with id = 0
    Vec_Bit_t * vConfig = Vec_BitStart((2<<d)); // all config bits are zero
    // invert all input pins ( unconnected are const0 )
    for(int i=(1<<d);i<(2<<d); i++)
        Vec_BitSetEntry(vConfig, i, 1);
    pCell->vBitConfig = vConfig;
    return pCell;
} 

/**Function*************************************************************

  Synopsis    [Inverts the main output]

  Description []

  SideEffects [This may also invert all side outputs]

  SeeAlso     [MiMo_CmIsClassNN]

***********************************************************************/
void MiMo_CmInvertMoAIC2( MiMo_Cell_t * pCell )
{
    if ( pCell->vBitConfig )
        Vec_BitInvertEntry( pCell->vBitConfig, 1 );
}

/**Function*************************************************************

  Synopsis    [Returns 1 iff main output is inverted]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_CmMoInvertedAIC2( MiMo_Cell_t * pCell )
{
    return Vec_BitEntry(pCell->vBitConfig, 1);
}

/**Function*************************************************************

  Synopsis    [Returns 1 iff side output is inverted]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_CmSoInvertedAIC2( MiMo_Cell_t * pCell, int soPos )
{
    return Vec_BitEntry(pCell->vBitConfig, soPos);
}

/**Function*************************************************************

  Synopsis    [Updates the cell configuration to account for an input
               signal inversion at faninId.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_CmInvertInputAIC2( MiMo_Cell_t * pCell, int faninId )
{
    MiMo_CellPinIn_t * pPinIn = pCell->pPinInList;
    while (pPinIn)
    {
        if (pPinIn->FaninId == faninId )
        {
            int pinId = pPinIn->pPinIn->Id;
            int inputLayerPos = (1<< pCell->pGate->Depth);;
            Vec_BitInvertEntry(pCell->vBitConfig, pinId + inputLayerPos);
        }
        pPinIn = pPinIn->pNext;
    } 
}

/**Function*************************************************************

  Synopsis    [Creates an functional equivalent AIG from the cell]

  Description [Only one output is currently supported]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * MiMo_CmToAigAIC2( MiMo_Cell_t * pCell, Hop_Man_t * p, MiMo_PinOut_t * pPinOut )
{
    const int d = pCell->pGate->Depth;
    int faninPos = pPinOut->Pos;
    Hop_Obj_t * pObjLayer[(2<<d)];
    int layerStart = faninPos;
    int layerSize = 1;
    while(layerStart < (1<<(d-1)) )
    {
        layerStart *= 2;
        layerSize *= 2;
    }
    MiMo_CmCreateInputLayer2(pCell, p, pObjLayer, layerStart, layerStart + layerSize);

    // create layers above
    while ( layerSize )
    {
        for(int i=layerStart; i<layerStart+layerSize; i++)
        {
            pObjLayer[i] = Hop_And( p, pObjLayer[2*i], pObjLayer[2*i+1] );
            if ( Vec_BitEntry(pCell->vBitConfig, i) )
                pObjLayer[i] = Hop_Not( pObjLayer[i] );
        }
        layerStart /= 2;
        layerSize /= 2;
    }
    return pObjLayer[faninPos];
}


////////////////////////////////////////////////////////////////////////
///                         AIC3 FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////
/*
 *  Meaning of config bits:
 *    0 -> select direct signal
 *    1 -> select inverted signal
 *    same meaning on input layer; if not connected assume const0
 */

/**Function*************************************************************

  Synopsis    [Creates a new cell from the given FaninArray and gate.]

  Description [if fMoCompl, the output will be inverted.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_Cell_t * MiMo_CmCellFromFaAIC3( MiMo_Gate_t * pGate, void ** pFa, int fMoCompl )
{
    MiMo_Cell_t * pCell = MiMo_CellCreate( pGate );
    static int msg = 0;
    if (!msg)
    {
        printf("AIC3 config generation is currently not implemented\n");
        msg = 1;
    }
 
    return pCell;
}
/**Function*************************************************************

  Synopsis    [Creates an cell out of the given gate, which inverts the
               input signal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_Cell_t * MiMo_CmCreateInvertingCellAIC3( MiMo_Gate_t * pGate )
{
    static int msg = 0;
    if (!msg)
    {
        printf("TODO: Implementd CreateInvertingCellAIC3\n");
        msg = 1;
    } 
    return NULL;
} 

/**Function*************************************************************

  Synopsis    [Inverts the main output]

  Description []

  SideEffects [This may also invert all side outputs]

  SeeAlso     [MiMo_CmIsClassNN]

***********************************************************************/
void MiMo_CmInvertMoAIC3( MiMo_Cell_t * pCell )
{
    if ( pCell->vBitConfig)
        Vec_BitInvertEntry( pCell->vBitConfig, 1 );
}

/**Function*************************************************************

  Synopsis    [Returns 1 iff main output is inverted]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_CmMoInvertedAIC3( MiMo_Cell_t * pCell )
{
    return Vec_BitEntry(pCell->vBitConfig, 1);
}

/**Function*************************************************************

  Synopsis    [Returns 1 iff side output is inverted]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_CmSoInvertedAIC3( MiMo_Cell_t * pCell, int soPos )
{
    return Vec_BitEntry(pCell->vBitConfig, soPos);
}

/**Function*************************************************************

  Synopsis    [Updates the cell configuration to account for an input
               signal inversion at faninId.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_CmInvertInputAIC3( MiMo_Cell_t * pCell, int faninId )
{
    static int msg = 0;
    if (!msg)
    {
        printf("TDOD: Implement CmInvertInputAIC3\n");
        msg = 1;
    }
}

/**Function*************************************************************

  Synopsis    [Creates an functional equivalent AIG from the cell]

  Description [Only one output is currently supported]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * MiMo_CmToAigAIC3( MiMo_Cell_t * pCell, Hop_Man_t * p, MiMo_PinOut_t * pPinOut )
{
    static int msg = 0;
    if (!msg)
    {
        printf("TODO: Implement MiMo_CmToAigAIC3\n");
        msg = 1;
    }
    return NULL;
}


////////////////////////////////////////////////////////////////////////
///                         NNC2 FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////
/*
 *  Meaning of config bits at gate layers
 *    0 -> select NOR
 *    1 -> select NAND
 *  Meaning on input layer
 *    0 -> direct signal
 *    1 -> inverted signal
 *  Assumes const0 for unconnected pins
 */ 


/**Function*************************************************************

  Synopsis    [Creates a new cell from the given FaninArray and gate.]

  Description [if fMoCompl, the output will be inverted.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_Cell_t * MiMo_CmCellFromFaNNC2( MiMo_Gate_t * pGate, Cm_Obj_t ** pFa, int fMoCompl )
{
    MiMo_Cell_t * pCell = MiMo_CellCreate( pGate );
    const int d = pGate->Depth;
    Vec_Bit_t * vConfig = Vec_BitStart((2<<d));
    int fInv[(2<<d)];
    fInv[1] = fMoCompl;
    for(int i=1; i<(1<<d); i++)
    {
        Vec_BitWriteEntry(vConfig, i, fInv[i]);
        fInv[2*i] = 1 ^ fInv[i];
        fInv[2*i+1] = 1 ^ fInv[i];
        if ( pFa[i] )
        {
            fInv[2*i] ^= pFa[i]->fCompl0;
            fInv[2*i+1] ^= pFa[i]->fCompl1;
        }
    }
    for(int i=(1<<d); i<(2<<d); i++)
        Vec_BitWriteEntry(vConfig, i, pFa[i] ? fInv[i] : 1 ^ fInv[i] );
    pCell->vBitConfig = vConfig;
    return pCell;
}

/**Function*************************************************************

  Synopsis    [Creates an cell out of the given gate, which inverts the
               input signal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_Cell_t * MiMo_CmCreateInvertingCellNNC2( MiMo_Gate_t * pGate )
{
    int d = pGate->Depth;
    MiMo_Cell_t * pCell = MiMo_CellCreate( pGate );
    MiMo_CellAddPinIn( pCell, MiMo_PinInAt(pGate, 0), 0); // first pin with id = 0
    Vec_Bit_t * vConfig = Vec_BitStart( (2<<d) ); // all config bits are zero
    pCell->vBitConfig = vConfig; 
    return pCell;
} 

/**Function*************************************************************

  Synopsis    [Inverts the main output]

  Description []

  SideEffects [This may also invert all side outputs]

  SeeAlso     [MiMo_CmIsClassNN]

***********************************************************************/
void MiMo_CmInvertMoNNC2( MiMo_Cell_t * pCell )
{
    if ( pCell->vBitConfig)
        Vec_BitInvert( pCell->vBitConfig );
}

/**Function*************************************************************

  Synopsis    [Returns 1 iff main output is inverted]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_CmMoInvertedNNC2( MiMo_Cell_t * pCell )
{
    return Vec_BitEntry(pCell->vBitConfig, 1);
}

/**Function*************************************************************

  Synopsis    [Returns 1 iff side output is inverted]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_CmSoInvertedNNC2( MiMo_Cell_t * pCell, int soPos )
{
    return Vec_BitEntry(pCell->vBitConfig, soPos);
}

/**Function*************************************************************

  Synopsis    [Updates the cell configuration to account for an input
               signal inversion at faninId.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_CmInvertInputNNC2( MiMo_Cell_t * pCell, int faninId )
{
    MiMo_CellPinIn_t * pPinIn = pCell->pPinInList;
    while (pPinIn)
    {
        if (pPinIn->FaninId == faninId )
        {
            int pinId = pPinIn->pPinIn->Id;
            int inputLayerPos = (1<< pCell->pGate->Depth);;
            Vec_BitInvertEntry(pCell->vBitConfig, pinId + inputLayerPos);
        }
        pPinIn = pPinIn->pNext;
    } 
}

/**Function*************************************************************

  Synopsis    [Creates an functional equivalent AIG from the cell]

  Description [Only one output is currently supported]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * MiMo_CmToAigNNC2( MiMo_Cell_t * pCell, Hop_Man_t * p, MiMo_PinOut_t * pPinOut )
{
    const int d = pCell->pGate->Depth;
    int faninPos = pPinOut->Pos;
    Hop_Obj_t * pObjLayer[(2<<d)];
    int layerStart = faninPos;
    int layerSize = 1;
    while(layerStart < (1<<(d-1)) )
    {
        layerStart *= 2;
        layerSize *= 2;
    }
    MiMo_CmCreateInputLayer2(pCell, p, pObjLayer, layerStart, layerStart + layerSize);
    // create layers above
    while ( layerSize )
    {
        for(int i=layerStart; i<layerStart+layerSize; i++)
        {
            if ( Vec_BitEntry(pCell->vBitConfig, i) )
                pObjLayer[i] = Hop_Not ( Hop_And( p, pObjLayer[2*i], pObjLayer[2*i+1]) );
            else
                pObjLayer[i] = Hop_And ( p, Hop_Not(pObjLayer[2*i]), Hop_Not(pObjLayer[2*i+1]) );
        }
        layerStart /= 2;
        layerSize /= 2;
    }
    return pObjLayer[faninPos];
}

////////////////////////////////////////////////////////////////////////
///                         NNC3 FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////
/*
 *  Meaning of config bits at gate layers
 *    0 -> select NOR
 *    1 -> select NAND
 *  Meaning on input layer
 *    0 -> direct signal
 *    1 -> inverted signal ( also for unconnected const0)
 *  Assumes const0 for unconnected inputs 
 */ 


int Cm_Fa3FaninCompl(Cm_Obj_t ** pFa, int index)
{
    int pi = (index+1)/3;
    if ( pi < 1 )
        printf("Problematic for %d\n", pFa[1]->Id);
    if ( !pFa[pi] ) return 0;
    switch(index % 3)
    {
        case 0: return pFa[pi]->fCompl1; break;
        case 1: return pFa[pi]->fCompl2; break;
        case 2: return pFa[pi]->fCompl0; break;
        default: return 0;
    }
}

/**Function*************************************************************

  Synopsis    [Creates a new cell from the given FaninArray and gate.]

  Description [if fMoCompl, the output will be inverted.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_Cell_t * MiMo_CmCellFromFaNNC3( MiMo_Gate_t * pGate, Cm_Obj_t ** pFa, int fMoCompl )
{
    MiMo_Cell_t * pCell = MiMo_CellCreate( pGate );
    const int d = pGate->Depth;
    Vec_Bit_t * vConfig = Vec_BitStart(Cm_Fa3Size(d+1) + 1);
    int fInv[Cm_Fa3Size(d+1)];
    fInv[1] = fMoCompl;
    for(int i=1; i<Cm_Fa3LayerStart(d); i++)
    {
        if ( pFa[i] && pFa[i]->Type == CM_CONST1 && Cm_Fa3FaninCompl(pFa, i) )
            fInv[i] ^= 1;
        Vec_BitWriteEntry(vConfig, i, fInv[i]);
        fInv[3*i-1] = 1 ^ fInv[i];
        fInv[3*i] = 1 ^ fInv[i];
        fInv[3*i+1] =  1 ^ fInv[i];
        if ( pFa[i] )
        {
            fInv[3*i-1] ^= pFa[i]->fCompl0;
            fInv[3*i] ^= pFa[i]->fCompl1;
            fInv[3*i+1] ^= pFa[i]->fCompl2;
        }
    }
    for(int i=Cm_Fa3LayerStart(d); i<Cm_Fa3LayerStart(d+1); i++)
    {
        if ( ! pFa[i] ||  (pFa[i]->Type == CM_CONST1 && !Cm_Fa3FaninCompl(pFa, i)) )
            fInv[i] ^= 1;
        Vec_BitWriteEntry(vConfig, i, fInv[i] );
    }
    pCell->vBitConfig = vConfig;
    return pCell;
}

/**Function*************************************************************

  Synopsis    [Creates an cell out of the given gate, which inverts the
               input signal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_Cell_t * MiMo_CmCreateInvertingCellNNC3( MiMo_Gate_t * pGate )
{
    static int msg = 0;
    if (!msg)
    {
        printf("TODO: Implement MiMo_CmCreateInvertingCellNNC3\n");
        msg = 1;
    }
    return NULL;
} 

/**Function*************************************************************

  Synopsis    [Inverts the main output]

  Description []

  SideEffects [This may also invert all side outputs]

  SeeAlso     [MiMo_CmIsClassNN]

***********************************************************************/
void MiMo_CmInvertMoNNC3( MiMo_Cell_t * pCell )
{
    if ( pCell->vBitConfig)
        Vec_BitInvert( pCell->vBitConfig );
}

/**Function*************************************************************

  Synopsis    [Returns 1 iff main output is inverted]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_CmMoInvertedNNC3( MiMo_Cell_t * pCell )
{
    return Vec_BitEntry(pCell->vBitConfig, 1);
}

/**Function*************************************************************

  Synopsis    [Returns 1 iff side output is inverted]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_CmSoInvertedNNC3( MiMo_Cell_t * pCell, int soPos )
{
    return Vec_BitEntry(pCell->vBitConfig, soPos);
}

void MiMo_CmInvertInputNNC3( MiMo_Cell_t * pCell, int faninId )
{
    MiMo_CellPinIn_t * pPinIn = pCell->pPinInList;
    while (pPinIn)
    {
        if (pPinIn->FaninId == faninId )
        {
            int pinId = pPinIn->pPinIn->Id;
            int inputLayerPos = Cm_Fa3LayerStart(pCell->pGate->Depth);
            Vec_BitInvertEntry(pCell->vBitConfig, pinId + inputLayerPos);
        }
        pPinIn = pPinIn->pNext;
    }
}

/**Function*************************************************************

  Synopsis    [Creates an functional equivalent AIG from the cell]

  Description [Only one output is currently supported]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * MiMo_CmToAigNNC3( MiMo_Cell_t * pCell, Hop_Man_t * p, MiMo_PinOut_t * pPinOut )
{
    const int d = pCell->pGate->Depth;
    int faninPos = pPinOut->Pos;
    Hop_Obj_t * pObjLayer[Cm_Fa3Size(d+1) + 1];
    int layerStart = faninPos;
    int layerSize = 1;
    // go to lowest gate layer
    while(layerStart < Cm_Fa3LayerStart(d-1) )
    {
        layerStart = layerStart * 3 - 1;
        layerSize *= 3;
    }
    MiMo_CmCreateInputLayer3(pCell, p, pObjLayer, layerStart, layerStart + layerSize);
    // create layers above
    while ( layerSize )
    {
        for(int i=layerStart; i<layerStart+layerSize; i++)
        {
            if ( Vec_BitEntry(pCell->vBitConfig, i) )
                pObjLayer[i] = Hop_Not( Hop_And(p, pObjLayer[3*i-1], 
                                                Hop_And( p, pObjLayer[3*i], pObjLayer[3*i+1]) ));
            else
                pObjLayer[i] = Hop_And(p, Hop_Not(pObjLayer[3*i-1]),
                                       Hop_And( p, Hop_Not(pObjLayer[3*i]), Hop_Not(pObjLayer[3*i+1]) ));
        }
        layerStart = (layerStart + 1) / 3;
        layerSize /= 3;
    }
    return pObjLayer[faninPos];
}

////////////////////////////////////////////////////////////////////////
///                       WRAPPER FUNCTIONS                          ///
////////////////////////////////////////////////////////////////////////



/**Function*************************************************************

  Synopsis    [Creates a new cell from the given FaninArray and gate.]

  Description [if fMoCompl, the output will be inverted.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_Cell_t * MiMo_CmCellFromFa( MiMo_Gate_t * pGate, void **pFa, int fMoCompl)
{
    if ( pGate->Type == MIMO_AIC2 )
        return MiMo_CmCellFromFaAIC2( pGate, (Cm_Obj_t**)pFa, fMoCompl );
    if ( pGate->Type == MIMO_AIC3 )
        return MiMo_CmCellFromFaAIC3( pGate, pFa, fMoCompl );
    if ( pGate->Type == MIMO_NNC2 )
        return MiMo_CmCellFromFaNNC2( pGate, (Cm_Obj_t**)pFa, fMoCompl );
    if ( pGate->Type == MIMO_NNC3 )
        return MiMo_CmCellFromFaNNC3( pGate, (Cm_Obj_t**)pFa, fMoCompl );
    assert(0);
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Creates an cell out of the given gate, which inverts the
               input signal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_Cell_t * MiMo_CmCreateInvertingCell( MiMo_Gate_t * pGate )
{
    if ( pGate->Type == MIMO_AIC2 )
        return MiMo_CmCreateInvertingCellAIC2( pGate );
    if ( pGate->Type == MIMO_AIC3 )
        return MiMo_CmCreateInvertingCellAIC3( pGate );
    if ( pGate->Type == MIMO_NNC2 )
        return MiMo_CmCreateInvertingCellNNC2( pGate );
    if ( pGate->Type == MIMO_NNC3 )
        return MiMo_CmCreateInvertingCellNNC3( pGate );
    assert(0);
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Inverts the main output]

  Description []

  SideEffects [This may also invert all side outputs]

  SeeAlso     [MiMo_CmIsClassNN]

***********************************************************************/
void MiMo_CmInvertMo( MiMo_Cell_t * pCell)
{
    if ( pCell->pGate->Type == MIMO_AIC2 )
        return MiMo_CmInvertMoAIC2( pCell );
    if ( pCell->pGate->Type == MIMO_AIC3 )
        return MiMo_CmInvertMoAIC3( pCell );
    if ( pCell->pGate->Type == MIMO_NNC2 )
        return MiMo_CmInvertMoNNC2( pCell );
    if ( pCell->pGate->Type == MIMO_NNC3 )
        return MiMo_CmInvertMoNNC3( pCell );
    assert(0);
}

/**Function*************************************************************

  Synopsis    [Returns 1 iff main output is inverted]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_CmMoInverted(MiMo_Cell_t * pCell)
{
    if ( pCell->pGate->Type == MIMO_AIC2 )
        return MiMo_CmMoInvertedAIC2( pCell );
    if ( pCell->pGate->Type == MIMO_AIC3 )
        return MiMo_CmMoInvertedAIC3( pCell );
    if ( pCell->pGate->Type == MIMO_NNC2 )
        return MiMo_CmMoInvertedNNC2( pCell );
    if ( pCell->pGate->Type == MIMO_NNC3 )
        return MiMo_CmMoInvertedNNC3( pCell );
    if ( MiMo_GateIsConst1( pCell->pGate ) )
        return 0;
    if ( MiMo_GateIsConst0( pCell->pGate ) )
        return 1;
    assert(0);
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 iff side output is inverted]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_CmSoInverted( MiMo_Cell_t *pCell, int soPos )
{
    if ( pCell->pGate->Type == MIMO_AIC2 )
        return MiMo_CmSoInvertedAIC2( pCell, soPos );
    if ( pCell->pGate->Type == MIMO_AIC3 )
        return MiMo_CmSoInvertedAIC3( pCell, soPos );
    if ( pCell->pGate->Type == MIMO_NNC2 )
        return MiMo_CmSoInvertedNNC2( pCell, soPos );
    if ( pCell->pGate->Type == MIMO_NNC3 )
        return MiMo_CmSoInvertedNNC3( pCell, soPos );
    assert(0);
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 iff the main and side output have different
               inversion states.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_CmMoSoInverted( MiMo_Cell_t * pCell, int soPos)
{
    return MiMo_CmSoInverted(pCell, soPos) ^ MiMo_CmMoInverted(pCell);
}

/**Function*************************************************************

  Synopsis    [Updates the cell configuration to account for an input
               signal inversion at faninId.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_CmInvertInput( MiMo_Cell_t * pCell, int faninId)
{
    if ( pCell->pGate->Type == MIMO_AIC2 )
        return MiMo_CmInvertInputAIC2( pCell, faninId );
    if ( pCell->pGate->Type == MIMO_AIC3 )
        return MiMo_CmInvertInputAIC3( pCell, faninId );
    if ( pCell->pGate->Type == MIMO_NNC2 )
        return MiMo_CmInvertInputNNC2( pCell, faninId );
    if ( pCell->pGate->Type == MIMO_NNC3 )
        return MiMo_CmInvertInputNNC3( pCell, faninId );
    assert(0);
}

/**Function*************************************************************

  Synopsis    [Creates an functional equivalent AIG from the cell]

  Description [Only one output is currently supported]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * MiMo_CmToAig( MiMo_Cell_t * pCell, Hop_Man_t * p, MiMo_PinOut_t * pPinOut )
{
    if ( pCell->pGate->Type == MIMO_AIC2 )
        return MiMo_CmToAigAIC2( pCell, p, pPinOut );
    if ( pCell->pGate->Type == MIMO_NNC2 )
        return MiMo_CmToAigNNC2( pCell, p, pPinOut );
    if ( pCell->pGate->Type == MIMO_AIC3 )
        return MiMo_CmToAigAIC3( pCell, p, pPinOut );
    if ( pCell->pGate->Type == MIMO_NNC3 )
        return MiMo_CmToAigNNC3( pCell, p, pPinOut );
    if ( pCell->pGate->Type == MIMO_SPECIAL )
        return MiMo_SpecialToAig( pCell, p );
    assert(0);
    return NULL;
}

////////////////////////////////////////////////////////////////////////
///                  CLASSIFICATION FUNCTIONS                        ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Returns 1 iff the cell is of class NN ]

  Description [NN means inverting all config bits is identical to
               inverting all input and output signals.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_CmIsClassNN( MiMo_Cell_t * pCell)
{
    return pCell->pGate->Type == MIMO_NNC2 || pCell->pGate->Type == MIMO_NNC3;
}

ABC_NAMESPACE_IMPL_END

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
