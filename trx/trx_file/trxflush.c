/* TRXFLUSH.C -- Functions related to TRX file updates*/

#include <a__trx.h>

apfcn_i _trx_ErrReadOnly(void)
{
  /*This function is called at the start of any file update operation.
  It checks for read only status. It also makes sure that if no flush
  modes are in effect (user or auto) the disk version of the file
  is marked as "non-closed".*/

  UINT m=_trx_pFileUsr->UsrMode&(TRX_ReadOnly+TRX_AutoFlush+TRX_UserFlush);

  if(m&TRX_ReadOnly) return trx_errno=TRX_ErrReadOnly;
  if(!m && _trx_pFile->FileID==TRX_CLOSED_FILE_ID) {
    _trx_pFile->FileID=TRX_OPENED_FILE_ID;
    return _trx_TransferHdr(_trx_pFile,2);
  }
  return 0;
}

apfcn_i _trx_FlushHdr(TRX_pFILE trxp)
{
  return (trxp->HdrChg && _trx_TransferHdr(trxp,1)) ?
    (trx_errno=TRX_ErrWrite):0;
}

apfcn_i _trx_ErrFlushHdr(void)
{
  return (_trx_pFileUsr->UsrMode&TRX_AutoFlush)?_trx_FlushHdr(_trx_pFile):0;
}

apfcn_i _trx_Flush(TRX_pFILE trxp)
{
  /*Any flush failure is non-recoverable (file is corrupted).
    _trx_FlushHdr() can set Cf.Errno. _csf_Flush() may set Cf.Errno but
    returns it in any case. If an error occurs we must try to insure
    that the file on disk is flagged "non-closed" (if we have not
    already done so).*/

  _trx_FlushHdr(trxp);
  trx_errno=_csf_Flush((CSF_NO)trxp);

  if(trx_errno && trxp->FileID!=TRX_OPENED_FILE_ID) {
    trxp->FileID=TRX_OPENED_FILE_ID;
    _trx_TransferHdr(trxp,2);
  }
  return trx_errno;
}

apfcn_i _trx_ErrFlush(void)
{
  return (_trx_pFileUsr->UsrMode&TRX_AutoFlush)?_trx_Flush(_trx_pFile):0;
}

int TRXAPI trx_Flush(TRX_NO trx)
{
  if(!_GETTRXP) return TRX_ErrArg;
  if((_trx_pFileUsr->UsrMode&TRX_ReadOnly)!=0) return 0;
  return _trx_Flush(_TRXP);
}
