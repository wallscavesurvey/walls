//zlib related fcns and data --

#include <nti_file.h>
#include <assert.h>
#include <stdio.h>
#include <zlib.h>

int nti_errno_zlib=0;

LPSTR nti_zlib_errmsg()
{
	static char msg[32]; //Careful!
	char *s=NULL;
    switch (nti_errno_zlib) {
		case 1:
			s="Compression not enabled";
			break;
		case Z_STREAM_ERROR:
			s="Invalid compression level"; //Max size!!
			break;
		case Z_DATA_ERROR:
			s="Invalid deflate data";
			break;
		case Z_MEM_ERROR:
			s="Out of memory";
			break;
		case Z_VERSION_ERROR:
			s="Version mismatch!";
			break;
    }
	strcpy(msg,"zlib: ");
	if(s) strcpy(msg+6,s);
	else sprintf(msg+6,"Unknown error: %d",nti_errno_zlib);
	nti_errno_zlib=0;
	return msg;
}

int nti_zlib_d(CSH_HDRP hp,UINT typ)
{
	static bool bInfInit=false;
	static z_stream strm_inf;

	if(!hp) {
		//Inflate uninitialize
		//===Modify for different methods ===
		(void)inflateEnd(&strm_inf);
		//============
		bInfInit=false;
		return 0;
	}

	if(typ) {
		assert(false); //Bad call from trx_csh.lib
		return nti_errno=NTI_ErrArg;
	}

	if(!bInfInit) {
		//Inflate initialize --
		//===Modify for different methods ===
		/* allocate inflate state */
		memset(&strm_inf,0,sizeof(strm_inf));
		if((typ=inflateInit(&strm_inf))!=Z_OK) {
			nti_errno_zlib=typ;
			nti_xfr_errmsg=nti_zlib_errmsg;
			return nti_errno=NTI_ErrDecompressInit;
		}
		//=====================
		bInfInit=true;
	}

	NTI_NO nti=(NTI_NO)hp->File;
	int nComp=(nti->H.nColors && hp->Recno>=nti->Lvl.rec[1])?3:nti->H.nBands;

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

	for(int c=0;c<nComp;c++) {
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
			//_nti_buffer_pos is now positioned at the next header WORD
#ifdef _DEBUG
			lenEncTotal+=lenDecoded;
#endif
		}
		else {
			//===Modify for different methods that process components separately.
			//The encoded data is already loaded and begins at _nti_buffer_pos --
			strm_inf.avail_in = lenEncoded;
			strm_inf.next_in = _nti_buffer_pos;
			strm_inf.avail_out = lenDecoded;
			strm_inf.next_out = hp->Reserved[c];
			typ = inflate(&strm_inf,Z_FINISH);
			if(typ!=Z_STREAM_END || strm_inf.avail_in || strm_inf.avail_out) {
				inflateReset(&strm_inf);
				if(!typ) typ=Z_ERRNO;
				nti_errno_zlib=typ;
				nti_xfr_errmsg=nti_zlib_errmsg;
				return nti_errno=NTI_ErrDecompress;
			}
			inflateReset(&strm_inf);
			_nti_buffer_pos+=lenEncoded;
			_nti_bytes_toread-=lenEncoded;
			//=====================================================================
		}
		if(c+1<nComp && !_nti_FillBuffer())
			return nti_errno;
	}

#ifdef _DEBUG
	UINT n=hp->Recno;
	n=nti->pRecIndex[n]-(n?nti->pRecIndex[n-1]:nti->Cf.Offset);
	if(lenEncTotal!=n) {
		return nti_errno=NTI_ErrFormat;
	}
#endif

	if(width<NTI_BLKWIDTH) {
		//restore partial blocks --
		for(int c=0;c<nComp;c++) {
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
