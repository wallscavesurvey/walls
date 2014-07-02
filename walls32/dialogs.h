// dialogs.h : header file
//

#ifndef __DIALOGS_H
#define __DIALOGS_H

#ifndef LINEDOC_H
#include "linedoc.h"
#endif

class CLineNoDlg : public CDialog
{
// Construction
public:
	CLineNoDlg(CWnd* pParent,LINENO nLineNo,LINENO nMaxLineNo);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CLineNoDlg)
	enum { IDD = IDD_LINENODLG };
	LINENO	m_nLineNo;
	//}}AFX_DATA
	LINENO  m_nMaxLineNo;

// Implementation
protected:
	
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
    virtual BOOL OnInitDialog();

	// Generated message map functions
	//{{AFX_MSG(CLineNoDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
class CApplyDlg : public CDialog
{
public:
	CApplyDlg(UINT nID,BOOL *pbNoPrompt,const char *pTitle,CWnd* pParent = NULL);	// standard constructor
	
	BOOL *m_pbNoPrompt;
	
	//{{AFX_DATA(CApplyDlg)
	//enum { IDD = IDD_APPLYTOALL };
	BOOL	m_bNoPrompt;
	//}}AFX_DATA
protected:
    const char *m_pTitle;
	// Generated message map functions
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//{{AFX_MSG(CApplyDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CErrorDlg dialog
class CErrorDlg : public CDialog
{
// Construction
public:
	CErrorDlg(LPCSTR msg,CWnd* pParent = NULL,LPCSTR pTitle=NULL);   // standard constructor

// Dialog Data
	LPCSTR m_msg,m_title;

	//{{AFX_DATA(CErrorDlg)
	enum { IDD = IDD_ERRORDLG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	LRESULT OnRetFocus(WPARAM bSubclass,LPARAM);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CErrorDlg)
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CErrorDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CMoveDlg dialog

class CMoveDlg : public CDialog
{
// Construction
public:

// Dialog Data
	//{{AFX_DATA(CMoveDlg)
	enum { IDD = IDD_MOVEBRANCH };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	CMoveDlg(BOOL bMove,CWnd* pParent = NULL) : 
		CDialog(CMoveDlg::IDD, pParent),m_bCopy(bMove) {}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMoveDlg)
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL m_bCopy;

	// Generated message map functions
	//{{AFX_MSG(CMoveDlg)
	afx_msg void OnLinkToFiles();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CMapHelpDlg dialog

class CMapHelpDlg : public CDialog
{
// Construction
public:
	CMapHelpDlg(LPCSTR title,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMapHelpDlg)
	enum { IDD = IDD_MAPHELP };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapHelpDlg)
	//}}AFX_VIRTUAL

// Implementation
protected:
	LPCSTR m_Title;

	// Generated message map functions
	//{{AFX_MSG(CMapHelpDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CFileStatDlg dialog

class CFileStatDlg : public CDialog
{
// Construction
public:
	CFileStatDlg(LPCSTR pPathName,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CFileStatDlg)
	enum { IDD = IDD_FILESTATDLG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFileStatDlg)
	//}}AFX_VIRTUAL

// Implementation
protected:

	LPCSTR m_pPathName;

	// Generated message map functions
	//{{AFX_MSG(CFileStatDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CNotLoopDlg dialog

class CNotLoopDlg : public CDialog
{
// Construction
public:
	CNotLoopDlg(LPCSTR pMsg,LPCSTR pButtonMsg,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNotLoopDlg)
	enum { IDD = IDD_NOTINLOOP };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNotLoopDlg)
	//}}AFX_VIRTUAL

// Implementation
protected:
	LPCSTR m_pMsg;
	LPCSTR m_pButtonMsg;

	// Generated message map functions
	//{{AFX_MSG(CNotLoopDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class COpenExDlg : public CDialog
{
// Construction
public:
	COpenExDlg(CString &msg,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	CString &m_msg;

	//{{AFX_DATA(CErrorDlg)
	enum { IDD = IDD_OPENEXISTING };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CErrorDlg)
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CErrorDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnOpenExisting();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif


#pragma once


// CDeleteDlg dialog

class CDeleteDlg : public CDialog
{
	DECLARE_DYNAMIC(CDeleteDlg)

public:
	CDeleteDlg(LPCSTR pName,CWnd* pParent = NULL);   // standard constructor
	virtual ~CDeleteDlg();

// Dialog Data
	enum { IDD = IDD_DELETEDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	LPCSTR m_pName;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedDeleteFiles();
	virtual BOOL OnInitDialog();
};
