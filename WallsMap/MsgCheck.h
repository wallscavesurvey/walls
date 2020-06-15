#pragma once


// CMsgCheck dialog

class CMsgCheck : public CDialog
{
	DECLARE_DYNAMIC(CMsgCheck)

	LPCSTR m_pMsg;
	LPCSTR m_pTitle, m_pPromptMsg;

public:
	CMsgCheck(UINT idd, LPCSTR pMsg, LPCSTR pTitle = NULL, LPCSTR pPromptMsg = NULL, CWnd* pParent = NULL);   // standard constructor
	virtual ~CMsgCheck();

	// Dialog Data
		//enum { IDD = IDD_MSGCHECK };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	BOOL m_bNotAgain;
	virtual BOOL OnInitDialog();
};
