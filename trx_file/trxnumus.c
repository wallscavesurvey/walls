#include "a__trx.h"

int TRXAPI trx_NumUsers(TRX_NO trx)
{
  int i=0;
  TRX_pFILEUSR usrp;

  if(_GETTRXP)
    for(usrp=_TRXP->pFileUsr;usrp;usrp=usrp->pNextFileUsr) i++;
  return i;
}
