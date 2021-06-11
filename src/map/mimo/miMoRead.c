/**CFile****************************************************************

  FileName    [miMoRead.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Multiple Input Output Gate Library]

  Synopsis    [Input file reading functionality]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

***********************************************************************/

#include "miMo.h"

ABC_NAMESPACE_IMPL_START

static inline char * MiMo_ReadFile(char *pFileName, int *bufferLength);
static inline void MiMo_ReadPreprocess(char *pStr);
static inline MiMo_GateType_t MiMo_ReadParseGateType(char *pStr);

static inline MiMo_Gate_t *  MiMo_ReadParseGateBegin(MiMo_Library_t * pLib, char * pLine);
static inline int MiMo_ReadParseGateInputs(MiMo_Gate_t * pGate, char * pLine);
static inline int MiMo_ReadParseGateOutputs(MiMo_Gate_t * pGate, char * pLine);
static inline int MiMo_ReadTryToParseFloat(char * pStr, float *val);
static inline int MiMo_ReadParseDelayList(MiMo_Gate_t * pGate, char * pLine);

/**Function*************************************************************

  Synopsis    [Returns a new string without comments (#),
               line extenstions (\), duplicated spaces, and empty lines]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_ReadPreprocess(char *pStrIn )
{
    char *pStrOut = pStrIn;
    int fLineEnding = 0;
    int fSpaceBefore = 0;
    while(*pStrIn)
    {
        fLineEnding = 0;
        if ( *pStrIn == '\\' )
        {
            while(*pStrIn && *pStrIn != '\r' && *pStrIn != '\n')
                pStrIn++;
            while(*pStrIn == '\r' || *pStrIn == '\n')
                pStrIn++;
        }
        while(*pStrIn && (*pStrIn == '\r' || *pStrIn == '\n'))
        {
            fLineEnding = 1;
            pStrIn++;
        }
        while (*pStrIn && *pStrIn == '#')
        {
            fLineEnding = 1;
            while(*pStrIn && *pStrIn != '\r' && *pStrIn != '\n')
                pStrIn++;
            while(*pStrIn == '\r' || *pStrIn == '\n')
                pStrIn++;
        }
        if (fLineEnding)
        {
            *pStrOut++ = '\n';
            fSpaceBefore = 0;
        }

        if ( *pStrIn != ' ' )
        {
            fSpaceBefore = 0;
            *pStrOut++ = *pStrIn++;
        }
        else
        {
            if ( fSpaceBefore )
                pStrIn++;
            else
                *pStrOut++ = *pStrIn++;
            fSpaceBefore = 1;
        }
    }
    *pStrOut = '\0';
}


/**Function*************************************************************

  Synopsis    [Returns string of file content ( pFilename) and sets bufferLength]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * MiMo_ReadFile (char *pFileName, int *bufferLength )
{
    FILE * pFile;
    pFile = Io_FileOpen( pFileName, "open_path", "rb", 1 );
    // if we got this far, file should be okay otherwise would
    // have been detected by caller
    assert ( pFile != NULL );
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    int nFileSize = ftell( pFile );  
    // move the file current reading position to the beginning
    rewind( pFile ); 
    // load the contents of the file into memory
    char *pBuffer = ABC_ALLOC( char, nFileSize + 1 );
    *bufferLength = nFileSize + 1;
    int fOk = fread( pBuffer, nFileSize, 1, pFile );
    assert(fOk);
    pBuffer[ nFileSize ] = '\0';
    fclose( pFile );
    return pBuffer;
}


/**Function*************************************************************

  Synopsis    [Converts string into gate type]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_GateType_t MiMo_ReadParseGateType(char * pStr)
{
    if ( !strcmp(pStr, "AIC2") )
        return MIMO_AIC2;
    if ( !strcmp(pStr, "AIC3") )
        return MIMO_AIC3;
    if ( !strcmp(pStr, "NNC2") )
        return MIMO_NNC2;
    if ( !strcmp(pStr, "NNC3") )
        return MIMO_NNC3;
    return MIMO_GENERIC;
}

/**Function*************************************************************

  Synopsis    [Parses first line of gate description]

  Description [Returns 1 on success; Format: name type area]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_Gate_t *  MiMo_ReadParseGateBegin(MiMo_Library_t * pLib, char * pLine)
{
    printf("Input line: %s\n", pLine);
    char wd [] = "\t ";
    char * pWord = strtok(pLine, wd);
    if (!pWord || ! *pWord)
        return NULL;
    MiMo_Gate_t * pGate = MiMo_LibCreateGate(pLib, pWord);
    if (! *pWord)
        return pGate;
    pWord = strtok(NULL, wd);
    if (! pWord )
        return pGate;
    pGate->Type = MiMo_ReadParseGateType(pWord);
    pWord = strtok(NULL, wd);
    if (!pWord || ! *pWord)
        return pGate;
    pGate->Area = atof(pWord);
    return pGate;
}

/**Function*************************************************************

  Synopsis    [Parses input definition line of gate description]

  Description [Returns 1 on success; Format [pinInputName]+ ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_ReadParseGateInputs(MiMo_Gate_t * pGate, char * pLine)
{
    char wd [] = "\t ";
    char * pWord = strtok(pLine, wd);
    while ( pWord )
    {
        MiMo_GateCreatePinIn(pGate, pWord);
        pWord = strtok(NULL, wd);
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses output definition line of gate description]

  Description [Returns 1 on success; Format [pinOutputName]+ ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_ReadParseGateOutputs(MiMo_Gate_t * pGate, char * pLine)
{
    char wd [] = "\t ";
    char * pWord = strtok(pLine, wd);
    while ( pWord )
    {
        MiMo_GateCreatePinOut(pGate, pWord);
        pWord = strtok(NULL, wd);
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Tries to convert string into float, returns 1 on success]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_ReadTryToParseFloat(char * pStr, float *val)
{
    char *pEnd;
    float result = strtof(pStr, &pEnd);
    if ( pEnd != (pStr + strlen(pStr)) )
        return 0;
    *val = result;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses the delay description line]

  Description [Returns 1 on success; Format [outPin [inPins]+ delay ]+]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_ReadParseDelayList(MiMo_Gate_t * pGate, char * pLine)
{
    char wd [] = "\t ";
    int fOut = 1;
    char * pWord = strtok(pLine, wd);
    MiMo_PinOut_t * pPinOut;
    MiMo_PinDelay_t * pInitialDelayList;
    float delay = 0;
    while ( pWord )
    {
        if (fOut)
        {
            pPinOut = MiMo_GateFindPinOut(pGate, pWord);
            if ( ! pPinOut )
            {
                printf("Pinout %s not in gate %s found\n", pWord, pGate->pName);
                return 0;
            }
            pInitialDelayList = pPinOut->pDelayList;
            fOut = 0;
        }
        else
        {
            if (MiMo_ReadTryToParseFloat(pWord, &delay))
            {
                MiMo_DelayListSetDelay(pPinOut, pInitialDelayList, delay);
                fOut = 1;
            } else 
                MiMo_DelayListAdd(pGate, pPinOut, pWord);
        }
        pWord = strtok(NULL, wd);
    }
    return 1;
}
 

/**Function*************************************************************

  Synopsis    [Parses a single gate description and adds the resulting
               gate to given library]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_Gate_t *  MiMo_ReadParseGate(MiMo_Library_t * pLib, char *pLines [])
{
    MiMo_Gate_t * pGate = MiMo_ReadParseGateBegin(pLib, pLines[0]);
    int fOk = 0;
    if ( pGate )
        fOk = MiMo_ReadParseGateInputs(pGate, pLines[1]);
    if ( fOk )
        fOk = MiMo_ReadParseGateOutputs(pGate, pLines[2]);
    if ( fOk )
        fOk = MiMo_ReadParseDelayList(pGate, pLines[3]);
    return fOk ? pGate : NULL;
}

/**Function*************************************************************

  Synopsis    [Creates a new mimolibrary from given file]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
MiMo_Library_t * MiMo_ReadLibrary(char *pFileName, int fVerbose)
{

    MiMo_Library_t * pLib = MiMo_LibStart(Extra_FileNameWithoutPath(pFileName));

    int length = 0;
    char *pStr = MiMo_ReadFile(pFileName, &length);
    MiMo_ReadPreprocess(pStr);

    char * pLine = strtok(pStr, "\r\n");
    while ( pLine )
    {
        // 0.line -> name, type area
        // 1.line -> inputs 
        // 2.line -> outputs
        // 3.line -> [output time inputsList]+
        char * pParseLines[4] = { pLine, NULL, NULL, NULL};
        for(int i=1; i<4 && pLine; i++)
        {
            int lineLength = strlen(pLine);
            char *pNextLine = pLine + lineLength + 1;
            pParseLines[i] = pNextLine;
            pLine = strtok(pNextLine, "\r\n");
        }
        if ( !pParseLines[3] )
            break;
        pLine = strtok(pLine + strlen(pLine) + 1, "\r\n");
        MiMo_ReadParseGate(pLib, pParseLines);
    }
    ABC_FREE(pStr);
    MiMo_LibCheck(pLib);

    if (fVerbose)
        MiMo_PrintLibStatistics(pLib);
    return pLib;
}

ABC_NAMESPACE_IMPL_END
