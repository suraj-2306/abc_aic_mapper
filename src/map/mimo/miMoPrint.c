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
    printf("Library statistics:\n");
    printf("\tNumber of Gates: %d\n", Vec_PtrSize(pLib->pGates));
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
    int i, j;
    MiMo_Gate_t *pGate;
    MiMo_PinIn_t *pPinIn;
    MiMo_PinOut_t *pPinOut;
    MiMo_LibForEachGate(pLib, pGate, i)
    {
        printf("Gate: %s (Area %4.2f)\n", pGate->pName, pGate->Area);
        printf("Input pins:");
        MiMo_GateForEachPinIn(pGate, pPinIn, j)
            printf(" %s", pPinIn->pName);
        printf("\nOutput pins:");
        if(fVerbose)
            printf("\n");
        MiMo_GateForEachPinOut(pGate, pPinOut, j)
        {
            printf(" %s", pPinOut->pName);
            if (fVerbose )
            {
                printf(" [");
                MiMo_PinDelay_t * pPinDelay = pPinOut->pDelayList;
                while (pPinDelay)
                {
                    void * pPin = pPinDelay->pFromPin;
                    char * pName = pPinDelay->fFromPinOut ?
                          ((MiMo_PinOut_t*)pPin)->pName : ((MiMo_PinIn_t*)pPin)->pName;
                    printf("(%s, %5.2f) ", pName, pPinDelay->Delay);
                    pPinDelay = pPinDelay->pNext;
                }
                printf("]\n");
            }
        }
        printf("\n");
    }
}

ABC_NAMESPACE_IMPL_END
