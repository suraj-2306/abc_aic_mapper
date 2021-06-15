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

#include "cmMiMo.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the cone gates of pLib into ppGates,from minDepth to 
               maxDepth, with fixed expected names.
               Internal gates are assumed to have exactly 2 inputs.]

  Description [Returns 1 on success.]
               
  SideEffects [Updates the depth of the gates]

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

ABC_NAMESPACE_IMPL_END

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
