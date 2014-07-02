#include <a__trx.h>

int TRXAPI trx_Close(TRX_NO trx)
{
  int e;

  if(!_GETTRXP) return TRX_ErrArg;

  if(_trx_pFileUsr->pNextFileUsr || _trx_pFile->pFileUsr!=_trx_pFileUsr) {
    /*If we are NOT the last user, a file close still forces a flush if
    we are in a writeable mode --*/
    if((_trx_pFileUsr->UsrMode&TRX_ReadOnly)==0) e=_trx_Flush(_TRXP);
    else e=_TRXP->Cf.Errno;
    _trx_FreeUsr(_trx_pFileUsr);
  }
  else {
    /*We are the last user. Regardless of our mode we must flush
    the file and restore the file integrity flag if no errors have
    occurred --*/

    if(_TRXP->Cf.Errno==0 && _TRXP->FileID!=TRX_CLOSED_FILE_ID) {
      _TRXP->FileID=TRX_CLOSED_FILE_ID;
      if(!_TRXP->HdrChg) _trx_TransferHdr(_TRXP,2);
    }
    e=_trx_Flush(_TRXP);
    
 #ifndef _NO_TRXD
    if(io_prec) {
       /*Detach from the trxd cache but don't delete it --*/
       if(_csf_DetachCache((CSF_NO)_TRXP) && !e) e=_TRXP->Cf.Errno;
    }
    else {
      /*This will detach from and also free the cache if it is the default cache
        and we are the last file --*/
      if(_csf_DeleteCache((CSF_NO)_TRXP,(CSH_NO *)&trx_defCache) && !e)
            e=_TRXP->Cf.Errno;
    }
 #else
      /*This will detach from and also free the cache if it is the default cache
        and we are the last file --*/
      if(_csf_DeleteCache((CSF_NO)_TRXP,(CSH_NO *)&trx_defCache) && !e)
            e=_TRXP->Cf.Errno;
 #endif

    if(_TRXP->Cf.Handle!=-1 && dos_CloseFile(_TRXP->Cf.Handle) && !e)
      e=TRX_ErrClose;
    _trx_FreeUsr(_trx_pFileUsr);
    _trx_FreeFile(_TRXP);
  }
  return trx_errno=e;
}

int DBFAPI trx_Clear(void)
{
	/*Close all TRX's opened by this process*/
	UINT usrIndex;
	int e=0;

	for(usrIndex=0;usrIndex<_trx_max_users;usrIndex++) 
		if(_trx_usrMap[usrIndex]) {
            if(trx_Close(GET_TRX_NO(usrIndex)) && !e) e=trx_errno;
		}
	return trx_errno=e;
}
