#include <nti_file.h>
#include <trx_str.h>
#include <sys/stat.h>
#include <assert.h>

char *nti_dot_extension = ".nti";

//Change as supported compression methods and filters are added --
#ifdef _USE_LZMA
LPCSTR nti_xfr_name[NTI_XFR_METHODS + 1] = { "None","Deflate","LOCO-I","JP2K","WebP","Lzma" };
#else
LPCSTR nti_xfr_name[NTI_XFR_METHODS + 1] = { "None","Deflate","LOCO-I","JP2K","WebP" };
#endif
LPCSTR nti_filter_name[NTI_FILTER_METHODS + 1] = { "none","R, G-R, B-R","R-G, G, B-G","R-B, G-B, B" };

NTI_XFR_ERRMSG nti_xfr_errmsg; //Initialized by nti_xxx() when error occurs

LPBYTE _nti_buffer = NULL;
LPBYTE _nti_buffer_pos, _nti_buffer_end, _nti_buffer_lim;
HANDLE _nti_handle;
DWORD _nti_bytes_toread, _nti_bytes_written;
BOOL nti_bTruncateOnClose;

TRX_CSH nti_defCache = NULL;
int nti_errno = 0;

unsigned __int64 nti_tm_inf, nti_tm_def;
UINT nti_tm_inf_cnt, nti_tm_def_cnt;
bool nti_inf_timing = false, nti_def_timing = false;

apfcn_v _nti_FreeBuffer()
{
	free(_nti_buffer);
	_nti_buffer = NULL;
}

apfcn_i _nti_AllocBuffer()
{
	assert(!_nti_buffer);
	try {
		if (!(_nti_buffer = (LPBYTE)malloc(NTI_BLKSIZE))) return nti_errno = NTI_ErrMem;
		_nti_buffer_end = _nti_buffer + NTI_BLKSIZE;
	}
	catch (char* e)
	{
		e = 0;
		return nti_errno = NTI_ErrMem;
	}
	return 0;
}

apfcn_i _nti_FillBuffer()
{
	//Fill buffer with as much record data as possible
	//while shifting any unread data it already contains
	//to the beginning. Return length of data in buffer
	//after operation.

	DWORD cnt = (DWORD)(_nti_buffer_end - _nti_buffer_pos);
	if (cnt == NTI_BLKSIZE) {
		//buffer already filled with unread bytes --
		assert(false);
		return cnt;
	}
	if (cnt > _nti_bytes_toread) cnt = _nti_bytes_toread;
	//cnt = unread bytes already in buffer
	if (cnt) memmove(_nti_buffer, _nti_buffer_pos, cnt);
	_nti_buffer_pos = _nti_buffer;
	if (cnt == _nti_bytes_toread) return cnt;

	//cnt==unread bytes at beginning of buffer
	_nti_buffer_pos += cnt; //position of ReadFile

	DWORD count = (DWORD)(_nti_buffer_end - _nti_buffer_pos); //maximum length of data to read
	if (count + cnt > _nti_bytes_toread) count = _nti_bytes_toread - cnt;

	if (!ReadFile(_nti_handle, _nti_buffer_pos, count, &cnt, NULL) || count != cnt) {
		nti_errno = NTI_ErrRead;
		return 0;
	}

	cnt += (DWORD)(_nti_buffer_pos - _nti_buffer);
	_nti_buffer_pos = _nti_buffer;
	return cnt;
}

apfcn_s _nti_getPathname(LPCSTR pszPathName)
{
	char *pathName;

	bool bExt = !_stricmp(trx_Stpext(pszPathName), nti_dot_extension);

	if (pathName = dos_FullPath(pszPathName, NULL)) {
		if (bExt || (strlen(pathName) <= MAX_PATH - 4 && strcat(pathName, nti_dot_extension))) {
			nti_errno = 0;
			return pathName;
		}
	}
	nti_errno = NTI_ErrName;
	return NULL;
}

apfcn_v _nti_InitLvl(NTI_LVL *pLvl, UINT nWidth, UINT nHeight, UINT nColors, UINT nBands)
{
	UINT recs = 0, shf = 0;

	for (; shf < NTI_MAXLEVELS; shf++) {
		pLvl->rec[shf] = recs;
		pLvl->width[shf] = nWidth;
		pLvl->height[shf] = nHeight;
		pLvl->cols[shf] = (nWidth + NTI_BLKWIDTH - 1) / NTI_BLKWIDTH;
		pLvl->rows[shf] = (nHeight + NTI_BLKWIDTH - 1) / NTI_BLKWIDTH;
		UINT w = pLvl->cols[shf] * pLvl->rows[shf];
		recs += w;
		if (w == 1) break;
		nWidth = ((nWidth + 1) >> 1);
		nHeight = ((nHeight + 1) >> 1);
	}
	if (shf == NTI_MAXLEVELS) {
		assert(false);
		shf = NTI_MAXLEVELS - 1;
	}
	pLvl->nOvrBands = (shf && nColors) ? 3 : nBands;
	pLvl->nLevels = ++shf;
	pLvl->nRecTotal = pLvl->rec[shf] = recs;

}

static int nti_CloseAndTruncate(NTI_NO nti)
{
	int e;
	char pathName[_MAX_PATH];
	strcpy(pathName, ((DOS_FP)nti->Cf.Handle)->PathName);
	if (e = dos_CloseFile(nti->Cf.Handle)) goto _fail;
	if (!(e = _dos_OpenFile(&nti->Cf.Handle, pathName, O_RDWR, DOS_Exclusive))) {
		e = nti_TruncateOverviews(nti);
		dos_CloseFile(nti->Cf.Handle);
	}
_fail:
	dos_FreeNear(nti);
	return nti_errno = e;
}

apfcn_i nti_Close(NTI_NO nti)
{
	int e = 0;
	if (!nti) e = DOS_ErrArg;
	else e = _csf_DetachCache(&nti->Cf);

	if (nti_bTruncateOnClose) {
		nti_bTruncateOnClose = FALSE;
		if (!e)
			return nti_CloseAndTruncate(nti);
	}
	if (nti) {
		if (dos_CloseFile(nti->Cf.Handle) && !e) e = DOS_ErrClose;
		dos_FreeNear(nti);
	}
	return nti_errno = e;
}

CSH_HDRP nti_GetRecord(NTI_NO nti, DWORD rec)
{
	CSH_HDRP hp = _csf_GetRecord(&nti->Cf, rec, 0);
	if (!hp) nti_errno = nti->Cf.Errno;
	return hp;
}

apfcn_i nti_AllocCache(NTI_NO nti, UINT numbufs)
{
	UINT e;
	TRX_CSH csh;

	if ((e = csh_Alloc(&csh, NTI_BLKSIZE, numbufs)) == 0 &&
		(e = nti_AttachCache(nti, csh)) != 0) csh_Free(csh);

	return nti_errno = e;
}

apfcn_i nti_AttachCache(NTI_NO nti, TRX_CSH csh)
{
	if (!csh) return nti_errno = DOS_ErrArg;
	if (((CSH_NO)csh)->Bufsiz != NTI_BLKSIZE) return nti_errno = NTI_ErrCacheRecsiz;
	return nti_errno = _csf_AttachCache(&nti->Cf, (CSH_NO)csh);
}

apfcn_i nti_DetachCache(NTI_NO nti, BOOL bDeleteIfDflt)
{
	//Detach, and if bDeleteIfDflt==TRUE and if this is the default cache
	//*and* if no other files are attached, then delete the default cache --
	return nti_errno = _csf_DeleteCache(&nti->Cf, bDeleteIfDflt ? (CSH_NO *)&nti_defCache : NULL);
}

apfcn_v nti_GetTransform(NTI_NO nti, double *fTrans)
{
	fTrans[0] = nti->H.UnitsOffX;
	fTrans[1] = nti->H.UnitsPerPixelX;
	fTrans[2] = 0.0;
	fTrans[3] = nti->H.UnitsOffY;
	fTrans[4] = 0.0;
	fTrans[5] = nti->H.UnitsPerPixelY;
}

#ifdef _CONVERT_PAL
apfcn_i nti_ReadPaletteBGR(NTI_NO nti, LPBYTE pBuffer, UINT offX, UINT offY, UINT sizX, UINT sizY, UINT rowBytes)
{
	//Read an RGB image rectangle: TL=(offX,offY), BR=(offX+sizX,offY+sizY) at lvl=0.
	//This function assumes that the desired rectangle is completely covered by the image!
	//Notes: rowBytes is normally either sizX*3 or (3*sizX+3)&~3 for DWORD alignment.
	//This function should be used only for retrieving RGB triples at level 0 of palleted images.

	assert(nti->H.nBands == 1 && nti->H.nColors > 1);
	assert(offX + sizX <= nti->H.nWidth && offY + sizY <= nti->H.nHeight);

	UINT lvlCols = nti->Lvl.cols[0];

	UINT recOff = (offY / NTI_BLKWIDTH)*lvlCols + offX / NTI_BLKWIDTH; //record with UL corner of retrieved image
	RGBQUAD *pal = nti->Pal;

	//starting offsets within first record retrieved --
	offX %= NTI_BLKWIDTH;
	offY %= NTI_BLKWIDTH;

	while ((int)sizY > 0) {
		LPBYTE pDst = pBuffer;
		UINT rec = recOff;
		UINT width = sizX;
		UINT height = min(sizY, NTI_BLKWIDTH - offY);
		for (UINT x0 = offX; (int)width > 0; x0 = 0) {
			//Obtain record for one block --
			UINT lenX = min(width, NTI_BLKWIDTH - x0);
#ifdef _DEBUG
			UINT w, h;
			nti_GetRecDims(nti, rec, &w, &h);
			assert(h <= offY + height);
			assert(w <= x0 + lenX);
#endif
			CSH_HDRP hp = _csf_GetRecord(&nti->Cf, rec++, 0);
			if (!hp) return nti_errno = nti->Cf.Errno;
			assert(hp->Buffer != NULL);

			LPBYTE pIndex = hp->Buffer + x0;
			for (UINT y = 0; y < height; y++) {
				LPBYTE p = pDst;
				LPBYTE pi = pIndex;
				for (UINT x = 0; x < lenX; x++) {
					RGBQUAD rgb = pal[*pi++];
					*p++ = rgb.rgbBlue;
					*p++ = rgb.rgbGreen;
					*p++ = rgb.rgbRed;
				}
				pIndex += NTI_BLKWIDTH;
				pDst += 3 * (NTI_BLKWIDTH - x0); //??? 
			}
			//pDst+=3*(NTI_BLKWIDTH-x0); //???
			width -= NTI_BLKWIDTH - x0;
		}
		pBuffer += NTI_BLKWIDTH * rowBytes;
		sizY -= NTI_BLKWIDTH - offY;
		offY = 0;
		recOff += lvlCols;
	}

	return 0;
}
#endif

apfcn_i	nti_ReadBGR(NTI_NO nti, LPBYTE pBuffer, UINT lvl,
	UINT offX, UINT offY, UINT sizX, UINT sizY, UINT rowBytes)
{
	//Read an RGB image rectangle: TL=(offX,offY), BR=(offX+sizX,offY+sizY)
	//This function assumes that the desired rectangle is completely covered by the image!
	//Notes: rowBytes is normally either sizX*3 or (3*sizX+3)&~3 for DWORD alignment.
	//lvl is normally >1 for palleted images or >=0 for BGR images.

	assert(lvl < nti->Lvl.nLevels && offX + sizX <= nti->Lvl.width[lvl] && offY + sizY <= nti->Lvl.height[lvl]);

	int  iFilter = (nti->H.fType&NTI_FILTERMASK);
	UINT lvlCols = nti->Lvl.cols[lvl];

	UINT recOff = nti->Lvl.rec[lvl] + (offY / NTI_BLKWIDTH)*lvlCols + offX / NTI_BLKWIDTH; //record with UL corner of retrieved image

	//starting offsets within first record retrieved --
	offX %= NTI_BLKWIDTH;
	offY %= NTI_BLKWIDTH;

	while ((int)sizY > 0) {
		//Read one row of blocks
		LPBYTE pDst = pBuffer;
		UINT rec = recOff;
		UINT width = sizX;
		UINT height = min(sizY, NTI_BLKWIDTH - offY);

		for (UINT x0 = offX; (int)width > 0;) {
			//Obtain record for one block --
			UINT lenX = min(width, NTI_BLKWIDTH - x0);
			CSH_HDRP hp = _csf_GetRecord(&nti->Cf, rec++, 0);
			if (!hp) return nti_errno = nti->Cf.Errno;
			assert(hp->Reserved[0] != NULL && hp->Reserved[1] != NULL && hp->Reserved[2] != NULL);

			LPBYTE pBlue = hp->Reserved[0] + x0;
			LPBYTE pGreen = hp->Reserved[1] + x0;
			LPBYTE pRed = hp->Reserved[2] + x0;
			LPBYTE dstBuf = pDst;

			for (UINT y = 0; y < height; y++) {
				LPBYTE p = dstBuf;
				LPBYTE pB = pBlue;
				LPBYTE pG = pGreen;
				LPBYTE pR = pRed;
				if (iFilter) {
					if (iFilter == NTI_FILTERGREEN) {
						for (UINT x = 0; x < lenX; x++) {
							*p++ = (*pG + *pB++);
							*p++ = *pG;
							*p++ = (*pR++ + *pG++);
						}
					}
					else if (iFilter == NTI_FILTERRED) {
						for (UINT x = 0; x < lenX; x++) {
							*p++ = (*pR + *pB++);
							*p++ = (*pR + *pG++);
							*p++ = *pR++;
						}
					}
					else for (UINT x = 0; x < lenX; x++) {
						*p++ = *pB;
						*p++ = (*pB + *pG++);
						*p++ = (*pR++ + *pB++);
					}
				}
				else {
					for (UINT x = 0; x < lenX; x++) {
						*p++ = *pB++;
						*p++ = *pG++;
						*p++ = *pR++;
					}
				}

				dstBuf += rowBytes;
				pBlue += NTI_BLKWIDTH;
				pGreen += NTI_BLKWIDTH;
				pRed += NTI_BLKWIDTH;
			}

			pDst += (NTI_BLKWIDTH - x0) * 3;
			width -= NTI_BLKWIDTH - x0;
			x0 = 0;
		}
		sizY -= NTI_BLKWIDTH - offY;
		offY = 0;
		recOff += lvlCols;
		pBuffer += NTI_BLKWIDTH * rowBytes;
	}
	return 0;
}

//try __inline
static void copy_filtered_row(LPBYTE p, CSH_HDRP hp, int iFilter, UINT bandOff, UINT recBufOff, UINT lenX)
{
	LPBYTE pB = hp->Reserved[0] + recBufOff;
	LPBYTE pG = hp->Reserved[1] + recBufOff;
	LPBYTE pR = hp->Reserved[2] + recBufOff;

	if (bandOff == 0) {
		//blue band --
		if (iFilter == NTI_FILTERGREEN) {
			for (UINT x = 0; x < lenX; x++) {
				*p++ = (*pG++ + *pB++);
			}
		}
		else for (UINT x = 0; x < lenX; x++) {
			*p++ = (*pR++ + *pB++);
		}
	}
	else if (bandOff == 1) {
		//Green band --
		if (iFilter == NTI_FILTERRED) {
			for (UINT x = 0; x < lenX; x++) {
				*p++ = (*pR++ + *pG++);
			}
		}
		else for (UINT x = 0; x < lenX; x++) {
			*p++ = (*pB++ + *pG++);
		}
	}
	else {
		//Red band --
		if (iFilter == NTI_FILTERGREEN) {
			for (UINT x = 0; x < lenX; x++) {
				*p++ = (*pR++ + *pG++);
			}
		}
		else for (UINT x = 0; x < lenX; x++) {
			*p++ = (*pR++ + *pB++);
		}
	}
}

apfcn_i	nti_ReadBytes(NTI_NO nti, LPBYTE pBuffer, UINT lvl,
	UINT offX, UINT offY, UINT sizX, UINT sizY, UINT rowBytes, UINT bandOff/*=0*/)
{
	//Read one band of an image rectangle: TL=(offX,offY), BR=(offX+sizX,offY+sizY)
	//This function assumes that the desired rectangle is completely covered by the image,
	//which means offX+sizX<=nti->Lvl.width[lvl]!
	//
	//rowBytes is normally either sizX or (sizX+3)&~3 for DWORD alignment.
	//lvl is normally 0 (palleted images for which only the byte indices are needed), or
	//lvl>=0 for grayscale images, or (rarely) lvl>=0 when only one band, bandOff, is needed.

	assert(lvl < nti->Lvl.nLevels && (offX + sizX) <= nti->Lvl.width[lvl] && (sizY + offY) <= nti->Lvl.height[lvl]);

#ifdef _DEBUG
	LPBYTE pbMax = pBuffer + rowBytes * sizY;
#endif

	int  iFilter = (nti->H.fType&NTI_FILTERMASK);
	if (iFilter != NTI_FILTERNONE) {
		if (!lvl && nti->H.nBands == 1 || bandOff == 0 && iFilter == NTI_FILTERBLUE || bandOff == 1 && iFilter == NTI_FILTERGREEN ||
			bandOff == 2 && iFilter == NTI_FILTERRED) iFilter = NTI_FILTERNONE;
	}

	UINT lvlCols = nti->Lvl.cols[lvl];

	UINT recOff = nti->Lvl.rec[lvl] + (offY / NTI_BLKWIDTH)*lvlCols + offX / NTI_BLKWIDTH; //record with UL corner of retrieved image

	//starting offsets within first record retrieved --
	offX %= NTI_BLKWIDTH;
	offY %= NTI_BLKWIDTH;

	while ((int)sizY > 0) {
		//Read one row of blocks
		LPBYTE pDst = pBuffer;
		UINT rec = recOff;
		UINT x0 = offX;
		UINT lenY = min(sizY, NTI_BLKWIDTH - offY); //add "*rowBytes" later
		for (UINT width = sizX; width;) {
			//Obtain record for one block --
			UINT lenX = min(width, NTI_BLKWIDTH - x0);
#ifdef _DEBUG
			UINT w, h;
			nti_GetRecDims(nti, rec, &w, &h);
			assert(h >= offY + lenY);
			assert(w >= x0 + lenX);

#endif
			CSH_HDRP hp = _csf_GetRecord(&nti->Cf, rec++, 0);
			if (!hp) return nti_errno = nti->Cf.Errno;
			assert(hp->Reserved[bandOff] != NULL);
			LPBYTE recBuf = hp->Reserved[bandOff];
			UINT recBufOff = offY * NTI_BLKWIDTH + x0;

			LPBYTE dstBufLim = pDst + lenY * rowBytes; //After debugging, move mult outside loop (see above)

			//read lenY rows of this block starting at offY --
			for (LPBYTE dstBuf = pDst; dstBuf < dstBufLim; dstBuf += rowBytes, recBufOff += NTI_BLKWIDTH) {
				if (iFilter != NTI_FILTERNONE) {
					copy_filtered_row(dstBuf, hp, iFilter, bandOff, recBufOff, lenX);
				}
				else	memcpy(dstBuf, recBuf + recBufOff, lenX);
			}
			width -= lenX;
			pDst += lenX;
			x0 = 0;
		}
		pBuffer += (lenY*rowBytes); //NTI_BLKWIDTH*rowBytes;
		sizY -= lenY; //(NTI_BLKWIDTH-offY);
		offY = 0;
		recOff += lvlCols;
	}

	return 0;
}

apfcn_v nti_GetRecDims(NTI_NO nti, UINT rec, UINT *width, UINT *height)
{
	UINT lvl = 0;
	while (lvl < nti->Lvl.nLevels && rec >= nti->Lvl.rec[lvl]) ++lvl;
	assert(rec < nti->Lvl.rec[lvl]);
	rec -= nti->Lvl.rec[--lvl];

	*width = *height = NTI_BLKWIDTH;

	UINT c = nti->Lvl.cols[lvl];
	if (!((rec + 1) % c)) *width = nti->Lvl.width[lvl] - (c - 1)*NTI_BLKWIDTH;

	UINT r = nti->Lvl.rows[lvl] - 1;
	if (rec >= r * c) *height = nti->Lvl.height[lvl] - r * NTI_BLKWIDTH;
}

__int64 nti_FileSize(NTI_NO nti)
{
	return nti_GetRecOffset(nti, nti->Lvl.nRecTotal).QuadPart;
}

__int64 nti_SizeEncoded(NTI_NO nti)
{
	return nti_GetRecOffset(nti, nti->Lvl.nRecTotal).QuadPart - nti->Cf.Offset;
}

__int64 nti_SizeDecodedLvl(NTI_NO nti, UINT lvl)
{
	UINT nBands = lvl ? nti->Lvl.nOvrBands : nti->H.nBands;
	return (__int64)nti->Lvl.width[lvl] * nti->Lvl.height[lvl] * nBands;
}

__int64 nti_SizeDecoded(NTI_NO nti)
{
	__int64 size = 0;
	for (UINT lvl = 0; lvl < nti->Lvl.nLevels; lvl++) {
		size += nti_SizeDecodedLvl(nti, lvl);
	}
	return size;
}

__int64 nti_SizeEncodedLvl(NTI_NO nti, UINT lvl)
{
	return nti_GetRecOffset(nti, nti->Lvl.rec[lvl + 1]).QuadPart
		- nti_GetRecOffset(nti, nti->Lvl.rec[lvl]).QuadPart;
}

apfcn_u nti_NumLvlRecs(NTI_NO nti, UINT lvl)
{
	return nti->Lvl.rec[lvl + 1] - nti->Lvl.rec[lvl];
}

LARGE_INTEGER nti_GetRecOffset(NTI_NO nti, DWORD recno)
{
	LARGE_INTEGER offset;

	offset.HighPart = 0;

	if (!recno) offset.LowPart = nti->Cf.Offset;
	else {
		offset.LowPart = nti->pRecIndex[recno - 1];
		if (*nti->H.dwOffsetRec) {
			DWORD *pRecOff = nti->H.dwOffsetRec;
			while (*pRecOff && recno >= *pRecOff++) offset.HighPart++;
		}
	}
	return offset;
}

apfcn_i nti_TruncateOverviews(NTI_NO nti)
{
	HANDLE h = ((DOS_FP)nti->Cf.Handle)->DosHandle;
	if (SetFilePointerEx(h, nti_GetRecOffset(nti, nti->Lvl.rec[1]), NULL, FILE_BEGIN) &&
		SetEndOfFile(h)) return nti_errno = 0;
	return nti_errno = NTI_ErrTruncate;
}

apfcn_i _nti_AddOffsetRec(NTI_NO nti, DWORD recno, DWORD lenEncoded)
{
	//Set nti->pRecIndex[recno], which is the offset of recno+1 --
	if (!recno) {
		nti->pRecIndex[0] = nti->Cf.Offset + lenEncoded;
		return 0;
	}
	DWORD w = nti->pRecIndex[recno - 1]; //w is offset of recno, the record just written
	if ((nti->pRecIndex[recno] = w + lenEncoded) > w)
		//recno+1 starts in the same 4 GB segment as recno --
		return 0;

	//We're passing a 4GB boundary --
	for (DWORD *pRecOff = nti->H.dwOffsetRec;
		pRecOff < nti->H.dwOffsetRec + NTI_MAX_OFFSETRECS; pRecOff++)
		if (!*pRecOff) {
			//File offsets of subsequent records will will be computed
			//as pRecIndex[recno-1]+(w+1)*4 GB -- 
			*pRecOff = recno + 1;
			return 0;
		}
	return nti_errno = NTI_ErrSizeImage;
}

apfcn_i nti_OpenRecord(NTI_NO nti, DWORD recno)
{
	//Initialize transfer buffer --
	if (!_nti_buffer && _nti_AllocBuffer()) return nti_errno;
	_nti_buffer_pos = _nti_buffer_end;
	_nti_handle = ((DOS_FP)nti->Cf.Handle)->DosHandle;

	_nti_bytes_toread = nti->pRecIndex[recno] -
		(recno ? nti->pRecIndex[recno - 1] : nti->Cf.Offset);
	if (!SetFilePointerEx(_nti_handle, nti_GetRecOffset(nti, recno), NULL, FILE_BEGIN))
		return nti_errno = NTI_ErrRead;
	return nti_errno = 0;
}

apfcn_i nti_AllocCacheBuffers(CSH_HDRP hp, int nComp)
{
	LPBYTE *pDstBufs = hp->Reserved;
	*pDstBufs++ = hp->Buffer;
	if (nComp == 3) {
		if (!*pDstBufs && !(*pDstBufs = (LPBYTE)malloc(NTI_BLKSIZE)) ||
			!*++pDstBufs && !(*pDstBufs = (LPBYTE)malloc(NTI_BLKSIZE)))
			return nti_errno = NTI_ErrMem;
	}
	else {
		free(*pDstBufs); *pDstBufs++ = NULL;
		free(*pDstBufs); *pDstBufs = NULL;
	}
	return 0;
}


