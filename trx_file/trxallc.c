#include <a__trx.h>

int TRXAPI trx_AllocCache(TRX_NO trx, UINT bufcnt, BOOL makedef)
{
	CSH_NO csh;

	if (!_trx_GetTrx(trx)) return TRX_ErrArg;
	if ((trx_errno = csh_Alloc((TRX_CSH *)&csh, _trx_pFile->Fd.SizNode,
		bufcnt)) == 0 && trx_AttachCache(trx, csh)) csh_Free(csh);
	if (makedef && !trx_errno) trx_defCache = csh;
	return trx_errno;
}
