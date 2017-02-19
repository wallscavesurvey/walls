#pragma once


// CFldsIgnoreDlg dialog

class CFldsIgnoreDlg : public CDialog
{
	DECLARE_DYNAMIC(CFldsIgnoreDlg)
	VEC_WORD &m_vFldsIgnored;
	CShpLayer *m_pShp;
	CListBox m_lbFldsCompared;
	CListBox m_lbFldsIgnored;

public:
	CFldsIgnoreDlg(CShpLayer *pShp,VEC_WORD &vFldsIgnored,CWnd* pParent = NULL);   // standard constructor
	virtual ~CFldsIgnoreDlg();

// Dialog Data
	enum { IDD = IDD_FLDS_IGNORED };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void Enable(UINT id,BOOL bEnable) {GetDlgItem(id)->EnableWindow(bEnable);}

	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();

	void UpdateButtons();
	void UpdateSelFldChange();
	void MoveAllFields(CListBox &list);
	void MoveOne(CListBox &list1,CListBox &list2,int iSel1,BOOL bToRight);

	afx_msg void OnBnClickedMoveallLeft();
	afx_msg void OnBnClickedMoveallRight();
	afx_msg void OnBnClickedMoveFldLeft();
	afx_msg void OnBnClickedMoveFldRight();
};
