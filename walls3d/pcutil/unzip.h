// unzip.h : decompresses gzip and pkunzip files
// written in January 1995
// Gerbert Orasche
// thesis at the Institute For Information Processing And
// Computer Supported New Media (IICM)
// Graz University Of Technology
//

#ifndef PS_VIEWER_UNZIP_H
#define PS_VIEWER_UNZIP_H

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include <iostream>
using namespace std;


/* PKZIP header definitions */
#define LOCSIG 0x04034b50L      /* four-byte lead-in (lsb first) */
#define LOCFLG 6                /* offset of bit flag */
#define  CRPFLG 1               /*  bit for encrypted entry */
#define  EXTFLG 8               /*  bit for extended local header */
#define LOCHOW 8                /* offset of compression method */
#define LOCTIM 10               /* file mod time (for decryption) */
#define LOCCRC 14               /* offset of crc */
#define LOCSIZ 18               /* offset of compressed size */
#define LOCLEN 22               /* offset of uncompressed length */
#define LOCFIL 26               /* offset of file name field length */
#define LOCEXT 28               /* offset of extra field length */
#define LOCHDR 30               /* size of local header, including sig */
#define EXTHDR 16               /* size of extended local header, inc sig */

/* Compression methods (see algorithm.doc) */
#define STORED      0
#define COMPRESSED  1
#define PACKED      2
#define LZHED       3
/* methods 4 to 7 reserved */
#define DEFLATED    8
#define MAX_METHODS 9
#define NO_METHOD   0xff

#define	PACK_MAGIC     "\037\036" /* Magic header for packed files */
#define	GZIP_MAGIC     "\037\213" /* Magic header for gzip files, 1F 8B */
#define	OLD_GZIP_MAGIC "\037\236" /* Magic header for gzip 0.5 = freeze 1.x */
#define	LZH_MAGIC      "\037\240" /* Magic header for SCO LZH Compress files*/
#define PKZIP_MAGIC    "\120\113\003\004" /* Magic header for pkzip files */
#define	LZW_MAGIC      "\037\235"   /* Magic header for lzw files, 1F 9D */

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define CONTINUATION 0x02 /* bit 1 set: continuation of multi-part gzip file */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define ENCRYPTED    0x20 /* bit 5 set: file is encrypted */
#define RESERVED     0xC0 /* bit 6,7:   reserved */

#define UNZIP_OK    0
#define UNZIP_ERROR 1

#define INBUFSIZ  0x8000  /* input buffer size */
#define INBUF_EXTRA  64     /* required by unlzw() */

#define OUTBUFSIZ  16384  /* output buffer size */
#define OUTBUF_EXTRA 2048   /* required by unlzw() */

#define DIST_BUFSIZE 0x8000 /* buffer for distances, see trees.c */
#define WSIZE 0x8000     /* window size--must be a power of two, and */

#define OUTNAME_SIZE 512

#define BITS 16


#define DUMPBITS(n) {b>>=(n);k-=(n);}

#define flush_output(w) (m_OutCount=(w),flush_window())

/* If BMAX needs to be larger than 16, then h and x[] should be ulg. */
#define BMAX 16         /* maximum bit length of any code (16 for explode) */
#define N_MAX 288       /* maximum number of codes in any set */

/* Huffman code lookup table entry--this entry is four bytes for machines
   that have 16-bit pointers (e.g. PC's in the small or medium model).
   Valid extra bits are 0..13.  e == 15 is EOB (end of block), e == 16
   means that v is a literal, 16 < e < 32 means that v is a pointer to
   the next table, which codes e - 16 bits, and lastly e == 99 indicates
   an unused code.  If a code with e == 99 is looked up, this implies an
   error in the data. */
struct huft {
	unsigned char e;                /* number of extra bits or operation */
	unsigned char b;                /* number of bits in this code or subcode */
	union {
		unsigned short n;              /* literal, length base, or distance base */
		struct huft *t;     /* pointer to next level of table */
	} v;
};


/////////////////////////////////////////////////////////////////////////////
// CUnzip

class CUnzip
{
private:
	unsigned long orig_crc;
	unsigned long orig_len;
	char m_pcOutName[OUTNAME_SIZE];
	// file streams
	ifstream InSt;
	ofstream OutSt;
	int ext_header;
	// pointer to buffer
	char* m_pcInBuf;
	unsigned int m_iInPtr;
	unsigned int m_iInSize;
	unsigned char* m_pcOutBuf;
	unsigned int m_OutCount;
	unsigned long bytes_out;
	/*unsigned char* slide;*/
	 // stores used method
	unsigned char m_iMethod;

	// for inflate:
	// bit buffer 
	unsigned long bb;
	// bits in bit buffer      
	unsigned int bk;
	// bits in base literal/length lookup table 
	int lbits;
	// bits in base distance lookup table 
	int dbits;
	// track memory usage 
	unsigned hufts;

	// Attributes
public:

	// Operations
private:
	unsigned char CUnzip::GetByte(ifstream& InSt);
	int CUnzip::Inflate(ifstream& InSt);
	int CUnzip::inflate_block(int* e);
	int CUnzip::inflate_stored();
	int CUnzip::inflate_fixed();
	int CUnzip::inflate_dynamic();
	void CUnzip::flush_window();
	unsigned long CUnzip::updcrc(unsigned char* s, unsigned int n);
	int CUnzip::huft_build(unsigned int* b, unsigned int n, unsigned int s,
		unsigned short* d, unsigned short* e, struct huft** t,
		int* m);
	int CUnzip::huft_free(struct huft* t);
	int CUnzip::inflate_codes(struct huft* tl, struct huft* td, int bl, int bd);
	int CUnzip::NeedBits(unsigned int BitNum, unsigned long* pBitBuffer,
		unsigned int* pBitNum);

	// Implementation
public:
	CUnzip();
	virtual ~CUnzip();
	int CUnzip::Unzip(char* pOutFile, char* pszTempPath, char* pszExt);
	int CUnzip::CheckMagic(const char* pInFile);

	/*#ifdef _DEBUG
	  virtual void AssertValid() const;
	  virtual void Dump(CDumpContext& dc) const;
	#endif*/

};
#endif
