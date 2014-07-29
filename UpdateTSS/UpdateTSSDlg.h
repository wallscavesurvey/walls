// UpdateTSSDlg.h : header file
//

#pragma once

#ifndef __FILEBUF_H
//#include "filebuf.h"
#endif
#ifndef _DBTFILE_H
//#include "dbtfileRO.h"
#endif
#include "afxwin.h"
//#include "fltrect.h"

typedef std::vector<int> VEC_INT;
typedef std::vector<CString> VEC_CSTR;
typedef std::vector<BYTE> VEC_BYTE;

// CUpdateTSSDlg dialog
class CUpdateTSSDlg : public CDialog
{
// Construction
public:
	CUpdateTSSDlg(CString &csPathName,CWnd* pParent = NULL);	// standard constructor
	~CUpdateTSSDlg();

// Dialog Data
	enum { IDD = IDD_UPDATETSS_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
private:
	CString m_pathINI,m_pathMDB_tmp,m_pathMDB,m_pathDBF;
	LPCSTR m_pNamTable;
	BOOL m_bSaveStats;
	CDaoDatabase m_db;
	CDaoRecordset m_rs;
	CDBFile m_dbf;

	HICON m_hIcon;

 	BOOL OpenDBF();
	BOOL OpenMDB_Table();
	void CloseDBF();
	BOOL OpenMDB();
	void CloseMDB();

	BOOL GetRequiredFlds();
	#ifdef _USE_TEMPLATE
	BOOL ProcessTmp();
	#endif

	void AppendStats(CString &msg);
	int ScanDBF(int *mdb_fldnum,int *mdb_fldsrc,int num_mdb_fields);
	//void StoreRecFld(const FLD_DAT &fdat);

	virtual BOOL OnInitDialog();

	// Generated message map functions
	afx_msg void OnBnClickedBrowseDBF();
	afx_msg void OnBnClickedOk();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};
