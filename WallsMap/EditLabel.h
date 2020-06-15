#pragma once

LPCSTR CheckAccent(UINT wParam, BYTE &bAccent);
void SetLowerNoAccents(LPSTR s);

class CEditLabel : public CEdit
{
	// Construction
public:
	CEditLabel() : m_bAccent(0), m_cmd(0), m_pmes(NULL), m_pParent(NULL) {}

	void SetCustomCmd(UINT cmd, LPCSTR pmes = NULL, CWnd *pParent = NULL) { m_cmd = cmd; m_pmes = pmes; m_pParent = pParent; }
	virtual ~CEditLabel() {}

	// Overrides
	virtual BOOL PreTranslateMessage(MSG* pMsg);

private:
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

	BYTE m_bAccent;
	UINT m_cmd;
	LPCSTR m_pmes;
	CWnd *m_pParent;
	DECLARE_MESSAGE_MAP()
};
