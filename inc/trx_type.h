/* TRX_TYPE.H -- Types used by TRX_*.H header files --*/
/* Linux and MSC Version 9/21/98 */

#ifndef __TRX_TYPE_H
#define __TRX_TYPE_H

#define _OLD_TYPES

#ifdef _MSC_VER

#ifndef _WINDOWS_        
#include <windows.h>
#endif

#ifndef DLLExportC
#define DLLExportC extern "C" __declspec(dllexport)
#endif

#ifndef DLLExport
#define DLLExport __declspec(dllexport)
#endif

#else

#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif

#define MAX_PATH 260

#ifndef NULL
#define NULL ((void *)0)
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE  1
#endif
#ifndef CONST
#define CONST const
#endif

#undef far
#undef near
#undef NEAR
#undef FAR
#undef pascal
#undef CDECL
#undef PASCAL

#define far
#define near
#define NEAR
#define FAR
#define pascal

#define PASCAL     pascal
#define CDECL
#define CALLBACK FAR PASCAL

typedef unsigned long       DWORD;
typedef long                LONG;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned char       UCHAR;
typedef unsigned short      WORD;
typedef float               FLOAT;
typedef FLOAT               *PFLOAT;
typedef BOOL near           *PBOOL;
typedef BOOL far            *LPBOOL;
typedef BYTE near           *PBYTE;
typedef BYTE far            *LPBYTE;
typedef int near            *PINT;
typedef int far             *LPINT;
typedef WORD near           *PWORD;
typedef WORD far            *LPWORD;
typedef long far            *LPLONG;
typedef DWORD near          *PDWORD;
typedef DWORD far           *LPDWORD;
typedef void far            *LPVOID;
typedef CONST void far      *LPCVOID;
typedef char far            *LPSTR;
typedef CONST char far      *LPCSTR;
typedef char near           *NPSTR;
typedef char                *PSTR;

typedef int                 INT;
typedef unsigned int        UINT;
typedef unsigned int        *PUINT;

#endif
/*=============================================================*/

/*To avoid "segment lost in conversion" warnings --*/
#define _NPTR(p) (p)
#define _NCHR(p) (p)

typedef void * PVOID;

#define HPVOID LPVOID
#define NPVOID PVOID
#define NPBYTE PBYTE

#define TRXAPI FAR PASCAL
#define DBFAPI FAR PASCAL

#define TRXFCN_S LPSTR PASCAL
#define TRXFCN_I int PASCAL
#define TRXFCN_DWP DWORD * PASCAL
#define TRXFCN_DW DWORD PASCAL
#define TRXFCN_V void PASCAL
typedef int (CALLBACK *TRXFCN_IF)(int);

typedef int (TRXAPI * TRXAPI_CB)(int);
#pragma pack(1)
typedef struct {
	BYTE Year;
	BYTE Month;
	BYTE Day;
} DBF_HDRDATE;
#pragma pack()

/*A_TYPES.H -- Types used by low-level A_*.h header files --*/

#ifndef NULLF
#define NULLF 0
#endif

#define apvar_u extern UINT PASCAL
#define apvar_w extern WORD PASCAL
#define apvar_i extern int PASCAL
#define apvar_c extern UCHAR PASCAL
#define apvar_vp extern LPVOID PASCAL

#define apfcn NEAR PASCAL
#define apfcn_c BYTE apfcn
#define apfcn_v void apfcn
#define apfcn_u UINT apfcn
#define apfcn_i int apfcn
#define apfcn_in PINT apfcn
#define apfcn_l LONG apfcn
#define apfcn_ul DWORD apfcn
#define apfcn_p LPBYTE apfcn
#define apfcn_n PBYTE apfcn
#define apfcn_vn PVOID apfcn
#define apfcn_vp LPVOID apfcn
#define apfcn_s LPSTR apfcn
#define apfcn_d double apfcn
#define apfcn_b BOOL apfcn
#define apfcn_nc char NEAR * apfcn 

#define acfcn NEAR CDECL
#define acfcn_p LPBYTE acfcn
#define acfcn_vp LPVOID acfcn
#define acfcn_vn PVOID acfcn
#define acfcn_n PBYTE acfcn
#define acfcn_v void acfcn
#define acfcn_u UINT acfcn
#define acfcn_i int acfcn
#define acfcn_l LONG acfcn

#ifdef _OLD_TYPES

//#define boolean BOOL

#ifdef _MSC_VER
typedef DWORD ulong; /*in sys/types.h under linux*/
#endif
typedef unsigned char byte;
typedef WORD word;
typedef byte flag;
typedef byte FAR *ptr;
typedef byte NEAR *nptr;
typedef void FAR *vptr;
typedef void NEAR *vnptr;
typedef short *inptr;
typedef word NEAR *wnptr;
typedef long NEAR *lnptr;
typedef DWORD NEAR *ulnptr;
typedef void (NEAR *fnptr)(void);
typedef void (NEAR *fnptr_vp)(void *);
typedef float FAR *flfptr;
typedef float NEAR *flnptr;
typedef double NEAR *dlnptr;
typedef double FAR *dlfptr;

/*Upon _dos_exit(0), call each fcn that starts with this macro, as fcn(0),
  in the reverse order that they were setup with _dos_exit(fcn) --*/
#define FIX_EX(f) {\
  static fnptr_vp ex=0;\
  if(ex) ex(f);\
  else ex=(fnptr_vp)f;\
  if(f) return;\
  ex=0;\
 }

#define ARC_MINYEAR 50
#endif

  /*For lack of a better place --*/
#ifdef _MSC_VER
_CRTIMP int    	_cdecl _kbhit(void); /*nonzero if char avail - in conio.h*/
_CRTIMP int    	_cdecl _getch(void); /*in conio.h*/
#else
int	_kbhit(void); /*num chars in buffer - in trx_csh/getch.c*/
int	_getch(void); /*in trx_csh/getch.c*/
#endif
#define kbhit _kbhit
#define	instat() ((_kbhit()>0)?getch():0)

#endif
/*End of TRX_TYPES.H */

