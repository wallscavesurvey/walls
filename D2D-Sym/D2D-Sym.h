
// D2D-Sym.h : main header file for the D2D-Sym application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols

// CD2DSymApp:
// See D2D-Sym.cpp for the implementation of this class
//

class CD2DSymApp : public CWinApp
{
public:
	CD2DSymApp();


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CD2DSymApp theApp;
