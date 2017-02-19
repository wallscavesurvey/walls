// HeaderCtrlH.cpp : implementation file
//
#include "stdafx.h"
#ifdef _USE_THEMES
#include "HeaderCtrlH.h"

// CHeaderCtrlH

IMPLEMENT_DYNAMIC(CHeaderCtrlH, CHeaderCtrl)

CHeaderCtrlH::CHeaderCtrlH()
{
}

CHeaderCtrlH::~CHeaderCtrlH()
{
}

BEGIN_MESSAGE_MAP(CHeaderCtrlH, CHeaderCtrl)
ON_MESSAGE(HDM_LAYOUT, OnLayout)
END_MESSAGE_MAP()

LRESULT CHeaderCtrlH::OnLayout( WPARAM wParam, LPARAM lParam )
{
	LRESULT lResult = CHeaderCtrl::DefWindowProc(HDM_LAYOUT, 0, lParam);

	HD_LAYOUT &hdl = *( HD_LAYOUT * ) lParam;
	RECT *prc = hdl.prc;
	WINDOWPOS *pwpos = hdl.pwpos;
	int nHeight=pwpos->cy;

	if(wOSVersion>=0x0600)
		nHeight-=4;

	pwpos->cy = nHeight;
	prc->top = nHeight;

	return lResult;
}
#endif