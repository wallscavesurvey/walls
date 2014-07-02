#include <a__trx.h>

apfcn_i _trx_SetUsrFlag(TRX_NO trx,UINT flag,BOOL status)
{
  if(!_GETTRXU) return TRX_ErrArg;
  if(status) _TRXU->UsrFlags|=flag;
  else _TRXU->UsrFlags&=~flag;
  return 0;
}
