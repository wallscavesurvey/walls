#include "a__trx.h"

UINT TRXAPI trx_UserTreeFlags(TRX_NO trx)
{
  return _GETTRXU?_TRXU->UsrFlags:0;
}
