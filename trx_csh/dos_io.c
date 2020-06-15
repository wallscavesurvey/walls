/*DOS_IO.C -- Callbacks for TRX library --*/

#ifdef _MSC_VER
#include <windows.h>
#undef SHARE_ALL_OPENS
#else
#include <unistd.h>
#include <statbuf.h>
#endif

#include <malloc.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

#include <dos_io.h>

#ifndef _MSC_VER
#define HANDLE int
#endif

#define IO_ERROR(e) (GetDosLastError()?_dos_LastError:(e))
//#define IO_ERROR(e) (e)

/*======================================================*/
/*Get today's date in struct {BYTE year,month,day;} format --*/

void TRXAPI dos_GetSysDate(DBF_HDRDATE *pDate)
{
#ifdef _MSC_VER
	SYSTEMTIME t;
	GetLocalTime(&t);
	pDate->Year = (BYTE)(t.wYear - 1900);
	pDate->Month = (BYTE)t.wMonth;
	pDate->Day = (BYTE)t.wDay;
#else
	struct tm *t;
	time_t tim;
	time(&tim);
	t = localtime(&tim);
	pDate->Year = (BYTE)(t->tm_year);
	pDate->Month = (BYTE)t->tm_mon + 1;
	pDate->Day = (BYTE)t->tm_mday;
#endif
}

/*Memory management callback functions --*/

PVOID TRXAPI dos_Alloc(UINT size)
{
	return calloc(1, size);
}

void TRXAPI dos_Free(PVOID p)
{
	free(p);
}

/*======================================================*/

LPSTR TRXAPI dos_FullPath(LPCSTR pathName, LPCSTR defExt)
{
	/*Get absolute filename. Append extension defExt if
	  an extension of any kind (including trailing '.') is absent,
	  or force extension override if defExt has a '.' prefix.
	  Return pointer to static buffer, or NULL if path is invalid. */
#ifdef _MSC_VER
	  /*Static buffer for for returned pathname --*/
	static char _dos_nambuf[MAX_PATH + 2];
	LPSTR pName = NULL;
	DWORD len;

	if (!(len = GetFullPathName(pathName, MAX_PATH + 1, _dos_nambuf, &pName)) ||
		len > MAX_PATH) return NULL;
	if (defExt && *defExt) {
		if (!pName || *pName == 0) {
			pName = _dos_nambuf + strlen(_dos_nambuf);
		}
		else while (*pName && *pName != '.') pName++;
		if (*defExt == '.' || *pName == 0) {
			//Extension override required --
			if (*pName == 0) *pName = '.';
			pName[1] = 0;
			if (*defExt == '.') defExt++;
			if (strlen(_dos_nambuf) + strlen(defExt) > MAX_PATH) return NULL;
			strcpy(pName + 1, defExt);
		}
	}
	return _dos_nambuf;
#else
	  /*Static buffer for for returned pathname --*/
	static char _dos_nambuf[MAX_PATH + 1];
	LPSTR pExt;
	int len;

	if (!(*pathName == '/')) {
		if (!getcwd(_dos_nambuf, MAX_PATH) || *_dos_nambuf != '/') return NULL;
		pExt = _dos_nambuf + strlen(_dos_nambuf);
		if (pExt[-1] != '/') *pExt++ = '/';
		while (*pathName == '.') {
			if (*++pathName == '/') {
				pathName++;
				continue; /*ignore "/./" */
			}
			if (*pathName++ != '.' || *pathName++ != '/') return NULL;
			/*Trim one directory level off path --*/
			while (--pExt > _dos_nambuf && pExt[-1] != '/');
			if (pExt == _dos_nambuf) return NULL;
		}
	}
	else pExt = _dos_nambuf;

	if ((len = (pExt - _dos_nambuf) + strlen(pathName)) >= MAX_PATH) return NULL;
	strcpy(pExt, pathName);

	if (defExt && *defExt) {
		pExt = strrchr(_dos_nambuf, '/'); /*can't return NULL*/
		if (!strchr(pExt + 1, '.')) {
			if (*defExt != '.') _dos_nambuf[len++] = '.';
			if (len + strlen(defExt) >= MAX_PATH) return NULL;
			strcpy(_dos_nambuf + len, defExt);
		}
	}
	return _dos_nambuf;
#endif
}

UINT _dos_LastError = 0;
DWORD _sys_LastError = 0;

#ifdef _USE_WIN32LASTERROR
LPTSTR TRXAPI win32_LastErrorMsg(void)
{
	LPTSTR lpMsgBuf = NULL;
	if (!_sys_LastError) return NULL;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		_sys_LastError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);
	return lpMsgBuf;
}
#endif

static UINT GetDosLastError(void)
{
	UINT e = 0;

#ifdef _MSC_VER
	switch (_sys_LastError = GetLastError())
	{
		//Invalid file name or path
	case ERROR_DIRECTORY:
	case ERROR_NOT_DOS_DISK:
	case ERROR_WRONG_DISK:
	case ERROR_INVALID_DRIVE:
	case ERROR_NOT_SAME_DEVICE:
	case ERROR_DEV_NOT_EXIST:
	case ERROR_BAD_FORMAT:
	case ERROR_DUP_NAME:
	case ERROR_BAD_NET_NAME:
	case ERROR_BAD_NETPATH:
	case ERROR_INVALID_NAME:
	case ERROR_INVALID_LEVEL:
	case ERROR_NO_VOLUME_LABEL:
	case ERROR_BAD_REM_ADAP:
	case ERROR_BAD_DEV_TYPE:
	case ERROR_ALREADY_ASSIGNED:
	case ERROR_BUFFER_OVERFLOW:
	case ERROR_DIR_NOT_ROOT:
	case ERROR_LABEL_TOO_LONG:
	case ERROR_BAD_PATHNAME:
	case ERROR_FILENAME_EXCED_RANGE:
	case ERROR_META_EXPANSION_TOO_LONG:
		e = DOS_ErrName; break;

		//File or path not found
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
	case ERROR_NO_MORE_FILES:
	case ERROR_INVALID_HANDLE:
	case ERROR_DISK_CHANGE:
		e = DOS_ErrFind; break;

		//Too many open files
	case ERROR_TOO_MANY_NAMES:
	case ERROR_TOO_MANY_OPEN_FILES:
	case ERROR_SHARING_BUFFER_EXCEEDED:
	case ERROR_NO_MORE_SEARCH_HANDLES:
		e = DOS_ErrLimit; break;

		//File is read only or protected
	case ERROR_ACCESS_DENIED:
	case ERROR_INVALID_ACCESS:
	case ERROR_WRITE_PROTECT:
	case ERROR_BAD_NET_RESP:
	case ERROR_INVALID_PASSWORD:
	case ERROR_DRIVE_LOCKED:
	case ERROR_BUSY:
		e = DOS_ErrReadOnly; break;

		//File already open or not sharable
	case ERROR_ALREADY_EXISTS:
	case ERROR_SHARING_VIOLATION:
	case ERROR_LOCK_VIOLATION:
	case ERROR_NETNAME_DELETED:
	case ERROR_NETWORK_ACCESS_DENIED:
	case ERROR_NETWORK_BUSY:
	case ERROR_REQ_NOT_ACCEP:
	case ERROR_FILE_EXISTS:
	case ERROR_CANNOT_MAKE:
	case ERROR_SHARING_PAUSED:
		e = DOS_ErrShare; break;

		//Low-level disk error
	case ERROR_BAD_UNIT:
	case ERROR_NOT_READY:
	case ERROR_BAD_COMMAND:
	case ERROR_CRC:
	case ERROR_BAD_LENGTH:
	case ERROR_SEEK:
	case ERROR_SECTOR_NOT_FOUND:
	case ERROR_WRITE_FAULT:
	case ERROR_READ_FAULT:
	case ERROR_ADAP_HDW_ERR:
	case ERROR_UNEXP_NET_ERR:
	case ERROR_NET_WRITE_FAULT:
	case ERROR_OPERATION_ABORTED:
	case ERROR_IO_INCOMPLETE:
	case ERROR_IO_PENDING:
	case ERROR_INVALID_CATEGORY:
		e = DOS_ErrDiskIO; break;

		//Not enough disk space
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_NO_SPOOL_SPACE:
	case ERROR_DISK_FULL:
		e = DOS_ErrSpace; break;

		//Invalid file format or size
	case ERROR_NEGATIVE_SEEK:
	case ERROR_SEEK_ON_DEVICE:
	case ERROR_HANDLE_EOF:
		e = DOS_ErrFormat; break;

		/* Not used here as of yet --
		case ERROR_CURRENT_DIRECTORY:
			return CFileException::removeCurrentDir;
		case ERROR_INVALID_TARGET_HANDLE:
			return CFileException::invalidFile;
		case ERROR_DIR_NOT_EMPTY:
			return CFileException::removeCurrentDir;
		case ERROR_LOCK_FAILED:
			return CFileException::lockViolation;
		case ERROR_INVALID_ORDINAL:
			return CFileException::invalidFile;
		case ERROR_INVALID_EXE_SIGNATURE:
			return CFileException::invalidFile;
		case ERROR_BAD_EXE_FORMAT:
			return CFileException::invalidFile;
		case ERROR_SWAPERROR:
			return CFileException::accessDenied;
		*/
	}
#else
	switch (_sys_LastError = errno) {
	case EMFILE: e = DOS_ErrLimit; break;
	case EEXIST:
	case EACCES: e = DOS_ErrShare; break;
	case ENOENT: e = DOS_ErrFind; break;
	case ENOSPC: e = DOS_ErrSpace; break;
	case EBADF: e = DOS_ErrReadOnly; break;
	}
#endif

	return _dos_LastError = e; //Set by caller if 0
}

int TRXAPI _dos_OpenFile(int *pHandle, LPSTR fullPath, DWORD dwFlags, DWORD shFlag)
{
	DOS_FP fp;
#ifndef _MSC_VER
	int e;
#endif

	if ((fp = (DOS_FP)dos_Alloc(strlen(fullPath) + sizeof(DOS_FILE))) == 0)
		return DOS_ErrMem;

#ifdef _MSC_VER
	if ((fp->DosHandle = CreateFile(
		fullPath,
		(dwFlags == O_RDONLY) ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE),
#ifdef SHARE_ALL_OPENS
		(FILE_SHARE_READ | FILE_SHARE_WRITE),
#else
		//((shFlag&DOS_Share)?(FILE_SHARE_READ/*|FILE_SHARE_WRITE*/):0,
		(dwFlags == O_RDONLY) ? FILE_SHARE_READ : 0,
#endif
		NULL,
		(dwFlags&O_CREAT) ? CREATE_ALWAYS : OPEN_EXISTING,
		(shFlag&DOS_Sequential) ? FILE_FLAG_SEQUENTIAL_SCAN : FILE_FLAG_RANDOM_ACCESS,
		NULL)
		) == INVALID_HANDLE_VALUE
		)
	{
		dos_Free(fp);
		return IO_ERROR((dwFlags&O_CREAT) ? DOS_ErrCreate : DOS_ErrOpen);
	}
#else
	if (shFlag&DOS_Share) dwFlags |= O_SYNC;
	if ((fp->DosHandle = open(fullPath, dwFlags, 0666)) == -1 ||
		dos_Lock(DOS_NoWaitLock + ((shFlag&DOS_Share) ? DOS_ShareLock : DOS_ExclusiveLock), (int)fp, 0, 1))
	{
		if (fp->DosHandle == -1) {
			e = IO_ERROR((dwFlags&O_CREAT) ? DOS_ErrCreate : DOS_ErrOpen);
		}
		else {
			close(fp->DosHandle);
			e = DOS_ErrShare;
		}
		dos_Free(fp);
		return e;
	}
#endif
	_dos_LastError = _sys_LastError = 0;
	strcpy(fp->PathName, fullPath);
	fp->Flags = shFlag;
	*pHandle = (int)fp;
	return 0;
}

int TRXAPI dos_OpenFile(int *pHandle, LPSTR fullPath, DWORD shFlag)
{
	return _dos_OpenFile(pHandle, fullPath, O_RDWR, shFlag);
}

int TRXAPI dos_CreateFile(int *pHandle, LPSTR fullPath, DWORD shFlag)
{
	return _dos_OpenFile(pHandle, fullPath, O_RDWR | O_TRUNC | O_CREAT, shFlag);
}

//===================================================================
//Handle functions --
#define _FP ((DOS_FP)Handle)

int TRXAPI dos_CloseFile(int Handle)
{
#ifdef _MSC_VER
	int e = CloseHandle(_FP->DosHandle) ? 0 : IO_ERROR(DOS_ErrClose);
#else
	int e = (close(_FP->DosHandle) == 0) ? 0 : IO_ERROR(DOS_ErrClose);
#endif
	dos_FreeNear(_FP);
	return e;
}

int TRXAPI dos_CloseDelete(int Handle)
{
	UINT e = 0;
#ifdef _MSC_VER
	CloseHandle(_FP->DosHandle);
	e = DeleteFile(_FP->PathName) ? 0 : IO_ERROR(DOS_ErrDelete);
#else
	e = (remove(_FP->PathName) == 0) ? 0 : IO_ERROR(DOS_ErrDelete);
#endif
	dos_FreeNear(_FP);
	return e;
}

int	TRXAPI dos_Lock(int bLock, int Handle, long fileoff, UINT nbytes)
{
	HANDLE hFile = _FP->DosHandle;
	int retries;

#ifdef _MSC_VER
	DWORD flags = LOCKFILE_FAIL_IMMEDIATELY;
	OVERLAPPED ov;

	ov.Offset = fileoff;
	ov.OffsetHigh = 0;
	ov.Internal = ov.InternalHigh = 0;
	ov.hEvent = 0;
	if (bLock&DOS_LockMask) {
		flags = LOCKFILE_FAIL_IMMEDIATELY;
		if (!(bLock&DOS_ShareLock)) flags |= LOCKFILE_EXCLUSIVE_LOCK;
		retries = (bLock&DOS_NoWaitLock) ? 0 : 20;
		while (TRUE) {
			if (LockFileEx(hFile, flags, 0, nbytes, 0, &ov)) return 0;
			if (!retries--) break;
			Sleep(500); /*Sleep 1/2 second*/
		}
	}
	else if (UnlockFileEx(hFile, 0, nbytes, 0, &ov)) return 0;
#else
	struct flock flk;
	retries = (bLock&DOS_NoWaitLock) ? 0 : 10;
	while (TRUE) {
		flk.l_type = (bLock&DOS_LockMask) ? ((bLock&DOS_ShareLock) ? F_RDLCK : F_WRLCK) : F_UNLCK;
		flk.l_whence = SEEK_SET;
		flk.l_start = fileoff;
		flk.l_len = nbytes;
		flk.l_pid = 0; /*ignored*/
		if (-1 != fcntl(hFile, F_SETLK, &flk)) return 0;
		if (!retries--) break;
		sleep(1); /*sleep one second*/
	}
#endif
	return DOS_ErrLock;
}

int TRXAPI dos_Transfer(int io_fcn, int Handle, LPVOID buffer, long fileoff, UINT nbytes)
{
	HANDLE hFile = _FP->DosHandle;

#ifdef _MSC_VER
	DWORD count;

	if (SetFilePointer(hFile, fileoff, NULL, FILE_BEGIN) == 0xFFFFFFFF) goto _err;

	if (io_fcn) {
		if (!WriteFile(hFile, buffer, nbytes, &count, NULL) || count != nbytes) goto _err;
	}
	else {
		if (!ReadFile(hFile, buffer, nbytes, &count, NULL) || count != nbytes) goto _err;
	}
#else
	if (lseek(hFile, fileoff, SEEK_SET) == -1L) goto _err;

	if (io_fcn) {
		if ((UINT)write(hFile, buffer, nbytes) != nbytes) goto _err;
	}
	else {
		if ((UINT)read(hFile, buffer, nbytes) != nbytes) goto _err;
	}
#endif

	return 0;
_err:
	return IO_ERROR(DOS_ErrRead + io_fcn);
}

int TRXAPI dos_FlushFileBuffers(int Handle)
{
	return FlushFileBuffers(_FP->DosHandle) ? 0 : IO_ERROR(DOS_ErrWrite);
}

int TRXAPI dos_TransferEx(int io_fcn, int Handle, LPVOID buffer, LARGE_INTEGER fileoff, UINT nbytes)
{
	HANDLE hFile = _FP->DosHandle;

#ifdef _MSC_VER
	DWORD count;

	if (!SetFilePointerEx(hFile, fileoff, NULL, FILE_BEGIN)) goto _err;

	if (io_fcn) {
		if (!WriteFile(hFile, buffer, nbytes, &count, NULL) || count != nbytes) goto _err;
	}
	else {
		if (!ReadFile(hFile, buffer, nbytes, &count, NULL) || count != nbytes) goto _err;
	}
#else
	if (_lseeki64(hFile, fileoff, SEEK_SET) == -1L) goto _err;

	if (io_fcn) {
		if ((UINT)write(hFile, buffer, nbytes) != nbytes) goto _err;
	}
	else {
		if ((UINT)read(hFile, buffer, nbytes) != nbytes) goto _err;
	}
#endif

	return 0;
_err:
	return IO_ERROR(DOS_ErrRead + io_fcn);
}

int	TRXAPI dos_TruncateFile(int Handle, UINT length)
{
#ifdef _MSC_VER
	HANDLE hFile = _FP->DosHandle;
	if (SetFilePointer(hFile, length, NULL, FILE_BEGIN) == 0xFFFFFFFF ||
		!SetEndOfFile(hFile)) return -1;
	return 0;
#else
	return ftruncate(_FP->DosHandle, length);
#endif
}

LPSTR TRXAPI dos_FileName(int Handle)
{
#ifdef _MSC_VER
	int len = strlen(_FP->PathName);
	char *p = _FP->PathName + len;
	while (len--)
		if (*--p == '\\' || *p == '/') {
			p++;
			break;
		}
#else
	char *p = strrchr(_FP->PathName, '/');
	if (!p) p = _FP->PathName;
	else p++;
#endif
	return p;
}

LPSTR TRXAPI dos_PathName(int Handle)
{
	return _FP->PathName;
}

int TRXAPI dos_MatchName(int Handle, LPSTR fullPath)
{
#ifdef _MSC_VER
	return _stricmp(fullPath, _FP->PathName) == 0;
#else
	return strcmp(fullPath, _FP->PathName) == 0;
#endif
}

void _dos_exit(fnptr_vp f)
{
	FIX_EX(f);
}
/*End of DOS_IO.C*/
