// XlsExportDlg.h : header file
//

#pragma once

#include "resource.h"
#include "utility.h"
#include "filebuf.h"
#include "XlsShpDef.h"

UINT dbt_AppendRec(LPCSTR pData); //defined in  XlsShpfile.cpp

#define SHP_ERR_WRITE -1
#define SHP_ERR_CREATE -2

// CXlsExportDlg dialog
class CXlsExportDlg : public CDialog
{
// Construction
public:
	CXlsExportDlg(int argc, char **argv, CWnd* pParent = NULL);

// Dialog Data
	enum { IDD = IDD_EXPORT_DIALOG };

// Implementation

private:
	UINT m_nLogMsgs;
	CString m_dbPath;
	CString m_shpName;
	CString m_tableName;
	CString m_pathBuf;
	CString m_fldKeyVal;
	UINT m_nPathLen;
	LPCSTR m_pdbName;

	CShpDef m_shpdef;
	CDaoDatabase m_database;
	CDaoRecordset m_rs;
	CStdioFile m_csvfile;
	V_CSTRING m_vcsv;

	BOOL m_bXLS;
	
	CShpDef m_sd;
	CFileBuffer m_fbLog;
	BOOL m_bLogFailed;
	HICON m_hIcon;

	void CloseDB()
	{
		if(m_bXLS<2) {
			m_rs.Close();
			m_database.Close();
		}
		else m_csvfile.Close();
	}

	double GetDbDouble(int f)
	{
		return (m_bXLS<2)?GetDouble(m_rs,f):atof(m_vcsv[f]);
	}

	long GetDbLong(int f)
	{
		return (m_bXLS<2)?GetLong(m_rs,f):atoi(m_vcsv[f]);
	}

	int GetDbText(LPSTR pBuf, int fSrc, int fLen)
	{
		if(m_bXLS<2) return GetText(pBuf, m_rs, fSrc, fLen);
		trx_Stncc(pBuf,m_vcsv[fSrc],fLen+1);
		return m_vcsv[fSrc].GetLength();
	}

	void GetDbBoolStr(LPSTR pBuf, int fSrc)
	{
		if(m_bXLS<2) GetBoolStr(pBuf, m_rs, fSrc);
		else GetBoolCharVal(pBuf,*(LPCSTR)m_vcsv[fSrc]);
	}

	LPCSTR FixPath(LPCSTR name, LPCSTR ext);
	LPCSTR FixShpPath(LPCSTR ext) { return FixPath(m_shpName,ext); }
	bool GetCsvRecord();
	int InitShapefile();
	void InitFldKey();
	int GetCoordinates(double *pLat, double *pLon);
	int StoreFldData(int f);
	int WriteShpRec(double fLat,double fLon); //also appends blank record to dbf
	static int StoreDouble(DBF_FLDDEF &fd, int fld, double d);
	static void StoreText(int fld,LPCSTR p);
	void WritePrjFile(bool bNad83, int utm_zone);
	void WriteLog(const char *format,...);
	int errexit(int e,int typ);
	void DeleteFiles();
	int CloseFiles();

	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnInitDialog();

	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedBrowse();
	DECLARE_MESSAGE_MAP()

public:
	int m_argc;
	char **m_argv;
};
