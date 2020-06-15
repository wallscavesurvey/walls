#pragma once
#include "afxwin.h"

// CExportNTI_Adv dialog

#ifdef _USE_LZMA
enum { M_loco, M_lzma, M_zlib, M_jp2k, M_webp }; //order of radio button for methods
#else
enum { M_loco, M_zlib, M_jp2k, M_webp }; //order of radio button for methods
#endif

typedef struct {
	WORD bMethod;
	WORD bFilter;
	WORD bOverwrite;
	WORD iTolerance;
	WORD bPreset;
	WORD bGet_ssim;
	float fQuality;
	float fRate;
} NTI_ADV_PARAMS;

class CExportNTI_Adv : public CDialog
{
	DECLARE_DYNAMIC(CExportNTI_Adv)

public:
	CExportNTI_Adv(CImageLayer *pLayer, BOOL bSSIM_ok, CWnd* pParent = NULL);   // standard constructor
	virtual ~CExportNTI_Adv();

	static void LoadAdvParams();
	static void SaveAdvParams();
	static BOOL m_AdvParamsLoaded;
	static BOOL m_bNoOverviews;
	static NTI_ADV_PARAMS m_Adv;
	static bool IsLossyMethod() { return m_Adv.bMethod == NTI_FCNWEBP || m_Adv.bMethod == NTI_FCNJP2K && m_Adv.fRate >= 0.0f; }
	static bool CExportNTI_Adv::GetRateStr(CString &args);

	// Dialog Data
	enum { IDD = IDD_EXPORT_NTI_ADV2 };

private:
	void Enable(UINT id, BOOL bEnable) { GetDlgItem(id)->EnableWindow(bEnable); }
	void EnableControls();
	bool CheckRate();
	float ParseRate();

	BOOL m_bSSIM_ok;
	bool m_bRateLossless;
	BOOL ComparableWithSSIM()
	{
		return m_bSSIM_ok && (m_bMethod == M_webp || (m_bMethod == M_jp2k && !m_bRateLossless));
	}
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	CComboBox m_cbTolerance;
	CComboBox m_cbQuality;
	CString m_csQuality;
	CComboBox m_cbRate;
	CString m_csRate;

public:
	BOOL m_bOverwrite;
	BOOL m_bMethod;
	BOOL m_bFilter;
	CImageLayer *m_pLayer;
	BOOL m_bPreset, m_bGet_ssim;

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedDefault();
	afx_msg void OnChangeJP2KRate();
	afx_msg void OnBnClickedMethodLoco();
#ifdef _USE_LZMA
	afx_msg void OnBnClickedMethodLzma();
#endif
	afx_msg void OnBnClickedMethodZlib();
	afx_msg void OnBnClickedMethodJp2k();
	afx_msg void OnBnClickedMethodWebp();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
};
