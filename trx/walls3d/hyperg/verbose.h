// This may look like C code, but it is really -*- C++ -*-
// <copyright> 
//  
//  Copyright (c) 1993 
//  Institute for Information Processing and Computer Supported New Media (IICM), 
//  Graz University of Technology, Austria. 
//  
// </copyright> 

// <file> 
// 
// Name:        verbose.h
// 
// Purpose:     verbose (debugging) output
// 
// Created:     11 Jan 94   Michael Pichler
// 
// Modified:    29 Apr 94   Michael Pichler 
// 
// Description: 
//
// Code for verbose messages is generated when VERBOSE is defined (this
// may be done in some files specifically) - code generation can be
// turned off globally by defining NOVERBOSE (e.g. as APP_CCDEFINES).
//
// Debug output is written to cerr, if the global variable verbose is set
// non zero; DEBUG appends a space, DEBUGNL a newline, several output may
// be written at once, separated with <<;
//
// If VERBOSE is not defined or NOVERBOSE is defined, DEBUG has no
// effect.  The variable verbose has to be defined in the program.
//
// </file> 


#ifndef hg_hyperg_verbose_h
#define hg_hyperg_verbose_h


#if defined (VERBOSE) && !defined (NOVERBOSE) && defined(_DEBUG)

#include "../pcutil/debug.h"

#include <iostream.h>

extern int verbose;
#define DEBUG(x)  \
{ if (::verbose)   \
    afxDump << x << ' ';  \
}
#define DEBUGNL(x)  \
{ if (::verbose)   \
    afxDump << x << endl;  \
}

#else

#define DEBUG(x)
#define DEBUGNL(x)

#endif



#endif
