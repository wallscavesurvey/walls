/* NTIERRS.C -- NTI file error strings*/

#include <nti_file.h>
#include <stdio.h>

/* Error codes defined in nti_file.h - NTI_ERR_BASE=100 --
enum {
		NTI_ErrVersion=NTI_ERR_BASE,
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
*/

LPSTR _nti_errstr[] = {
 "Unsupported NTI file version",
 "File missing or unavailable",
 "Image size too large for NTI format",
 "NTI's compression type not supported",

 "Compressing tile",
 "Compression init",
 "Decompressing tile",
 "Decompression init", //Max size
 "Incompatible image data - compression failed",

 "Wrong cache record size",
 "Cannot resize cache with multiple files attached",
 "File has no overviews",
 "Failure truncating file"
};

#define NTI_ERR_LIMIT (NTI_ERR_BASE+sizeof(_nti_errstr)/sizeof(LPSTR))

apfcn_s nti_Errstr(UINT e)
{
	nti_errno = 0;
	static char msg[64];  //32+28 currently used
	if (e < NTI_ERR_BASE || e >= NTI_ERR_LIMIT) return csh_Errstr(e);
	if (e >= NTI_ErrCompress && e <= NTI_ErrDecompressInit) {
		_snprintf(msg, sizeof(msg) - 1, "%s - %s", _nti_errstr[e - NTI_ERR_BASE], nti_xfr_errmsg());
		return msg;
	}
	return _nti_errstr[e - NTI_ERR_BASE];
}
