#include "a__trx.h"

void TRXAPI trx_Purge(TRX_NO trx)
{
  _csf_Purge((CSF_NO)_GETTRXP);
}

