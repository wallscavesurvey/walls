#if !defined(AFX_EXPAVDLG_H__FC084DF1_200D_11D2_8AFE_000000000000__INCLUDED_)
#define AFX_EXPAVDLG_H__FC084DF1_200D_11D2_8AFE_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ExpavDlg.h : header file
//
#ifndef __STATDLG_H
#include "statdlg.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// CExpavDlg dialog

class CExpavDlg : public CDialog
{
// Construction
public:
	CExpavDlg(CSegListNode *pNode,SEGSTATS *st,CWnd* pParent = NULL);   // standard constructor
	void StatusMsg(int veccount);
// Dialog Data
	//{{AFX_DATA(CExpavDlg)
	enum { IDD = IDD_EXPAVDLG };
	BOOL	m_bFlags;
	BOOL	m_bNotes;
	BOOL	m_bStations;
	BOOL	m_bVectors;
	CString	m_pathname;
	CString	m_shpbase;
#ifdef _USE_SHPX
	BOOL	m_bWallshpx;
#endif
	BOOL	m_bWalls;
	BOOL	m_b3DType;
	BOOL	m_bUTM;
	//}}AFX_DATA

	void Enable(int id,BOOL bEnable)
	{
	   GetDlgItem(id)->EnableWindow(bEnable);

	}
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExpavDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

#ifdef _USE_SHPX
	BOOL m_bWallshpx_enable;
#endif

protected:

	SEGSTATS *m_st;
	CSegListNode *m_pNode;
	UINT m_uTypes;

	// Generated message map functions
	LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	//{{AFX_MSG(CExpavDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowse();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXPAVDLG_H__FC084DF1_200D_11D2_8AFE_000000000000__INCLUDED_)
