/* TRXNEXT.C */
#include "a__trx.h"

int TRXAPI trx_Next(TRX_NO trx)
{
	/*This function is a more efficient version of trx_Skip(trx,1).*/

	_trx_GetNodePos(trx, 1);
	return trx_errno;
}
