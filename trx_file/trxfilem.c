#include <a__trx.h>

int TRXAPI trx_FileMode(TRX_NO trx)
{
  return _GETTRXM?_TRXM->UsrMode:0;
}
