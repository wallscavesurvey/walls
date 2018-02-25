/* DBF_FILE.H -- Index File Declarations */

#ifndef __DBF_FILE_H
#define __DBF_FILE_H

#ifndef __TRX_TYPE_H
#include "trx_type.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*File oriented user flags - used with dbf_Open() and dbf_Create() --*/
enum {DBF_ReadWrite=0,DBF_Share=1,DBF_DenyWrite=2,DBF_ReadOnly=4,
      DBF_UserFlush=8,DBF_AutoFlush=16,DBF_ForceOpen=32,
      DBF_Sequential=64,DBF_NoCloseDate=128,DBF_TypeDBF2=16384};

#define DBF_DFLT_EXT "dbf"
#define DBF_DFLT_FLDTYPES "FMDLNC"

#define DBF2_MAXFLD             32
#define DBF2_FILEID		0x02
#define DBF_MAXFLD		255
#define DBF_FILEID      0x03
#define DBF_FILEID_MEMO 0x83

/*Max numbers of DBF files and file/user connections open at once --*/
#define DBF_MAX_FILES 300    /*changed from 200, 2016-01-12*/
#define DBF_MAX_USERS 300
#define DBF_MIN_BUFSIZ 512 /*Used by dbf_AllocCache*/

/*Basic error codes common to DBF and TRX file types --*/
enum {
	  DBF_OK,
      DBF_ErrName,
      DBF_ErrFind,
      DBF_ErrArg,
      DBF_ErrMem,
      DBF_ErrRecsiz,
      DBF_ErrNoCache,
      DBF_ErrLocked,
      DBF_ErrRead,
      DBF_ErrWrite,
      DBF_ErrCreate,
      DBF_ErrOpen,
      DBF_ErrClose,
      DBF_ErrDelete,
      DBF_ErrLimit,
      DBF_ErrShare,
      DBF_ErrFormat,
      DBF_ErrReadOnly,
      DBF_ErrSpace,
      DBF_ErrFileType,
      DBF_ErrDiskIO,
	  DBF_ErrLock,
	  DBF_ErrFileShared,
 	  DBF_ErrUsrAbort,
	  DBF_ErrRename
    };

/*Specific errors for DBF files --*/
#define DBF_ERR_BASE 100

enum {DBF_ErrEOF=DBF_ERR_BASE,
      DBF_ErrNotClosed,
      DBF_ErrFldDef,
	  DBF_ErrRecChanged
     };

#ifndef __TRX_CSH_H
#ifndef __TRX_FILE_H
typedef PVOID TRX_CSH;
#endif
#endif

typedef UINT DBF_NO;

#define DBF_BIN_PREFIX '_'

typedef struct { /*16 bytes*/
  char  F_Nam[11];           /*Null-terminated field name, capitalized*/
  char  F_Typ;               /*Field type - member of dbf_fldTypes[]*/
  BYTE  F_Len;               /*Field length*/
  BYTE  F_Dec;               /*Decimal places*/
  WORD  F_Off;               /*Offset of field in record - Initialization
                               not required by dbf_Create()*/
} DBF_FLDDEF;
typedef DBF_FLDDEF NEAR *DBF_pFLDDEF;

/*Global variables referenced by applications --*/
extern TRX_CSH dbf_defCache;
extern UINT dbf_errno;
extern LPCSTR dbf_ext;
extern NPSTR dbf_fldTypes;

/*Set by FORMAT_ERROR() in _DBF_TEST version of library --*/
extern int  _dbf_errLine;

/*Basic cache management functions --*/
int   TRXAPI csh_Alloc(TRX_CSH *pcsh,UINT bufsiz,UINT bufcnt);
int   TRXAPI csh_Flush(TRX_CSH csh);  /*Flush all updated buffers*/
int   TRXAPI csh_Free(TRX_CSH csh);   /*Flush and delete cache*/
int   TRXAPI csh_Purge(TRX_CSH csh);  /*Discard all buffers unconditionally*/

extern int csh_errno;  /*Error code set or cleared by the above*/

LPSTR TRXAPI csh_Errstr(UINT e);
UINT  TRXAPI csh_NumBuffers(TRX_CSH csh);
UINT  TRXAPI csh_SizBuffer(TRX_CSH csh);

/* File open, initialization, and flush functions --*/
int DBFAPI dbf_AllocCache(DBF_NO dbf,UINT bufsiz,UINT numbufs,BOOL makedef);
int DBFAPI dbf_AttachCache(DBF_NO dbf,TRX_CSH csh);
int DBFAPI dbf_Clear(void); /*Unconditionally close all DBF's opened by this process*/
int DBFAPI dbf_Close(DBF_NO dbf);
int DBFAPI dbf_CloseDel(DBF_NO dbf);
int DBFAPI dbf_Create(DBF_NO *pdbf,LPCSTR pathName,UINT mode,
                UINT numflds, DBF_FLDDEF *pFldDef);
int DBFAPI dbf_DetachCache(DBF_NO dbf);
int DBFAPI dbf_DeleteCache(DBF_NO dbf); /*Frees cache if last file*/
int DBFAPI dbf_Flush(DBF_NO dbf);
int DBFAPI dbf_Open(DBF_NO *pdbf,LPSTR pathName,UINT mode);

/* Positioning functions. All will first commit to the file (or cache)
   the record at the current position (if revised) before repositioning
   and loading the user's buffer with a copy of the new current record.*/

#define		 dbf_First(dbf) dbf_Go(dbf,1)
int DBFAPI   dbf_Go(DBF_NO dbf,DWORD position);
int DBFAPI   dbf_Last(DBF_NO dbf);
int DBFAPI   dbf_Next(DBF_NO dbf);
int DBFAPI   dbf_Prev(DBF_NO dbf);
int DBFAPI   dbf_Skip(DBF_NO dbf,long len);

/*Functions that add a new file record. The new record
  becomes the user's current record --*/

int DBFAPI   dbf_AppendBlank(DBF_NO dbf);
int DBFAPI   dbf_AppendZero(DBF_NO dbf); /*For binary records only*/
int DBFAPI   dbf_AppendRec(DBF_NO dbf,LPVOID pRec); /*Includes delete flag*/
/*Use current record buffer, marked or umarked --*/
int DBFAPI   dbf_AppendRecPtr(DBF_NO dbf); 

/*Record update functions. All but the last mark the current
  record (user's buffer copy) as having been modified --*/

int DBFAPI   dbf_Mark(DBF_NO dbf);
int DBFAPI   dbf_PutFld(DBF_NO dbf,LPVOID pBuf,UINT nFld);
int DBFAPI   dbf_PutFldStr(DBF_NO dbf,LPCSTR pStr,UINT nFld);
int DBFAPI   dbf_PutRec(DBF_NO dbf,LPVOID pBuf);
int DBFAPI   dbf_UnMark(DBF_NO dbf); /*Clears record's "modified" flag*/

/*Marks header as modified - attempt this before actual
  modification via dbf_HdrRes16() to confirm access --*/
int DBFAPI   dbf_MarkHdr(DBF_NO dbf);

/* Data retrieving functions returning dbf_errno --*/
int DBFAPI   dbf_GetFld(DBF_NO dbf,LPVOID pBuf,UINT nFld);
int DBFAPI   dbf_GetFldStr(DBF_NO dbf,LPSTR pStr,UINT nFld);
int DBFAPI   dbf_GetRec(DBF_NO dbf,LPVOID pBuf);

/*Data retrieving function returning an object address (0 if error) --*/
LPVOID DBFAPI dbf_Fld(DBF_NO dbf,LPVOID pBuf,UINT nFld);
LPSTR DBFAPI  dbf_FldStr(DBF_NO dbf,LPSTR pStr,UINT nFld);
LPVOID DBFAPI dbf_Rec(DBF_NO dbf,LPVOID pBuf);
LPBYTE DBFAPI dbf_HdrRes16(DBF_NO dbf); /*Ptr to 16-byte area for app use*/
LPBYTE DBFAPI dbf_HdrRes2(DBF_NO dbf); /*Ptr to 2-byte area for app use*/

/* Return file status or description --*/
TRX_CSH DBFAPI dbf_Cache(DBF_NO dbf);
int DBFAPI     dbf_Errno(DBF_NO dbf);
LPSTR DBFAPI   dbf_Errstr(UINT e);
DWORD DBFAPI   dbf_FileMode(DBF_NO dbf);
NPSTR DBFAPI   dbf_FileName(DBF_NO dbf);
DBF_pFLDDEF DBFAPI dbf_FldDef(DBF_NO dbf,UINT nFld);
int DBFAPI     dbf_GetFldDef(DBF_NO dbf,DBF_FLDDEF *pF,UINT nFld,int count);
int DBFAPI     dbf_GetHdrDate(DBF_NO dbf,DBF_HDRDATE *pDate);
int DBFAPI 	   dbf_SetHdrDate(DBF_NO dbf,DBF_HDRDATE *pDate);
UINT DBFAPI	   dbf_FileID(DBF_NO dbf);
LPVOID DBFAPI  dbf_FldPtr(DBF_NO dbf,UINT nFld);
PSTR DBFAPI    dbf_FldNam(DBF_NO dbf,PSTR pBuf,UINT nFld);
LPCSTR DBFAPI   dbf_FldNamPtr(DBF_NO,UINT nFld);
UINT DBFAPI    dbf_FldNum(DBF_NO dbf,LPCSTR pFldNam);
UINT DBFAPI    dbf_FldDec(DBF_NO dbf,UINT nFld);
LPSTR DBFAPI   dbf_FldKey(DBF_NO dbf,LPSTR pStr,UINT nFld);
UINT DBFAPI    dbf_FldLen(DBF_NO dbf,UINT nFld);
int DBFAPI     dbf_FldTyp(DBF_NO dbf,UINT nFld);
void DBFAPI    dbf_FreeFldDef(DBF_NO dbf);
HANDLE DBFAPI  dbf_Handle(DBF_NO dbf);
UINT DBFAPI    dbf_NumUsers(DBF_NO dbf);
DWORD DBFAPI   dbf_NumRecs(DBF_NO dbf);
UINT DBFAPI    dbf_NumFlds(DBF_NO dbf);
NPSTR DBFAPI   dbf_PathName(DBF_NO dbf);
DWORD DBFAPI   dbf_Position(DBF_NO dbf);
LPVOID DBFAPI  dbf_RecPtr(DBF_NO dbf);
UINT DBFAPI    dbf_SizRec(DBF_NO dbf);
UINT DBFAPI    dbf_SizHdr(DBF_NO dbf);
DWORD DBFAPI   dbf_OpenFlags(DBF_NO dbf);
UINT DBFAPI    dbf_Type(DBF_NO dbf);
#ifdef __cplusplus
}
#endif

#endif
/*End of DBF_FILE.H*/
