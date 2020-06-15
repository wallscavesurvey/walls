/* CSHERRC.C -- Cache manager error strings*/

#include <a__csh.h>

/*Basic error codes common to DBF and TRX file types --
enum {
	  TRX_OK,
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
*/


static char * _csh_errstr[] = {
	"No error",
	"Invalid file name or path",
	"File or path not found",
	"Bad argument",
	"Not enough memory",
	"Buffer size too small",
	"No active cache",
	"All buffers locked",
	"Read failure",
	"Write failure",
	"Create failure",
	"Open failure",
	"Close failure",
	"Delete failure",
	"Too many open files",
	"File is protected by another process",
	"Invalid file format or size",
	"File is read only or inaccessible",
	"Not enough disk space",
	"Wrong type of file",
	"Low-level disk error",
	"Record protected by another process",
	"Operation illegal with shared file",
	"Operation aborted by user",
	"System cannot rename file"
};

#define CSH_ERR_LIMIT (sizeof(_csh_errstr)/sizeof(char *))

LPSTR TRXAPI csh_Errstr(UINT e)
{
	return (LPSTR)_csh_errstr[e < CSH_ERR_LIMIT ? e : DOS_ErrArg];
}

