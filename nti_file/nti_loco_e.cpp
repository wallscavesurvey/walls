//libloco related fcns and data --

#include <nti_file.h>
#include <assert.h>
#include <libloco.h>

int nti_loco_e(CSH_HDRP hp,UINT typ)
{
	if(!typ) return nti_loco_d(hp,typ);

	if(!hp) {
		//===Modify for different methods ===
		//Since loco encode uses the same buffers as lco decode,
		//we'll free them only there by calling lco_UnInit().
		//============
		return 0;
	}

	NTI_NO nti=(NTI_NO)hp->File;
	DWORD recno=hp->Recno;
	UINT nComp=nti->H.nBands;

	if(nti->H.nColors) {
		if(recno<nti->Lvl.rec[1]) {
			//Use lossless for palette index compression --
			return nti_zlib_e(hp, typ);
		}
		nComp=nti->Lvl.nOvrBands;
	}

	//Check buffer pointer array --
	assert(hp->Buffer==*hp->Reserved);
	assert(nComp==1 && !hp->Reserved[1] && !hp->Reserved[2] ||
		nComp==3 && hp->Reserved[1] && hp->Reserved[2]);

	UINT width,height;
	nti_GetRecDims(nti,recno,&width,&height);
	UINT size=width*height;

    if(nti_CreateRecord(nti,recno)) return nti_errno;
	//_nti_buffer_pos has been set to _nti_buffer with _nti_bytes_written=0

	unsigned __int64 tm0;
	if(nti_def_timing)
		QueryPerformanceCounter((LARGE_INTEGER *)&tm0);

	if(width<NTI_BLKWIDTH) {
		//Compact partial blocks --
		for(UINT c=0;c<nComp;c++) {
			LPBYTE recbuf=hp->Reserved[c];
			for(UINT r=1;r<height;r++) {
				memcpy(recbuf+r*width,recbuf+r*NTI_BLKWIDTH,width);
			}
		}
	}

	//Maximum position we'll allow for the end of encoded data --
	_nti_buffer_lim=_nti_buffer_pos+(width*height);

	typ=0;
	for(UINT c=0;c<nComp;c++) {
		assert(_nti_buffer_pos==_nti_buffer);
		_nti_buffer_pos+=sizeof(WORD);

		int e=0;

		if(size>=NTI_MIN_ENCODE_SIZE) {

			//===Modify for different methods that compress components separately --

			if(nti_errno_loco=lco_Encode(hp->Reserved[c],height,width)) {
				nti_xfr_errmsg=nti_loco_errmsg;
				return nti_errno=NTI_ErrCompress;
			}
			if(!nti_errno) e=(int)(_nti_buffer_pos-_nti_buffer)-sizeof(WORD);
			else nti_errno=0;

			//======================================================================
		}

		//At this point e==0 if either the block was too small to compress or
		//if more than (width*height)-2 bytes were needed for the encoded data.
		//This is uncommon but certainly possible when width and/or height is
		//less than NTI_BLKWIDTH.

		if(!e) {
			//Let's discard what was generated and store the data uncompressed.
			e=size;
			*(WORD *)_nti_buffer=0; //indicate no compression
			_nti_buffer_pos=_nti_buffer+sizeof(WORD);
			if(e>NTI_BLKSIZE-sizeof(WORD)) {
				if(_nti_FlushBuffer()) return nti_errno;
				assert(_nti_bytes_written==sizeof(WORD) && _nti_buffer_pos==_nti_buffer);
			}
			memcpy(_nti_buffer_pos,hp->Reserved[c],e);
			_nti_buffer_pos+=e;
		}
		else *(WORD *)_nti_buffer=e; //length of compressed data
		if(_nti_FlushBuffer()) return nti_errno;
		//_nti_buffer_pos has been reset to _nti_buffer
		assert(_nti_bytes_written==e+sizeof(WORD));
		_nti_bytes_written=0;
		typ+=(e+sizeof(WORD));
	}

	if(width<NTI_BLKWIDTH) {
		//Restore partial blocks --
		for(UINT c=0;c<nComp;c++) {
			LPBYTE recbuf=hp->Reserved[c];
			for(int r=height-1;r>0;r--) {
				memmove(recbuf+r*NTI_BLKWIDTH,recbuf+r*width,width);
			}
		}
	}

	if(nti_def_timing) {
		unsigned __int64 tm1;
		QueryPerformanceCounter((LARGE_INTEGER *)&tm1);
		nti_tm_def+=(tm1-tm0);
		nti_tm_def_cnt++;
	}

	return _nti_AddOffsetRec(nti,recno,typ);
}
