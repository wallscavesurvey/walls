#pragma once
#include "afxcmn.h"

#include "ThreadJob.h"

#ifndef __NTIILE_H
#include <ntifile.h>
#endif

// CExportJob class used by CExpandNTI and CExportNTI --

// Message used for progress notification
const UINT WM_MYPROGRESS = WM_USER+1;

#define NTI_JP2K_NUMRATES 16
//extern float jp2k_rates[NTI_JP2K_NUMRATES];
//extern char *jp2k_percents[NTI_JP2K_NUMRATES];

#define NTI_WEBP_NUM_PRESETS 6
#define NTI_WEBP_NUM_QUALITIES 18

extern char *webp_preset[NTI_WEBP_NUM_PRESETS];
//extern BYTE webp_quality[NTI_WEBP_NUM_QUALITIES];

#ifdef SSIM_RGB
extern double ssim_total[3];
#else
extern double ssim_total;
#endif

extern UINT ssim_count;

class CWallsMapDoc;
class CImageLayer;
class CExportNTI;

// Dump record job is a special case of CThreadJob, a worker thread
//
class CExportJob : public CThreadJob {
public:
	virtual UINT DoWork();
	void SetLayer(CImageLayer *pLayer) {m_pImgLayer=pLayer;}
	void SetWindow(const CRect &rect,BOOL bExtentWindow) {crWindow=rect;m_bExtentWindow=bExtentWindow;}
	void SetExpanding(BOOL bExpanding) {m_bExpanding=bExpanding;}
	void SetPathName(LPCSTR pathName) {m_pathName=pathName;}

	static CNTIFile nti;
private:
	CRect crWindow;
	CString m_pathName;
	BOOL m_bExpanding;
	BOOL m_bExtentWindow;
	UINT exitNTI(LPCSTR msg);
	CImageLayer *m_pImgLayer;
};

