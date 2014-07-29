// UpdateTSS.h : main header file for the PROJECT_NAME application
//

/*
You may not need this next bit of info but I include it for my future reference at least. One
Access problem I ran into and wasted a lot of time with relates to the SEQ autonumber field.
Somehow in my manipulations I created an astronomically large gap in the numbering for the next
added record. It so happens that Access 2007 has a bug that prevents correcting the problem by
compacting the database. I finally came across a fix in the form of some VBA code at
http://support.microsoft.com/kb/287756. That's the TSSTEX_SetSeq.bas file in the attached ZIP.
It should already be incorporated in the MDB I uploaded. To use it in Access you make sure the
TSSTEX table is closed, then open the Visual Basic project window and type the following in the
"Immediate" window to specify that 9835 will be the next SEQ assigned:
 
b=ChangeSeed("TSSTEX","SEQ",9835)
?b
 
The second statement (?b) should cause "true" to display if it worked. Fortunately it did in my
case, but only after I opened the references window and checked the boxes "Microsoft ActiveX
Data Objects 2.1 Library" and "Microsoft ADO Ext 2.1 for DDL and Security." To prevent an error
I also had to uncheck a box for some "missing" reference that apparently wasn't needed. It
wouldn't suprise me if you've already dealt with this, although I know small gaps are nothing
to worry about.

*/

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CUpdateTSSApp:
// See UpdateTSS.cpp for the implementation of this class
//

class CUpdateTSSApp : public CWinApp
{
public:
	CUpdateTSSApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CUpdateTSSApp theApp;