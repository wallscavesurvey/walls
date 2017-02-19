
#if !defined(_TABCOMBO_H)
#define _TABCOMBO_H
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "EditLabel.h"

typedef void (* TABCOMBO_CB)(void);

enum {F_MATCHCASE=1,F_MATCHWHOLEWORD=2,F_MATCHMASK=3,F_SEARCHUP=4,F_MATCHRTF=8,F_VIEWONLY=(1<<14),F_USEACTIVE=(1<<15)};
#define FTYP_SHIFT 2
#define FTYP_MASK 7
#define GET_FTYP(flags) ((flags>>FTYP_SHIFT)&FTYP_MASK)
enum {FTYP_SELMATCHED,FTYP_SELUNMATCHED,FTYP_ADDMATCHED,FTYP_ADDUNMATCHED,FTYP_KEEPMATCHED,FTYP_KEEPUNMATCHED};

#define MAX_COMBOHISTITEMS 8

class CComboHistItem
{
public:
	CComboHistItem(CComboHistItem *pNext,LPCSTR pstr,WORD flags=0) : m_pNext(pNext),m_wFlags(flags),m_Str(pstr) {};
	virtual ~CComboHistItem() {
		delete m_pNext;
		m_pNext=NULL;
	}
	CComboHistItem *m_pNext;
	WORD m_wFlags;
	CString m_Str;
};

class CTabComboHist
{
public:
	CTabComboHist(int maxItems) : m_nMaxItems(maxItems),m_nItems(0),m_pFirst(NULL) {}

	virtual ~CTabComboHist() {
		delete m_pFirst;
	}
	bool InsertAsLast(LPCSTR pstr,WORD flags=0);
	void InsertAsFirst(LPCSTR pstr,WORD flags=0);
	BOOL IsEmpty() {return m_nItems==0;}
	void SetEmpty() {delete m_pFirst; m_pFirst=NULL; }
	LPCSTR GetFirstString() {return m_nItems?m_pFirst->m_Str:"";}
	WORD GetFirstFlags() {return m_nItems?m_pFirst->m_wFlags:0;}
	CComboHistItem *GetItem(int index);
	int m_nItems;
	int m_nMaxItems;
	CComboHistItem *m_pFirst;
};

class CTabCombo : public CComboBox
{
// Construction
public:
	CTabCombo() {};
	virtual ~CTabCombo();

	void LoadHistory(CTabComboHist *phist_find);
	void ClearListItems();
	void SetCustomCmd(UINT cmd,LPCSTR pmes,CWnd *pParent)
	{
		m_editLabel.SetCustomCmd(cmd,pmes,pParent);
	}

private:
	CEditLabel m_editLabel;

protected:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()
};

void OnSelChangeFindStr(CDialog *pDlg,CTabComboHist *phist_find);

#endif
