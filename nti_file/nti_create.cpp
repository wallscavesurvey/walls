#include <nti_file.h>
#include <trx_str.h>
#include <sys/stat.h>
#include <assert.h>

apfcn_i	nti_Create(NTI_NO *pnti, LPCSTR pszPathName, UINT nWidth, UINT nHeight,
	UINT nBands, UINT nColors, UINT fType, UINT nSzExtra)
{
	// Note: If (fType&NTI_FCNMASK), compression type is 1<<((fType&NTI_FCNMASK)-1).
	// Also, if compression is used, caller must define nti_XfrFcn(fType&NTI_FCNMASK)
	// so that it returns a function pointer of type CSF_XFRFCN
	//
	// If this function returns 0, caller will initialize remaining portion of NTI_FILE header:

	int e;
	NTI_NO pNTI;
	CSF_XFRFCN xfrFcn = NULL;
	NTI_LVL lvl;
	LPSTR pathName;

	*pnti = NULL;

	if (!nWidth || !nHeight ||
		(nBands != 1 && nBands != 3) || nColors > 256) return nti_errno = NTI_ErrFormat;

	_nti_InitLvl(&lvl, nWidth, nHeight, nColors, nBands);

	//No filtering needed if grayscale or pure palette --
	if (lvl.nOvrBands == 1) fType &= ~NTI_FILTERMASK;

	if ((fType&NTI_FCNMASK) && !(xfrFcn = nti_XfrFcn((fType&NTI_FCNMASK) | NTI_FCNENCODE)))
		return nti_errno = NTI_ErrCompressTyp;

	//Get timestamp from original file if it exists --
	long fTime = 0;
	struct _stat status;
	if (!_stat(pszPathName, &status)) fTime = (long)status.st_mtime;

	//Set pathName to point to the new NTI file name in a static buffer --
	if (!(pathName = _nti_getPathname(pszPathName))) return nti_errno;

	//Make nSzExtra divisible by 4 --
	nSzExtra = ((nSzExtra + 3)&~3);

	int szFullHdr = SZ_NTI_HDR + nSzExtra + nColors * sizeof(RGBQUAD);
	if (xfrFcn) szFullHdr += lvl.nRecTotal * sizeof(DWORD);

	lvl.nRecTotal = 0; //Will increment in AppendRec()

	//Allocate zero-filled block for complete file header
	if (!(pNTI = (NTI_NO)dos_AllocNear(sizeof(NTI_FILE_PFX) + szFullHdr))) return nti_errno = NTI_ErrMem;

	pNTI->H.Version = 0;
	pNTI->H.fType = (WORD)fType;

	pNTI->H.nWidth = nWidth;
	pNTI->H.nHeight = nHeight;
	pNTI->H.nBands = (WORD)nBands;
	pNTI->H.nColors = (WORD)nColors;
	pNTI->H.nBlkShf = (WORD)NTI_BLKSHIFT;
	pNTI->H.nSzExtra = (WORD)nSzExtra;
	pNTI->H.SrcTime = fTime;

	pNTI->pszExtra = (char *)(pNTI->Pal + nColors);
	pNTI->pRecIndex = (DWORD *)(pNTI->pszExtra + nSzExtra);

	memcpy(&pNTI->Lvl, &lvl, sizeof(lvl));

	pNTI->Cf.Recsiz = NTI_BLKSIZE;
	pNTI->Cf.Offset = szFullHdr;
	pNTI->Cf.XfrFcn = xfrFcn;
	e = dos_CreateFile(&pNTI->Cf.Handle, pathName, DOS_Exclusive);

	if (e) free(pNTI);
	else *pnti = pNTI;

	return nti_errno = e;
}

apfcn_i nti_CloseDel(NTI_NO nti)
{
	if (!nti) return DOS_ErrArg;
	_csf_Purge(&nti->Cf);
	_csf_DetachCache(&nti->Cf);
	int e = dos_CloseDelete(nti->Cf.Handle);
	dos_FreeNear(nti);
	return nti_errno = e;
}

apfcn_i nti_FlushFileHdr(NTI_NO nti)
{
	nti->H.Version = NTI_VERSION;
	return nti_errno = dos_Transfer(1, nti->Cf.Handle, &nti->H, 0L, nti->Cf.Offset);
}

apfcn_hdrp nti_AppendRecord(NTI_NO nti)
{
	CSH_HDRP hp = _csf_GetRecord(&nti->Cf, nti->Lvl.nRecTotal++, CSH_New | CSH_Updated);
	if (hp) {
		int nComp = (nti->H.nColors && hp->Recno >= nti->Lvl.rec[1]) ? 3 : nti->H.nBands;
		if (nti_AllocCacheBuffers(hp, nComp)) {
			_csh_PurgeHdr(hp);
			return NULL;
		}
		if ((nti->H.fType&NTI_FCNMASK) == NTI_FCNJP2K) {
			for (int i = 0; i < nComp; i++) {
				memset(hp->Reserved[i], 0, NTI_BLKSIZE);
			}
		}
		_csh_LockHdr(hp);
	}
	else nti_errno = nti->Cf.Errno;
	return hp;
}

apfcn_i nti_FlushRecord(CSH_HDRP hp)
{
	_csh_UnLockHdr(hp);
	return nti_errno = _csh_FlushHdr(hp);
}

apfcn_v nti_SetTransform(NTI_NO nti, const double *fTrans)
{
	nti->H.UnitsOffX = fTrans[0];
	nti->H.UnitsOffY = fTrans[3];
	nti->H.UnitsPerPixelX = fTrans[1];
	nti->H.UnitsPerPixelY = fTrans[5];
}

apfcn_i _nti_FlushBuffer()
{
	if (!nti_errno) {
		DWORD count;
		DWORD cnt = _nti_buffer_pos - _nti_buffer;
		if (!WriteFile(_nti_handle, _nti_buffer, cnt, &count, NULL) || count != cnt)
			nti_errno = NTI_ErrWrite;
		else {
			_nti_bytes_written += count;
			_nti_buffer_pos = _nti_buffer;
		}
	}
	return nti_errno;
}

apfcn_v _nti_FlushAndWriteByte(BYTE c)
{
	if (!_nti_FlushBuffer()) *_nti_buffer_pos++ = c;
}

apfcn_i nti_CreateRecord(NTI_NO nti, DWORD recno)
{
	//Initialize transfer buffer --
	if (!_nti_buffer && _nti_AllocBuffer()) return nti_errno;
	_nti_buffer_pos = _nti_buffer;
	_nti_bytes_written = 0;
	_nti_handle = ((DOS_FP)nti->Cf.Handle)->DosHandle;
	if (!SetFilePointerEx(_nti_handle, nti_GetRecOffset(nti, recno), NULL, FILE_BEGIN)) {
		//DWORD w=GetLastError();
		return nti_errno = NTI_ErrWrite;
	}
	return nti_errno = 0;
}

