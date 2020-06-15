/* TRXERRT.C -- TRX file error strings*/

#include <a__trx.h>

/* Error codes defined in a_trx.h - TRX_ERR_BASE=30 --
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
*/
static char * _trx_errstr[] = {
 "Too many tree levels",
 "Invalid node size",
 "File node limit reached",
 "Duplicate key rejected",
 "Tree already nonempty",
 "Index empty or at EOF",
 "Tree not initialized",
 "No match found",
 "Tree is wrong type",
 "Current key was deleted",
 "Key too large",
 "File not in closed form",
 "Data truncated",
 "Key buffer not assigned"
};

#define TRX_ERR_LIMIT (TRX_ERR_BASE+sizeof(_trx_errstr)/sizeof(char *))

LPSTR TRXAPI trx_Errstr(UINT e)
{
	if (e >= TRX_ERR_BASE && e < TRX_ERR_LIMIT)
		return (LPSTR)_trx_errstr[e - TRX_ERR_BASE];
	return csh_Errstr(e);
}
