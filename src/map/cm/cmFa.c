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
                if ( pFaninArray[index]->Type == CM_AND || pFaninArray[index]->Type == CM_AND_EQ )
                {
                    pFaninArray[2*index] = pFaninArray[index]->pFanin0;
                    pFaninArray[2*index+1] = pFaninArray[index]->pFanin1;
                    hasConeOfDepth = 1;
                } 
                else
                {
                    pFaninArray[2*index] = pFaninArray[2*index+1] = NULL;
                }
                assert(pFaninArray[index]->Type == CM_AND || pFaninArray[index]->Type == CM_AND_EQ
                                                          || pFaninArray[index]->Type == CM_CI);
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


void Cm_FaBuildSub_rec(Cm_Obj_t * pObj, Cm_Obj_t **pNodes, int pos, int depth)
{
    pNodes[pos] = pObj;
    if ( (pObj->fMark & CM_MARK_LEAF_SUB) )
        return;
    Cm_FaBuildSub_rec(pObj->pFanin0, pNodes, 2*pos, depth);
    Cm_FaBuildSub_rec(pObj->pFanin1, pNodes, 2*pos+1, depth);
}
void Cm_FaBuildSub(Cm_Obj_t **pNodes, int rootPos, Cm_Cut_t * pCut, int depth)
{
    for(int i=0; i<pCut->nFanins; i++)
        pCut->Leafs[i]->fMark |= CM_MARK_LEAF_SUB;
    Cm_FaBuildSub_rec(pNodes[rootPos], pNodes, rootPos, depth);
    for(int i=0; i<pCut->nFanins; i++)
        pCut->Leafs[i]->fMark &= ~CM_MARK_LEAF_SUB;
}

/**Function*************************************************************

  Synopsis    [Removes all leafs of the pLeafs array for which no path
               from the root (pObj) to the leafs without intermediate leaf
               exists ]

  Description [Removal is done inline in linear leaf array, which is ended
               by first NULL pointer or maximum size]
               
  SideEffects [nodes fMark CM_MARK_LEAF_CUT and CM_MARK_VALID used]

  SeeAlso     []

***********************************************************************/
void Cm_FaMarkValidLeafs_rec(Cm_Obj_t * pObj)
{
    if( (pObj->fMark & CM_MARK_LEAF_CUT) )
    {
        pObj->fMark |= CM_MARK_VALID;
        return;
    }
    Cm_FaMarkValidLeafs_rec(pObj->pFanin0);
    Cm_FaMarkValidLeafs_rec(pObj->pFanin1);
}
void Cm_FaRemoveDanglingLeafs(Cm_Obj_t *pObj, Cm_Obj_t ** pLeafs, int maxSize)
{
    int k = 0;
    while(k<maxSize && pLeafs[k])
        pLeafs[k++]->fMark |= CM_MARK_LEAF_CUT;

    Cm_FaMarkValidLeafs_rec(pObj);
    
    k = 0;
    int r = 0;
    while(k<maxSize && pLeafs[k])
    {
        if ( (pLeafs[k]->fMark & CM_MARK_VALID) )
            pLeafs[r++] = pLeafs[k];
        pLeafs[k++]->fMark &= ~(CM_MARK_LEAF_CUT|CM_MARK_VALID);
    }
    if ( r < k )
        pLeafs[r] = NULL;
}

/**Function*************************************************************

  Synopsis    [Merges to leaf arrays in the resulting array]

  Description [The arrays must be sorted. Returns sorted array
               of unique leafs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_FaMergeLeafArrays(Cm_Obj_t ** pA, Cm_Obj_t **pB, Cm_Obj_t **pMerged, int size)
{
    int posA = 0;
    int posB = 0;
    int posM = 0;
    while(posA < size && pA[posA] && posB < size && pB[posB])
    {
        if ( pA[posA]->Id > pB[posB]->Id )
            pMerged[posM++] = pA[posA++];
        else
        {
            if ( pA[posA]->Id == pB[posB]->Id )
                posA++;
            pMerged[posM++] = pB[posB++];
        }
    }
    while(posA < size && pA[posA] )
        pMerged[posM++] = pA[posA++];
    while(posB < size && pB[posB] )
        pMerged[posM++] = pB[posB++];
    if ( posM != 2*size)
        pMerged[posM] = NULL;
}
/**Function*************************************************************

  Synopsis    [Extract the leafs from the fanin array and updates cut]

  Description [The leafs are extracted in kind of merge sort of the
               subtrees. Addtionally any dangling leafs are removed.]
               
  SideEffects [fMark of CM_MARK_LEAF_CUT and CM_MARK_VALID is cleared
               for fanin nodes.]

  SeeAlso     [Cm_FaBuildWithMaximumDepth]

***********************************************************************/
void Cm_FaExtractLeafs(Cm_Obj_t **pNodes, Cm_Cut_t *pCut)
{
    short depth = pCut->Depth;
    Cm_ObjClearMarkFa(pNodes, pCut->Depth, CM_MARK_LEAF_CUT|CM_MARK_VALID);
    Cm_Obj_t * pLeafMem[2*CM_MAX_NLEAFS];
    for(int i=0; i<2*CM_MAX_NLEAFS; i++)
        pLeafMem[i] = NULL;
    Cm_Obj_t ** pLeafs = pLeafMem;
    Cm_Obj_t ** pMergedLeafs = pLeafMem + CM_MAX_NLEAFS;
    for(int i=0; i<(1<<depth); i++)
        pLeafs[i] = pNodes[(1<<depth) + i];
    int csize = 1;
    for(int cdepth=depth-1; cdepth >=0; cdepth--)
    {
        for(int k=0; k<(1<<cdepth); k++)
        {
            int nodePos = (1<<cdepth) + k;
            if ( pNodes[nodePos] && pNodes[2*nodePos] && pNodes[2*nodePos+1] )
            {
                Cm_FaMergeLeafArrays(pLeafs+(2*k)*csize, pLeafs+(2*k+1)*csize, pMergedLeafs+(2*k)*csize, csize);
                Cm_FaRemoveDanglingLeafs(pNodes[nodePos], pMergedLeafs+2*k*csize, 2*csize);
            }
            else
            {
                pMergedLeafs[k*(2*csize)] = pNodes[nodePos];
                pMergedLeafs[k*(2*csize)+1] = NULL;
            }
        }
        // swap arrays for next depth
        Cm_Obj_t ** pTemp = pLeafs;
        pLeafs = pMergedLeafs;
        pMergedLeafs = pTemp;
        csize *= 2;
    }
    pCut->nFanins = 0;
    while( pCut->nFanins < (1<<depth) && pLeafs[pCut->nFanins])
    {
        pCut->Leafs[pCut->nFanins] = pLeafs[pCut->nFanins];
        pCut->nFanins++;
    }
    if ( pCut->nFanins == 0)
        printf("No Fanins for %d with depth %d\n", pNodes[1]->Id, depth);
}

/**Function*************************************************************

  Synopsis    [Shifts the leafs down to maximum layer.]

  Description [Leafs must me marked with CM_MARK_LEAF.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_FaShiftDownLeafs(Cm_Obj_t **pFaninArray, int depth)
{
    for(int i=1; i<(1<<depth); i++)
    {
        // calculate pos in cone structure, (shifting leaf to bottom) and clearing undesired pointers
        if ( ! pFaninArray[i] || !(pFaninArray[i]->fMark & CM_MARK_LEAF) )
            continue;
        int pos = i;
        int layerNodeCount = 1;
        while( pos < (1<<depth) )
        {
            pos *= 2;
            layerNodeCount *= 2;
            for(int k=pos; k<pos+layerNodeCount; k++)
                pFaninArray[k] = NULL;
        }
        if ( pos!=i )
        {
            pFaninArray[pos] = pFaninArray[i];
            pFaninArray[i] = NULL;
        } 
    }
}

/**Function*************************************************************

  Synopsis    [Clears the fanin array elements, which are predecessors
               of the node at given pos.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_FaClearSub(Cm_Obj_t **pFa, int pos, int depth)
{
    pos *= 2;
    int layerSize = 2;
    while(pos < (2<<depth))
    {
        for(int i=pos; i<pos+layerSize; i++)
            pFa[i] = NULL;
        pos *= 2;
        layerSize *= 2;
    }
}

/**Function*************************************************************

  Synopsis    [Returns the latest (from cut root coming) arrival time of
               all the potential leaf nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Cm_FaLatestMoInputArrival(Cm_Obj_t ** pFa, int depth)
{
    float latestInputArrival = -CM_FLOAT_LARGE;
    for(int i=1; i<(1<<depth); i++)
        if ( pFa[i] && !pFa[2*i] && !pFa[2*i+1] &&
             pFa[i]->BestCut.Arrival > latestInputArrival)
            latestInputArrival = pFa[i]->BestCut.Arrival;   
    for(int i=(1<<depth); i<(2<<depth); i++)
        if ( pFa[i] && pFa[i]->BestCut.Arrival > latestInputArrival)
            latestInputArrival = pFa[i]->BestCut.Arrival;
    return latestInputArrival;
}


ABC_NAMESPACE_IMPL_END
