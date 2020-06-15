#include "a__trx.h"

/*Variables used by trx_InsertKey() and trx_DeleteKey() to revise
  user tree positions. The descriptions apply to trx_InsertKey() --*/
HNode _trx_p_RevLeaf; /*Original target node*/
HNode _trx_p_NewLeaf; /*New leaf node when _trx_p_NewKeys>0 */
UINT _trx_p_RevKeys;  /*Keys remaining in original node*/
UINT _trx_p_OldKeys;  /*Keys originally in target node*/
UINT _trx_p_KeyPos;   /*Position of inserted key in complete sequence*/

/*Global line indicator for TRX file consistency tests --*/
#ifdef _TRX_TEST
int _trx_errLine;
#endif

apfcn_i _trx_FormatError(void)
{
	return _trx_pFile->Cf.Errno = trx_errno = TRX_ErrFormat;
}

#ifdef _TRX_TEST
apfcn_i _trx_FormatErrorLine(int line)
{
	_trx_errLine = line;
	return _trx_FormatError();
}
#endif
