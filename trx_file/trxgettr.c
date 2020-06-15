#include <a__trx.h>

TRX_pFILE NEAR PASCAL _trx_GetTrx(TRX_NO trx)
{
	_trx_tree = (trx & 0xFF);

	if ((trx >>= 16) > _trx_max_users ||
		(_trx_pFileUsr = _trx_usrMap[--trx]) == NULL ||
		trx != _trx_pFileUsr->UsrIndex ||
		_trx_tree >= (_trx_pFile = _trx_pFileUsr->pUsrFile)->Fd.NumTrees)
	{
		trx_errno = TRX_ErrArg;
		return NULL;
	}
	_trx_pTreeUsr = &_trx_pFileUsr->TreeUsr[_trx_tree];
	_trx_pTreeVar = &_trx_pFile->TreeVar[_trx_tree];
	_trx_pFileUsr->Errno = trx_errno = 0;
	return _trx_pFile;
}

