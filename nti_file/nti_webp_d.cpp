//webp decoder for nti_file.lib
//#include "stdafx.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <webp/decode.h>
#include <nti_file.h>

extern LPBYTE webp_buffer;

void webp_Decode_UnInit()
{
	free(webp_buffer); webp_buffer=NULL;
}

LPSTR nti_webp_errmsg()
{
	static char msg[80]; //Careful!
	_snprintf(msg,79,"webp: %s",nti_Errstr(nti_errno));
	return msg;
}

int nti_webp_d(CSH_HDRP hp,UINT typ)
{
	nti_xfr_errmsg = nti_webp_errmsg;

	if (!hp) {
		//Decode uninitialize
		//===Modify for different methods ===
		webp_Decode_UnInit();
		//============
		return 0;
	}

	if(typ) {
		assert(false); //Bad call from trx_csh.lib
		return nti_errno=NTI_ErrArg;
	}

	NTI_NO nti = (NTI_NO)hp->File;
	UINT nComp=nti->H.nBands;

	if(nti->H.nColors) {
		if(hp->Recno<nti->Lvl.rec[1]) {
			//Use zlib for palette index decompression --
			return nti_zlib_d(hp,0);
		}
		nComp=nti->Lvl.nOvrBands;
	}

	assert(nComp == 1 || nComp == 3);

	if(nti_AllocCacheBuffers(hp,nComp)) //bug1!!!
		return nti_errno;

	if (nti_OpenRecord(nti, hp->Recno)) return nti_errno;
	//_nti_bytes_toread is now set to record length with _nti_buffer_pos=_nti_buffer_end

	unsigned __int64 tm0;
	if(nti_inf_timing)
		QueryPerformanceCounter((LARGE_INTEGER *)&tm0);

	//===Modify for different methods ===

	//webp requires that the bytes to decode all be available in a buffer --

	if(!webp_buffer) {
		if (!(webp_buffer = (LPBYTE)malloc(6 * NTI_BLKSIZE))) //192K + 192K = 384K
			return nti_errno = NTI_ErrMem;
	}

	DWORD cnt;

	if(!::ReadFile(_nti_handle, webp_buffer, 4, &cnt, NULL) || 4!=cnt)
		return nti_errno=NTI_ErrRead;

	DWORD bSkip=0,lenEncoded=*(DWORD *)webp_buffer;

	//test if old WebP NTI format --
	if((WORD)(lenEncoded>>16)==0x4952) { //=="RI"
		*(WORD *)webp_buffer=0x4952;
		lenEncoded&=0xFFFF;
		bSkip=2;
	}

	if(!::ReadFile(_nti_handle, webp_buffer+bSkip, lenEncoded-bSkip, &cnt, NULL) || lenEncoded-bSkip!=cnt) {
		return nti_errno=NTI_ErrRead;
	}

	assert(*(DWORD *)webp_buffer==0x46464952); //"RIFF"

	UINT width, height;
	nti_GetRecDims(nti, hp->Recno, &width, &height);

#ifdef _DEBUG
	{
		int width0, height0;
		if(!WebPGetInfo(webp_buffer, lenEncoded, &width0, &height0) || width!=width0 || height!=height0) {
			assert(0);
			return nti_errno=NTI_ErrFormat;
		}
	}
#endif

	//LPBYTE pDecoded=WebPDecodeBGR(webp_buffer,lenEncoded, &width0, &height0);

	//Decoded bands are in interleaved BGR format (both grayscale and full-color) --
	LPBYTE pDecoded=WebPDecodeBGRInto(webp_buffer,lenEncoded,webp_buffer+3*NTI_BLKSIZE,3*NTI_BLKSIZE,width*3);

	if(!pDecoded) {
		return nti_errno=NTI_ErrDecompress;
	}

	//fill cache buffers --

	UINT gapBGR=NTI_BLKWIDTH-width;
	LPBYTE pSrcLim=pDecoded+height*width*3;
	LPBYTE pB = hp->Reserved[0];
	if (nComp == 1) {
		for (LPBYTE pSrc = pDecoded; pSrc < pSrcLim; pB += gapBGR) {
			for (UINT r = 0; r < width; r++) {
				*pB++ = *pSrc++; 
				pSrc += 2;
			}
		}
	}
	else {
		LPBYTE pG = hp->Reserved[1], pR = hp->Reserved[2];
		for (LPBYTE pSrc = pDecoded; pSrc < pSrcLim; pB += gapBGR, pG += gapBGR, pR += gapBGR /*,pSrc+=gapSrc*/) {
			for (UINT r = 0; r < width; r++) {
				*pB++ = *pSrc++;
				*pG++ = *pSrc++;
				*pR++ = *pSrc++;
			}
		}
	}

	//===================================

	if(nti_inf_timing) {
		unsigned __int64 tm1;
		QueryPerformanceCounter((LARGE_INTEGER *)&tm1);
		nti_tm_inf+=(tm1-tm0);
		nti_tm_inf_cnt++;
	}
	return 0;
}

