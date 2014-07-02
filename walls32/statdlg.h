// statdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStatDlg dialog
#ifndef __STATDLG_H
#define __STATDLG_H

#ifndef __NETLIB_H
#include "netlib.h"
#endif

struct SEGSTATS {
	double highpoint;
	double lowpoint;
	double hzlength;
	double vtlength;
	double ttlength;
	UINT   numvecs;
	UINT   numnotes;
	UINT   numflags;
	UINT   idlow;
	UINT   idhigh;
	char   lowstation[NET_SIZNAME+2];
	char   highstation[NET_SIZNAME+2];
	char   refstation[NET_SIZNAME+8];
	char   component[30];
    char   title[128+6]; //Any title + "/..."
};	
	

class CStatDlg : public CDialog
{
// Construction
public:
	CStatDlg(SEGSTATS *st,CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CStatDlg)
	enum { IDD = IDD_STATDLG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Implementation
protected:
    SEGSTATS *m_st;
    
    void SetText(UINT id,char *text)
    {
    	GetDlgItem(id)->SetWindowText(text);
    }
    void SetFloat(UINT id,double f);
    void SetUint(UINT id,UINT u);
    
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
    
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CStatDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnVectorList();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnArcVew();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CExpDlg dialog

class CExpDlg : public CDialog
{
// Construction
public:
	CExpDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CExpDlg)
	enum { IDD = IDD_3DEXPDLG };
	BOOL	m_bView;
	CString m_pathname;
	BOOL	m_bElevGrid;
	CString	m_DimGrid;
	//}}AFX_DATA
	CString m_scaleVert;
	double m_dScaleVert;
	int m_uDimGrid;
	BOOL m_bLRUD;
	BOOL m_bLRUDPT;

// Implementation
private:
	static BOOL m_bLRUD_init,m_bLRUDPT_init;
	afx_msg void OnBrowse(); 
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	// Generated message map functions
	//{{AFX_MSG(CExpDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
