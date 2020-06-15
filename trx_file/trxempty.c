#include "a__trx.h"

int TRXAPI trx_SetEmpty(TRX_NO trx)
{
	int i;
	TRX_pTREEVAR tree;
	TRX_pFILEUSR fusr;
	TRX_pTREEUSR tusr;

	if (!_GETTRXP) return trx_errno = TRX_ErrArg;

	fusr = _trx_pFileUsr;

	if (fusr->UsrMode&TRX_ReadOnly) return trx_errno = TRX_ErrReadOnly;
	if (fusr->UsrMode&TRX_Share) return trx_errno = TRX_ErrFileShared;

	/*First empty all cache buffers, ignoring updated nodes --*/
	_csf_Purge((CSF_NO)_TRXP);

	/*Modify tree header structs without changing InitTree() settings --*/
	tree = _TRXP->TreeVar;
	tusr = fusr->TreeUsr;
	for (i = _TRXP->Fd.NumTrees; i; i--, tree++, tusr++) {
		tree->NumKeys = 0;
		tree->Root = tree->NumNodes = tree->LowLeaf = tree->HighLeaf = 0;
		tusr->NodePos = 0; tusr->KeyPos = 0;
	}
	_TRXP->NumPreWrite = 0;
	_TRXP->NumNodes = _TRXP->FirstFreeNode = 0;
	_TRXP->NumFreeNodes = 0;

	dos_TruncateFile(_TRXP->Cf.Handle, _TRXP->Cf.Offset); /*Ignore error return*/

	_TRXP->HdrChg = TRUE;
	return _trx_FlushHdr(_TRXP);
}
