 /**CFile****************************************************************

  FileName    [cmArea.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cone mapping based on priority cuts.]

  Synopsis    [Functions to minimize the area of the mapping.]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: cmArea.c,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/


#include "cm.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Generates a cut of given depth by aiming to minimize the
               area flow using priority cuts. Returns the cut areaflow.]

  Description [The priority cuts are generated and merged bottom up from 
               the fanin array.]

  SideEffects [Only pNodes and the leafs of the cut are updated (nothing else).]

  SeeAlso     []

***********************************************************************/
float Cm_ManMinimizeCutAreaFlowPriority(Cm_Man_t *p, Cm_Obj_t **pNodes, float latestArrival, Cm_Cut_t * pCut)
{
    short depth = pCut->Depth;
    const int maxCutSize = p->pPars->MaxCutSize;
    const int maxNodeSize = (2<<depth);
    float eps = p->pPars->Epsilon;
    Cm_Obj_t * pCutNodes[maxNodeSize*maxCutSize];
    Cm_Obj_t * pTempNodes[maxNodeSize];
    Cm_Cut_t cuts[maxCutSize * maxCutSize];
    float afCuts[maxCutSize*maxCutSize];
    float af[maxNodeSize * maxCutSize];
    for(int i=0; i<maxNodeSize*maxCutSize; i++)
    {
        af[i] = -1;
        pCutNodes[i] = i<(2<<depth) ? pNodes[i] : NULL;
    }
    // initialize the lowest layer as trivial cuts
    for(int i=(1<<depth); i<(2<<depth); i++)
        if ( pNodes[i] )
            af[i] = pNodes[i]->BestCut.AreaFlow;
    int firstNonTrivialCutId = 0;
    for(int cdepth=depth-1; cdepth>=0; cdepth--)
    {
        for(int i=(1<<cdepth); i<(2<<cdepth); i++)
        {
            if ( !pNodes[i] )
                continue;  
            // calculate priority cuts based on fanin cuts
            pTempNodes[1] = pNodes[i];
            for(int r=0; r<maxCutSize*maxCutSize; r++)
                afCuts[r] = -1;
            for(int c0=0; c0<maxCutSize; c0++)
                for(int c1=0; c1<maxCutSize; c1++)
                {
                    int cIndex = c0*maxCutSize + c1;
                    if ( af[(2*i)+c0*maxNodeSize] < -0.5 || af[2*i+1+c1*maxNodeSize] < -0.5)
                    {
                        afCuts[cIndex] = -1;
                        continue;
                    }
                    // copy the structure into the temp nodes
                    int cutPos = 2*i;
                    int tempPos = 2;
                    int layerHalfSize = 1;
                    while(cutPos < (2<<depth))
                    {
                        for(int r=0; r<layerHalfSize; r++)
                            pTempNodes[tempPos+r] = pCutNodes[(cutPos+r) + c0*maxNodeSize];
                        for(int r=layerHalfSize; r<2*layerHalfSize; r++)
                            pTempNodes[tempPos+r] = pCutNodes[(cutPos+r) + c1*maxNodeSize];
                        layerHalfSize *= 2;
                        tempPos *= 2;
                        cutPos *= 2;
                    }
                    // extract cut
                    cuts[cIndex].Depth = depth - cdepth;
                    Cm_FaExtractLeafs(pTempNodes, &cuts[cIndex]);
                    afCuts[cIndex] = Cm_CutLeafAreaFlowSum(&cuts[cIndex]);
                }
            // add the trivial cut if valid 
            firstNonTrivialCutId = 0;
            if (pNodes[i]->BestCut.Arrival <= latestArrival + eps)
            {
                af[i] = pNodes[i]->BestCut.AreaFlow;
                if ( i < (1<<depth))
                {
                    pCutNodes[2*i] = NULL;
                    pCutNodes[2*i+1] = NULL;
                }
                firstNonTrivialCutId = 1;
            }
            // add the remaining priority cuts
            for(int cPos=firstNonTrivialCutId; cPos<maxCutSize; cPos++)
            {
                int bestPos = -1;
                float bestAf = CM_FLOAT_LARGE;
                for(int r=0; r<maxCutSize*maxCutSize; r++)
                    if ( afCuts[r] > -0.5 && bestAf >= afCuts[r] )
                    {
                        bestAf = afCuts[r];
                        bestPos = r;
                    }
                if ( bestPos < 0)
                    break;
                Cm_FaClearSub(pCutNodes + cPos*maxNodeSize, i, depth);
                pCutNodes[i + cPos*maxNodeSize] = pNodes[i];
                Cm_FaBuildSub(pCutNodes + cPos*maxNodeSize, i, &cuts[bestPos], depth);
                af[i + cPos*maxNodeSize] = afCuts[bestPos];
                afCuts[bestPos] = -1;
            }
        }
    }
    Cm_FaExtractLeafs(pCutNodes + firstNonTrivialCutId * maxNodeSize, pCut);
    return Cm_ManCutAreaFlow(p, pCut);
}

ABC_NAMESPACE_IMPL_END
