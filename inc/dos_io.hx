/*DOS_IO.H -- Callback functions for TRX_FILE library*/

#ifndef __DOS_IO_H
#define __DOS_IO_H

#ifndef __TRX_TYPE_H
#include <trx_type.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

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
	  DOS_ErrDisk
      };

typedef struct {
  HANDLE DosHandle;
  DWORD Flags;
  char PathName[1];
} DOS_FILE,*DOS_FP;

/*dwFlagsAndAttributes argument passed to CreateFile -
  The following flags are ignored except for file creation --
  FILE_ATTRIBUTE_NORMAL	(Use if no other flags specified.)
  FILE_ATTRIBUTE_ARCHIVE
  FILE_ATTRIBUTE_READONLY
  FILE_ATTRIBUTE_HIDDEN
  FILE_ATTRIBUTE_SYSTEM
  These flags (high order bits) affect buffering strategy --
  FILE_ATTRIBUTE_TEMPORARY (kept in memory if possible)
  FILE_FLAG_NO_BUFFERING (buf addr, file offset, xfer size must be sector multiple)
  FILE_FLAG_WRITE_THROUGH (no lazy flushing)
  FILE_FLAG_RANDOM_ACCESS (no read ahead)
  FILE_FLAG_SEQUENTIAL_SCAN (faster sequential access)
  FILE_FLAG_DELETE_ON_CLOSE (useful for temporary files - avoids flush)
*/

/*Callback functions for low-level file I/O --*/

//#define   dos_GetDosHandle(h) (((DOS_FP)h)->Handle)
int  TRXAPI dos_OpenFile(int *pHandle,LPSTR fullpath,DWORD flags);
int  TRXAPI dos_CreateFile(int *pHandle,LPSTR fullpath,DWORD flags);

#ifdef _USE_WIN32LASTERROR
LPTSTR TRXAPI dos_LastErrorMsg(void);
#endif
int  TRXAPI dos_CloseFile(int Handle);
int  TRXAPI dos_CloseDelete(int Handle);
int  TRXAPI dos_Transfer(int io_fcn,int Handle,LPVOID buffer,
                         long fileoff,UINT nbytes);
int  TRXAPI dos_MatchName(int Handle,LPSTR fullPath);
//int  TRXAPI dos_FreeDosHandle(int Handle); //Closes file handle
//HANDLE TRXAPI dos_CheckDosHandle(int Handle); //Reopens if necessary
LPSTR TRXAPI dos_FileName(int Handle);
LPSTR TRXAPI dos_PathName(int Handle);
LPSTR TRXAPI dos_FullPath(LPSTR pathName,LPSTR defExt);

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

#ifdef __cplusplus
}
#endif
#endif
