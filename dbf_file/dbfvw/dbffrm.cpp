// dbffrm.cpp : implementation of the CDbfFrame class
//
#include "stdafx.h"
#include "dbfvw.h"

IMPLEMENT_DYNCREATE(CDbfFrame, CMDIChildWnd)

#define SIZ_WINDOW_NAME 20
#define SIZ_FILE_EXT 3  //must be fixed

BOOL CDbfFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// By turning off the default MFC-defined FWS_ADDTOTITLE style,
	// the framework will use first string in the document template
	// STRINGTABLE resource instead of the document name.
	// cs.style &= ~(LONG)FWS_ADDTOTITLE;
    cs.dwExStyle&=~(DWORD)(WS_EX_CLIENTEDGE|WS_EX_WINDOWEDGE);
	if(!CMDIChildWnd::PreCreateWindow(cs)) return FALSE;
    cs.dwExStyle&=~(DWORD)(WS_EX_CLIENTEDGE|WS_EX_WINDOWEDGE);

    //if(cs.lpszName) cs.cx=CDbfView::InitialFrameWidth();
	return TRUE;	  
}

