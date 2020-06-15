#include "a__trx.h"

int TRXAPI trx_SetMinPreWrite(TRX_NO trx, UINT minPreWrite)
{
	if (minPreWrite > TRX_MAX_MINPREWRITE || !_GETTRXM) return TRX_ErrArg;
	if (_TRXM->UsrMode&TRX_ReadOnly) return TRX_ErrReadOnly;
	if (!minPreWrite && (_TRXM->UsrMode&(TRX_UserFlush + TRX_AutoFlush)))
		minPreWrite++;
	_trx_pFile->MinPreWrite = minPreWrite;
	return 0;
}
