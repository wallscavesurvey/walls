/* TRX_FILE.H -- Index File Declarations */

#ifndef __TRX_FILE_H
#define __TRX_FILE_H

#ifndef __TRX_TYPE
#include "trx_type.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*File oriented user flags - used with trx_Open() and trx_Create --*/
enum {TRX_ReadWrite=0,TRX_Share=1,TRX_DenyWrite=2,TRX_ReadOnly=4,
      TRX_UserFlush=8,TRX_AutoFlush=16,TRX_ForceOpen=32};

/*Tree oriented structural and user flags --
  TRX_KeyMaxSize through TRX_KeyUnique can be set by trx_InitTree()
  as permanent structural flags. TRX_KeyUnique through TRX_KeyDescend
  can be set as temporary user flags via trx_Set..() functions.*/

enum {TRX_KeyMaxSize=2,TRX_KeyBinary=4,
      TRX_RecMaxSize=8,TRX_RecPosition=16,TRX_KeyUnique=32,
      TRX_KeyExact=64,TRX_KeyDescend=128};

/*Define if trxd daemon not running --*/
#ifdef _MSC_VER
#define _NO_TRXD
#undef _TRXD
#endif

/*Max number of file/user connections open at once --*/
#ifdef _TRXD
#define TRX_MAX_USERS 300
#define TRX_MAX_FILES 50
#else
#define TRX_MAX_USERS 100
#define TRX_MAX_FILES 20
#endif

#define TRX_DFLT_EXT "trx"
#define TRX_MAX_KEYLEN 255
#define TRX_MAX_PATH 128
#define TRX_MAX_BRANCH_LEVELS 10
#define TRX_MAX_MINPREWRITE 256
#define TRX_MAX_SIZNODE (0x8000)
#define TRX_MIN_SIZNODE 512
#define TRX_MAX_SIZEXTRA (0x7FFF)
#define TRX_MAX_NUMTREES 254

/*Basic error codes common to DBF and TRX file types --*/
enum {TRX_OK,
      TRX_ErrName,
      TRX_ErrFind,
      TRX_ErrArg,
      TRX_ErrMem,
      TRX_ErrRecsiz,
      TRX_ErrNoCache,
      TRX_ErrLocked,
      TRX_ErrRead,
      TRX_ErrWrite,
	  TRX_ErrCreate,
      TRX_ErrOpen,
      TRX_ErrClose,
      TRX_ErrDelete,
      TRX_ErrLimit,
      TRX_ErrShare,
      TRX_ErrFormat,
      TRX_ErrReadOnly,
      TRX_ErrSpace,
      TRX_ErrFileType,
      TRX_ErrDiskIO,
      TRX_ErrLock,
	  TRX_ErrFileShared,
	  TRX_ErrUsrAbort,
	  TRX_ErrRename
     };

/*TRX file specific errors --*/
#define TRX_ERR_BASE 100

enum {TRX_ErrLevel=TRX_ERR_BASE,
      TRX_ErrSizNode,
      TRX_ErrNodLimit,
      TRX_ErrDup,
      TRX_ErrNonEmpty,
      TRX_ErrEOF,
      TRX_ErrNoInit,
      TRX_ErrMatch,
      TRX_ErrTreeType,
      TRX_ErrDeleted,
      TRX_ErrSizKey,
      TRX_ErrNotClosed,
      TRX_ErrTruncate,
	  TRX_ErrNoKeyBuffer
     };

typedef PVOID TRX_CSH;
typedef UINT TRX_NO;
typedef DWORD TRX_POSITION;

/*Global variables referenced by applications --*/
extern TRX_CSH trx_defCache;
extern UINT trx_errno;
extern NPSTR trx_ext;

int TRXAPI	 trx_BuildTree(TRX_NO trx,LPCSTR tmppath,TRXFCN_IF getkey);

/*trx_BuildTree() related variables --*/
extern BYTE *	trx_bld_keybuf;		/*Key buffer filled by caller*/
extern void *	trx_bld_recbuf;		/*Record buffer filled by caller*/
extern DWORD 	trx_bld_keycount;	/*Number of keys indexed or merged at this stage*/
extern DWORD	trx_bld_mrgcount;	/*Total number of keys indexed*/
extern DWORD	trx_bld_keydelta;	/*BLD_KEYCOUNT callback interval - no messages if 0*/
extern DWORD	trx_bld_mrgdelta;	/*BLD_MRGCOUNT callback interval*/
extern DWORD	trx_bld_treebufs;	/*(32)  Number of trees in temporary indexes*/
extern DWORD	trx_bld_cachebufs;	/*(128) Number of cache buffers assigned to temp indexes*/		
extern char *	trx_bld_message;	/*Info message when _getkey(1) call is made*/
/*getkey(typ) argument types--*/
enum e_bldmsg {BLD_GETKEY,BLD_KEYSTART,BLD_KEYCOUNT,BLD_MRGSTART,BLD_MRGCOUNT,BLD_MRGSTOP};

int TRXAPI	 trx_Pack(TRX_NO *ptrx,TRXFCN_IF pckmsg);
/*trx_Pack() related variables --*/
extern DWORD	trx_pck_siznode;		/*Size of file nodes in bytes*/
extern DWORD	trx_pck_oldnodes;       /*Tree nodes in old tree/file: PCK_TREE/FILETOTALS*/
extern DWORD	trx_pck_newnodes;		/*Tree nodes in new tree/file: PCK_TREE/FILETOTALS*/
extern DWORD 	trx_pck_keycount;		/*Number of tree keys copied: PCK_KEYCOUNT*/
extern DWORD	trx_pck_treecount;		/*Current tree number: PCK_TREESTART*/
extern DWORD	trx_pck_keydelta;		/*PCK_KEYCOUNT callback interval - none if 0*/
extern char *   trx_pck_message;		/*contains callback message if not set NULL*/
extern BOOL		trx_pck_nodescend;		/*controls cut point*/
/*pckmsg(typ) argument types--*/
enum e_pckmsg {PCK_TREESTART,PCK_KEYCOUNT,PCK_TREETOTALS,PCK_FILETOTALS};


/*Set by FORMAT_ERROR() in _TRX_TEST version of library --*/
extern int  _trx_errLine;

extern DWORD trx_findResult;
/*Result of last trx_Find() or trx_FindNext()*/
            /*0000 - exact match*/
            /*xx00 - prefix match, xx=next key's suffix length*/
            /*00xx - xx=length of unmatching suffix*/
            /*FFFF - error or EOF*/

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
int TRXAPI   trx_AllocCache(TRX_NO trx,UINT bufcnt,BOOL makedef);
int TRXAPI   trx_AttachCache(TRX_NO trx,TRX_CSH csh);
int	TRXAPI	 trx_Clear(void); /*Close all TRX files opened by this process*/
int TRXAPI   trx_Close(TRX_NO trx);
int TRXAPI   trx_CloseDel(TRX_NO trx);
int TRXAPI   trx_Create(TRX_NO FAR *ptrx,LPSTR pathName,UINT mode,
              UINT numTrees,UINT sizExtra,UINT sizNode,UINT sizNodePtr);
int TRXAPI   trx_CreateInd(TRX_NO FAR *ptrx,LPSTR pathName,UINT mode,LPVOID fileDef);
int TRXAPI   trx_DeleteCache(TRX_NO trx);
int TRXAPI   trx_DetachCache(TRX_NO trx);
int TRXAPI   trx_Flush(TRX_NO trx);
int TRXAPI   trx_InitTree(TRX_NO trx,UINT sizRec,UINT sizKey,UINT initFlags);
int TRXAPI   trx_Open(TRX_NO FAR *ptrx,LPSTR pathName,UINT mode);
void TRXAPI  trx_Purge(TRX_NO trx);
int TRXAPI   trx_SetMinPreWrite(TRX_NO trx,UINT minPreWrite);
int TRXAPI	 trx_SetEmpty(TRX_NO trx); /*empty all trees of keys (truncate file)*/

/* Positioning functions --*/
int TRXAPI   trx_Find(TRX_NO trx,LPBYTE pKey);                   /*type 1*/
int TRXAPI   trx_FindNext(TRX_NO trx,LPBYTE pKey);               /*type 1*/
int TRXAPI   trx_First(TRX_NO trx);
int TRXAPI   trx_Last(TRX_NO trx);
int TRXAPI   trx_Next(TRX_NO trx);
int TRXAPI   trx_Prev(TRX_NO trx);
int TRXAPI   trx_SetPosition(TRX_NO trx,DWORD position);
int TRXAPI   trx_Skip(TRX_NO trx,long len);

/* Record/key append or update functions --*/
int TRXAPI   trx_AppendRec(TRX_NO trx,LPVOID pRec);              /*type 2*/
int TRXAPI   trx_DeleteKey(TRX_NO trx);                          /*type 1*/
int TRXAPI   trx_InsertKey(TRX_NO trx,LPVOID pRec,LPBYTE pKey);  /*type 1*/
int TRXAPI   trx_ReplaceKey(TRX_NO trx,LPVOID pRec,LPBYTE pKey); /*type 1*/
int TRXAPI   trx_PutRec(TRX_NO trx,LPVOID pRec);                 /*type 2*/

/* Data retrieving functions --*/
int TRXAPI	 trx_FillKeyBuffer(TRX_NO trx,int offset); /*offset=-2,..,2*/
int TRXAPI   trx_Get(TRX_NO trx,LPVOID recBuf,UINT sizRecBuf,
              LPBYTE keyBuf,UINT sizKeyBuf);                     /*type 1*/
int TRXAPI   trx_GetRec(TRX_NO trx,LPVOID recBuf,UINT sizRecBuf);
int TRXAPI   trx_GetKey(TRX_NO trx,LPVOID keyBuf,UINT sizKeyBuf);/*type 1*/
DWORD TRXAPI trx_GetPosition(TRX_NO trx);


/* Unrrestricted manipulation of file's extra data in header --*/
PVOID TRXAPI trx_ExtraPtr(TRX_NO trx);
int TRXAPI   trx_MarkExtra(TRX_NO trx);

/*Adjustment of tree operational modes --*/
int TRXAPI   trx_SetDescend(TRX_NO trx,BOOL status);
int TRXAPI   trx_SetExact(TRX_NO trx,BOOL status);
int TRXAPI	 trx_SetKeyBuffer(TRX_NO trx,void *keyBuffer); /*trees share it*/
int TRXAPI   trx_SetUnique(TRX_NO trx,BOOL status);

/* Return file status or description --*/
TRX_CSH TRXAPI trx_Cache(TRX_NO trx);
int TRXAPI     trx_Errno(TRX_NO trx);
LPSTR TRXAPI   trx_Errstr(UINT e);
LPVOID TRXAPI  trx_FileDef(TRX_NO trx);
int TRXAPI     trx_FileMode(TRX_NO trx);
NPSTR TRXAPI   trx_FileName(TRX_NO trx);
DWORD TRXAPI   trx_NumFreeNodes(TRX_NO trx);
DWORD TRXAPI   trx_NumFileNodes(TRX_NO trx);
DWORD TRXAPI   trx_SizFile(TRX_NO trx);
int TRXAPI     trx_NumTrees(TRX_NO trx);
int TRXAPI     trx_NumUsers(TRX_NO trx);
NPSTR TRXAPI   trx_PathName(TRX_NO trx);
UINT TRXAPI    trx_SizExtra(TRX_NO trx);
UINT TRXAPI    trx_SizNode(TRX_NO trx);
int TRXAPI     trx_SizNodePtr(TRX_NO trx);
UINT TRXAPI    trx_MinPreWrite(TRX_NO trx);
UINT TRXAPI    trx_NumPreWrite(TRX_NO trx);

/* Return tree status or description --*/
DWORD TRXAPI   trx_NumTreeNodes(TRX_NO trx);
DWORD TRXAPI   trx_NumKeys(TRX_NO trx);
DWORD TRXAPI   trx_NumRecs(TRX_NO trx);
DWORD TRXAPI   trx_RunLength(TRX_NO trx);
int TRXAPI     trx_SizRec(TRX_NO trx);
int TRXAPI     trx_SizKey(TRX_NO trx);
UINT TRXAPI    trx_InitTreeFlags(TRX_NO trx);
UINT TRXAPI    trx_UserTreeFlags(TRX_NO trx);
UINT TRXAPI    trx_UserFileFlags(TRX_NO trx);
#ifdef __cplusplus
}
#endif
#endif
/*End of A_TRX.H*/
