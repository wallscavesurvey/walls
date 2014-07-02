#pragma once

// CDlgProperties dialog

class CWallsMapDoc;

class CDlgProperties : public CDialog
{
	DECLARE_DYNAMIC(CDlgProperties)

public:
	CDlgProperties(CWallsMapDoc *pDoc,CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgProperties();

// Dialog Data
	enum { IDD = IDD_FILE_PROPERTIES };

private:
	CWallsMapDoc *m_pDoc;	

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};
