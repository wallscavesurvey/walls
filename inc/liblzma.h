/*  LzmaEnc.h -- LZMA Encoder
2011-01-27 : Igor Pavlov : Public domain */

#ifndef __LIBLZMA_H
#define __LIBLZMA_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LZMA_PROPS_SIZE 5

	typedef struct _CLzmaEncProps
	{
		int level;       /*  0 <= level <= 9 */
		UINT32 dictSize; /* (1 << 12) <= dictSize <= (1 << 27) for 32-bit version
							(1 << 12) <= dictSize <= (1 << 30) for 64-bit version
							 default = (1 << 24) */
		UINT32 reduceSize; /* estimated size of data that will be compressed. default = 0xFFFFFFFF.
							  Encoder uses this value to reduce dictionary size */
		int lc;          /* 0 <= lc <= 8, default = 3 */
		int lp;          /* 0 <= lp <= 4, default = 0 */
		int pb;          /* 0 <= pb <= 4, default = 2 */
		int algo;        /* 0 - fast, 1 - normal, default = 1 */
		int fb;          /* 5 <= fb <= 273, default = 32 */
		int btMode;      /* 0 - hashChain Mode, 1 - binTree mode - normal, default = 1 */
		int numHashBytes; /* 2, 3 or 4, default = 4 */
		UINT32 mc;        /* 1 <= mc <= (1 << 30), default = 32 */
		unsigned writeEndMark;  /* 0 - do not write EOPM, 1 - write EOPM, default = 0 */
		int numThreads;  /* 1 or 2, default = 2 */
	} CLzmaEncProps;

	//Constants for 64K block compression requirements --

	//281120 + 65536 + 2*12288 --
#define LZMA_BUFLEN_ENC1 371232
//fcn of dict size: 64K, level=5, needs 1052680, 128K needs 1576968 --
//level<5 needs 790532 instead of 1052680
#define LZMA_BUFLEN_ENC2 1052680  
#define LZMA_BUFLEN_DEC 15980

	extern CLzmaEncProps Lzma_props;
	extern const BYTE Lzma_outProps[5];

	extern LPBYTE lzma_bufdec, lzma_bufenc1, lzma_bufenc2;
	extern UINT lzma_bufdec_avail, lzma_bufenc1_avail, lzma_bufenc2_avail;

	BOOL LzmaBufInitEnc();
	void LzmaBufFreeEnc();

	BOOL LzmaBufInitDec();
	void LzmaBufFreeDec();

	//Called from liblzma code --
	void *LzmaAllocEnc(size_t size);
	void *LzmaAllocDec(size_t size);

#define SZ_OK 0
#define SZ_ERROR_DATA 1
#define SZ_ERROR_MEM 2
#define SZ_ERROR_CRC 3
#define SZ_ERROR_UNSUPPORTED 4
#define SZ_ERROR_PARAM 5
#define SZ_ERROR_INPUT_EOF 6
#define SZ_ERROR_OUTPUT_EOF 7
#define SZ_ERROR_READ 8
#define SZ_ERROR_WRITE 9
#define SZ_ERROR_PROGRESS 10
#define SZ_ERROR_FAIL 11
#define SZ_ERROR_THREAD 12

	/* ---------- One Call Interface ---------- */

	/* LzmaEncode
	Return code:
	  SZ_OK               - OK
	  SZ_ERROR_MEM        - Memory allocation error
	  SZ_ERROR_PARAM      - Incorrect paramater
	  SZ_ERROR_OUTPUT_EOF - output buffer overflow
	  SZ_ERROR_THREAD     - errors in multithreading functions (only for Mt version)
	*/

	int LzmaEncode(LPBYTE dest, size_t *destLen, const LPBYTE src, size_t srcLen,
		const CLzmaEncProps *props, LPBYTE propsEncoded);

	/* LzmaDecode
	finishMode:
	It has meaning only if the decoding reaches output limit (*destLen).
	LZMA_FINISH_ANY - Decode just destLen bytes.
	LZMA_FINISH_END - Stream must be finished after (*destLen).

	Returns:
	SZ_OK
	status:
	LZMA_STATUS_FINISHED_WITH_MARK
	LZMA_STATUS_NOT_FINISHED
	LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK
	SZ_ERROR_DATA - Data error
	SZ_ERROR_MEM  - Memory allocation error
	SZ_ERROR_UNSUPPORTED - Unsupported properties
	SZ_ERROR_INPUT_EOF - It needs more bytes in input buffer (src).
	*/

	int LzmaDecode(LPBYTE dest, size_t *destLen, const BYTE *src, size_t *srcLen, const BYTE *propData);
	//dmck , unsigned propSize, ELzmaFinishMode finishMode, ELzmaStatus *status, ISzAlloc *alloc);

#ifdef __cplusplus
}
#endif

#endif
