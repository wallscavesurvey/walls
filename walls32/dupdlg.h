// dupdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDupDlg dialog

typedef struct {
  char *pName;
  char File[10];
  DWORD nLine;
} DUPLOC;

class CDupDlg : public CDialog
{
// Construction
public:
	CDupDlg(DUPLOC *pLoc1,DUPLOC *pLoc2,CWnd* pParent = NULL);	
// Dialog Data
	//{{AFX_DATA(CDupDlg)
	enum { IDD = IDD_DUPDLG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA 
	
// Implementation
protected:
    DUPLOC *m_pLoc1;
    DUPLOC *m_pLoc2;
    BOOL OnInitDialog();
	//{{AFX_MSG(CDupDlg)
	afx_msg void OnAbort();
	afx_msg void OnDupEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
