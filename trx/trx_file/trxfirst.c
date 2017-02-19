#include <a__trx.h>

int TRXAPI trx_First(TRX_NO trx)
{
   _trx_GetNodePos(trx,-2);
   return trx_errno;
}
