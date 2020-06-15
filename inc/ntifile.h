// NTIILE.H -- C++ Wrapper for NTI_FILE.LIB --

#ifndef __NTIILE_H
#define __NTIILE_H

#ifndef __NTI_FILE_H
#include "nti_file.h"
#endif

class CNTIFile {
	// Attributes
protected:
	NTI_NO m_nti;

	// Constructors
public:
	CNTIFile() : m_nti(NULL) {}
	virtual ~CNTIFile() { if (m_nti) nti_Close(m_nti); }

	void TransferHandle(CNTIFile &s_nti)
	{
		m_nti = s_nti.m_nti;
		s_nti.m_nti = NULL;
	}

	void NullifyHandle()
	{
		m_nti = NULL;
	}

	// Operations 
	int Open(LPCSTR pszPathName, BOOL bReadOnly = TRUE)
	{
		return nti_Open(&m_nti, pszPathName, bReadOnly);
	}

	int Create(LPCSTR pszPathName, UINT nWidth, UINT nHeight,
		UINT nBands, UINT nColors, UINT fType = 0, UINT nSzExtra = 0)
	{
		return nti_Create(&m_nti, pszPathName, nWidth, nHeight, nBands, nColors, fType, nSzExtra);
	}

	void SetTransform(const double *fTrans) { return nti_SetTransform(m_nti, fTrans); }

	int Close()
	{
		int e = nti_Close(m_nti);
		m_nti = 0;
		return e;
	}
	int CloseDel()
	{
		int e = nti_CloseDel(m_nti);
		m_nti = 0;
		return e;
	}

	//Cache operations --
	int	ResizeCache(UINT numbufs) { return nti_ResizeCache(m_nti, numbufs); }

	static TRX_CSH DefaultCache() { return nti_defCache; }

	int AttachCache(UINT numbufs = NTI_DFLTCACHE_BLKS) {
		if (!nti_defCache && (nti_errno = csh_Alloc(&nti_defCache, NTI_BLKSIZE, numbufs))) return nti_errno;
		return nti_errno = nti_AttachCache(m_nti, nti_defCache);
	}

	int DetachCache(bool bDeleteIfDflt = false) { return nti_DetachCache(m_nti, bDeleteIfDflt); }

	static int FreeDefaultCache() {
		if (!nti_defCache) return 0;
		nti_errno = csh_Free(nti_defCache);
		nti_defCache = NULL;
		return nti_errno;
	}

	void PurgeCache() { _csf_Purge(&m_nti->Cf); }

	//Data retrieving functions --*/
	int	ReadBGR(LPBYTE pBuffer, UINT lvl, UINT offX, UINT offY, UINT sizX, UINT sizY, UINT rowBytes)
	{
		return nti_ReadBGR(m_nti, pBuffer, lvl, offX, offY, sizX, sizY, rowBytes);
	}

	int	ReadBytes(LPBYTE pBuffer, UINT lvl, UINT offX, UINT offY, UINT sizX, UINT sizY, UINT rowBytes, UINT bandOff = 0)
	{
		return nti_ReadBytes(m_nti, pBuffer, lvl, offX, offY, sizX, sizY, rowBytes, bandOff);
	}
#ifdef _CONVERT_PAL
	int	ReadPaletteBGR(LPBYTE pBuffer, UINT offX, UINT offY, UINT sizX, UINT sizY, UINT rowBytes)
	{
		return nti_ReadPaletteBGR(m_nti, pBuffer, offX, offY, sizX, sizY, rowBytes);
	}
#endif
	void SetReadOnly()
	{
		m_nti->Cf.XfrFcn = DecodeFcn();
	}

	CSH_HDRP AppendRecord() { return nti_AppendRecord(m_nti); } //Requires FlushRecord() after block is filled
	TRX_CSH Cache() { return (TRX_CSH)m_nti->Cf.Csh; }
	int		DosHandle() { return m_nti->Cf.Handle; }
	CSF_XFRFCN EncodeFcn() { return nti_XfrFcn((m_nti->H.fType&NTI_FCNMASK) | NTI_FCNENCODE); }
	CSF_XFRFCN DecodeFcn() { return nti_XfrFcn(m_nti->H.fType&NTI_FCNMASK); }
	static  int     Errno() { return nti_errno; }
	static  LPSTR   Errstr() { return nti_Errstr(Errno()); }
	static  LPSTR   Errstr(UINT e) { return nti_Errstr(e); }
	LPSTR   FileName() { return dos_FileName(m_nti->Cf.Handle); }
	__int64 FileSize() { return nti_FileSize(m_nti); }
	int		FilterFlag() const { return (m_nti->H.fType&NTI_FILTERMASK); }
	LPCSTR	FilterName() const { return nti_filter_name[FilterFlag() >> NTI_FILTERSHFT]; }
	int		FlushFileHdr() { return nti_FlushFileHdr(m_nti); }
	static  void FreeDecode() { nti_XfrFcn(-1); }
	static  void FreeBuffer() { _nti_FreeBuffer(); }
	static  int FlushRecord(CSH_HDRP hp) { return nti_FlushRecord(hp); }
	LPSTR	GetDatumName() { return m_nti->H.csDatumName; }
	double	GetMetersPerUnit() { return m_nti->H.MetersPerUnit; }
	NTI_NO	GetNTI() const { return m_nti; }
	LPCSTR  GetParams() const { return m_nti->pszExtra + strlen(m_nti->pszExtra) + 1; };
	LPCSTR	GetProjectionRef() { return m_nti->pszExtra; }
	void	GetRecDims(UINT rec, UINT *width, UINT *height) { return nti_GetRecDims(m_nti, rec, width, height); }
	DWORD	*GetRecIndex() { return m_nti->pRecIndex; }
	LARGE_INTEGER GetRecOffset(DWORD recno) { return nti_GetRecOffset(m_nti, recno); }
	CSH_HDRP GetRecord(DWORD rec) { return nti_GetRecord(m_nti, rec); }
	long    GetSrcTime() { return m_nti->H.SrcTime; }
	LPCSTR  GetTransColorStr() const
	{
		LPCSTR p = GetParams();
		p += strlen(p) + 1;
		return ((p - m_nti->pszExtra) < m_nti->H.nSzExtra && strlen(p) > 6) ? p : (p - 1);
	}
	void	GetTransform(double *fTrans) { nti_GetTransform(m_nti, fTrans); }
	LPCSTR	GetUnitsName() { return m_nti->H.csUnitsName; }
	int		GetUTMZone() { return m_nti->H.cZone; }
	UINT	HdrSize() { return m_nti->Cf.Offset; }
	UINT	Height() const { return m_nti->H.nHeight; }
	bool	IsCompressed() const { return (m_nti->H.fType&NTI_FCNMASK) != 0; }
	bool	IsGeoreferenced() {
		return m_nti->H.UnitsPerPixelY < 0.0 && rint(10.0*(m_nti->H.UnitsPerPixelY + m_nti->H.UnitsPerPixelX)) == 0.0;
	}
	bool    IsGrayscale() { return NumColors() == 0 && NumBands() == 1; }
	bool	IsProjected() {
		return strlen(m_nti->pszExtra) > 6 && !memcmp(m_nti->pszExtra, "PROJCS[", 0);
	}
	bool	IsZoneNorth() { return m_nti->H.cZone > 0; }
	static void	Lco_UnInit() { lco_UnInit(); }
	UINT	LvlBands(int lvl) const { return lvl ? m_nti->Lvl.nOvrBands : m_nti->H.nBands; }
	UINT	LvlHeight(int lvl) const { return m_nti->Lvl.height[lvl]; }
	double	LvlPercent(int lvl) const { return (100.0*(double)SizeEncoded(lvl)) / (double)SizeDecoded(lvl); }
	UINT	LvlRec(int lvl) const { return m_nti->Lvl.rec[lvl]; }
	UINT	LvlWidth(int lvl) const { return m_nti->Lvl.width[lvl]; }
	double  & UnitsOffX() { return m_nti->H.UnitsOffX; }
	double  & UnitsOffY() { return m_nti->H.UnitsOffY; }
	UINT    NumBands() { return m_nti->H.nBands; }
	UINT    NumColors() { return m_nti->H.nColors; }
	UINT	NumLevels() { return m_nti->Lvl.nLevels; }
	UINT	NumOvrBands() { return m_nti->Lvl.nOvrBands; }
	UINT    NumRecs() { return m_nti->Lvl.nRecTotal; }
	UINT	NumRecs(UINT lvl) { return nti_NumLvlRecs(m_nti, lvl); }
	bool	Opened() const { return m_nti != NULL; }
	RGBQUAD *Palette() { return NumColors() ? m_nti->Pal : NULL; }
	LPCSTR  PathName() { return dos_PathName(m_nti->Cf.Handle); }
	double  Percent() const
	{
		return (100.0*(double)SizeEncoded()) / ((double)SizeDecoded());
	}
	HANDLE  Handle() { return ((DOS_FP)m_nti->Cf.Handle)->DosHandle; }
	UINT	RecordCapacity() { return  LvlRec(m_nti->Lvl.nLevels); }
	void	SetDatumName(LPCSTR s) { strncpy(m_nti->H.csDatumName, s, NTI_SIZDATUMNAME - 1); }
	int		SetErrno(int err) { return nti_errno = err; }
	void	SetMetersPerUnit(double fMetersPerUnit) { m_nti->H.MetersPerUnit = fMetersPerUnit; }
	void	SetRecTotal(DWORD rec) { m_nti->Lvl.nRecTotal = rec; }
	void	SetUnitsName(LPCSTR s) { strncpy(m_nti->H.csUnitsName, s, NTI_SIZUNITSNAME - 1); }
	void	SetUTMZone(int zone) { m_nti->H.cZone = (char)zone; }
	__int64	SizeDecoded() const { return nti_SizeDecoded(m_nti); }
	__int64	SizeDecoded(UINT lvl) const { return nti_SizeDecodedLvl(m_nti, lvl); }
	__int64	SizeEncoded() const { return nti_SizeEncoded(m_nti); }
	__int64	SizeEncoded(UINT lvl) const { return nti_SizeEncodedLvl(m_nti, lvl); }
	UINT	SizeExtra() const { return m_nti->H.nSzExtra; }
	int		TruncateOverviews() { return nti_TruncateOverviews(m_nti); }
	UINT	Width() const { return m_nti->H.nWidth; }
	UINT	XfrFcnID() const { return (m_nti->H.fType&NTI_FCNMASK); }
	LPCSTR	XfrFcnName() const { return nti_xfr_name[XfrFcnID()]; }
};
#endif

