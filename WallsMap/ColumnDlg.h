#pragma once
#include "DBGridDlg.h"
#include "ListCtrlEx.h"

// CColumnDlg dialog

struct CDBField
{
	CDBField() : pInfo(NULL), wFld(0), bVis(false) {}
	CDBGridDlg::FldInfo *pInfo;
	WORD wFld;
	bool bVis; 
};

class CColumnDlg : public CResizeDlg
{
	DECLARE_DYNAMIC(CColumnDlg)

public:
	CColumnDlg(CDBField *pFld,int nFlds,CWnd* pParent = NULL);   // standard constructor
	virtual ~CColumnDlg();

// Dialog Data
	enum { IDD = IDD_COLUMNS };

	CDBField *m_pFld; //written and read exernally
	int m_nFlds;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnMoveItem(WPARAM mode, LPARAM pt);
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

private:
	CListCtrlEx m_List;

	virtual BOOL OnInitDialog();
public:
	afx_msg void OnBnClickedResetrows();
};
