/* TRX_TYPE.H -- Types used by TRX_*.H header files --*/
/* DMcK 3/15/93 */

#ifndef __TRX_TYPE_H
#define __TRX_TYPE_H

#ifndef _WINDOWS_        
#include <windows.h>
#endif

#define _OLD_TYPES

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

#ifndef DLLExportC
#define DLLExportC extern "C" __declspec(dllexport)
#endif

#ifndef DLLExport
#define DLLExport __declspec(dllexport)
#endif

typedef int (TRXAPI * TRXAPI_CB)(int);
typedef struct {
                BYTE Year;
                BYTE Month;
                BYTE Day;
               } DBF_HDRDATE;

/*A_TYPES.H -- Types used by low-level A_*.h header files --*/

#ifndef __A_TYPES_H
#define __A_TYPES_H

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
typedef unsigned char byte;
typedef WORD word;
typedef DWORD ulong;
typedef byte flag;
typedef byte FAR *ptr;
typedef byte NEAR *nptr;
typedef void FAR *vptr;
typedef void NEAR *vnptr;
typedef int NEAR *inptr;
typedef word NEAR *wnptr;
typedef long NEAR *lnptr;
typedef ulong NEAR *ulnptr;
typedef void (NEAR *fnptr)(void);
typedef float FAR *flfptr;
typedef float NEAR *flnptr;
typedef double NEAR *dlnptr;
typedef double FAR *dlfptr;
#endif

#endif
/*End of A_TYPES.H */

#endif
/*End of TRX_TYPES.H */
