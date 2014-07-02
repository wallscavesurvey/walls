/* TRX_CSH.H -- Cache Manager Declarations for TRX_FILE.LIB*/

#ifndef __TRX_CSH_H
#define __TRX_CSH_H

#ifndef __TRX_TYPE_H
  #include <trx_type.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CSH_MAX_BUFSIZ ((UINT)192*1024)
#define CSH_DEF_BUFSIZ 1024
#define CSH_MIN_BUFSIZ 256

#define CSH_MAX_BUFCNT 4096
#define CSH_DEF_BUFCNT 32
#define CSH_MIN_BUFCNT 2

#ifndef __DBF_FILE_H
#ifndef __TRX_FILE_H
typedef PVOID TRX_CSH;
#endif
#endif

LPSTR TRXAPI csh_Errstr(UINT e);
UINT  TRXAPI csh_NumBuffers(TRX_CSH csh);
UINT  TRXAPI csh_SizBuffer(TRX_CSH csh);

extern int csh_errno;  /*Error code set or cleared by the following fcns*/

/*Basic cache management functions --*/
int   TRXAPI csh_Alloc(TRX_CSH *pcsh,UINT bufsiz,UINT bufcnt);
int   TRXAPI csh_Flush(TRX_CSH csh);  /*Flush all updated buffers*/
int   TRXAPI csh_Free(TRX_CSH csh);   /*Flush and delete cache*/
int   TRXAPI csh_Purge(TRX_CSH csh);  /*Discard all buffers unconditionally*/

#ifdef __cplusplus
}
#endif

#endif
/*End of TRX_CSH.H*/










