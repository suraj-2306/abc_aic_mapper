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
    if ((!fFirst && Cm_IsComplement(pObj)) || Vec_PtrSize(vSuper) > 10000 || !Cm_ObjIsAnd(pObj)) {
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
    return RetValue1 || RetValue0;
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
    Cm_Obj_t *pObjNew, *pObj1, *pObj2;
    int i;
    Vec_Ptr_t* vSuper;
    assert(!Cm_IsComplement(pObj));
    // return if the result if known
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
        pObj1 = (Cm_Obj_t*)Vec_PtrPop(vSuper);
        pObj2 = (Cm_Obj_t*)Vec_PtrPop(vSuper);
        Cm_VecObjPushUniqueOrderByLevel(vSuper, Cm_ManCreateAnd(p, pObj1, pObj2));
    } // make sure the balanced node is not assigned
    assert(pObj->pCopy == NULL);
    pObj->pCopy = (Cm_Obj_t*)vSuper->pArray[0];
    vSuper->nSize = 0;
    return Cm_Regular(pObj->pCopy);
}
Vec_Ptr_t* Cm_ManBalancePerform(Cm_Man_t* p) {
    Vec_Vec_t* vStorage;
    Vec_Ptr_t* vCo = Vec_PtrAlloc(10);
    Cm_Obj_t* pObj = ABC_ALLOC(Cm_Obj_t, 1);
    int i;
    vStorage = Vec_VecStart(10);
    Cm_ManForEachCi(p, pObj, i)
        pObj->pCopy
        = pObj;
    Cm_ManForEachCo(p, pObj, i) {
        Vec_PtrPush(vCo, Cm_NodeBalance_rec(p, pObj->pFanin0, vStorage, 0));
        // Cm_ManCreateCo(p, (Cm_Obj_t*)vCo->pArray[i]);
        // pObj->Type = CM_CO;
    }
    // Vec_PtrForEachEntry(Vec_Ptr_t*, vCo, vTemp, i)
    //     Cm_ManCreateCo(p, (Cm_Obj_t*)vTemp);

    Vec_VecFree(vStorage);
    return vCo;
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
    if (pObj->Type == CM_AND) {
        // visit the transitive fanin of the node
        Cm_ManDfs_rec(p, pObj->pFanin0, vNodes);
        Cm_ManDfs_rec(p, pObj->pFanin1, vNodes);
    } else if (pObj->Type == CM_CI)
        Cm_ManDfs_rec(p, pObj, vNodes);

    // add the node after the fanins have been added
    Vec_PtrPush(vNodes, pObj);
}

Vec_Ptr_t* Cm_ManDfs(Cm_Man_t* p, Vec_Ptr_t* vCoTemp) {
    Vec_Ptr_t *vNodes, *vTemp;
    // Cm_Obj_t* pObj;
    int i;
    // set the traversal ID
    Cm_ManIncrementTravId(p);
    // start the array of nodes
    vNodes = Vec_PtrAlloc(100);
    // Cm_ManForEachCo(p, pObj, i) {
    Vec_PtrForEachEntry(Vec_Ptr_t*, vCoTemp, vTemp, i) {
        Cm_ManDfs_rec(p, (Cm_Obj_t*)vTemp, vNodes);
        Cm_ObjSetTravIdCurrent(p, (Cm_Obj_t*)vTemp);
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
        p->vObjs->pArray[i]
            = vTemp;
    }
}
Cm_Man_t* Cm_ManBalance(Cm_Man_t* p) {
    Cm_Man_t* pNew;
    int i;
    Vec_Ptr_t *vCo, *vTemp;
    Cm_Obj_t* pObjTemp;
    vCo = Cm_ManBalancePerform(p);
    Vec_PtrForEachEntry(Vec_Ptr_t*, vCo, vTemp, i) {
        pObjTemp = (Cm_Obj_t*)vTemp;
    }
    pNew = Cm_ManStartFromCo(p, vCo);
    return pNew;
}

ABC_NAMESPACE_IMPL_END
