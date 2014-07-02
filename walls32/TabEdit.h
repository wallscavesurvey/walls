#if !defined(AFX_TABEDIT_H__6E7BC2C0_DECE_11D1_AFAE_000000000000__INCLUDED_)
#define AFX_TABEDIT_H__6E7BC2C0_DECE_11D1_AFAE_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TabEdit.h : header file
//

#ifndef _DLGSH_INCLUDED_
#include <dlgs.h>
#endif

#ifndef _TABCOMBO_H
#include "tabcombo.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabEdit window

class CTabEdit : public CEdit
{
// Construction
public:
	CTabEdit();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTabEdit)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTabEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CTabEdit)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CFindTabDlg dialog

class CFindTabDlg : public CFindReplaceDialog
{
// Construction
public:
	CFindTabDlg(BOOL bFindOnly);   // standard constructor
	CTabCombo m_tabEdit,m_tabReplace;
	BOOL m_bFindOnly;
	BOOL GetUseRegEx() {return ((CButton *)GetDlgItem(IDC_USEREGEX))->GetCheck();}
	LPCSTR GetFindStringPtr() const {
		GetDlgItem(IDC_FINDSTRING)->GetWindowText(m_fr.lpstrFindWhat,128);
		return m_fr.lpstrFindWhat;
	}
	LPCSTR GetReplaceStringPtr() const {
		GetDlgItem(IDC_REPLSTRING)->GetWindowText(m_fr.lpstrReplaceWith,128);
		return m_fr.lpstrReplaceWith;
	}
	void SetUseRegEx(BOOL bUse) {((CButton *)GetDlgItem(IDC_USEREGEX))->SetCheck(bUse);}
	void SetFocusToEdit() {GetDlgItem(edt1)->SetFocus();}

// Dialog Data
	//{{AFX_DATA(CFindTabDlg)
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFindTabDlg)
	//}}AFX_VIRTUAL

// Implementation

protected:
	afx_msg void OnEditchangeFindstring();
	afx_msg void OnSelchangeFindStr();
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CFindTabDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TABEDIT_H__6E7BC2C0_DECE_11D1_AFAE_000000000000__INCLUDED_)
