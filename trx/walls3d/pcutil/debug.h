#ifndef HG_PCUTIL_DEBUG_H
#define HG_PCUTIL_DEBUG_H

#ifdef _DEBUG

#include "../stdafx.h"

extern int verbose;

#define Debug(x) {if (::verbose) { afxDump << ">>" << x;}}
#define DebugNl(x) {if (::verbose) { afxDump << ">>" << x << "\n";}}

#else

#define Debug(x)
#define DebugNl(x)

#endif // _DEBUG

// dummies for UNIX ..

#define DebFixWidth()
#define DebSetWidth(x)


#endif

