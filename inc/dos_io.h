/*DOS_IO.H -- Callback functions for TRX_FILE library*/

#ifndef __DOS_IO_H
#define __DOS_IO_H

#ifndef __TRX_TYPE_H
#include <trx_type.h>
#endif

#ifndef _INC_FCNTL
#include <fcntl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum {DOS_Exclusive=0,DOS_Share=1,DOS_Sequential=64}; /*file open flags*/
enum {DOS_UnLock=0,DOS_ShareLock=1,DOS_ExclusiveLock=2,
      DOS_LockMask=3,DOS_NoWaitLock=4}; /*lock flags*/
	
/*Basic error codes common to DBF and TRX file types --*/
enum {DOS_OK,
      DOS_ErrName,
      DOS_ErrFind,
      DOS_ErrArg,
      DOS_ErrMem,
      DOS_ErrRecsiz,
      DOS_ErrNoCache,
      DOS_ErrLocked,
      DOS_ErrRead,
      DOS_ErrWrite,
      DOS_ErrCreate,
      DOS_ErrOpen,
      DOS_ErrClose,
      DOS_ErrDelete,
      DOS_ErrLimit,
      DOS_ErrShare,
      DOS_ErrFormat,
      DOS_ErrReadOnly,
      DOS_ErrSpace,
      DOS_ErrFileType,
      DOS_ErrDiskIO,
      DOS_ErrLock,
	  DOS_ErrFileShared,
	  DOS_ErrUsrAbort
      };

/*Callback functions for low-level file I/O --*/

typedef struct {
  HANDLE DosHandle;
  DWORD Flags;
  char PathName[1];
} DOS_FILE,*DOS_FP;

int	 TRXAPI _dos_OpenFile(int *pHandle,LPSTR fullPath,DWORD dwFlags,DWORD shFlag); //used by nti_file.lib
int  TRXAPI dos_OpenFile(int *pHandle,LPSTR fullpath,DWORD flags);
int  TRXAPI dos_CreateFile(int *pHandle,LPSTR fullpath,DWORD flags);

#ifdef _USE_WIN32LASTERROR
LPTSTR TRXAPI dos_LastErrorMsg(void);
#endif

int   TRXAPI dos_CloseFile(int Handle);
int   TRXAPI dos_CloseDelete(int Handle);
LPSTR TRXAPI dos_FileName(int Handle);
int	  TRXAPI dos_FlushFileBuffers(int Handle);
LPSTR TRXAPI dos_FullPath(LPCSTR pathName,LPCSTR defExt);
int	  TRXAPI dos_Lock(int bLock,int Handle,long fileoff,UINT nbytes);
int   TRXAPI dos_MatchName(int Handle,LPSTR fullPath);
LPSTR TRXAPI dos_PathName(int Handle);
int   TRXAPI dos_Transfer(int io_fcn,int Handle,LPVOID buffer,
                         long fileoff,UINT nbytes);
int TRXAPI dos_TransferEx(int io_fcn,int Handle,LPVOID buffer,
						 LARGE_INTEGER fileoff,UINT nbytes);
int	  TRXAPI dos_TruncateFile(int handle,UINT length);

void _dos_exit(fnptr_vp f);

/*Get today's date for updating DBF file header --*/
void TRXAPI dos_GetSysDate(DBF_HDRDATE *pDate);

/*Callback functions for memory management --*/
PVOID		TRXAPI dos_Alloc(UINT size);
void		TRXAPI dos_Free(PVOID p);
#define		dos_AllocHuge	dos_Alloc
#define		dos_FreeHuge	dos_Free
#define		dos_AllocNear	dos_Alloc
#define		dos_FreeNear	dos_Free
#define		dos_AllocFar	dos_Alloc
#define		dos_FreeFar		dos_Free

#ifdef _MSC_VER
#define _PATH_SEP '\\'
#else
#define _PATH_SEP '/'
#endif

#ifdef __cplusplus
}
#endif
#endif






