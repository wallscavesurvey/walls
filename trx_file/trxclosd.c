#include <a__trx.h>

int TRXAPI trx_CloseDel(TRX_NO trx)
{
	int e;

	if (!_GETTRXP) return TRX_ErrArg;
	if (_trx_pFileUsr->pNextFileUsr || _trx_pFile->pFileUsr != _trx_pFileUsr) {
		_trx_FreeUsr(_trx_pFileUsr);
		e = TRX_ErrShare;
	}
	else {
		_csf_Purge((CSF_NO)_TRXP);
		e = _csf_DeleteCache((CSF_NO)_TRXP, (CSH_NO *)&trx_defCache);
		if (_trx_pFileUsr->UsrMode&TRX_ReadOnly) {
			if (dos_CloseFile(_TRXP->Cf.Handle) && !e) e = TRX_ErrClose;
			else if (!e) e = TRX_ErrReadOnly;
		}
		else if (dos_CloseDelete(_TRXP->Cf.Handle) && !e) e = TRX_ErrDelete;
		_trx_FreeUsr(_trx_pFileUsr);
		_trx_FreeFile(_TRXP);
	}
	return trx_errno = e;
}
