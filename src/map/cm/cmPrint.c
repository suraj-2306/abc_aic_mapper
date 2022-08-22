/**CFile****************************************************************

  FileName    [cmCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cone mapping based on priority cuts.]

  Synopsis    [Printing functionality.]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

  Revision    [$Id: ifCore.c,v 1.00 2021/02/15 00:00:00 thm Exp $]

***********************************************************************/

#include <stdio.h>

#include "cm.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Prints given parameter set to stdout]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_PrintPars(Cm_Par_t* pPars) {
    int w = 35;
    printf("%-*s%d\n", w, "Cone mapping depth", pPars->nConeDepth);
    printf("%-*s%s\n", w, "3-input gate cone", pPars->fThreeInputGates ? "yes" : "no");
    printf("%-*s%s\n", w, "MiMo library", pPars->pMiMoLib->pName);
    if (pPars->fPriorityCuts)
        printf("%-*s%d\n", w, "Priority cuts with maxSize", pPars->MaxCutSize);
    if (pPars->fDirectCuts)
        printf("%-*s\n", w, "Direct cut calculation");
    printf("%-*s%s\n", w, "Enable side outputs", pPars->fEnableSo ? "yes" : "no");
    if (pPars->fEnableSo) {
        printf("%-*s%d\n", w, "Minimum side outputs height", pPars->MinSoHeight);
        printf("%-*s%s\n", w, "Respect slack for side outputs", pPars->fRespectSoSlack ? "yes" : "no");
    }
    printf("%-*s%d\n", w, "Number of area recovery rounds", pPars->nAreaRounds);
    printf("%-*s%f\n", w, "Area flow weighting factor", pPars->AreaFlowAverageWeightFactor);
    printf("%-*s%f\n", w, "Arrival relaxation factor", pPars->ArrivalRelaxFactor);
    printf("%-*s%s\n", w, "Cut balancing", pPars->fCutBalancing ? "yes" : "no");
    printf("%-*s%s\n", w, "Required time calculation", pPars->fStructuralRequired ? "Structure" : "Choice");
    printf("%-*s%f\n", w, "Epsilon", pPars->Epsilon);
    printf("%-*s%f\n", w, "Wire delay", pPars->WireDelay);
    printf("%-*s%s\n", w, "Verbose", pPars->fVerbose ? "yes" : "no");
    printf("%-*s%s\n", w, "Very verbose", pPars->fVeryVerbose ? "yes" : "no");
    printf("%-*s%s\n", w, "Extra validity checks", pPars->fExtraValidityChecks ? "yes" : "no");
    printf("\n");
}

/**Function*************************************************************

  Synopsis    [Prints AIG statistic and limited number of nodes with 
               fanin references to stdout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline char Cm_ManCompl0ToChar(Cm_Obj_t* pObj) { return (pObj->fCompl0 ? '!' : ' '); }
static inline char Cm_ManCompl1ToChar(Cm_Obj_t* pObj) { return (pObj->fCompl1 ? '!' : ' '); }
static inline char Cm_ManCompl2ToChar(Cm_Obj_t* pObj) { return (pObj->fCompl2 ? '!' : ' '); }
void Cm_PrintAigStructure(Cm_Man_t* pMan, int lineLimit) {
    printf("Found: %d CIs, %d ANDs, and %d COs\n", pMan->nObjs[CM_CI],
           pMan->nObjs[CM_AND], pMan->nObjs[CM_CO]);
    printf("Printing up to %d first nodes of AIG\n", lineLimit);

    int i;
    Cm_Obj_t* pObj;
    Cm_ManForEachObj(pMan, pObj, i) {
        if (i >= lineLimit)
            break;
        switch (pObj->Type) {
            case CM_CONST1:
                printf("Constant 1: %d\n", pObj->Id);
                break;
            case CM_CI:
                printf("CI %d\n", pObj->Id);
                break;
            case CM_AND:
                if (pObj->pFanin2)
                    printf("N %d: (%c%d,%c%d,%c%d)\n", pObj->Id, Cm_ManCompl0ToChar(pObj), pObj->pFanin0->Id,
                           Cm_ManCompl1ToChar(pObj), pObj->pFanin1->Id,
                           Cm_ManCompl2ToChar(pObj), pObj->pFanin2->Id);
                else
                    printf("N %d: (%c%d,%c%d)\n", pObj->Id, Cm_ManCompl0ToChar(pObj), pObj->pFanin0->Id,
                           Cm_ManCompl1ToChar(pObj), pObj->pFanin1->Id);

                break;
            case CM_CO:
                printf("Co %d: (%c%d)\n", pObj->Id, Cm_ManCompl0ToChar(pObj), pObj->pFanin0->Id);
                break;
            default:
                printf("Unrecognized type\n");
        }
    }
}

/**Function*************************************************************

  Synopsis    [Prints the fanin array up to depth as binary tree to stdout]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_PrintFa(Cm_Obj_t** pFaninArray, int depth) {
    int nWidth = 6;
    if (!pFaninArray[1]) {
        printf("Cm_PrintFa: input is not well formed\n");
        return;
    }
    printf("%*d\n", nWidth / 2 + (nWidth << (depth - 1)), pFaninArray[1]->Id);
    for (int cdepth = 1; cdepth <= depth; cdepth++) {
        if (cdepth < depth)
            printf("%*s", nWidth / 2, "");
        for (int i = (1 << cdepth); i < (2 << cdepth); i++) {
            int indent = (i == (1 << cdepth) && cdepth < depth) ? nWidth << (depth - cdepth - 1) : nWidth << (depth - cdepth);
            printf("%*d", indent, pFaninArray[i] ? pFaninArray[i]->Id : -1);
        }
        printf("\n");
    }
}

/**Function*************************************************************

  Synopsis    [Prints the fanin array up to depth as ternary tree to stdout]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_PrintFa3(Cm_Obj_t** pFaninArray, int depth) {
    int nWidth = 6;
    if (!pFaninArray[1]) {
        printf("Cm_PrintFa3: input is not well formed\n");
        return;
    }
    printf("%*d\n", nWidth * (1 + Cm_Pow3(depth) / 2), pFaninArray[1]->Id);
    int ind = Cm_Pow3(depth);
    for (int cdepth = 1; cdepth <= depth; cdepth++) {
        for (int i = Cm_Fa3LayerStart(cdepth); i < Cm_Fa3LayerStart(cdepth + 1); i++) {
            int indent = (i == Cm_Fa3LayerStart(cdepth) && cdepth < depth) ? nWidth * (1 + ind / 6) : nWidth * (ind / 3);
            printf("%*d", indent, pFaninArray[i] ? pFaninArray[i]->Id : -1);
        }
        ind /= 3;
        printf("\n");
    }
}

/**Function*************************************************************

  Synopsis    [Prints the Bestcut of the given node ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_PrintBestCut(Cm_Obj_t* pObj) {
    printf("Bestcut at %d (Arrival: %3.1f, AF : %3.1f depth: %d, nFanins %d): ",
           pObj->Id, pObj->BestCut.Arrival, pObj->BestCut.AreaFlow,
           pObj->BestCut.Depth, pObj->BestCut.nFanins);
    for (int i = 0; i < pObj->BestCut.nFanins; i++)
        printf(" %d", pObj->BestCut.Leafs[i]->Id);
    printf("\n");
}

/**Function*************************************************************

  Synopsis    [Prints statistics of all visible best cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_PrintBestCutStats(Cm_Man_t* p) {
    Cm_ManAreaAnal_t* paAnal = Cm_ManGetAreaMetrics(p);

    printf("\n");
    printf("\tgateCount: %d\n", paAnal->GateCountAll);
    printf("\tarea: %3.1f\n", paAnal->GateAreaAll);
}

/**Function*************************************************************

  Synopsis    [Prints the delay of the cones sorted by depth.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_PrintConeDelays(Cm_Man_t* p) {
    printf("Cone delays:");
    for (int i = 1; i <= p->pPars->nConeDepth; i++)
        printf(" (%d: %5.2f)", i, p->pPars->AicDelay[i]);
    printf("\n");
}

/**Function*************************************************************

  Synopsis    [Prints for each CO the arrival time. ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_PrintCoArrival(Cm_Man_t* p) {
    int i;
    Cm_Obj_t* pObj;
    printf("Co arrival:");
    Cm_ManForEachCo(p, pObj, i)
        printf(" %3.1f", pObj->pFanin0->BestCut.Arrival);
    printf("\n");
}

/**Function*************************************************************

  Synopsis    [Prints for each CI the required time.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_PrintCiRequired(Cm_Man_t* p) {
    int enumerator;
    Cm_Obj_t* pObj;
    printf("Ci Required at:");
    Cm_ManForEachCi(p, pObj, enumerator)
        printf(" %3.1f", pObj->Required);
    printf("\n");
}
/**Function*************************************************************

  Synopsis    [Prints for the required time for all the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_PrintAllRequired(Cm_Man_t* p) {
    int i;
    Cm_Obj_t* pObj;
    printf("Required time for all nodes \n");
    Cm_ManForEachNode(p, pObj, i)
        printf("Node %d: %3.1f\n", i, pObj->Required);
    printf("\n");
}
/**Function*************************************************************

  Synopsis    [Prints for the Area Information along with the AreaFactor chosen ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_PrintAreaMetrics(Cm_Man_t* p) {
    Cm_ManAreaAnal_t* paAnal = Cm_ManGetAreaMetrics(p);
    printf("Area Metrics:\n");
    printf("Area Factor: %f\n", p->pPars->AreaFactor);
    printf("\tTotal gate area: %.1f\n", paAnal->GateAreaAll);
    printf("\tTotal gate count: %d\n", paAnal->GateCountAll);
    printf("\tThe number of gates used depth wise:\n \t\t");
    for (int i = 0; i < p->pPars->nConeDepth; i++) {
        printf("%d:%d\n\t\t", p->pPars->nConeDepth - i + 1, paAnal->GateCount[i]);
    }
    printf("\n");

    if (p->pPars->fVerboseCSV) {
    }
}
/**Function*************************************************************

  Synopsis    [Prints for the Area Information along with the AreaFactor chosen in a CSV file ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cm_PrintAreaMetricsCSV(Cm_Man_t* p) {
    Cm_ManAreaAnal_t* paAnal = Cm_ManGetAreaMetrics(p);
    int i;
    // Counting the number of individual gates
    char* gateInfoString = ABC_ALLOC(char, 100);
    char* cAreaMetricsFileName = ABC_ALLOC(char, 100);
    char* cAreaMetricsBaseName = ABC_ALLOC(char, 100);
    sprintf(gateInfoString, "%s", "");
    sprintf(cAreaMetricsBaseName, "%s", "");
    sprintf(cAreaMetricsFileName, "%s", "");

    if (paAnal->GateArea > 0) {
        sprintf(cAreaMetricsFileName, "%s_k%1.20fAreaMetrics.csv", p->pName, p->pPars->AreaFactor);
        sprintf(cAreaMetricsBaseName, "%s_k%1.20f", p->pName, p->pPars->AreaFactor);
        FILE* fpt;

        //Collecting the string which contains the usage of number of cones
        for (i = 0; i < CM_MAX_DEPTH + 1; i++) {
            sprintf(gateInfoString, "%s%d", gateInfoString, paAnal->GateCount[i]);
            if (i < CM_MAX_DEPTH)
                sprintf(gateInfoString, "%s,", gateInfoString);
        }

        fpt = fopen(cAreaMetricsFileName, "w+");
        fprintf(fpt, "Area_Factor,Gate_area,Gate_count,FileName,");
        fprintf(fpt, "cone_2,cone_3,cone_4,cone_5,cone_6\n");
        fprintf(fpt, "%1.20f,%f,%d,%s", p->pPars->AreaFactor, paAnal->GateAreaAll, paAnal->GateCountAll, cAreaMetricsBaseName);
        fprintf(fpt, "%s", gateInfoString);
        fclose(fpt);
    }
}

ABC_NAMESPACE_IMPL_END
