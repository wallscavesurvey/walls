/* TRXPUTR.C */
#include <string.h>
#include "a__trx.h"

int TRXAPI trx_PutRec(TRX_NO trx, LPVOID recBuf)
{
	CSH_HDRP hp;
	LPVOID vp;

	/*Note: _trx_Btrecp() can fail only if the node is corrupted.
	  We must set the file error flag to indicate this fatal condition!*/

	if ((hp = _trx_GetNodePos(trx, 0)) != 0) {
		if ((vp = _trx_Btrecp(hp, _trx_pTreeUsr->KeyPos)) == 0)
			trx_errno = _trx_pFile->Cf.Errno = TRX_ErrFormat;
		else {
			memcpy(vp, recBuf, _trx_pTreeVar->SizRec);
			_csh_MarkHdr(hp);
			_trx_ErrFlush();
		}
	}
	return(trx_errno);
}
