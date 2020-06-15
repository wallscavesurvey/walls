#ifndef _SHPLAYER_H
#define _SHPLAYER_H

#ifndef __TRXFILE_H
#include <trxfile.h>
#endif

#ifndef __DBFILE_H
#include <ShpDBF.h>
#endif

#ifndef _UTILITY_H
#include "utility.h"
#endif

#ifndef _MAPLAYER_H
#include "MapLayer.h"
#endif

#ifndef _LOGFONT_H
#include "LogFont.h"
#endif

#ifndef _DBTFILE_H
#include "dbtfile.h"
#endif

#ifndef _PLACEMARK_H
#include "PlaceMark.h"
#endif

#ifndef __SAFEMIRRORFILE_H
#include "SafeMirrorFile.h"
#endif

#ifndef _SHPSTRUCT_H
#include "ShpStruct.h"
#endif

#include "XComboList.h"
#include "FileMap.h"

class CShpDef;
class CDIBWrapper;
class CWallsMapDoc;
class CShpLayer;
class CDlgExportXLS;
class CMemoEditDlg;
class CCompareDlg;

#define _XLS

#define MAX_VEC_SHPREC_SIZE 10000

#define NUM_SHP_DBFILES 256
#define SHP_SIZ_TREELABEL 256
#define LEN_NEW_POINT_STR 11

enum e_shp_edit { SHP_EDITSHP = 1, SHP_EDITDBF = 2, SHP_EDITADD = 4, SHP_EDITDEL = 8, SHP_SELECTED = 16, SHP_EDITCLR = 32, SHP_EDITLOC = 64 };
//#define SHP_MINVAL (-1.0e+38)

struct SHP_COLUMNS {
	VEC_INT width;
	VEC_INT order;
};

struct SHP_POLY_DATA {
	bool IsDeleted() const { return (len & 0x80000000) != 0; }
	void SetLen(UINT l) { len = l; }
	void Delete() { len |= 0x80000000; }
	void UnDelete() { len &= ~0x80000000; }
	UINT MaskedLen() const { return len & ~0x80000000; }
	UINT Len() const { return len; }
	UINT off; //file offset to NumParts
private:
	UINT len; //length in bytes of data to read starting at NumParts (high bit set if deleted)
};

struct SHP_LBLPOS {
	SHP_LBLPOS() : rec(0) {}
	SHP_LBLPOS(CPoint &p, DWORD r) : pos(p), rec(r) {}
	CPoint pos;
	DWORD rec;
};
typedef std::vector<SHP_LBLPOS> VEC_LBLPOS;
typedef VEC_LBLPOS::iterator it_lblpos;

class CDBGridDlg;
class CShpLayer;
class CQuadTree;

enum { LF_LAT, LF_LON, LF_EAST, LF_NORTH, LF_ZONE, LF_DATUM, LF_UPDATED, LF_CREATED, NUM_LOCFLDS };

struct SHP_DBFILE { //size=84
	SHP_DBFILE() : nUsers(0), pbuf(NULL), pbuflen(0), pShpEditor(NULL), pvxc(NULL), pvxc_ignore(NULL), pvxd(NULL), ppoly(NULL),
		pDBGridDlg(NULL), lblfld(0), keyfld(0), pfxfld(0), noloc_dir(6), noloc_lat(0.0), noloc_lon(0.0), bTypeZ(0), bHaveTmpshp(0) {}
	CShpDBF db;
	CDBTFile dbt;   //memo file
	VEC_BYTE vdbe;  //edit flags
	VEC_BYTE locFldTyp;  //indexes of special fields (+1) for each field
	VEC_BYTE vxsrch; //fld numbers of label followed by searchable fields based on template alone
	double noloc_lat, noloc_lon; //out-of-range corner lat/lon, direction = noloc_dir 
	PVEC_XCOMBO pvxc; //field template parameters
	PVEC_XCOMBO pvxc_ignore;
	PVEC_XDESC pvxd; //field descriptions from template (tooltips)
	BYTE locFldNum[NUM_LOCFLDS];  //field number of each type (index)
	SHP_POLY_DATA *ppoly;    //array of record offsets in shp component (poly types only)
	SHP_COLUMNS col;
	CFileMap *pfShp;
	CDBGridDlg *pDBGridDlg; //points to modeless CDBGridDlg if not NULL
	LPBYTE pbuf;    //edit record buffer
	CShpLayer *pShpEditor; // layer that's editing this file, NULL if no editor
	UINT pbuflen;	//current length of pbuf;
	WORD nUsers;
	WORD wErrBoundary;
	INT8 iNad;	//0 if unspecified, 1-nad82, 2-nad27, 3-other
	INT8 iZone; //0 if lat/long, 1 <= abs(iZone) <=60  if UTM, -99 if unspecified system, 99 if unsupported proj
	BYTE shpType;
	BYTE shpTypeOrg;
	BYTE bHaveTmpshp;
	BYTE bTypeZ;
	BYTE pfxfld; //e.g., COUNTY
	BYTE lblfld;
	BYTE keyfld;
	BYTE noloc_dir;
	//==================
	static bool bEditorPrompted;
	static bool bEditorPreserved;
	static CString csEditorName;

	static LPCSTR sLocflds[NUM_LOCFLDS], sLocflds_old[NUM_LOCFLDS];
	static bool GetTimestampName(SHP_DBFILE *pdbfile, BOOL bUpdating);

	static int GetLocFldTyp(LPCSTR pNam);
	int GetLocFldTyp(UINT fNum) { return locFldTyp.size() ? locFldTyp[fNum - 1] : 0; }
	bool InitRecbuf();
	void Clear();
	bool InitLocFlds(); //fills locFldTyp and locFldNum
	bool HasLocFlds();
	bool HasTimestamp(BOOL bUpdated);
	int  Reopen(bool bReadOnly);
	bool SaveAllowed(BOOL bEditing)
	{
		return bEditorPrompted || !HasTimestamp(bEditing) || GetTimestampName(this, bEditing);
	}
};

#define SHP_LABEL_SIZE 80
#define SHP_EXT_DBF	CShpLayer::shp_ext[CShpLayer::EXT_DBF]
#define SHP_EXT_SHP	CShpLayer::shp_ext[CShpLayer::EXT_SHP]
#define SHP_EXT_SHX	CShpLayer::shp_ext[CShpLayer::EXT_SHX]
#define SHP_EXT_DBT	CShpLayer::shp_ext[CShpLayer::EXT_DBT]
#define SHP_EXT_DBE	CShpLayer::shp_ext[CShpLayer::EXT_DBE]
#define SHP_EXT_PRJ	CShpLayer::shp_ext[CShpLayer::EXT_PRJ]
#define SHP_EXT_TMPSHP	CShpLayer::shp_ext[CShpLayer::EXT_TMPSHP]

struct SEQ_DATA {
	SEQ_DATA() : seq_fcn(NULL), seq_pval(NULL), seq_buf(NULL), seq_len(0), seq_siz(0) {}
	SEQ_DATA(TRXFCN_IF fcn, LPVOID *pval) :
		seq_fcn(fcn), seq_pval(pval), seq_buf(NULL), seq_len(0), seq_siz(0) {}
	~SEQ_DATA() { free(seq_buf); }

	void Free() { free(seq_buf); seq_buf = NULL; }

	void Init(TRXFCN_IF fcn, LPVOID *pval)
	{
		seq_fcn = fcn;
		seq_pval = pval;
		Free();
		seq_len = seq_siz = 0;
	}
	TRXFCN_IF seq_fcn;
	LPVOID *seq_pval;
	PDWORD seq_buf;
	DWORD seq_len;
	DWORD seq_siz;
};

struct TRUNC_DATA {
	TRUNC_DATA() : count(0) {}
	UINT count;
	DWORD rec;
	BYTE fld;
};

//used by ZipUpdDlg and CompareShp() --
struct TRXREC {
	CShpLayer *pShp;
	DWORD rec;
	WORD *pcFld; //pcFld[f], if !0, is field in pShp matching field f in reference shapefile
};
typedef std::vector<TRXREC> VEC_TRXREC;
typedef VEC_TRXREC::iterator it_trxrec;

struct COMPSHP {
	COMPSHP() : pShp(NULL), bSel(FALSE) {};
	COMPSHP(CShpLayer *pS, BOOL b) : pShp(pS), bSel(FALSE) {};
	CShpLayer *pShp;
	BOOL bSel;
};
typedef std::vector<COMPSHP> VEC_COMPSHP;
typedef VEC_COMPSHP::iterator it_compshp;

//globals used by ZipDlg --
extern SEQ_DATA seq_data;
extern LPCSTR seq_pstrval;
extern VEC_CSTR seq_vlinks;
int CALLBACK seq_fcn(int i);

//globals used by compare fcns --
extern CTRXFile c_Trx, c_Trxref, c_Trxlinks, c_Trxkey; //global indexes

class CShpLayer : public CMapLayer
{
public:
	enum { MARKER_MAXLINEW = 15, MARKER_MAXSIZE = 61 };
	enum { EXT_SHP, EXT_DBF, EXT_SHX, EXT_DBT, EXT_DBE, EXT_PRJ, EXT_TMPSHP, EXT_QPJ, EXT_SBN, EXT_SBX, EXT_COUNT };
	static const LPCSTR shp_ext[EXT_COUNT];
	static const LPCSTR lpstrFile_pfx;
	enum e_shp {
		SHP_NULL,
		SHP_POINT,
		SHP_POLYLINE = 3,
		SHP_POLYGON = 5,
		SHP_MULTIPOINT = 8,
		SHP_POINTZ = 11,
		SHP_POLYLINEZ = 13,
		SHP_POLYGONZ = 15,
		SHP_MULTIPOINTZ = 18,
		SHP_POINTM = 21,
		SHP_POLYLINEM = 23,
		SHP_POLYGONM = 25,
		SHP_MULTIPOINTM = 28,
		SHP_MULTIPATCH = 31
	};
	enum { SHP_NUMTYPES = 13 };
	static LPCSTR shp_typname[SHP_NUMTYPES];
	static int shp_typcode[SHP_NUMTYPES];
	static LPCSTR ShpTypName(int code);

	CShpLayer();
	virtual ~CShpLayer();
	virtual int LayerType() const;
	virtual BOOL Open(LPCTSTR lpszPathName, UINT uFlags);
	//virtual BOOL GetDIBits();
	virtual int CopyToDIB(CDIBWrapper *pDestDIB, const CFltRect &geoExt, double fScale);
	virtual void AppendInfo(CString &s) const;
	//void AppendCoordSystemOrg(CString &cs) const;
	int SelectEditedShapes(UINT &count, UINT uFlags, BOOL bAddToSel, CFltRect *pRectView);
	void ClearDBE();
	BOOL SetLblFld(UINT fld);
	BOOL CheckFldTypes(LPCSTR pathName, UINT uFlags);
	void InitSrchFlds(UINT nLblFld);
	BYTE & RecEditFlag(UINT uRec) { return (*m_vdbe)[uRec - 1]; }
	void ChkEditDBF(UINT rec);
	void SetSrchFlds(const std::vector<CString> &vcs);
	BOOL GetSrchFlds(CString &fldnam) const;
	static void Init();
	static void UnInit();
	static void SetGEDefaults();
	static int	DbfileIndex(LPCSTR pathName);
	static bool DeleteComponents(LPCSTR pathName);
	static bool check_overwrite(LPCSTR pathName);
	static BOOL FileCreate(CSafeMirrorFile &cf, LPSTR ebuf, LPCSTR pathName, LPCSTR pExt);
	static void InitShpHdr(SHP_MAIN_HDR &hdr, int typ, UINT sizData, const CFltRect &extent, const CFltRect *pextz);
	static __int32 FlipBytes(__int32 nData)
	{
		__asm mov eax, [nData];
		__asm bswap eax;
		__asm mov[nData], eax; // you can take this out but leave it just to be sure
		return(nData);
	}

#ifdef _XLS
	BOOL ExportXLS(CDlgExportXLS *pDlg, LPCSTR xlsName, CShpDef &shpdef, VEC_DWORD &vRec);
	bool ExportRecordsXLS(VEC_DWORD &vRec);
#endif

	BOOL InitShpDef(CShpDef &shpdef, BOOL bUseLayout, bool bXLS = false);
	BOOL ExportShapes(LPCSTR shpName, CShpDef &shpdef, VEC_DWORD &vRec);
	bool ExportRecords(VEC_DWORD &vRec, bool bSelected);
	bool FitExtent(VEC_DWORD &vRec, BOOL bZoom);
	LPCSTR GetMemoTitle(CString &title, const EDITED_MEMO &memo);
	BOOL ConfirmAppendShp(CShpLayer *pShpSrc, BYTE *pFldSrc);
	UINT CopyShpRec(CShpLayer *pSrcLayer, UINT uSrcRec, BYTE *pFldSrc, bool bUpdateLocFlds, TRUNC_DATA &td);
	static void WritePrjFile(LPCSTR pathName, int bNad83, int utm_zone);
	static void Sort_Vec_ptNode(VEC_PTNODE &vec_pn);
	static void Sort_Vec_ShpRec(VEC_SHPREC &vec_shprec);

	static int AddMemoLink(LPCSTR fpath, VEC_CSTR &vlinks, SEQ_DATA &s, LPCSTR pRootDir, int rootLen);
	int GetFieldMemoLinks(LPSTR pStart, LPSTR fPath, VEC_CSTR &vlinks, SEQ_DATA &s, LPCSTR pRootDir, int rootLen, VEC_CSTR *pvBadLinks = NULL);
	int GetMemoLinks(VEC_CSTR &vlinks, SEQ_DATA &s, LPCSTR pRootDir, int rootLen);

	bool HasOpenRecord();
	bool HasPendingEdits();
	bool HasEditedText(CMemoEditDlg *pDlg);
	void SetDlgEditable(CMemoEditDlg *pDlg, bool bEditable);
	bool HasEditedMemo();

	bool OpenMemo(CDialog *pParentDlg, EDITED_MEMO &memo, BOOL bViewMode);
	bool IsMemoOpen(DWORD nRec, UINT nFld);
	void OpenTable(VEC_DWORD *pVec);
	bool ToggleDelete(DWORD rec);
	bool IsAdditionPending();
	void OutOfZoneMsg(int iZone);

	static int XC_ParseDefinition(XCOMBODATA &xc, DBF_FLDDEF &Fld, LPCSTR p0, BOOL bDynamic = 0);
	PXCOMBODATA	XC_GetXComboData(int nFld);
	LPCSTR XC_GetXDesc(int nFld);


	BOOL		XC_RefreshInitFlds(LPBYTE pRec, const CFltPoint &fpt, BOOL bReinit, CFltPoint *pOldPt = NULL);
	void		XC_GetFldOptions(LPSTR pathName);
	LPCSTR		XC_GetInitStr(CString &s, XCOMBODATA &xc, const CFltPoint &fpt, LPBYTE pRec = NULL, CFltPoint *pOldPt = NULL);
	BOOL	    XC_PutInitStr(LPCSTR p, DBF_pFLDDEF pFld, LPBYTE pDst, CDBTFile *pdbt);
	void		XC_GetDefinition(CString &s, PXCOMBODATA pxc);
	void		XC_FlushFields(UINT nRec);

	bool IsFldReadOnly(int nFld)
	{
		PXCOMBODATA pXCMB;
		return !bIgnoreEditTS && (pXCMB = XC_GetXComboData(nFld)) && (pXCMB->wFlgs&XC_READONLY);
	}
	bool HasXCFlds() { return m_pdbfile->pvxc != NULL; }
	bool HasXDFlds() { return m_pdbfile->pvxd != NULL; }
	bool HasXFlds() { return m_pdbfile->pvxc != NULL || m_pdbfile->pvxd != NULL; }
	bool HasMemos() { return m_pdb->HasMemos(); }
	void UpdateLocFlds(DWORD rec, LPBYTE pRecBuf);
	bool HasLocFlds() { return m_pdbfile->HasLocFlds(); }
	bool HasTimestamp(BOOL bEditing) { return m_pdbfile->HasTimestamp(bEditing); }
	bool IsShpEditable() const { return (m_uFlags&NTL_FLG_EDITABLE) != 0 && ShpTypeOrg() == SHP_POINT; }

	BYTE KeyFld() { return m_pdbfile->keyfld; }

	//void ShowTimestamps(UINT rec);
	void UpdateTimestamp(LPBYTE pRecBuf, BOOL bCreating);
	static void GetTimestampString(CString &s, LPCSTR pName);

	int GetLocFldTyp(UINT fNum) { return m_pdbfile->GetLocFldTyp(fNum); }

	static double GetDouble(LPCSTR pSrc, UINT len);
	UINT GetFieldValue(CString &s, int nFld);
	UINT GetPolyRange(UINT rec);

	void UpdateTitleDisplay();
	void UpdateSizeStr();

	bool CanAppendShpLayer(CShpLayer *pLayer)
	{
		return ShpTypeOrg() == SHP_POINT && IsEditable() && m_pdb->FieldsCompatible(pLayer->m_pdb) > 0;
	}

	bool IsGridDlgOpen();
	void TestMemos(BOOL bOnLoad = FALSE);

	BOOL SaveShp();
	BOOL SetEditable();
	void SaveEditedMemo(EDITED_MEMO &memo, CString &csData);

	//Compare functions --
	int ParseFldKey(UINT f, LPSTR p0); //in ShpLayer_CC.cpp
	LPSTR m_pKeyGenerator, m_pUnkName, m_pUnkPfx;
	int CompareUpdates(BOOL bIncludeAll);
	int CompareNewRecs();
	int CompareShp(CString &csPathName, CString &summary, VEC_COMPSHP &vCompShp, WORD *pwFld, VEC_WORD &vFldsIgnored, BOOL bIncludeAll);
	static void GetCompareStats(UINT &nLinks, UINT &nNew, UINT &nfiles, __int64 &sz);
	LPCSTR InitRootPath(CString &path, WORD &rootPathLen); //returns (LPCSTR)path
	int AppendUpdateRec(CShpDBF &db, CDBTFile *pdbt, TRXREC &trxrec, int iFlags);
	int ExportUpdateShp(LPSTR pathbuf, CString &pathName, int iFlags);
	int ExportUpdateDbf(LPSTR pathBuf, CString &pathName, int iFlags);
	int InitTrxkey(LPCSTR *ppMsg);
	int ResetTrxkey(int *pnLenPfx);

	bool ListRecHeader(TRXREC *pTrxrec = NULL, bool bCrlf = true);
	bool OpenCompareLog();
	bool CompareLocs(bool bChangedPfx);
	int  CheckTS(WORD f, WORD fc, bool bRevised);

	bool IsNotLocated(double lat, double lon);
	bool IsNotLocated(const CFltPoint &fpt) { return IsNotLocated(fpt.y, fpt.x); }
	bool ChkNotLocated(CFltPoint &fpt);
	void ListFieldContent(LPCSTR pL, LPCSTR pR);
	void ListMemoFields();
	LPCSTR GetRecordLabel(LPSTR pLabel, UINT sizLabel, DWORD rec);
	LPCSTR GetTitleLabel(CString &s, DWORD rec)
	{
		char label[80];
		s.Format("%s:  %s", Title(), GetRecordLabel(label, 80, rec));
		return (LPCSTR)s;
	}
	void InitFlyToPt(CFltPoint &fpt, DWORD rec);
	void OnLaunchGE(VEC_DWORD &vRec, CFltPoint *pfpt = NULL);
	void OnLaunchWebMap(DWORD rec, CFltPoint *pfpt = NULL);
	void OnOptionsGE(VEC_DWORD &vRec, CFltPoint *fpt = NULL);
	bool GetFieldTextGE(CString &text, DWORD rec, UINT nFld, BOOL bSkipEmpty);
	bool InitDescGE(CString &text, DWORD rec, VEC_BYTE &vFld, BOOL bSkipEmpty);

	int  ConvertPointsTo(CFltPoint *pFpt, int iNad, int iZone, UINT cnt);
	void ConvertPointsFrom(CFltPoint *pFpt, int iNad, int iZone, UINT iCount);
	void ConvertPointsFromOrg(CFltPoint *pt, int iCnt);

	BOOL ConvertProjection(int iNad, int iZone, BOOL bUpdateViews);
	void UpdateShpExtent(bool bPrj = true);

	static void UpdateExtent(CFltRect &extent, const CFltPoint &fpt);
	static void UpdateExtent(CFltRect &extent, const CFltRect &ext);

	void CancelMemoMsg(EDITED_MEMO &memo);

	void UpdateDocsWithPoint(UINT iPoint);
	BYTE ShpType() const { return m_pdbfile->shpType; }
	BYTE ShpTypeOrg() const { return m_pdbfile->shpTypeOrg; }
	int NadOrg() const { return m_pdbfile->iNad; }
	INT8 ZoneOrg() const { return m_pdbfile->iZone; }

	BYTE & EditFlag(UINT rec0) { return (*m_vdbe)[rec0]; }

	bool IsRecDeleted(UINT rec)
	{
		return ShpType() == SHP_POINT && ((*m_vdbe)[rec - 1] & SHP_EDITDEL) != 0 || m_pdbfile->ppoly && m_pdbfile->ppoly[rec - 1].IsDeleted();
	}
	bool IsPtRecDeleted(UINT rec)
	{
		return ((*m_vdbe)[rec - 1] & SHP_EDITDEL) != 0;
	}
	bool IsRecSelected(UINT rec) { return ((*m_vdbe)[rec - 1] & SHP_SELECTED) != 0; }
	bool IsRecEdited(UINT rec) { return ((*m_vdbe)[rec - 1] & ~(SHP_SELECTED | SHP_EDITDEL)) != 0; }
	bool IsSameProjection(CShpLayer *pLayer) { return pLayer->m_iNadOrg == m_iNadOrg && pLayer->m_iZoneOrg == m_iZoneOrg; }
	bool IsBeingEdited() { return HasOpenRecord() || HasEditedMemo() || !IsEditable() && m_pdbfile->pShpEditor; }

	void SetRecSelected(UINT rec)
	{
		(*m_vdbe)[rec - 1] |= SHP_SELECTED;
	}

	void SetRecUnselected(UINT rec)
	{
		(*m_vdbe)[rec - 1] &= ~SHP_SELECTED;
	}

	void FixTitle()
	{
		if (!_stricmp(trx_Stpext(m_csTitle), SHP_EXT))
			m_csTitle.Truncate(m_csTitle.GetLength() - 4);
	}

	LPSTR GetFullFromRelative(LPSTR buf, LPCSTR pPath)
	{
		//see utility.h
		return ::GetFullFromRelative(buf, pPath, m_pathName);
	}

	LPSTR GetRelativePath(LPSTR buf, LPCSTR pPath, BOOL bNoNameOnPath)
	{
		//see utility.h
		return ::GetRelativePath(buf, pPath, PathName(), bNoNameOnPath);
	}

	bool SaveAllowed(BOOL bEditing)
	{
		return m_pdbfile->SaveAllowed(bEditing);
	}
	int ConfirmSaveMemo(HWND hWnd, const EDITED_MEMO &memo, BOOL bForceClose);
	//bool DiscardChgOK(const EDITED_MEMO &memo);
	BOOL OpenLinkedDoc(LPCSTR pPath);
	void UpdateImage(CImageList *pImageList);
	void GetTooltip(UINT rec, LPSTR buf);
	BOOL GetMatches(VEC_BYTE *pvSrchFlds, LPCSTR target, WORD wFlags, const CFltRect &frRect);
	DWORD InitEditBuf(CFltPoint &fpt, DWORD rec);
	bool SwapSrchFlds(VEC_BYTE &vSrchFlds);
	int FindInRecord(VEC_BYTE *pvSrchFlds, LPCSTR target, int lenTarget, WORD wFlags);
	SHP_POLY_DATA * Init_ppoly();
	void UnInit_ppoly();
	DWORD ShpPolyTest(const CFltPoint &fpt, DWORD fRec = 0, double *pdist = NULL);
	void UpdatePolyExtent();
	void AllocateQT();

	VEC_FLTPOINT m_fpt; //for points only
	VEC_FLTRECT m_ext;  //for polys only
	CQuadTree *m_pQT; //ptr to optional quad tree for poly types
	BYTE m_bQTdepth;  //depth of quadtree, non-zero to buld in Init_ppoly()
	BYTE m_bQTusing;  //Using quadtree
	UINT m_uQTnodes;  //nodes in last generated tree, can be used for initial allocation to avoid realloc

	VEC_BYTE *m_vdbe;

	SHP_MRK_STYLE m_mstyle;
	COLORREF m_crLbl;
	CLogFont m_font;
	UINT m_nLblFld, m_nLayerIndex;
	//UINT m_nKmlFld;
	CShpDBF *m_pdb;
	SHP_DBFILE *m_pdbfile;
	UINT m_nNumRecs, m_uDelCount;
	CBitmap m_bmImage;

	static UINT m_uKmlRange, m_uIconSize, m_uLabelSize;
	static CPtNode *m_pPtNode;
	static HFONT m_hfFieldList;
	static WORD m_nFieldListCharWidth;
	static char *new_point_str;
	static bool m_bShpReeditMsg;
	static bool m_bSelectedMarkerChg;
	static bool m_bChangedGEPref, m_bChangedGEPath;
	static VEC_DWORD m_vGridRecs;
	static CShpLayer *m_pGridShp;
	static SHP_MRK_STYLE m_mstyle_sD, m_mstyle_hD, m_mstyle_s, m_mstyle_h;
	static CString m_csIconSingle;
	static CString m_csIconMultiple;
	static LPCSTR pIconMultiple, pIconSingle;

	BYTE m_bEditFlags;
	bool m_bConverted;
	bool m_bDragExportPrompt;
	bool m_bFillPrompt;
	bool m_bDupEditPrompt;
	bool m_bSelChanged;
	bool m_bSearchExcluded;
	VEC_BYTE *m_pvSrchFlds;
	VEC_BYTE m_vSrchFlds;
	VEC_BYTE m_vKmlFlds;

	//Assumed orig values when converted --
	INT8 m_iZoneOrg; //either 0 (lat/long) or valid UTM zone (signed)
	INT8 m_iNadOrg; //either 1 (NAD83) or 2 (NAD27)

private:
	void ResourceAllocMsg(LPCSTR pathName);
	BOOL OpenDBF(LPSTR pathName, int shpTyp, UINT uFlags);
	BOOL OpenDBE(LPSTR pathName, UINT uFlags);
	void CloseDBF();
	bool InitTextLabels(CDC *pDC, BOOL bInit);
	void UpdateLayerWithPoint(UINT iPoint, CFltPoint &fpt);
	int CreateShp(LPSTR ebuf, LPCSTR shpPathName, VEC_DWORD &vRec, VEC_FLTPOINT &vFpt, const CShpDef &shpdef);
	void LaunchGE(CString &kmlPath, VEC_DWORD &vRec, BOOL bFly, CFltPoint *pfpt = NULL);
	UINT ParseNextFieldName(LPSTR &p0);
	bool EditFlagsCleared();

	LPBYTE Realloc_pbuf(UINT size)
	{
		return m_pdbfile->pbuf = (LPBYTE)realloc(m_pdbfile->pbuf, m_pdbfile->pbuflen = size);
	}

	COLORREF &MainColor()
	{
		return (ShpType() == SHP_POINT) ? m_mstyle.crBkg : m_mstyle.crMrk;
	}
	void SetShapeClr();
	void SetShapeClr(COLORREF clr)
	{
		MainColor() = clr;
	}

	int CopyPolygonsToDIB(CDIBWrapper *pDestDIB, const CFltRect &geoExt, double fScale);
	int CopyPolylinesToDIB(CDIBWrapper *pDestDIB, const CFltRect &geoExt, double fScale);

	static bool StoreText(LPCSTR s, int lenf, LPBYTE pDest);
	static void StoreZone(int i, char typ, int len, LPBYTE pDest);
	static bool StoreDouble(double x, int len, int dec, LPBYTE pDest);
	static bool StoreExponential(double x, int len, int dec, LPBYTE pDest);

	void StoreRecDouble(double x, LPBYTE pRec, UINT f)
	{
		StoreDouble(x, m_pdb->FldLen(f), m_pdb->FldDec(f), pRec + m_pdb->FldOffset(f));
	}

	void StoreRecZone(int i, LPBYTE pRec, UINT f)
	{
		StoreZone(i, m_pdb->FldTyp(f), m_pdb->FldLen(f), pRec + m_pdb->FldOffset(f));
	}

	void StoreRecText(LPCSTR s, LPBYTE pRec, UINT f)
	{
		StoreText(s, m_pdb->FldLen(f), pRec + m_pdb->FldOffset(f));
	}

	//Data --
	static SHP_DBFILE dbfile[NUM_SHP_DBFILES];
	static LPBYTE m_pRecBuf;
	static UINT m_uRecBufLen;
};

struct SHPOPT {
	SHPOPT() {}
	SHPOPT(CShpLayer *pS, bool bAdv)
		: pShp(pS)
		, pvSrchFlds(bAdv ? pS->m_pvSrchFlds : &pS->m_vSrchFlds)
		, bSearchExcluded(bAdv ? pS->m_bSearchExcluded : pS->IsSearchExcluded())
		, bChanged(false) {}

	~SHPOPT()
	{
		if (bChanged) {
			ASSERT(pvSrchFlds != &pShp->m_vSrchFlds);
			delete pvSrchFlds;
		}
	}
	void ReplaceFlds(VEC_BYTE &vnew)
	{
		if (!bChanged) {
			pvSrchFlds = new VEC_BYTE(vnew);
			bChanged = true;
		}
		else {
			ASSERT(pvSrchFlds != &pShp->m_vSrchFlds);
			pvSrchFlds->swap(vnew);
		}
	}
	CShpLayer *pShp;
	VEC_BYTE *pvSrchFlds;
	bool bSearchExcluded;
	bool bChanged;
};

typedef std::vector<SHPOPT> VEC_SHPOPT;
typedef VEC_SHPOPT::iterator it_shpopt;

extern VEC_SHPOPT vec_shpopt;

void __inline ClearVecShpopt()
{
	VEC_SHPOPT().swap(vec_shpopt);
}

class CConvertHint : public CObject
{
	//	DECLARE_DYNAMIC(CSyncHint)
public:
	CConvertHint(CShpLayer *pShp, int iNad, int iZone) :m_pShp(pShp), m_iNad(iNad), m_iZone(iZone) {}
	CShpLayer *m_pShp;
	int m_iNad, m_iZone;
};

#endif
