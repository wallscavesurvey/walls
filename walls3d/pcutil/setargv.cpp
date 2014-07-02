#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>
//#include <malloc.h>

#if defined(_DEBUG) & defined(_VERBOSE)
#include <afxwin.h>
#define new DEBUG_NEW
#endif


#define PSPCMDLEN   0x80
#define PSPENV      0x2C
#define MAXFNAMELEN 80

#define CR          '\r'
#define EOS         '\0'
#define QUOTE       '"'
#define SPACE       ' '
#define QUESTION    '?'
#define ASTERICK    '*'
#define TRUE        1
#define FALSE       0


// This function parses a commandline like string and splits its options in
// an array of strings.
//
void setArgV(   const char *pszCmdLine, 
                const char *argV0,                  // program name
                int &argC,
                char **&argV
            )
{
    const char *farCmdLine;
    char *cmdLine;
    char *saveCmd;
    char *endPoint;
    char *start;
    int inquote, done, found;
    int length;
                             
    argC = 1;
    argV = NULL;
                             
    farCmdLine = pszCmdLine;
    length = strlen(farCmdLine);

//    saveCmd = cmdLine = (char *)malloc((length)+1);
    saveCmd = cmdLine = new char[length+1];

    strncpy(cmdLine, farCmdLine, (length)+1);

    endPoint = cmdLine + length;

    while (*cmdLine == SPACE)           // strip leading white space
        *cmdLine++ = EOS;

    inquote = done = FALSE;
    start = cmdLine;
    while (!done)
    {
        found = 0;
        while(*cmdLine != EOS)
        {
            switch (*cmdLine)
            {
                case QUOTE:
                    // If not in quotes then one of two possibilities:
                    // 1) Start of new argument
                    // 2) End of argument and start of new one
                    if (!inquote)
                    {
                        if (start == cmdLine)
                        {
                            inquote = TRUE;   // in qoutes
                            *cmdLine = EOS;   // overwrite quote char
                            cmdLine++;
                            start = cmdLine;
                        }
                        else
                        {
                            *cmdLine = EOS;   // overwrite quote char
                            inquote = TRUE;   // but note that we got it
                        }
                    }
                    // Else were in quotes
                    else
                    {
                        *cmdLine = EOS;       // overwrite quote char
                        inquote = FALSE;      // and out of quotes
                    }
                    break;

                case SPACE:
                    // If not in quotes then overwtite, else increment
                    // and note that we are in an argument
                    if (!inquote)
                        *cmdLine = EOS;
                    else
                    {
                        found++;
                        cmdLine++;
                    }
                    break;

                case CR:
                    // Overwite and set done flag
                    *cmdLine = EOS;
                    done = TRUE;
                    break;

                default:
                    // Otherwise increment and note that we have an argument
                    cmdLine++;
                    found++;
                    break;
            }
        }

        if (cmdLine >= endPoint)
            done = TRUE;

        if (!done)
        {
            cmdLine++;          // move past NULL
            if (!inquote)       // skip spaces if we didn't start quotes
                while (*cmdLine == SPACE)
                    *cmdLine++ = EOS;
            start = cmdLine;
        }

        if (found)
            argC++;
    }

/*========================================================================
 *  1) number of arguments
 *  2) size of arguments (slightly bloated)
 *  3) length needed for Program path name
 *
 *  So, ((#1)*sizeof(* argV))+(#2)+(#3) should be total size
 *========================================================================
 */

// set curptr to first byte past last argument pointer
// set up argV[0]: copy commandline into beginning of buffer and update curptr
// loop: set argV[i] and copy next argument to buffer updating curptr
//          
    char *start2;

    argV = (char **) new char[(sizeof(*argV)*(argC+1))+(length)+strlen(argV0)+2];
    start2 = argV[0] = (char *) &(argV[argC+1]);
    strcpy(start2, argV0);
    start2 += strlen(start2)+1;
    cmdLine = saveCmd;

    for (done=1; done < argC; done++)
    {
        argV[done] = (char *)start2;
        while (*cmdLine == EOS)     // skip to next argument
            cmdLine++;
            
        strcpy(start2, cmdLine);
        found = strlen(start2)+1;
        start2 += found;
        cmdLine += found;
    }
    argV[done] = NULL;
    delete saveCmd;
    
    return;
} // setArgV


extern "C" void setArg(const char *pszCmdLine, 
                       const char *argV0,                  // program name
                       int* argC,
                       char ***argV
                      )
{
    setArgV(pszCmdLine, argV0, *argC, *argV);
}
