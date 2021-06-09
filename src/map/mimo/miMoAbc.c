/**CFile****************************************************************

  FileName    [miMoAbc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Multiple Input Output Gate Library]

  Synopsis    [Interface to Abc]

  Author      [Martin Thuemmler]
  
  Affiliation [TU Dresden]

  Date        [Ver. 1.0. Started - February 15, 2021.]

***********************************************************************/

#include "miMo.h"
#include "base/main/mainInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int MiMo_CommandReadLibrary(Abc_Frame_t * pAbc, int argc, char **argv);
static int MiMo_CommandPrintLibrary(Abc_Frame_t * pAbc, int argc, char **argv);


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Registers mimolib commands]

  Description []
               
  SideEffects []

  SeeAlso     [Called in MainInit ]

***********************************************************************/
void MiMo_Init ( Abc_Frame_t * pAbc )
{
    Cmd_CommandAdd( pAbc, "Cone Mapping", "read_mimolib", MiMo_CommandReadLibrary, 1);
    Cmd_CommandAdd( pAbc, "Cone Mapping", "print_mimolib", MiMo_CommandPrintLibrary, 0);
    Abc_FrameSetLibMiMo(NULL); 
}

/**Function*************************************************************

  Synopsis    [Frees the memory allocated by the last mimolib]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void MiMo_End ( Abc_Frame_t * pAbc )
{
    if ( Abc_FrameReadLibMiMo() )
        MiMo_LibFree(Abc_FrameReadLibMiMo());
}

/**Function*************************************************************

  Synopsis    [Executes read_mimolib command by invoking MiMo_ReadLibrary]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_CommandReadLibrary(Abc_Frame_t * pAbc, int argc, char **argv)
{
    int fVerbose = 0;
    int c;
    FILE * pFile;
    Extra_UtilGetoptReset();
    while ( (c = Extra_UtilGetopt(argc, argv, "vh")) != EOF ) 
    {
        switch (c) 
        {
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
                break;
            default:
                goto usage;
        }
    }

    if ( argc != globalUtilOptind + 1 )
        goto usage;

    // check that the input file is valid
    char * pFileName = argv[globalUtilOptind];
    if ( (pFile = Io_FileOpen( pFileName, "open_path", "r", 0 )) == NULL )
    {
        Abc_Print(-2,  "Cannot open input file \"%s\". \n", pFileName );
        return 1;
    }
    fclose( pFile );
    MiMo_Library_t * mLib = MiMo_ReadLibrary( pFileName, fVerbose);
    if ( Abc_FrameReadLibMiMo() && mLib )
        MiMo_LibFree(Abc_FrameReadLibMiMo());
    if (mLib)
        Abc_FrameSetLibMiMo(mLib);
    return 0;

usage:
    Abc_Print( -2, "usage: read_mimolib file [-vh]\n");
    Abc_Print( -2, "\t     read multiple input multiple output\n" );  
    Abc_Print( -2, "\t     cell library in custom format\n" );  
    Abc_Print( -2, "\t-v   toggle verbose printout [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h   print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    [Executes print_mimolib: by invoking MiMo_PrintLibrary]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int MiMo_CommandPrintLibrary(Abc_Frame_t * pAbc, int argc, char **argv)
{
    int fVerbose = 0;
    int c;
    Extra_UtilGetoptReset();
    while ( (c = Extra_UtilGetopt(argc, argv, "vh")) != EOF ) 
    {
        switch (c) 
        {
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
                break;
            default:
                goto usage;
        }
    }
    if ( Abc_FrameReadLibMiMo() == NULL)
    {
        printf( "Library is not available.\n");
        return 1;
    }
    MiMo_PrintLibrary( (MiMo_Library_t *)Abc_FrameReadLibMiMo(), fVerbose);
    return 0;

usage:
    Abc_Print( -2, "usage: print_mimolib [-vh]\n");
    Abc_Print( -2, "\t     prints the current mimo library\n" );  
    Abc_Print( -2, "\t-v   toggle verbose output [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h   print command usage\n");
    return 1;
}

ABC_NAMESPACE_IMPL_END
