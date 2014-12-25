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

	~CXlsExportDlg() {if(m_psd) delete m_psd;}

// Dialog Data
	enum { IDD = IDD_EXPORT_DIALOG };

// Implementation

private:
	UINT m_nLogMsgs;
	CString m_pathBuf;
	CString m_fldKeyVal;
	CString m_tableName,m_sourcePath,m_shpdefPath,m_shpPath;
	UINT m_nPathLen;

	char m_szModule[_MAX_PATH];
	char m_szSourcePath[_MAX_PATH];
	char m_szShpdefPath[_MAX_PATH];
	char m_szShpPath[_MAX_PATH];

	LPSTR m_pSourceName,m_pShpdefName,m_pShpName;

	CShpDef *m_psd;
	CDaoDatabase m_database;
	CDaoRecordset m_rs;
	CStdioFile m_csvfile;
	V_CSTRING m_vcsv;

	BOOL m_bLatLon;
	BOOL m_bXLS;
	int m_iDatum;
	
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

	LPCSTR FixShpPath(LPCSTR ext);
	bool GetCsvRecord();
	int InitShapefile();
	void InitFldKey();
	int GetCoordinates(double *pLat, double *pLon, double *pEast, double *pNorth, int *pZone);
	int StoreFldData(int f);
	int WriteShpRec(double x,double y); //also appends blank record to dbf
	static int StoreDouble(DBF_FLDDEF &fd, int fld, double d);
	static void StoreText(int fld,LPCSTR p);
	void WritePrjFile(BOOL bNad83, int utm_zone);
	void WriteLog(const char *format,...);
	int errexit(int e,int typ);
	void DeleteFiles();
	int CloseFiles();
	void SaveOptions();

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
	afx_msg void OnBnClickedBrowseShpdef();
	afx_msg void OnBnClickedBrowseShp();
};
