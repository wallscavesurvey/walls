#ifndef __A__NTI_H
#define __A__NTI_H

#ifndef __A__CSH_H
#include <a__csh.h>
#endif

/*NTI file header --*/
#define NTI_VERSION 0x7006

//not used but increases lib file size and obj sizes by 20 bytes each when used as processor define in *.vcxproj --
//#define _USE_PARTIAL_BLKS

//Minimum size of image data in a block that will be stored compressed  --
#define NTI_MIN_ENCODE_SIZE (16*16)

#define NTI_MAXLEVELS 16
//For now, allow file sizes of (6+2)*4 GB == 32 GB
#define NTI_MAX_OFFSETRECS 6 
//Allow maximum of 7 compression functions, preserving 5 flag bits
#define NTI_FCNBITS 3
#define NTI_FCNMASK ((1<<NTI_FCNBITS)-1)

//Supported I/O functions --
#ifdef _USE_LZMA
enum {NTI_FCNNONE,NTI_FCNZLIB,NTI_FCNLOCO,NTI_FCNJP2K,NTI_FCNWEBP,NTI_FCNLZMA}; //etc.. lower 4 bits of fType parameter of nti_Create()
#else
enum {NTI_FCNNONE,NTI_FCNZLIB,NTI_FCNLOCO,NTI_FCNJP2K,NTI_FCNWEBP}; //etc.. lower 4 bits of fType parameter of nti_Create()
#endif

//One flag bit for encode/decode --
#define NTI_FCNENCODE (1<<NTI_FCNBITS)

//Allow 3 possible RGB filters (2 flag bits) to test compression improvement --
enum {NTI_FILTERNONE=0,NTI_FILTERRED=(1<<14),NTI_FILTERGREEN=(2<<14),NTI_FILTERBLUE=(3<<14)};
#define NTI_FILTERMASK NTI_FILTERBLUE
#define NTI_FILTERSHFT 14

#define NTI_BLKSHIFT 8
#define NTI_BLKSIZE (256*256)
#define NTI_BLKWIDTH 256
#define NTI_DFLTCACHE_BLKS 64

#pragma pack(1)

#define NTI_SIZDATUMNAME 15
#define NTI_SIZUNITSNAME 8

/*All NTI files begin with this structure --*/
struct  NTI_HDR {	/* 4 + 4*4 + 4 + 5*8 + 1 + 15 + 8 = 88 bytes */

	WORD Version;		/*== NTI_VERSION*/
	WORD fType;			/*Compression type, etc*/

	DWORD nWidth;		/*image width in pixels*/	
	DWORD nHeight;		/*image height in pixels*/
	WORD nBands;		/*1 if either grayscale or nColors>0, elese 3*/
	WORD nColors;		/*length of RGBQUAD array (palette) following this structure*/
	WORD nSzExtra;		/*Size of miscellaneus data (stored after palette)*/
	WORD nBlkShf;		/*tile size is (1<<nBlkShf) pixels squared*/
	/*Allow file sizes of (NTI_MAX_OFFSETRECS+2)*4 GB = 32 GB*/
    DWORD dwOffsetRec[NTI_MAX_OFFSETRECS+1];
	long  SrcTime;		/*timestamp of original source file*/
	double UnitsPerPixelX;
	double UnitsPerPixelY;
	double UnitsOffX;
	double UnitsOffY;
	double MetersPerUnit;
	char cZone;			/*south if negative*/
	char csDatumName[NTI_SIZDATUMNAME];
	char csUnitsName[NTI_SIZUNITSNAME];
};

#define SZ_NTI_HDR sizeof(NTI_HDR)

/*Structure in NTI_FILE used to access file but not stored in file*/
struct NTI_LVL { /* 4*4+5*(16*4) = 336 bytes */
	DWORD nRecTotal;
	DWORD nLevels;
	DWORD nOvrBands;
	DWORD rec[NTI_MAXLEVELS+1];  /*first record number (0 based) of level*/
	DWORD width[NTI_MAXLEVELS];  /*lvl_width[0]=nWidth*/
	DWORD height[NTI_MAXLEVELS];
	DWORD cols[NTI_MAXLEVELS];
	DWORD rows[NTI_MAXLEVELS];
};

struct NTI_FILE_PFX {		/*For sizing purposes only -- must match NTI_FILE below*/
  CSH_FILE Cf;
  char  *pszExtra;
  DWORD *pRecIndex;
  NTI_LVL Lvl;
};

struct NTI_FILE {			/*208 + 4 * nColors*/
  CSH_FILE Cf;              /*40-byte struct defined in A__CSH.H*/
  char  *pszExtra;			/*Normally points to WKT georeference string returned by GDAL*/
  DWORD *pRecIndex;			/*Points to record index*/
  NTI_LVL Lvl;				/*336-byte struct initialized by nti_Open() and nti_Create()*/
  NTI_HDR H;                /*52-byte physical file header*/
  RGBQUAD Pal[1];			/*Creator will store extra data after palette and before record index*/
};

typedef NTI_FILE *NTI_NO;

#pragma pack()

extern char *nti_dot_extension;
extern UINT _nti_xfr_buffer_sz;
extern LPBYTE _nti_aux_buffer,_nti_xfr_buffer;

apfcn_i	_nti_AddOffsetRec(NTI_NO nti,DWORD recno,DWORD lenEncoded);
apfcn_v _nti_Free_xfr_buffer();
apfcn_s _nti_getPathname(LPCSTR pathName);
apfcn_v _nti_InitLvl(NTI_LVL *pLvl,UINT nWidth,UINT nHeight,UINT nColors,UINT nBands);
#endif
