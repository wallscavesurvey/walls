//libloco related fcns and data --

#include <nti_file.h>
#include <assert.h>
#include <libloco.h>

int nti_errno_loco=0;

LPSTR nti_loco_errmsg()
{
	static char msg[64]; //Careful!
	char *s=NULL;
    switch (nti_errno_loco) {
		case LCO_ErrNoCompression: s="Compression not enabled"; break;
		case LCO_ErrEncodeLen : s="Compressed size too large"; break;
		case LCO_ErrNoSOI : s="SOI not found"; break;
		case LCO_ErrInitParams : s="Bad init params"; break;
		case LCO_ErrNoEOI : s="EOI not found or bad length"; break;
		case LCO_ErrDecodeLen: s="Wrong decoded length"; break;
		case LCO_ErrEOF: s="Premature end of compressed data"; break;
		case LCO_ErrInit: s="Can't initialize (out of memory)"; break;
	}
	strcpy(msg,"loco: ");
	if(s) strcpy(msg+6,s);
	nti_errno_loco=0;
	return msg;
}

int nti_loco_d(CSH_HDRP hp,UINT typ)
{
	nti_xfr_errmsg=nti_loco_errmsg;

	if(!hp) {
		//Decode uninitialize
		//===Modify for different methods ===
		lco_UnInit();
		//============
		return 0;
	}

	if(typ) {
		assert(false); //Bad call from trx_csh.lib
		return nti_errno=NTI_ErrArg;
	}

	NTI_NO nti=(NTI_NO)hp->File;
	UINT nComp=nti->H.nBands;

	if(nti->H.nColors) {
		//Use zlib for palette index decompression --
		if(hp->Recno<nti->Lvl.rec[1]) {
			return nti_zlib_d(hp,0);
		}
		nComp=nti->Lvl.nOvrBands;
	}

	assert(nComp==1 || nComp==3);

	if(nti_AllocCacheBuffers(hp,nComp))
		return nti_errno;


	UINT width,height;

	nti_GetRecDims(nti,hp->Recno,&width,&height);

	UINT lenDecoded=width*height;


	//Decoding --
	if(nti_OpenRecord(nti,hp->Recno))
		return nti_errno;
	//_nti_bytes_toread is now set to record length with _nti_buffer_pos=_nti_buffer_end

#ifdef _DEBUG
	DWORD lenEncTotal=0;
#endif
	unsigned __int64 tm0;
	if(nti_inf_timing)
		QueryPerformanceCounter((LARGE_INTEGER *)&tm0);

	for(UINT c=0;c<nComp;c++) {
		WORD lenEncoded;
		if(_nti_read((LPBYTE)&lenEncoded,sizeof(WORD))!=sizeof(WORD)) {
			if(!nti_errno)
				nti_errno=NTI_ErrFormat;
			return nti_errno;
		}
		if((DWORD)lenEncoded>_nti_bytes_toread)
			return nti_errno=NTI_ErrFormat;

#ifdef _DEBUG
		lenEncTotal+=lenEncoded+2;
#endif

		if(!lenEncoded) {
			if(_nti_read(hp->Reserved[c],lenDecoded)!=lenDecoded) return nti_errno=NTI_ErrRead;
#ifdef _DEBUG
			lenEncTotal+=lenDecoded;
#endif
		}
		else {
			//===Modify for different methods that process components separately --
			if(nti_errno_loco=lco_Decode(hp->Reserved[c],lenDecoded,lenEncoded)) {
				return nti_errno=NTI_ErrDecompress;
			}
			//Note: lco_Decode() advances _nti_buffer_pos but does not
			//decrement _nti_bytes_toread!
			_nti_bytes_toread-=lenEncoded;
			//=====================================================================
		}
		if(c+1<nComp && !_nti_FillBuffer())
			return nti_errno;
	}

#ifdef _DEBUG
	assert(!_nti_bytes_toread);
	UINT n=hp->Recno;
	n=nti->pRecIndex[n]-(n?nti->pRecIndex[n-1]:nti->Cf.Offset);
	if(lenEncTotal!=n) {
		return nti_errno=NTI_ErrFormat;
	}
#endif

	if(width<NTI_BLKWIDTH) {
		//restore partial blocks --
		for(UINT c=0;c<nComp;c++) {
			LPBYTE recbuf=hp->Reserved[c];
			for(int r=height-1;r>0;r--) {
				memmove(recbuf+r*NTI_BLKWIDTH,recbuf+r*width,width);
			}
		}
	}

	if(nti_inf_timing) {
		unsigned __int64 tm1;
		QueryPerformanceCounter((LARGE_INTEGER *)&tm1);
		nti_tm_inf+=(tm1-tm0);
		nti_tm_inf_cnt++;
	}

	return 0;
}
