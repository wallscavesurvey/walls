//////////////////////////////////////////////////////////////////////
// GridDropTarget.h : header file
//
// MFC Grid Control - Drag/Drop target implementation
//
// Written by Chris Maunder <chris@codeproject.com>
// Copyright (c) 1998-2005. All Rights Reserved.
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name and all copyright 
// notices remains intact. 
//
// An email letting me know how you are using it would be nice as well. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability for any damage/loss of business that
// this product may cause.
//
// For use with CGridCtrl v2.10+
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEDROPTARGET_H__5C610981_BD36_11D1_97CD_00A0243D1382__INCLUDED_)
#define AFX_FILEDROPTARGET_H__5C610981_BD36_11D1_97CD_00A0243D1382__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include <afxole.h>

class CImageViewDlg;
class CMainFrame;

typedef std::vector<CString> V_FILELIST;

/////////////////////////////////////////////////////////////////////////////
// CFileDropTarget command target

class CFileDropTarget : public COleDropTarget
{
public:
	CFileDropTarget();

	virtual ~CFileDropTarget();
	virtual BOOL Register(CWnd *pWnd, DROPEFFECT dwEffect = DROPEFFECT_NONE);
	CMainFrame *m_pMainFrame;
	static V_FILELIST m_vFiles;
	BOOL ReadHdropData(COleDataObject* pDataObject);

	bool m_bAddingLayers;

#ifdef _DEBUG
	bool m_bTesting;
#endif

	static void FileTypeErrorMsg();

	// Attributes
private:
	CWnd *m_pWndRegistered;
	IDropTargetHelper* m_piDropHelper;
	bool m_bUseDnDHelper;
	DROPEFFECT m_dwEffect;
	static UINT m_nOutside;
	static bool m_bLinking;

	// Operations
public:
	// Overrides
	virtual void Revoke();

	virtual BOOL        OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	virtual DROPEFFECT  OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual void        OnDragLeave(CWnd* pWnd);
	virtual DROPEFFECT  OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRIDDROPTARGET_H__5C610981_BD36_11D1_97CD_00A0243D1382__INCLUDED_)
