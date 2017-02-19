#pragma once

class CModeless : public CDialog
{
	DECLARE_DYNAMIC(CModeless)

public:
	CModeless(LPCSTR title, LPCSTR msg, UINT delay);   // standard constructor
	virtual ~CModeless();

// Dialog Data
	enum { IDD = IDD_MODELESS_WIN };

	static CModeless *m_pModeless;
	void CloseOnTimeout();

protected:

	DECLARE_MESSAGE_MAP()
private:
	afx_msg void OnTimer(UINT nIDEvent);
	virtual void PostNcDestroy();
	bool m_bCloseOnTimeout;
	UINT_PTR m_nTimer;
};

#define ModelessClose() CModeless::m_pModeless->CloseOnTimeout()
