// WallsMapDoc.h : interface of the CWallsMapDoc class
//

#pragma once

#ifndef __FILECFG_H
#include "filecfg.h"
#endif

#ifndef _SHPLAYER_H
#include "ShpLayer.h"
#endif

extern const UINT WM_PROPVIEWDOC;

#define	WM_USER_NEWIMAGE WM_USER+1
#define WM_USER_PROGRESS WM_USER+2

enum {
	LHINT_NEWDOC = 0, LHINT_REFRESH, LHINT_FITIMAGE, LHINT_UPDATEDIB, LHINT_UPDATESIZE, LHINT_SYNCSTART,
	LHINT_SYNCPANSTART, LHINT_SYNCPAN, LHINT_SYNCSHOWCURSOR, LHINT_SYNCHIDECURSOR,
	LHINT_SYNCSTOP, LHINT_TESTPOINT, LHINT_CONVERT, LHINT_PRETILE, LHINT_POSTTILE,
	LHINT_REPAINT, LHINT_NEWLAYER, LHINT_SYMBOLOGY, LHINT_GPSTRACKPT, LHINT_CENTERGPS, LHINT_ENABLEGPS
};

enum e_WALLS_FORMATS {
	WALLS_FORMAT_ALL,
	WALLS_FORMAT_NTL,
	WALLS_FORMAT_NTI,
	WALLS_FORMAT_BMP,
	WALLS_FORMAT_ECW,
	WALLS_FORMAT_GIF,
	WALLS_FORMAT_JPG,
	WALLS_FORMAT_J2K,
	WALLS_FORMAT_PNG,
	WALLS_FORMAT_PPM,
	WALLS_FORMAT_SHP,
	WALLS_FORMAT_SID,
	WALLS_FORMAT_TIF,
	WALLS_FORMAT_WEBP,
	//WALLS_FORMAT_NTF,
	MAX_WALLS_FORMAT
};

enum { FTYP_NOSHP = 1, FTYP_NONTL = 2, FTYP_ALLTYPES = 4 };

struct CRestorePosition
{
	CRestorePosition() : geoHeight(0.0), uStatus(0) {}
	double geoHeight;
	CFltPoint ptGeo;
	UINT uStatus; //0 - unitialized, 1 - initialized/saved, 2 - needs saving
};

struct WallsDocType
{
	int nID;
	BOOL bRead;
	BOOL bWrite;
	const char* description;
	const char* ext;
};

class CExportNTI;
class CWallsMapView;
class CNtiLayer;
class CTabComboHist;

enum {
	NTL_CFG_CRS = 1,
	NTL_CFG_FOLDER,
	NTL_CFG_LAYER,
	NTL_CFG_LEND,
	NTL_CFG_LFONT,
	NTL_CFG_LLBLFLD,
	NTL_CFG_LMSTYLE,
	NTL_CFG_LNAME,
	NTL_CFG_LOPACITY,
	NTL_CFG_LPATH,
	NTL_CFG_LQTDEPTH,
	NTL_CFG_LSCALES,
	NTL_CFG_LSRCHFLD,
	NTL_CFG_LSTATUS,
	NTL_CFG_SLAYERS,
	NTL_CFG_STRFIND,
	NTL_CFG_VBKRGB,
	NTL_CFG_VFLAGS,
	NTL_CFG_VVIEW
};

#define NTL_CFG_NUMTYPES NTL_CFG_VVIEW

class CWallsMapDoc : public CDocument
{
protected: // create from serialization only
	CWallsMapDoc();
	DECLARE_DYNCREATE(CWallsMapDoc)

	// Attributes
public:
	CLayerSet & LayerSet() { return m_layerset; }
	int LayersWidth() { return m_layerset.m_PropStatus.nWinWidth; }
	int LayersHeight() { return m_layerset.m_PropStatus.nWinHeight; }
	void SetLayersWindowSize(int cx, int cy) { m_layerset.m_PropStatus.nWinWidth = cx; m_layerset.m_PropStatus.nWinHeight = cy; }
	PTL LayerPtr(int i) { return m_layerset.LayerPtr(i); }
	const CTreeLayer &Layer(int i) { return *LayerPtr(i); }
	const CTreeLayer &SelectedLayer() { return m_layerset.SelectedLayer(); }
	CTreeLayer *SelectedLayerPtr() { return m_layerset.SelectedLayerPtr(); }
	int SelectedLayerPos() { return m_layerset.SelectedLayerPos(); }
	void SetSelectedLayerPos(int pos) { m_layerset.SetSelectedLayerPos(pos); }
	it_Layer BeginLayerIt() { return m_layerset.BeginLayerIt(); }
	it_Layer EndLayerIt() { return m_layerset.EndLayerIt(); }
	rit_Layer BeginLayerRit() { return m_layerset.BeginLayerRit(); }
	rit_Layer EndLayerRit() { return m_layerset.EndLayerRit(); }
	int NumLayers() { return m_layerset.NumLayers(); }
	int NumMapLayers() { return m_layerset.NumMapLayers(); }
	int NumLayersVisible() { return m_layerset.NumLayersVisible(); }
	const CFltRect & Extent() { return m_layerset.Extent(); }
	bool ExtentOverlap(const CFltRect &ext) { return m_layerset.ExtentOverlap(ext); }
	void GetExtent(CFltRect *pfRect, BOOL bVisible) { m_layerset.GetExtent(pfRect, bVisible); }
	CWallsMapView *GetFirstView()
	{
		POSITION pos = GetFirstViewPosition();
		return pos ? (CWallsMapView *)GetNextView(pos) : NULL;
	}
	double ExtentWidth() { return Extent().r - Extent().l; }
	double ExtentHeight() { return fabs(Extent().b - Extent().t); }
	double ExtentHalfPerim() { return ExtentWidth() + ExtentHeight(); }
	CFltPoint ExtentSize(UINT winWidth, UINT winHeight) { return m_layerset.ExtentSize(); }
	void FlagSelectedRecs() { m_layerset.FlagSelectedRecs(); }
	int  IsLayer(LPCSTR lpszPathName) { return m_layerset.IsLayer(lpszPathName); }
	bool HasShpLayers() { return m_layerset.m_bShpLayers; }
	bool HasShpPtLayers() { return m_layerset.m_bShpPtLayers; }
	bool HasNteLayers() { return m_layerset.HasNteLayers(); }
	bool HasSearchableLayers() { return m_layerset.HasSearchableLayers(); }
	bool IsSetSearchable(BOOL bActive) { return m_layerset.IsSetSearchable(bActive) == TRUE; }
	bool IsFeetUnits() { return (m_uEnableFlags&NTL_FLG_FEETUNITS) != 0; }
	bool IsElevUnitsFeet() { return (m_uEnableFlags&NTL_FLG_ELEVUNITSMETERS) == 0; }
	bool IsDatumToggled() { return (m_uEnableFlags&NTL_FLG_DATUMTOGGLED) != 0; }
	void ToggleFeetUnits() { m_uEnableFlags ^= NTL_FLG_FEETUNITS; SetChanged(); }
	void ToggleElevUnits() { m_uEnableFlags ^= NTL_FLG_ELEVUNITSMETERS; SetChanged(); }
	void ToggleDatum() { m_uEnableFlags ^= NTL_FLG_DATUMTOGGLED; SetChanged(); }
	bool HasOneMapLayer() { return m_layerset.InitPML() && !m_layerset.NextPML(); }
	void ReplaceLayerSet(VEC_TREELAYER &vec) { m_layerset.ReplaceLayerSet(vec); }

	BOOL OpenLayer(LPCSTR lpszPathName, LPCSTR lpszTitle, UINT uFlags, int pos = -1, int posSib = 0)
	{
		return m_layerset.OpenLayer(lpszPathName, lpszTitle, uFlags, pos, posSib);
	}

	BOOL DynamicAddLayer(CString &pathName);
	CShpLayer *FindShpLayer(LPCSTR pName, UINT *pFldNum, UINT *pFldPfx = NULL);

	void ReplaceLayer(CMapLayer *pLayer) { m_layerset.ReplaceLayer(pLayer); }
	void AddNtiLayer(CString &path) { m_layerset.AddNtiLayer(path); }
	void RefreshViews() { UpdateAllViews(NULL, LHINT_REFRESH); }
	void RepaintViews() {
		UpdateAllViews(NULL, LHINT_REPAINT);
	}
	void DeleteLayer() { m_layerset.DeleteLayer(); }
	/*
	void MoveLayer(int iDst) {m_layerset.MoveLayer(iDst);}
	void MoveLayerDown() {m_layerset.MoveLayerDown();}
	void MoveLayerUp() {m_layerset.MoveLayerUp();}
	void MoveLayerToTop() {m_layerset.MoveLayerToTop();}
	void MoveLayerToBottom() {m_layerset.MoveLayerToBottom();}
	*/
	bool IsGeoWgs84() { return m_layerset.IsGeoWgs84(); }
	void SetChanged(BOOL bChanged = 2) { m_layerset.SetChanged(bChanged); }
	void SetUnChanged() { m_layerset.SetUnChanged(); }
	void UnflagSelectedRecs() { m_layerset.UnflagSelectedRecs(); }
	void StoreSelectedRecs() { m_layerset.StoreSelectedRecs(); }
	void InitLayerIndices() { m_layerset.InitLayerIndices(); }
	BOOL IsChanged() { return m_layerset.IsChanged(); }
	bool IsTransformed() { return m_layerset.IsTransformed(); }
	bool IsProjected() { return m_layerset.IsProjected(); }
	LPCSTR DatumName() { return m_layerset.m_datumName; }
	LPCSTR UnitsName() { return m_layerset.m_unitsName; }
	void GetDatumName(CString &name) { m_layerset.GetDatumName(name); }
	void GetOtherDatumName(CString &name) { m_layerset.GetOtherDatumName(name); }
	int  FirstVisibleIndex() { return m_layerset.FirstVisibleIndex(); }
	int  GetUTMCoordinates(CFltPoint &fpt) { return m_layerset.GetUTMCoordinates(fpt); }
	int  GetUTMOtherDatum(CFltPoint &fpt) { return m_layerset.GetUTMOtherDatum(fpt); }
	void GetDEGOtherDatum(CFltPoint &fpt) { m_layerset.GetDEGOtherDatum(fpt); }
	void GetDEGCoordinates(CFltPoint &fpt) { m_layerset.GetDEGCoordinates(fpt); }
	bool IsValidGeoPt(const CFltPoint &fpt) { return m_layerset.IsValidGeoPt(fpt); }

	bool IsGeoRefSupported() { return m_layerset.IsGeoRefSupported(); }

	void ToggleLabels() { m_uEnableFlags ^= NTL_FLG_LABELS; RefreshViews(); }
	void ToggleMarkers() { m_uEnableFlags ^= NTL_FLG_MARKERS; RefreshViews(); }
	bool MarkersHidden() { return !(m_uEnableFlags&NTL_FLG_MARKERS); }
	bool LabelsHidden() { return !(m_uEnableFlags&NTL_FLG_LABELS); }
	BOOL LabelLayersVisible() { return m_layerset.ShpLayersVisible(NTL_FLG_LABELS); }
	BOOL MarkerLayersVisible() { return m_layerset.ShpLayersVisible(NTL_FLG_MARKERS); }
	BOOL ShpLayersVisible() { return m_layerset.ShpLayersVisible(NTL_FLG_MARKERS | NTL_FLG_LABELS); }
	int GetAppendableLayers(CShpLayer *pShp)
	{
		return m_layerset.GetAppendableLayers(pShp);
	}
	BOOL EditedShapesVisible()
	{
		//After AND with first argument, flag must be greater than second argument, NTL_FLG_EDITABLE --
		return m_layerset.ShpLayersVisible(NTL_FLG_VISIBLE | NTL_FLG_EDITABLE, NTL_FLG_EDITABLE);
	}

	bool IsSyncCompatible(CWallsMapDoc *pDoc) {
		return m_layerset.IsSyncCompatible(pDoc->LayerSet());
	}

	HMENU AppendCopyMenu(CMapLayer *pLayer, CMenu *pPopup, LPCSTR pItemName) {
		return m_layerset.AppendCopyMenu(pLayer, pPopup, pItemName);
	}

	bool TrimExtent(CFltRect &ext) {
		return ((CMapLayer &)SelectedLayer()).TrimExtent(ext);
	}

	static WallsDocType doctypes[MAX_WALLS_FORMAT];

	CExportNTI *m_pExportNTI;
	CRestorePosition *m_pRestorePosition;
	CTabComboHist *m_phist_find;

	enum e_action_limits { ACTION_MINDRAG = 2 };
	enum e_action { ACTION_ZOOMIN = 1, ACTION_ZOOMOUT = 2, ACTION_PAN = 4, ACTION_PANNED = 8, ACTION_MEASURE = 16, ACTION_SELECT = 32, ACTION_NOCOORD = 64 };
	UINT m_uActionFlags;
	BOOL m_bExtensionNTL;
	BOOL m_bDrawOutlines;
	BOOL m_bSelecting;
	BOOL m_bMarkSelected;
	bool m_bReadOnly;
	bool m_bShowAltNad;
	bool m_bShowAltGeo;
	bool m_bHistoryUpdated, m_bLayersUpdated;

	UINT m_uEnableFlags;
	static CString m_csLastPath;
	static BOOL m_bOpenAsEditable;
	static LPSTR ntltypes[NTL_CFG_NUMTYPES];

	// Operations
public:
	static bool SelectionLimited(UINT size);
	//void UpdateGPSTrack();
	bool GetConvertedGPSPoint(CFltPoint &fp);

	static void SetLastPath(LPCSTR path)
	{
		int pathLen = trx_Stpnam(path) - path;
		if (pathLen) pathLen--;
		m_csLastPath.SetString(path, pathLen);
	}

	static CString GetFileTypes(UINT notyp);
	static BOOL PromptForFileName(char *pFileName, UINT maxFileNameSiz, LPCSTR pTitle, DWORD dwFlags, UINT fTyp);

	CRestorePosition * GetRestorePosition()
	{
		if (!m_pRestorePosition) m_pRestorePosition = new CRestorePosition();
		return m_pRestorePosition;
	}

	LPCSTR PathName() { return m_strPathName; }

	void DeleteRestorePosition()
	{
		delete m_pRestorePosition;
		m_pRestorePosition = NULL;
	}

	int CopyToDIB(CDIBWrapper *pDestDIB, const CFltRect &rectGeo, double fScale)
	{
		BeginWaitCursor();
		int iRet = m_layerset.CopyToDIB(pDestDIB, rectGeo, fScale);
		EndWaitCursor();
		return iRet;
	}
	double MaxImagePixelsPerGeoUnit() { return m_layerset.MaxImagePixelsPerGeoUnit(); }
	//CFltPoint GetGeoSize() {return m_layerset.GetGeoSize();}

	CPropStatus &GetPropStatus() { return m_layerset.m_PropStatus; }
	BOOL FindByLabel(const CString &text, WORD wFlags) { return m_layerset.FindByLabel(text, wFlags); }
	bool IsSavingHistory() { return (m_uEnableFlags&NTL_FLG_SAVEHISTORY) != 0; }
	bool IsSavingLayers() { return (m_uEnableFlags&NTL_FLG_SAVELAYERS) != 0; }
	bool InitVecShpopt(bool bAdv) { return m_layerset.InitVecShpopt(bAdv); }
	void ClearSelChangedFlags() { m_layerset.ClearSelChangedFlags(); }
	void RefreshTables() { m_layerset.RefreshTables(); }
	void ReplaceVecShprec();
	void ClearVecShprec();
	BOOL WriteNTL(CFileCfg *pFile, const CString &csPathName, BOOL bIncHidden = TRUE, BOOL bShpsOnly = FALSE);
	static bool HasZipContent(PTL pFolder, BOOL bIncHidden, BOOL bShpsOnly);

	void ExportNTI();
	BOOL AddLayer(CString &pathName, UINT bFlags, int pos = -1, int posSib = 0);
	void AddFileLayers(bool bFromTree = false, int posSib = 0);
	HBRUSH m_hbrBack;
	COLORREF m_clrBack, m_clrPrev;
	VEC_SHPREC m_vec_shprec;
	static bool m_bSelectionLimited;

private:
	//CString m_csInfo;
	CLayerSet m_layerset;
	static UINT m_nDocs;
	CFileCfg *m_pFileNTL;
	IFileIsInUse *m_pfiu;
	DWORD m_dwCookie;

	BOOL OpenLayerSet(LPCTSTR lpszPathName);
	BOOL SaveLayerSet(BOOL bSaveAs = FALSE);
	void ClearSelectMode();
	void Release_fiu();
	void Register_fiu(LPCSTR pPathName);

	// Implementation
public:
	virtual ~CWallsMapDoc();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnNewDocument();
	virtual void OnCloseDocument();
	virtual BOOL OnSaveDocument(const char* pszPathName);
	virtual BOOL SaveModified();

#ifdef _DEBUG
	virtual void AssertValid() const;
#endif
	afx_msg void OnFileProperties();
	afx_msg void OnUpdateFileProperties(CCmdUI *pCmdUI);
	afx_msg void OnFileSaveAs();
	afx_msg void OnUpdateFileSaveAs(CCmdUI *pCmdUI);

	// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnViewDefault();
	afx_msg void OnViewZoomin();
	afx_msg void OnViewZoomout();
	afx_msg void OnViewPan();
	afx_msg void OnToolMeasure();
	afx_msg void OnUpdateViewPan(CCmdUI *pCmdUI);
	afx_msg void OnFileSave();
	afx_msg void OnUpdateFileSave(CCmdUI *pCmdUI);
	afx_msg void OnFileConvert();
	afx_msg void OnUpdateFileConvert(CCmdUI *pCmdUI);
	afx_msg void OnFileTruncate();
	afx_msg void OnUpdateFileTruncate(CCmdUI *pCmdUI);
	afx_msg void OnToolSelect();
	afx_msg void OnUpdateToolSelect(CCmdUI *pCmdUI);
	afx_msg void OnSelectAdd();
	afx_msg void OnUpdateSelectAdd(CCmdUI *pCmdUI);
	afx_msg void OnExportZip();
};


