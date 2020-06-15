#include "a__trx.h"

int TRXAPI trx_SetUnique(TRX_NO trx, BOOL status)
{
	int e = _trx_SetUsrFlag(trx, TRX_KeyUnique, status);

	if (!e && !status && (_trx_pTreeVar->InitFlags&TRX_KeyUnique) != 0) {
		_trx_pTreeUsr->UsrFlags |= TRX_KeyUnique;
		e = TRX_ErrTreeType;
	}
	return e;
}
