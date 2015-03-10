#pragma once

#include "EditLabel.h"

// CFindDlgTbl

class CFindDlgTbl : public CFindReplaceDialog
{
	DECLARE_DYNAMIC(CFindDlgTbl)

public:
	CFindDlgTbl(LPCSTR fName,int iCol,BYTE fTyp);
	virtual ~CFindDlgTbl();

	CEditLabel m_ce;

	//virtual void OnOK();
	bool m_bSelectAll;
	bool m_bSearchMemos;
	bool m_bSearchRTF;
	bool m_bHasMemos;

	LPCSTR m_pName;
	BYTE m_fTyp;
	int m_iCol;

	void SetColumnData(LPCSTR pNam,int iCol,BYTE fTyp);

	int GetColumnSel()
	{
		return m_cbLookIn.GetCurSel();
	}

private:

	CComboBox m_cbLookIn;
	void EnableControls();
	void Enable(UINT id,BOOL bEnable) {GetDlgItem(id)->EnableWindow(bEnable);}

	afx_msg void OnBnClickedSelectAll();
	afx_msg void OnEnChangeFindWhat();
	afx_msg void OnFindNext();
	afx_msg void OnSelChangeLookIn(); 

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
};


