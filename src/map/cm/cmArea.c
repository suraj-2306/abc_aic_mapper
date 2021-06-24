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

  Synopsis    [Returns the cut and areaflow with the lowest area flow 
               calculated for all enabled heuristics.]

  Description [Options are described in Cm_Man parameters. If no option
               is enabled, only the cut is extracted from the faninarray ]

  SideEffects []

  SeeAlso     []

***********************************************************************/   
float Cm_ManMinimizeCutAreaFlow(Cm_Man_t *p, Cm_Obj_t **pNodes, float latestArrival, Cm_Cut_t *pCut)
{
    if ( !p->pPars->fDirectCuts && !p->pPars->fPriorityCuts )
    {
        Cm_FaExtractLeafs(pNodes, pCut);
        return Cm_ManCutAreaFlow(p, pCut);
    }
    if ( p->pPars->fDirectCuts && !p->pPars->fPriorityCuts )
        return Cm_ManMinimizeCutAreaFlowDirect(p, pNodes, latestArrival, pCut);
    if ( p->pPars->fPriorityCuts && !p->pPars->fDirectCuts )
        return Cm_ManMinimizeCutAreaFlowPriority(p, pNodes, latestArrival, pCut);

    Cm_Cut_t pCutTemp;
    pCutTemp.Depth = pCut->Depth;
    float priorityAF = Cm_ManMinimizeCutAreaFlowPriority(p, pNodes, latestArrival, &pCutTemp);
    float directAF = Cm_ManMinimizeCutAreaFlowDirect(p, pNodes, latestArrival, pCut);
    if (directAF < priorityAF)
        return directAF;
    Cm_CutCopy(&pCutTemp, pCut);
    return priorityAF;
}

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

/**Function*************************************************************

  Synopsis    [Generates a single cut of given depth by aiming to
               minimize an estimated area flow heuristic. Returns the
               cut area flow.]

  Description [Cone nodes are replaced from bottom up if estimated area 
               flow is reduced. The estimated area flow for a node is
               the area flow devided by the number of occurences
               inside the Faninarray]

  SideEffects [pNodes is changed]

  SeeAlso     []

***********************************************************************/
float Cm_ManMinimizeCutAreaFlowDirect(Cm_Man_t *p, Cm_Obj_t **pNodes, float latestArrival, Cm_Cut_t * pCut)
{
    short depth = pCut->Depth;
    const int maxNodeSize = (2<<depth);
    float eps = p->pPars->Epsilon;
    float af[maxNodeSize];
    // count number of occurences in iTemp
    for(int i=1; i<(2<<depth); i++)
        if ( pNodes[i] )
            pNodes[i]->iTemp = 0;
    for(int i=1; i<(2<<depth); i++)
        if ( pNodes[i] )
            pNodes[i]->iTemp++;
    for(int i=1; i<(2<<depth); i++)
        if ( pNodes[i] )
            af[i] = pNodes[i]->BestCut.AreaFlow / pNodes[i]->iTemp;
    // iterate now bottom up through the cone to optimize area flow
    // nodes are replaced by parent if area flow is [locally] decreased
    for ( int cdepth=depth-1; cdepth>0; cdepth--)
    {
        for(int i=(1<<cdepth); i<(2<<cdepth); i++)
        {
            if( !pNodes[i] )
                continue;
            if( pNodes[i]->BestCut.Arrival <= latestArrival + eps )
            {
                float nodesAreaFlow = (pNodes[2*i] ? af[2*i]: 0)
                                      + (pNodes[2*i+1] ? af[2*i+1] : 0);
                // remove children from cut if parent as leaf leads to reduced area flow
                if (nodesAreaFlow > af[i])
                    pNodes[2*i] = pNodes[2*i+1] = NULL;
                else
                    af[i] = nodesAreaFlow;
            }
        }
    }
    Cm_FaExtractLeafs(pNodes, pCut);
    return Cm_ManCutAreaFlow(p, pCut);
}




ABC_NAMESPACE_IMPL_END
