#pragma once
#include "afxwin.h"
#include "TabCombo.h"
#include "HScrollListBox.h"

// CAdvSearchDlg dialog
class CWallsMapDoc;

class CAdvSearchDlg : public CDialog
{
	DECLARE_DYNAMIC(CAdvSearchDlg)

public:
	CAdvSearchDlg(CWallsMapDoc *pDoc,CWnd* pParent = NULL);   // standard constructor
	virtual ~CAdvSearchDlg();

// Dialog Data
	enum { IDD = IDD_ADVANCED_SRCH };

	WORD m_wFlags;
	CString m_FindString;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

private:
	virtual BOOL OnInitDialog();
	CTabComboHist *m_phist_find;
	BOOL m_bMatchCase;
	BOOL m_bMatchWholeWord;
	CTabCombo m_cbFind;
	BOOL m_rbSelMatched;
	int	m_iSelOpt;
	bool m_bActiveChange,m_bSelFldChange;
	static bool m_bLastViewOnly;

	void Enable(UINT id,BOOL bEnable) {GetDlgItem(id)->EnableWindow(bEnable);}
	void ShowNoMatches();
	void HideNoMatches() {GetDlgItem(IDC_ST_NOMATCHES)->ShowWindow(SW_HIDE);}

	void EnableCurrent(BOOL bEnable)
	{
		Enable(IDC_ADDMATCHED,bEnable);
		Enable(IDC_ADDUNMATCHED,bEnable);
		Enable(IDC_KEEPMATCHED,bEnable);
		Enable(IDC_KEEPUNMATCHED,bEnable);
	}
	void InitLayerSettings();
	void UpdateButtons();
	void UpdateSelFldChange();
	void MoveAllLayers(bool bExclude);
	void MoveOne(CListBox &list1,CListBox &list2,int iSel1);
	void InitFlds();
	void SaveLayerOptions();

	CWallsMapDoc *m_pDoc;
	CHScrollListBox m_list1,m_list2;
	CListBox m_list3,m_list4;

	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedMoveallLeft();
	afx_msg void OnBnClickedMoveallRight();
	afx_msg void OnBnClickedMoveLayerLeft();
	afx_msg void OnBnClickedMoveLayerRight();
	afx_msg void OnSelChangeList2();
	afx_msg void OnBnClickedMoveFldRight();
	afx_msg void OnBnClickedMoveFldLeft();
	afx_msg void OnSelChangeFindCombo();
	afx_msg void OnEditChangeFindCombo();
	afx_msg void OnListDblClk();
	afx_msg LRESULT OnColorListbox(WPARAM wParam,LPARAM lParam);
	afx_msg void OnBnClickedLoadDefault();
	afx_msg void OnBnClickedSaveDefault();
	afx_msg void OnBnClickedMoveAllFldsLeft();
	afx_msg void OnBnClickedMoveAllFldsRight();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lNone);
	afx_msg LRESULT OnClearHistory(WPARAM wNone, LPARAM lNone);

	DECLARE_MESSAGE_MAP()
public:
	BOOL m_bViewOnly;
};
