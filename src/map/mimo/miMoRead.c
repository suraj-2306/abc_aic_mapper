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
    fread( pBuffer, nFileSize, 1, pFile );
    pBuffer[ nFileSize ] = '\0';
    fclose( pFile );
    return pBuffer;
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
    if (fVerbose)
        MiMo_PrintLibStatistics(pLib);
    return pLib;
}

ABC_NAMESPACE_IMPL_END
