/**CFile****************************************************************

  FileName    [cmCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cone mapping based on priority cuts.]

  Synopsis    [The central part of the mapper.]

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

  Synopsis    [Sets default parameter for Cone Mapping]

  Description []
               
  SideEffects []

  SeeAlso     [CommandCm in base/abc.c ]

***********************************************************************/
void Cm_ManSetDefaultPars(Cm_Par_t* pPars) {
    memset(pPars, 0, sizeof(Cm_Par_t));
    pPars->fThreeInputGates = 0;
    if (pPars->fThreeInputGates) {
        pPars->nConeDepth = 3;
        pPars->MinSoHeight = 1;
        pPars->fCutBalancing = 0;
    } else {
        pPars->nConeDepth = 6;
        pPars->MinSoHeight = 2;
        pPars->fCutBalancing = 1;
    }
    pPars->fVerbose = 0;
    pPars->fVeryVerbose = 0;
    pPars->fExtraValidityChecks = 0;
    pPars->fStructuralRequired = 1;
    pPars->fDirectCuts = 1;
    pPars->fPriorityCuts = 0;
    pPars->nAreaRounds = 3;
    pPars->AreaFlowAverageWeightFactor = (float)1.5;
    pPars->MaxCutSize = 10;
    pPars->fEnableSo = 1;
    pPars->fRespectSoSlack = 1;
    pPars->ArrivalRelaxFactor = (float)1.0;
    pPars->Epsilon = (float)0.005;
    pPars->WireDelay = (float)0;
    pPars->AreaFactor = 0;
    pPars->nMaxCycleDetectionRecDepth = 5;
    pPars->fVerboseCSV = 0;
    pPars->fAreaFlowHeuristicGlobal = 1;
    pPars->fAreaFlowHeuristicLocal = 1;
    pPars->fConeOccupancy = 1;
}

/**Function*************************************************************

  Synopsis    [Selects the required cuts for the circuit covering and 
               updates estimated reference count]

  Description [The root nodes of the selected cuts are marked VISIBLE.
               No side outputs are enabled.]
               
  SideEffects []

  SeeAlso     [CommandCm in base/abc.c ]

***********************************************************************/
void Cm_ManAssignCones(Cm_Man_t* p) {
    Cm_Obj_t* pObj;
    int enumerator;
    Cm_ManForEachObj(p, pObj, enumerator) {
        pObj->fMark = 0;
        pObj->nMoRefs = 0;
        pObj->nSoRefs = 0;
        pObj->BestCut.SoOfCutAt = NULL;
    }
    Cm_ManForEachObjReverse(p, pObj, enumerator) {
        if (pObj->Type == CM_CO) {
            pObj->pFanin0->fMark |= CM_MARK_VISIBLE;
            continue;
        }
        if (pObj->Type == CM_AND) {
            if (!(pObj->fMark & CM_MARK_VISIBLE))
                continue;
            Cm_Obj_t* pRepr = Cm_ObjGetRepr(pObj);
            for (int i = 0; i < pRepr->BestCut.nFanins; i++) {
                pRepr->BestCut.Leafs[i]->fMark |= CM_MARK_VISIBLE;
                pRepr->BestCut.Leafs[i]->nMoRefs++;
            }
        }
    }
    float alpha = p->pPars->AreaFlowAverageWeightFactor;
    Cm_ManForEachNode(p, pObj, enumerator) {
        if ((pObj->fMark & CM_MARK_VISIBLE))
            pObj->nRefsEstimate = (pObj->nRefsEstimate + alpha * pObj->nMoRefs) / (1 + alpha);
        else
            pObj->nRefsEstimate = 1;
    }
}

/**Function*************************************************************

  Synopsis    [Performs one round of area recovery]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_ManRecoverArea(Cm_Man_t* p, int nAreaRoundsIter) {
    float* AicDelay = p->pPars->AicDelay;
    float* AicArea = p->pPars->AicArea;
    float eps = p->pPars->Epsilon;
    const int minDepth = p->pPars->MinSoHeight;
    const int maxDepth = p->pPars->nConeDepth;
    const int fCutBalancing = p->pPars->fCutBalancing;
    double slackNode, slackFactor = 0, slackNodeSum = 0, prevSlackNode;
    char* tempDataLine = ABC_ALLOC(char, 1000);
    char* tempIndexLine = ABC_ALLOC(char, 1000);
    int fAreaFlowHeuristicSlack;
    int enumerator;
    Cm_Obj_t* pObj;
    Cm_Cut_t tCut;
    if (p->pPars->fThreeInputGates) {
        Cm_Obj_t* pNodes[Cm_Fa3Size(maxDepth + 1) + 1];
        Cm_ManForEachNode(p, pObj, enumerator) {
            int fUpdate = 0;
            float bestAreaFlow = CM_FLOAT_LARGE;
            for (int d = minDepth; d <= maxDepth; d++) {
                pNodes[1] = pObj;
                int cdepth = Cm_Fa3BuildWithMaximumDepth(pNodes, d);
                if (cdepth < d)
                    break;
                float latestInputArrival = Cm_Fa3LatestMoInputArrival(pNodes, d);
                float requiredInputArrival = pObj->Required - AicDelay[d];
                if (latestInputArrival > requiredInputArrival + eps)
                    continue;
                tCut.Depth = d;
                float areaFlow = Cm_ManMinimizeCutAreaFlow(p, pNodes, requiredInputArrival, &tCut);
                if (areaFlow + eps < bestAreaFlow) {
                    fUpdate = 1;
                    Cm_CutCopy(&tCut, &pObj->BestCut);
                    bestAreaFlow = areaFlow;
                }
            }
            if (fUpdate)
                pObj->BestCut.AreaFlow = bestAreaFlow / (pObj->nRefsEstimate);
            else
                pObj->BestCut.AreaFlow = Cm_ManCutAreaFlow(p, &pObj->BestCut) / pObj->nRefsEstimate;
            pObj->BestCut.Arrival = Cm_CutLatestLeafMoArrival(&pObj->BestCut) + AicDelay[pObj->BestCut.Depth];
        }

    } else {
        Cm_Obj_t* pNodes[(2 << maxDepth)];
        Cm_ManForEachNode(p, pObj, enumerator) {
            fAreaFlowHeuristicSlack = 0;
            int fUpdateCO = 0;
            int fUpdateAF = 0;
            float bestAreaFlow = CM_FLOAT_LARGE;
            float bestConeOccupancy = 0;
            for (int d = minDepth; d <= maxDepth; d++) {
                pNodes[1] = pObj;
                //This gives the maximum depth possible for the current node
                int cdepth = Cm_FaBuildWithMaximumDepth(pNodes, d);
                if (cdepth < d)
                    break;
                float latestInputArrival = Cm_FaLatestMoInputArrival(pNodes, d);
                float requiredInputArrival = pObj->Required - AicDelay[d];
                if (latestInputArrival > requiredInputArrival + eps)
                    continue;
                tCut.Depth = d;
                float areaFlow = Cm_ManMinimizeCutAreaFlow(p, pNodes, requiredInputArrival, &tCut);
                float coneOccupancy = Cm_ManGetConeOccupancy(p, pNodes, d);

                sprintf(tempIndexLine, "Node_%d_%d_co_%d,", pObj->Id, nAreaRoundsIter, d);
                sprintf(tempDataLine, "%4.4f,", coneOccupancy);

                Vec_StrAppend(p->indexLine, tempIndexLine);
                Vec_StrAppend(p->dataLine, tempDataLine);

                sprintf(tempIndexLine, "Node_%d_%d_af_%d,", pObj->Id, nAreaRoundsIter, d);
                sprintf(tempDataLine, "%4.4f,", areaFlow);

                Vec_StrAppend(p->indexLine, tempIndexLine);
                Vec_StrAppend(p->dataLine, tempDataLine);

                if (areaFlow + eps < bestAreaFlow) {
                    if (!fUpdateCO) {
                        fUpdateAF = 1;
                        Cm_CutCopy(&tCut, &pObj->BestCut);
                        bestAreaFlow = areaFlow;
                    } else {
                        if (areaFlow + eps < 0.8 * bestAreaFlow) {
                            fUpdateAF = 1;
                            Cm_CutCopy(&tCut, &pObj->BestCut);
                            bestAreaFlow = areaFlow;
                        }
                    }
                }
                if (p->pPars->fConeOccupancy) {
                    if (coneOccupancy > bestConeOccupancy) {
                        fUpdateCO = 1;
                        Cm_CutCopy(&tCut, &pObj->BestCut);
                        bestAreaFlow = areaFlow;
                        bestConeOccupancy = coneOccupancy;
                    }
                }
            }
            if (fUpdateAF || fUpdateCO)
                //Add the slack part here when you get the better
                pObj->BestCut.AreaFlow = bestAreaFlow / (pObj->nRefsEstimate);
            else
                pObj->BestCut.AreaFlow = Cm_ManCutAreaFlow(p, &pObj->BestCut) / pObj->nRefsEstimate;

            pObj->BestCut.Arrival
                = Cm_CutLatestLeafMoArrival(&pObj->BestCut) + AicDelay[pObj->BestCut.Depth];

            slackNode = pObj->Required - pObj->BestCut.Arrival;
            slackNodeSum += slackNode;
            Vec_FltWriteEntryGrow(p->prevSlackValue, enumerator, slackNode);
            if (nAreaRoundsIter > 0 && p->pPars->fAreaFlowHeuristicGlobal) {
                prevSlackNode = Vec_FltEntry(p->prevSlackValue, enumerator);
                if (prevSlackNode > slackNode)
                    fAreaFlowHeuristicSlack = 1;
            }
            if (nAreaRoundsIter == 0 && p->pPars->fAreaFlowHeuristicGlobal)
                fAreaFlowHeuristicSlack = 1;

            if (!(pObj->fMark & CM_MARK_VISIBLE) && fAreaFlowHeuristicSlack) {
                //This is for debugging purposes, i.e allows the user to actually view the slack of each node and the corresponding area flow
                sprintf(tempIndexLine, "slackNode_%d,", pObj->Id);
                sprintf(tempDataLine, "%4.4f,", slackNode);

                Vec_StrAppend(p->indexLine, tempIndexLine);
                Vec_StrAppend(p->dataLine, tempDataLine);

                slackFactor = (1 + (slackNode - p->slackNodeMean) / p->slackNodeMax);

                sprintf(tempIndexLine, "AreaFlow_%d,", pObj->Id);
                sprintf(tempDataLine, "%4.4f,", pObj->BestCut.AreaFlow);

                Vec_StrAppend(p->indexLine, tempIndexLine);
                Vec_StrAppend(p->dataLine, tempDataLine);

                pObj->BestCut.AreaFlow = pObj->BestCut.AreaFlow * slackFactor;
            }
            if (fCutBalancing && Cm_ManBalanceCut(p, pObj)) {
                pObj->fRepr = 0;
                Cm_Obj_t* pBest = pObj;
                Cm_Obj_t* pEq = pObj->pEquiv;
                while (pEq) {
                    float arrival = Cm_CutLatestLeafMoArrival(&pEq->BestCut) + AicDelay[pEq->BestCut.Depth];
                    if (arrival < pObj->Required + eps) {
                        float af = (Cm_CutLeafAreaFlowSum(&pEq->BestCut) + AicArea[pEq->BestCut.Depth]) / pObj->nRefsEstimate;
                        if (af < pBest->BestCut.AreaFlow) {
                            pEq->BestCut.AreaFlow = af;
                            pEq->BestCut.Arrival = arrival;
                            pBest = pEq;
                        }
                    }
                    pEq->fRepr = 0;
                    pEq = pEq->pEquiv;
                }
                pBest->fRepr = 1;
            }
        }
    }

    //Calculates the sum of the slack of all nodes in the mapping. It was used to determine the change in pattern of overall remaining slack in each iteration
    sprintf(tempIndexLine, "slackNodeSum_%d,", nAreaRoundsIter);
    sprintf(tempDataLine, "%4.4f,", slackNodeSum);

    Vec_StrPrintF(p->indexLine, tempIndexLine);
    Vec_StrPrintF(p->dataLine, tempDataLine);
}

/**Function*************************************************************

  Synopsis    [Performs the cone mapping]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cm_ManPerformMapping(Cm_Man_t* p) {
    // Cm_PrintAigStructure(p, 100);cmcore
    Cm_Obj_t* pObj;
    Cm_Obj_t* pNodes[CM_MAX_FA_SIZE];
    int enumerator;
    float* AicDelay = p->pPars->AicDelay;
    int nAreaRounds = p->pPars->nAreaRounds;
    Cm_ManSetCiArrival(p);
    p->pConst1->BestCut.AreaFlow = 0;
    p->pConst1->BestCut.Arrival = 0;
    p->pConst1->fRepr = 1;
    Cm_ManForEachCi(p, pObj, enumerator) {
        pObj->BestCut.AreaFlow = 0;
        pObj->nRefsEstimate = 1;
    }
    Cm_ManForEachNode(p, pObj, enumerator) {
        pObj->nRefsEstimate = 1;
        pNodes[1] = pObj;
        float arr = Cm_FaBuildDepthOptimal(pNodes, p->pPars);
        // FaE
        if (p->pPars->fThreeInputGates)
            Cm_Fa3ExtractLeafs(pNodes, &pObj->BestCut);
        else
            Cm_FaExtractLeafs(pNodes, &pObj->BestCut);
        pObj->BestCut.Arrival = arr + AicDelay[pObj->BestCut.Depth];
    }
    Cm_ManAssignCones(p);
    if (p->pPars->fVerbose)
        Cm_PrintBestCutStats(p);

    if (p->pPars->fStructuralRequired)
        Cm_ManCalcRequiredStructural(p);
    else {
        if (p->pPars->fExtraValidityChecks)
            Cm_TestMonotonicArrival(p);
        float arrival = Cm_ManLatestCoArrival(p);
        Cm_ManSetCoRequired(p, arrival);
        Cm_ManCalcVisibleRequired(p);
        Cm_ManSetInvisibleRequired(p);

        if (p->pPars->fVerbose)
            Cm_PrintBestCutStats(p);
    }
    if (nAreaRounds) {
        int nAreaRoundsIter = 0;
        while (nAreaRoundsIter < nAreaRounds) {
            Cm_ManSetSlackTimes(p);
            Cm_ManRecoverArea(p, nAreaRoundsIter);
            if (!p->pPars->fStructuralRequired) {
                Cm_ManCalcVisibleRequired(p);
                Cm_ManSetInvisibleRequired(p);
            }
            Cm_ManAssignCones(p);
            if (p->pPars->fExtraValidityChecks) {
                Cm_TestPositiveSlacks(p, 1);
                Cm_TestArrivalConsistency(p);
            }
            if (p->pPars->fVerbose)
                Cm_PrintBestCutStats(p);
            nAreaRoundsIter++;
        }
    } else
        Cm_ManCalcVisibleRequired(p);

    if (p->pPars->fVeryVerbose) {
        Cm_PrintCoArrival(p);
        Cm_PrintCiRequired(p);
    }
    if (p->pPars->fExtraValidityChecks) {
        // Cm_TestBestCutLeafsStructure(p);
        Cm_TestArrivalConsistency(p);
        Cm_TestPositiveSlacks(p, 1);
    }
    Cm_ManAssignCones(p);
    if (p->pPars->fEnableSo) {
        Cm_ManCalcVisibleRequired(p);
        Cm_ManInsertSos(p);
        if (p->pPars->fExtraValidityChecks) {
            Cm_TestArrivalConsistency(p);
            if (p->pPars->fRespectSoSlack)
                Cm_TestPositiveSlacks(p, 1);
        }
    }
    return 0;
}

ABC_NAMESPACE_IMPL_END
