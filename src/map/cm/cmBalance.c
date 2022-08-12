/**CFile****************************************************************

  FileName    [cmArea.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cone mapping based on priority cuts.]

  Synopsis    [Functions balance cuts.]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: cmArea.c,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/

#include "cm.h"
#include "misc/vec/vecPtr.h"
#include "misc/vec/vecVec.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

static void Cm_NodeBalancePermute(Cm_Man_t* p, Vec_Ptr_t* vSuper, int LeftBound);
/**Function*************************************************************

  Synopsis    [Creates Hop AIG from cut.]

  Description [iTemp has to be cut leaf number, leafs must be marked
               with CM_MARK_LEAF_CUT]

  SideEffects []

  SeeAlso     []

***********************************************************************/

Hop_Obj_t* Cm_ManCreateCutHop_rec(Hop_Man_t* pHm, Cm_Obj_t* pObj) {
    if ((pObj->fMark & CM_MARK_LEAF_CUT))
        return Hop_IthVar(pHm, pObj->iTemp);
    return Hop_And(pHm, Hop_NotCond(Cm_ManCreateCutHop_rec(pHm, pObj->pFanin0), pObj->fCompl0),
                   Hop_NotCond(Cm_ManCreateCutHop_rec(pHm, pObj->pFanin1), pObj->fCompl1));
}

/**Function*************************************************************

  Synopsis    [Creates equivalent nodes from (balanced) Hop AIG]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Obj_t* Cm_ManCreateEqCut_rec(Cm_Man_t* p, Hop_Obj_t* pHopObj, Cm_Obj_t* pCmRoot) {
    if (Hop_ObjIsPi(pHopObj)) {
        Cm_Obj_t* pL = pCmRoot->BestCut.Leafs[pHopObj->PioNum];
        pL->fMark |= CM_MARK_SEEN;
        return pL;
    }
    Hop_Obj_t* pH0 = Hop_ObjFanin0(pHopObj);
    Hop_Obj_t* pH1 = Hop_ObjFanin1(pHopObj);
    Cm_Obj_t* pRes = Cm_ManCreateAndEq(p, Cm_ManCreateEqCut_rec(p, pH0, pCmRoot),
                                       Cm_ManCreateEqCut_rec(p, pH1, pCmRoot));
    pRes->fCompl0 = Hop_ObjFaninC0(pHopObj);
    pRes->fCompl1 = Hop_ObjFaninC1(pHopObj);
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Tries do reduce depth of cut via balancing.]

  Description [If possible, a new equivalent node is created
               and returned. Returns otherwise NULL.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Obj_t* Cm_ManBalanceCut(Cm_Man_t* p, Cm_Obj_t* pObj) {
    Cm_Obj_t* pEqCut = NULL;
    int d = pObj->BestCut.Depth;
    for (int i = 0; i < pObj->BestCut.nFanins; i++) {
        pObj->BestCut.Leafs[i]->iTemp = i;
        pObj->BestCut.Leafs[i]->fMark |= CM_MARK_LEAF_CUT;
        pObj->BestCut.Leafs[i]->fMark &= ~CM_MARK_SEEN;
    }
    Hop_Man_t* pHm = Hop_ManStart();
    Hop_IthVar(pHm, pObj->BestCut.nFanins - 1);
    Hop_Obj_t* pHopRoot = Cm_ManCreateCutHop_rec(pHm, pObj);
    Hop_ObjCreatePo(pHm, pHopRoot);
    for (int i = 0; i < pObj->BestCut.nFanins; i++)
        pObj->BestCut.Leafs[i]->fMark &= ~CM_MARK_LEAF_CUT;
    Hop_Man_t* pN = Hop_ManBalance(pHm, 1);
    int balancedDepth = Hop_ManCountLevels(pN);
    Hop_ManStop(pHm);
    if (balancedDepth < d && d >= 2 && balancedDepth > 0) {
        if (p->pPars->fVerbose)
            Cm_PrintBestCut(pObj);
        pHopRoot = Hop_ObjFanin0(Hop_ManPo(pN, 0));
        // get last element in equivalence list
        Cm_Obj_t* pPre = pObj;
        while (pPre->pEquiv)
            pPre = pPre->pEquiv;

        Cm_Obj_t* pEq = Cm_ManCreateEqCut_rec(p, pHopRoot, pObj);

        // transfer only used leafs
        pEq->BestCut.nFanins = 0;
        for (int i = 0; i < pObj->BestCut.nFanins; i++)
            if ((pObj->BestCut.Leafs[i]->fMark & CM_MARK_SEEN))
                pEq->BestCut.Leafs[pEq->BestCut.nFanins++] = pObj->BestCut.Leafs[i];
        pEq->BestCut.Depth = balancedDepth;
        pEq->BestCut.SoOfCutAt = NULL;
        Cm_Obj_t* pNodes[128];
        pNodes[1] = pEq;
        Cm_FaBuildWithMaximumDepth(pNodes, balancedDepth);
        if (p->pPars->fVerbose) {
            Cm_PrintFa(pNodes, balancedDepth);
            Cm_PrintBestCut(pEq);
        }
        // add node
        pPre->pEquiv = pEq;
        pEqCut = pEq;
    }
    Hop_ManStop(pN);
    return pEqCut;
}

int Cm_ManFanoutNum(Cm_Obj_t* pObj) {
    // int fanoutNum = 0;
    return pObj->pIfFanout->nSize;
}

int Cm_NodeBalanceCone_rec(Cm_Obj_t* pObj, Vec_Ptr_t* vSuper, int fFirst) {
    int RetValue0, RetValue1, i;
    if ((pObj->fMark & CM_MARK_SEEN)) {
        for (i = 0; i < vSuper->nSize; i++)
            if (vSuper->pArray[i] == pObj)
                return 1;
        for (i = 0; i < vSuper->nSize; i++)
            if (vSuper->pArray[i] == Cm_Not(pObj))
                return -1;
        assert(0);
        return 0;
    }

    if (!fFirst && (Cm_IsComplement(pObj) || !Cm_ObjIsAnd(pObj) || (Cm_ManFanoutNum(pObj) > 1) || Vec_PtrSize(vSuper) > 10000)) {
        Vec_PtrPush(vSuper, pObj);
        pObj->fMark |= CM_MARK_SEEN;
        return 0;
    }
    assert(!Cm_IsComplement(pObj));
    assert(Cm_ObjIsAnd(pObj));

    RetValue0 = Cm_NodeBalanceCone_rec(pObj->pFanin0, vSuper, 0);
    RetValue1 = Cm_NodeBalanceCone_rec(pObj->pFanin1, vSuper, 0);
    if (RetValue1 == -1 || RetValue0 == -1)
        return -1;
    return RetValue0 || RetValue1;
}

Vec_Ptr_t* Cm_NodeBalanceCone(Cm_Obj_t* pObj, Vec_Vec_t* vStorage, int Level) {
    Vec_Ptr_t* vNodes;
    int RetValue, i;
    assert(!Cm_IsComplement(pObj));

    if (Vec_VecSize(vStorage) <= Level)
        Vec_VecPush(vStorage, Level, 0);

    vNodes = Vec_VecEntry(vStorage, Level);
    Vec_PtrClear(vNodes);

    RetValue = Cm_NodeBalanceCone_rec(pObj, vNodes, 1);
    assert(vNodes->nSize > 1);
    // unmark the visited nodes
    for (i = 0; i < vNodes->nSize; i++)
        Cm_Regular((Cm_Obj_t*)(vNodes->pArray[i]))->fMark = 0;
    // if we found the node and its complement in the same implication supergate,
    // return empty set of nodes (meaning that we should use constant-0 node)
    if (RetValue == -1)
        vNodes->nSize = 0;
    return vNodes;
}
Cm_Obj_t* Cm_NodeBalance_rec(Cm_Man_t* p, Cm_Obj_t* pObj, Vec_Vec_t* vStorage, int Level) {
    Cm_Obj_t *pObjNew, *pObj1, *pObj2, *pObjAnd, *pObjCopyAnd = ABC_ALLOC(Cm_Obj_t, 1);
    int i, LeftBound;

    Vec_Ptr_t* vSuper;
    if ((Cm_Obj_t*)(pObj->pCopy))
        return (Cm_Obj_t*)pObj->pCopy;
    assert(Cm_ObjIsAnd(pObj));

    vSuper = Cm_NodeBalanceCone(pObj, vStorage, Level);
    if (vSuper->nSize == 0) { // it means that the supergate contains two nodes in the opposite polarity
        Cm_ObjSetCopy(pObj, Cm_Not(p->pConst1));
        return pObj->pCopy;
    }

    for (i = 0; i < vSuper->nSize; i++) {
        pObjNew = Cm_NodeBalance_rec(p, Cm_Regular((Cm_Obj_t*)vSuper->pArray[i]), vStorage, Level + 1);
        vSuper->pArray[i] = Cm_NotCond(pObjNew, Cm_IsComplement((Cm_Obj_t*)vSuper->pArray[i]));
    }
    if (vSuper->nSize < 2)
        printf("BUG!\n");
    Vec_PtrSort(vSuper, (int (*)(void))Cm_NodeCompareLevelsDecrease);
    // balance the nodes
    assert(vSuper->nSize > 1);
    // p->nObjs[CM_AND] = 0;
    while (vSuper->nSize > 1) {
        LeftBound = Cm_ManNodeBalanceFindLeft(vSuper);
        // find the node that can be shared (if no such node, randomize choice)
        Cm_NodeBalancePermute(p, vSuper, LeftBound);
        pObj1 = (Cm_Obj_t*)Vec_PtrPop(vSuper);
        pObj2 = (Cm_Obj_t*)Vec_PtrPop(vSuper);
        pObjAnd = Cm_ManAnd(p, pObj1, pObj2);

        pObjCopyAnd = Cm_ObjCopy(pObjAnd);
        Vec_PtrPush(p->vRefNodes, pObjCopyAnd);
        Cm_VecObjPushUniqueOrderByLevel(vSuper, pObjAnd);

    } // make sure the balanced node is not assigned
    assert(pObj->pCopy == NULL);
    pObj->pCopy = (Cm_Obj_t*)vSuper->pArray[0];
    vSuper->nSize = 0;

    return Cm_Regular(pObj->pCopy);
}
void Cm_ManBalancePerform(Cm_Man_t* p) {
    Vec_Vec_t* vStorage;
    // Vec_Ptr_t* vCo = Vec_PtrAlloc(10);
    Cm_Obj_t* pObj = ABC_ALLOC(Cm_Obj_t, 1);
    int i;
    vStorage = Vec_VecStart(10);
    Cm_ManForEachCi(p, pObj, i)
        pObj->pCopy
        = pObj;

    //This is to indicate that this is fanin of the Co before balancing
    Cm_ManForEachCo(p, pObj, i) {
        pObj->pFanin0->fMark |= CM_MARK_CO;
    }
    Cm_ManStartFromCo(p);
    Cm_ManForEachCoTemp(p, pObj, i) {
        Cm_NodeBalance_rec(p, pObj->pFanin0, vStorage, 0);
    }

    // TESTING
    // Cm_ManForEachObj(p, pObj, i) {
    // printf("ninja %d\n", pObj->Id);
    // }
    Vec_VecFree(vStorage);
}

void Cm_ManDfs_rec(Cm_Man_t* p, Cm_Obj_t* pObj, Vec_Ptr_t* vNodes) {
    // if this node is already visited, skip

    if (Cm_ManIsTravIdCurrent(p, pObj))
        return;
    // mark the node as visited
    Cm_ObjSetTravIdCurrent(p, pObj);
    // skip the PI
    if (pObj->Type == CM_CONST1)
        return;
    if (pObj->Type == CM_CO)
        Cm_ManDfs_rec(p, pObj->pFanin0, vNodes);
    if (pObj->Type == CM_AND) {
        // visit the transitive fanin of the node
        Cm_ManDfs_rec(p, pObj->pFanin0, vNodes);
        Cm_ManDfs_rec(p, pObj->pFanin1, vNodes);
    } else if (pObj->Type == CM_CI)
        Cm_ManDfs_rec(p, pObj, vNodes);

    // add the node after the fanins have been added
    Vec_PtrPush(vNodes, pObj);
}

Vec_Ptr_t* Cm_ManDfs2(Cm_Man_t* p, Cm_Obj_t* pObj) {
    Vec_Ptr_t* vNodes;
    // set the traversal ID
    Cm_ManIncrementTravId(p);
    // start the array of nodes
    vNodes = Vec_PtrAlloc(100);
    Cm_ManDfs_rec(p, pObj, vNodes);
    Cm_ObjSetTravIdCurrent(p, pObj);
    return vNodes;
}
Vec_Ptr_t* Cm_ManDfs(Cm_Man_t* p) {
    Vec_Ptr_t* vNodes;
    Cm_Obj_t* pObj;
    int i;
    // set the traversal ID
    Cm_ManIncrementTravId(p);
    // start the array of nodes
    vNodes = Vec_PtrAlloc(100);
    // Cm_ManForEachCo(p, pObj, i) {
    Cm_ManForEachCo(p, pObj, i) {
        // Vec_PtrForEachEntry(Cm_Obj_t*, vCoTemp, vTemp, i) {
        Cm_ManDfs_rec(p, pObj, vNodes);
        Cm_ObjSetTravIdCurrent(p, pObj);
    }
    return vNodes;
}
void Cm_ManSortById(Cm_Man_t* p) {
    int i, vIndex, *iTemp;
    Cm_Obj_t* pObj;
    Vec_Int_t* vId = Vec_IntAlloc(10);
    Vec_Ptr_t* vObjsNew = Vec_PtrAlloc(10);
    Vec_Ptr_t* vTemp = Vec_PtrAlloc(1);
    Cm_ManForEachObj(p, pObj, i)
        Vec_IntPush(vId, pObj->Id);
    Vec_IntSort(vId, 0);

    Cm_ManForEachObj(p, pObj, i) {
        vIndex = Vec_IntFind(vId, pObj->Id);
        Vec_PtrSetEntry(vObjsNew, vIndex, p->vObjs->pArray[i]);
        iTemp = vObjsNew->pArray[vIndex];
        //Add removal of Vec_Int for faster performance
        // Vec_IntRemove(vId, vIndex);
    }
    Vec_PtrForEachEntry(Vec_Ptr_t*, vObjsNew, vTemp, i) {
        p->vObjs->pArray[i] = vTemp;
    }
}
/**Function*************************************************************

  Synopsis    [Removes the dangling nodes with no fanouts]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_ManCleanUp(Cm_Man_t* p) {
    Vec_Ptr_t *vNodes, *vNodesObjTemp = Vec_PtrAlloc(1), *pNode;

    int i, index;
    Cm_Obj_t* pObj;
    vNodes = Cm_ManDfs(p);
    vNodesObjTemp = Vec_PtrDup(p->vObjs);

    Vec_PtrForEachEntry(Vec_Ptr_t*, vNodes, pNode, i) {
        index = Vec_PtrFind(vNodesObjTemp, pNode);
        pObj = (Cm_Obj_t*)pNode;
        Vec_PtrDrop(vNodesObjTemp, index);
    }
    Vec_PtrForEachEntry(Vec_Ptr_t*, vNodesObjTemp, pNode, i) {
        Vec_PtrRemove(p->vObjs, pNode);
    }
}

Vec_Ptr_t* Cm_ManGetCi(Cm_Man_t* p, Cm_Obj_t* pObj) {
    Vec_Ptr_t *vNodes, *vNodesCi = Vec_PtrAlloc(1);
    int i;
    Cm_Obj_t* pObjTemp;

    vNodes = Cm_ManDfs2(p, pObj);
    Vec_PtrForEachEntry(Cm_Obj_t*, vNodes, pObjTemp, i) {
        if (pObjTemp->Type == CM_CI)
            Vec_PtrPush(vNodesCi, pObjTemp);
    }
    return vNodesCi;
}

unsigned Cm_ManHashConesCo(Cm_Man_t* p, Cm_Obj_t* pObj) {
    Vec_Ptr_t* vObjsCi = Vec_PtrAlloc(1);
    unsigned Key;

    vObjsCi = Cm_ManGetCi(p, pObj);
    Key = Cm_HashKeyX(vObjsCi, p->nBinsBal);
    return Key;
}
Vec_Ptr_t* Cm_ManCoFromHashCones(Cm_Man_t* p, Vec_Int_t* vObjCoKeys) {
    Vec_Ptr_t* vObjsCo = Vec_PtrAlloc(1);
    Vec_Int_t* vKeyCount = Vec_IntStart(10000);
    Vec_Ptr_t* vCoPossible = Vec_PtrAlloc(1);
    int i;
    Cm_Obj_t* pObj;
    unsigned Key;

    //Filter the nodes and only get the possible output nodes
    // first get the nodes marked by the flags
    // Then compute the hashkeys simultaneosyl
    //Then find the particular hash key in the above function, if not present then yeet the node

    //Accumulate the possible Co from the information based on the flags
    Vec_PtrForEachEntry(Cm_Obj_t*, p->vRefNodes, pObj, i) {
        if (pObj->fMark & CM_MARK_CO || pObj->fMark & CM_MARK_COBAL) {
            Vec_PtrPush(vCoPossible, pObj);
        }
    }

    Vec_PtrForEachEntry(Cm_Obj_t*, vCoPossible, pObj, i) {
        //Get the key from the pObj
        Key = Cm_ManHashConesCo(p, pObj);
        if (Vec_IntFind(vObjCoKeys, Key)) {
            //If the same key is found in the output list of keys then accumulate them
            // printf("find something %d\n", pObj->Id);

            pObj->pNextBal = p->pBinsBal[Key];
            //Initialize the array if no key is present
            if (vKeyCount->pArray[Key] == 0)
                p->pBinsBal[Key] = Vec_PtrAlloc(1);
            vKeyCount->pArray[Key]++;

            Vec_PtrPush(p->pBinsBal[Key], (Vec_Ptr_t*)pObj);
            p->nEntriesBal++;
            // Vec_IntPush(vObjsKeysTemp, Key);
            Vec_PtrPush(vObjsCo, pObj);
        }
    }
    return vObjsCo;
}
Vec_Ptr_t* Cm_ManBalLookup(Cm_Man_t* p, Vec_Ptr_t* vObjsCo) {
    Cm_Obj_t* pObj;
    int i;
    Vec_Ptr_t* vObjsCoSel = Vec_PtrAlloc(1);
    // unsigned Key = Cm_ManHashConesCo(p, (Cm_Obj_t*)vObjsCo);
    unsigned Key = Cm_HashKeyX(vObjsCo, p->nBinsBal);
    // Cm_ManBinForEachEntry(Cm_Obj_t*, p->pBinsBal[Key], pObj) {
    Vec_PtrForEachEntry(Cm_Obj_t*, p->pBinsBal[Key], pObj, i) {
        Vec_PtrPush(vObjsCoSel, pObj);
    }
    return vObjsCoSel;
}
void Cm_ManFinalize(Cm_Man_t* p) {
    Vec_Ptr_t *vObjsTemp = Vec_PtrAlloc(1), *vObjsCo, *vObjsCi, *vObjsCoSel;
    Vec_Int_t *vObjsCoCount = Vec_IntAlloc(1), *vObjCoKeys = Vec_IntAlloc(1);
    Cm_Obj_t *pObj, *pObjTemp;
    int i, j, k;
    unsigned KeyCoTemp, KeyCo;

    //Copy the objects array
    vObjsTemp = Vec_PtrDup(p->vObjs);

    //Compute the has keys for Co's Fanins
    Cm_ManForEachCoTemp(p, pObj, i) {
        KeyCoTemp = Cm_HashKeyX(Cm_ManGetCi(p, pObj->pFanin0), p->nBinsBal);
        Vec_IntPush(vObjCoKeys, KeyCoTemp);
    }

    //Get the Co from the Hashed output cones
    vObjsCo = Cm_ManCoFromHashCones(p, vObjCoKeys);

    //Iterate over all possible outputs
    Vec_PtrForEachEntry(Cm_Obj_t*, vObjsCo, pObj, i) {
        //Compute the primary input array for the given node
        // NOTE need to begin work from this commit
        vObjsCi = Cm_ManGetCi(p, pObj);
        //Select the subset of the allowable outputs for the given primary input array
        vObjsCoSel = Cm_ManBalLookup(p, vObjsCi);

        //TESTING
        // Vec_PtrForEachEntry(Cm_Obj_t*, vObjsCoSel, pObjtempo, i) {
        //     // printf("%d \n ", vObjsCoSel->nSize);
        //         // printf("%d\n", pObjtempo->Id);
        // }
        Vec_PtrForEachEntry(Cm_Obj_t*, vObjsCoSel, pObjTemp, j) {
            KeyCo = Cm_HashKeyX(Cm_ManGetCi(p, pObj), p->nBinsBal);

            //This means that the node was a primary output before and had a fanout hence it cannot be ignored
            // Or it can mean that it was legitimate new node formed only after balancing
            if ((!(Cm_ManFanoutNum(pObj) > 2) && !(pObjTemp->fMark & CM_MARK_COBAL)) || (pObjTemp->fMark & CM_MARK_COBAL && pObjTemp->fMark & CM_MARK_CO)) {
                for (k = 0; k < vObjCoKeys->nSize; k++) {
                    if (vObjCoKeys->pArray[k] == KeyCo) {
                        //For making sure that the Co are not duplicated again
                        if (Vec_IntFind(vObjsCoCount, pObjTemp->Id) == -1) {
                            Cm_ManCreateCo(p, pObjTemp);
                            // printf("Node is matching with node %d \n", pObjTemp->Id);
                        }
                        Vec_IntPush(vObjsCoCount, pObjTemp->Id);
                    }
                }
            }
        }
    }
}
/**Function*************************************************************


  Synopsis    [AND Balancing function]

  Description [Calls the procedures to AND balance the network]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cm_Man_t* Cm_ManBalance(Cm_Man_t* p) {
    Cm_ManBalancePerform(p);
    Cm_ManFinalize(p);
    // Cm_ManCleanUp(p);
    return p;
}
/**Function*************************************************************

  Synopsis    [Moves closer to the end the node that is best for sharing.

  Description [If there is no node with sharing, randomly chooses one of 
  the legal nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_NodeBalancePermute(Cm_Man_t* p, Vec_Ptr_t* vSuper, int LeftBound) {
    Cm_Obj_t *pNode1, *pNode2, *pNode3;
    int RightBound, i;
    // get the right bound
    RightBound = Vec_PtrSize(vSuper) - 2;
    assert(LeftBound <= RightBound);
    if (LeftBound == RightBound)
        return;
    // get the two last nodes
    pNode1 = (Cm_Obj_t*)Vec_PtrEntry(vSuper, RightBound + 1);
    pNode2 = (Cm_Obj_t*)Vec_PtrEntry(vSuper, RightBound);
    // find the first node that can be shared
    for (i = RightBound; i >= LeftBound; i--) {
        pNode3 = (Cm_Obj_t*)Vec_PtrEntry(vSuper, i);
        if (Cm_ManNodeLookup(p, pNode1, pNode3)) {
            if (pNode3 == pNode2)
                return;
            Vec_PtrWriteEntry(vSuper, i, pNode2);
            Vec_PtrWriteEntry(vSuper, RightBound, pNode3);
            return;
        }
    }
    /*
    // we did not find the node to share, randomize choice
    {
        int Choice = rand() % (RightBound - LeftBound + 1);
        pNode3 = Vec_PtrEntry( vSuper, LeftBound + Choice );
        if ( pNode3 == pNode2 )
            return;
        Vec_PtrWriteEntry( vSuper, LeftBound + Choice, pNode2 );
        Vec_PtrWriteEntry( vSuper, RightBound,         pNode3 );
    }
*/
}
/**Function*************************************************************

  Synopsis    [Finds the left bound on the next candidate to be paired.]

  Description [The nodes in the array are in the decreasing order of levels. 
  The last node in the array has the smallest level. By default it would be paired 
  with the next node on the left. However, it may be possible to pair it with some
  other node on the left, in such a way that the new node is shared. This procedure
  finds the index of the left-most node, which can be paired with the last node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cm_ManNodeBalanceFindLeft(Vec_Ptr_t* vSuper) {
    Cm_Obj_t *pNodeRight, *pNodeLeft;
    int Current;
    // if two or less nodes, pair with the first
    if (Vec_PtrSize(vSuper) < 3)
        return 0;
    // set the pointer to the one before the last
    Current = Vec_PtrSize(vSuper) - 2;
    pNodeRight = (Cm_Obj_t*)Vec_PtrEntry(vSuper, Current);
    // go through the nodes to the left of this one
    for (Current--; Current >= 0; Current--) {
        // get the next node on the left
        pNodeLeft = (Cm_Obj_t*)Vec_PtrEntry(vSuper, Current);
        // if the level of this node is different, quit the loop
        if (Cm_Regular(pNodeLeft)->Level != Cm_Regular(pNodeRight)->Level)
            break;
    }
    Current++;
    // get the node, for which the equality holds
    pNodeLeft = (Cm_Obj_t*)Vec_PtrEntry(vSuper, Current);
    assert(Cm_Regular(pNodeLeft)->Level == Cm_Regular(pNodeRight)->Level);
    return Current;
}
ABC_NAMESPACE_IMPL_END
