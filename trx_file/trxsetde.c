#include "a__trx.h"

int TRXAPI trx_SetDescend(TRX_NO trx, BOOL status)
{
	return _trx_SetUsrFlag(trx, TRX_KeyDescend, status);
}
