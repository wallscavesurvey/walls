//lzma encoding fcn -- main entry for trx_csh.lib

#include <nti_file.h>
#include <assert.h>
#include <stdio.h>
#include <liblzma.h>

int nti_lzma_e(CSH_HDRP hp, UINT typ)
{
	static bool bDefInit = false;

	if (!typ) return nti_lzma_d(hp, typ);

	if (!hp) {
		if (bDefInit) {
			//Encode uninitialize -- will free encode buffers
			//===Modify for different methods ===
			LzmaBufFreeEnc();
			//============
			bDefInit = false;
		}
		return 0;
	}

	NTI_NO nti = (NTI_NO)hp->File;
	DWORD recno = hp->Recno;

	if (!bDefInit) {
		//Encode initialize (allocate buffers)--
		//===Modify for different methods ===
		LzmaBufInitEnc(); //also initializes Lzma_props (see LzmaAlloc.c to modify Lzma_props.fb or Lzma_props.level)
		//============
		bDefInit = true;
	}

	int nComp = (recno < nti->Lvl.rec[1]) ? nti->H.nBands : nti->Lvl.nOvrBands;

	//Check buffer pointer array --
	assert(hp->Buffer == *hp->Reserved);
	assert(nComp == 1 && !hp->Reserved[1] && !hp->Reserved[2] ||
		nComp == 3 && hp->Reserved[1] && hp->Reserved[2]);

	UINT width, height;
	nti_GetRecDims(nti, recno, &width, &height);
	UINT size = width * height;

	unsigned __int64 tm0;
	if (nti_def_timing)
		QueryPerformanceCounter((LARGE_INTEGER *)&tm0);

	if (width < NTI_BLKWIDTH) {
		//Compact partial blocks --
		for (int c = 0; c < nComp; c++) {
			LPBYTE recbuf = hp->Reserved[c];
			for (UINT r = 1; r < height; r++) {
				memcpy(recbuf + r * width, recbuf + r * NTI_BLKWIDTH, width);
			}
		}
	}

	if (nti_CreateRecord(nti, recno)) return nti_errno;
	//_nti_buffer_pos has been set to _nti_buffer with _nti_bytes_written=0
	// Maximum position we'll allow for the end of encoded data --
	//_nti_buffer_lim=_nti_buffer_pos+size;

	BYTE outProps[5];
	typ = 0; //count of bytes written to _nti_buffer (NTI_BLKZIZE)

	for (int c = 0; c < nComp; c++) {
		assert(_nti_buffer_pos == _nti_buffer);
		_nti_buffer_pos += sizeof(WORD);

		int e = 0;

		if (size >= NTI_MIN_ENCODE_SIZE) {
			//===Modify for different methods that compress components separately --
			/*
			strm_def.next_out = _nti_buffer_pos;
			strm_def.next_in = hp->Reserved[c];
			strm_def.avail_in = size;
			strm_def.avail_out = size-sizeof(WORD);
			*/
			LzmaBufInitEnc(); //reset pointers only since buffers already allocated in previous call

			size_t outDataLen = size;

			nti_errno_lzma = LzmaEncode(_nti_buffer_pos, &outDataLen, hp->Reserved[c], size, &Lzma_props, outProps); //dmck , 0, NULL, &g_Alloc, &g_Alloc);

			if (nti_errno_lzma && nti_errno_lzma != SZ_ERROR_OUTPUT_EOF) {
				/*No bad return value possible?*/
				nti_xfr_errmsg = nti_lzma_errmsg;
				return nti_errno = NTI_ErrCompress;
			}

			if (!nti_errno_lzma) e = outDataLen;
			else e = 0;

			if (e) {
				_nti_buffer_pos += e;
			}
			//======================================================================
		}

		//At this point e==0 if either the block was too small to compress or
		//if more than (width*height)-2 bytes were needed for the encoded data.
		//This is uncommon but certainly possible when width and/or height is
		//less than NTI_BLKWIDTH.

		if (!e) {
			//Let's discard what was generated and store the data uncompressed.
			e = size;
			*(WORD *)_nti_buffer = 0; //indicate no compression
			_nti_buffer_pos = _nti_buffer + sizeof(WORD);
			if (e > NTI_BLKSIZE - sizeof(WORD)) {
				if (_nti_FlushBuffer()) return nti_errno;
				assert(_nti_bytes_written == sizeof(WORD) && _nti_buffer_pos == _nti_buffer);
			}
			memcpy(_nti_buffer_pos, hp->Reserved[c], e);
			_nti_buffer_pos += e;
		}
		else *(WORD *)_nti_buffer = e; //length of compressed data
		if (_nti_FlushBuffer()) return nti_errno;
		//_nti_buffer_pos has been reset to _nti_buffer
		assert(_nti_bytes_written == e + sizeof(WORD));
		_nti_bytes_written = 0;
		typ += (e + sizeof(WORD));
	}

	if (width < NTI_BLKWIDTH) {
		//Restore partial blocks --
		for (int c = 0; c < nComp; c++) {
			LPBYTE recbuf = hp->Reserved[c];
			for (int r = height - 1; r > 0; r--) {
				memmove(recbuf + r * NTI_BLKWIDTH, recbuf + r * width, width);
			}
		}
	}

	if (nti_def_timing) {
		unsigned __int64 tm1;
		QueryPerformanceCounter((LARGE_INTEGER *)&tm1);
		nti_tm_def += (tm1 - tm0);
		nti_tm_def_cnt++;
	}

	return _nti_AddOffsetRec(nti, recno, typ);
}
