#ifndef __NTI_FILE_H
#define __NTI_FILE_H

#ifndef __A__NTI_H
#include <a__nti.h>
#endif

#ifndef NTI_TIMER_H
#include <GetElapsedSecs.h>
#endif

#undef _CONVERT_PAL

/*Basic error codes common to DBF, TRX, and NTI file types --*/
enum {
	NTI_OK,
	NTI_ErrName,
	NTI_ErrFind,
	NTI_ErrArg,
	NTI_ErrMem,
	NTI_ErrRecsiz,
	NTI_ErrNoCache,
	NTI_ErrLocked,
	NTI_ErrRead,
	NTI_ErrWrite,
	NTI_ErrCreate,
	NTI_ErrOpen,
	NTI_ErrClose,
	NTI_ErrDelete,
	NTI_ErrLimit,
	NTI_ErrShare,
	NTI_ErrFormat,
	NTI_ErrReadOnly,
	NTI_ErrSpace,
	NTI_ErrFileType,
	NTI_ErrDiskIO,
	NTI_ErrLock,
	NTI_ErrFileShared,
	NTI_ErrUsrAbort
};

/*Specific errors for NTI files --*/
#define NTI_ERR_BASE 100

enum {
	NTI_ErrVersion = NTI_ERR_BASE,
	NTI_ErrOpenFile,
	NTI_ErrSizeImage,
	NTI_ErrCompressTyp,
	NTI_ErrCompress,
	NTI_ErrCompressInit,
	NTI_ErrDecompress,
	NTI_ErrDecompressInit,
	NTI_ErrCompressSize,
	NTI_ErrCacheRecsiz,
	NTI_ErrCacheResize,
	NTI_ErrNoOverviews,
	NTI_ErrTruncate
};

extern TRX_CSH	nti_defCache;
extern int		nti_errno;
extern BOOL		nti_bTruncateOnClose;

extern unsigned __int64 nti_tm_inf, nti_tm_def;
extern UINT nti_tm_inf_cnt, nti_tm_def_cnt;
extern bool nti_inf_timing, nti_def_timing;

apfcn_i		nti_AllocCache(NTI_NO nti, UINT numbufs);
apfcn_i		nti_AllocCacheBuffers(CSH_HDRP hp, int nComp);
apfcn_hdrp	nti_AppendRecord(NTI_NO nti);
apfcn_i		nti_AttachCache(NTI_NO nti, TRX_CSH csh);
apfcn_i		nti_Close(NTI_NO nti);
apfcn_i		nti_CloseDel(NTI_NO nti);
apfcn_i		nti_DetachCache(NTI_NO nti, BOOL bDeleteIfDefault);
apfcn_i		nti_Create(NTI_NO *pnti, LPCSTR pszPathName, UINT nWidth, UINT nHeight,
	UINT nBands, UINT nColors, UINT fType, UINT nSzExtra);
apfcn_i		nti_CreateRecord(NTI_NO nti, DWORD recno);
apfcn_s		nti_Errstr(UINT e);
__int64		nti_FileSize(NTI_NO nti);
apfcn_i		nti_FlushFileHdr(NTI_NO nti);
apfcn_i		nti_FlushRecord(CSH_HDRP hp);
apfcn_v		nti_GetRecDims(NTI_NO nti, UINT rec, UINT *width, UINT *height);
LARGE_INTEGER nti_GetRecOffset(NTI_NO nti, DWORD recno);
apfcn_u		net_GetRecLenEncoded(NTI_NO nti, DWORD recno, LARGE_INTEGER *recOffset);
CSH_HDRP	nti_GetRecord(NTI_NO nti, DWORD rec);
apfcn_v		nti_GetTransform(NTI_NO nti, double *fTrans);
CSF_XFRFCN	nti_XfrFcn(int typ);
apfcn_p		nti_InsertRecord(NTI_NO nti, DWORD rec);
apfcn_u		nti_NumLvlRecs(NTI_NO nti, UINT lvl);
apfcn_i		nti_Open(NTI_NO *pnti, LPCSTR pszPathname, BOOL bReadOnly);
apfcn_i		nti_OpenRecord(NTI_NO nti, DWORD recno);
apfcn_i		nti_ReadBytes(NTI_NO nti, LPBYTE pBuffer, UINT lvl, UINT offX, UINT offY, UINT sizX, UINT sizY, UINT rowBytes, UINT bandOff);
#ifdef	_CONVERT_PAL
apfcn_i		nti_ReadPaletteBGR(NTI_NO nti, LPBYTE pBuffer, UINT offX, UINT offY, UINT sizX, UINT sizY, UINT rowBytes);
#endif
apfcn_i		nti_ReadBGR(NTI_NO nti, LPBYTE pBuffer, UINT lvl, UINT offX, UINT offY, UINT sizX, UINT sizY, UINT rowBytes);
apfcn_i		nti_ResizeCache(NTI_NO nti, UINT numbufs);
apfcn_v		nti_SetTransform(NTI_NO nti, const double *fTrans);
__int64		nti_SizeDecoded(NTI_NO nti);
__int64		nti_SizeDecodedLvl(NTI_NO nti, UINT lvl);
__int64		nti_SizeEncoded(NTI_NO nti);
__int64		nti_SizeEncodedLvl(NTI_NO nti, UINT lvl);
apfcn_i		nti_TruncateOverviews(NTI_NO nti);

//General utilities --
double	rint(double x); //rint.cpp (round to nearest int)

//Compression fcns and data --
//Change as supported compression methods are added --

//static buffer fcns --
extern LPBYTE _nti_buffer, _nti_buffer_end, _nti_buffer_pos, _nti_buffer_lim;
extern HANDLE _nti_handle;
extern DWORD  _nti_bytes_toread, _nti_bytes_written;
apfcn_i		_nti_AllocBuffer();
apfcn_v		_nti_FreeBuffer();
apfcn_i		_nti_FlushBuffer();
apfcn_i		_nti_FillBuffer();

__inline bool _nti_write(const BYTE *pSrc, UINT lenSrc)
{
	while (lenSrc) {
		UINT len = (UINT)(_nti_buffer_end - _nti_buffer_pos);
		if (!len && _nti_FlushBuffer()) return false;
		if (len > lenSrc) len = lenSrc;
		memcpy(_nti_buffer_pos, pSrc, len);
		_nti_buffer_pos += len;
		pSrc += len;
		lenSrc -= len;
	}
	return true;
}

__inline int _nti_read(LPBYTE pDst, UINT lenDst)
{
	if (lenDst > _nti_bytes_toread) lenDst = _nti_bytes_toread;
	UINT rem = lenDst;
	while (rem) {
		UINT len = (UINT)(_nti_buffer_end - _nti_buffer_pos);
		if (!len && !(len = _nti_FillBuffer())) return 0;
		if (len > rem) len = rem;
		memcpy(pDst, _nti_buffer_pos, len);
		_nti_buffer_pos += len;
		_nti_bytes_toread -= len;
		pDst += len;
		rem -= len;
	}
	return lenDst;
}

#ifdef _USE_LZMA
#define NTI_XFR_METHODS 5
int nti_lzma_d(CSH_HDRP hp, UINT typ);
int nti_lzma_e(CSH_HDRP hp, UINT typ);
LPSTR nti_lzma_errmsg();
extern int nti_errno_lzma;
#else
#define NTI_XFR_METHODS 4
#endif

extern LPCSTR nti_xfr_name[NTI_XFR_METHODS + 1];
#define NTI_FILTER_METHODS 3
extern LPCSTR nti_filter_name[NTI_FILTER_METHODS + 1];
typedef LPSTR(*NTI_XFR_ERRMSG)();
extern NTI_XFR_ERRMSG nti_xfr_errmsg;

int nti_zlib_d(CSH_HDRP hp, UINT typ);
int nti_zlib_e(CSH_HDRP hp, UINT typ);
LPSTR nti_zlib_errmsg();
extern int nti_errno_zlib;

int nti_loco_e(CSH_HDRP hp, UINT typ);
int nti_loco_d(CSH_HDRP hp, UINT typ);
void lco_UnInit();
LPSTR nti_loco_errmsg();
extern int nti_errno_loco;

#define NTI_MAX_JP2KRATE 3.2
#define NTI_SCALE_JP2KRATE 1.2
int nti_jp2k_d(CSH_HDRP hp, UINT typ);
int nti_jp2k_e(CSH_HDRP hp, UINT typ);
LPSTR nti_jp2k_errmsg();
extern int nti_errno_jp2k;
extern double nti_jp2k_rate;
enum e_jp2k_err { JP2K_ErrNone, JP2K_ErrFormat, JP2K_ErrUnknown }; //To fill out later

int nti_webp_d(CSH_HDRP hp, UINT typ);
int nti_webp_e(CSH_HDRP hp, UINT typ);
LPSTR nti_webp_errmsg();
extern float nti_webp_quality;
extern int nti_webp_preset;
enum e_webp_err { WEBP_ErrNone, WEBP_ErrFormat, WEBP_ErrUnknown }; //To fill out later
extern LPBYTE webp_buffer;

#endif

