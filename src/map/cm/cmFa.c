/**CFile****************************************************************

  FileName    [cmFa.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cone mapping]

  Synopsis    [Operations on depth-feasible fanin sets in linear
               memory layout]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: ifCore.c,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/

#include "cm.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Builds the maximum possible fanin array limited only
               by maxDepth.]

  Description [root must be in pNodes[1], returns latest leaf arrival time.
               The array is build like a binary tree in linear memory layout]
               
  SideEffects []

  SeeAlso     [Cm_FaBuildWithMaximumDepth]

***********************************************************************/
int Cm_FaBuildWithMaximumDepth(Cm_Obj_t **pFaninArray, int maxDepth)
{
    int cdepth;
    int hasConeOfDepth = 1;
    for( cdepth=0; cdepth<maxDepth && hasConeOfDepth; cdepth++ )
    {
        hasConeOfDepth = 0;
        for(int index=(1<<cdepth); index<(2<<cdepth); index++)
        {
            // check if node exists in (sub-)graph
            if( pFaninArray[index] )
            {
                // add fanins
                if ( pFaninArray[index]->Type == CM_AND )
                {
                    pFaninArray[2*index] = pFaninArray[index]->pFanin0;
                    pFaninArray[2*index+1] = pFaninArray[index]->pFanin1;
                    hasConeOfDepth = 1;
                } 
                else
                {
                    pFaninArray[2*index] = pFaninArray[2*index+1] = NULL;
                }
                assert(pFaninArray[index]->Type == CM_AND || pFaninArray[index]->Type == CM_CI);
            }
            else
            {
                pFaninArray[2*index] = pFaninArray[2*index+1] = NULL;
            }
        }
    }
    return cdepth -1 + hasConeOfDepth;
}

/**Function*************************************************************

  Synopsis    [Builds the maximum possible fanin array with best depth,
               i.e. which minimizes the arrival time of the node.]

  Description [node must be in pNodes[1], returns latest leaf arrival time]
               
  SideEffects []

  SeeAlso     [Cm_FaBuildWithMaximumDepth]

***********************************************************************/
float Cm_FaBuildDepthOptimal(Cm_Obj_t **pNodes, Cm_Par_t *pPars)
{
    float eps = pPars->Epsilon;
    float *AicDelay = pPars->AicDelay;
 
    Cm_Obj_t * pObj = pNodes[1];
    float bestArr = CM_FLOAT_LARGE;
    float latestArrival = -CM_FLOAT_LARGE;
    float latestLeafArrival = -CM_FLOAT_LARGE; 
    int bestDepth = 0;
    int depth = Cm_FaBuildWithMaximumDepth(pNodes, pPars->nConeDepth);
    // iterate through the cone depth levels, setup the
    // cone tree pointers and calculate the best delay
    for( int cdepth=1; cdepth<=depth; cdepth++ )
    {
        float latestArrivalOnLayer = -CM_FLOAT_LARGE;
        for(int index=(1<<cdepth); index<(2<<cdepth); index++)
            if( pNodes[index] ){
                float cArrival = pNodes[index]->BestCut.Arrival;
                latestArrivalOnLayer = CM_MAX(latestArrivalOnLayer, cArrival);
                if ( cdepth == depth || (!pNodes[2*index] && !pNodes[2*index+1]) )
                    latestLeafArrival = CM_MAX(latestLeafArrival, cArrival);
            }
        // actual delay is sum of cone delay and (if valid) arrival delay
        float latestArrivalAtDepth = CM_MAX(latestArrivalOnLayer, latestLeafArrival);
        float arrivalDelay = latestArrivalAtDepth + AicDelay[cdepth];
        if ( arrivalDelay + eps < bestArr ) 
        {
            bestDepth = cdepth;
            bestArr = arrivalDelay;
            latestArrival = latestArrivalAtDepth;
        }
    }
    pObj->BestCut.Arrival = bestArr;
    pObj->BestCut.Depth = bestDepth;
    return latestArrival;
}

ABC_NAMESPACE_IMPL_END
