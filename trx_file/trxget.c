/* TRXGET.C */
#include "a__trx.h"

int TRXAPI trx_Get(TRX_NO trx, LPVOID recBuf, UINT sizRecBuf,
	LPBYTE keyBuf, UINT sizKeyBuf)
{
	int e;

	/*Note: _trx_Btrec() alone could conceivably encounter a bad node,
	 but trx_GetKey() would encounter it first!*/

	if (((e = trx_GetKey(trx, keyBuf, sizKeyBuf)) == 0 || e == TRX_ErrTreeType) &&
		_trx_pTreeVar->SizRec &&
		_trx_Btrec(_trx_hdrp, _trx_pTreeUsr->KeyPos, recBuf, sizRecBuf))
		e = TRX_ErrFormat;
	return e;
}
