/* TRXALLOC.C -- Functions required by TRXCREAT.C and TRXOPEN.C */

#include <a__trx.h>

UINT _trx_max_users = TRX_MAX_USERS;
UINT _trx_max_files = TRX_MAX_FILES;

/*Default TRX file extension --*/
LPSTR trx_ext = TRX_DFLT_EXT;

/*Global variables referenced by applications --*/
TRX_CSH       trx_defCache;
UINT          trx_errno;

/*Globals initialized by _trx_GetTrx() --*/
TRX_pFILE    _trx_pFile;
TRX_pFILEUSR _trx_pFileUsr;
TRX_pTREEVAR _trx_pTreeVar;
TRX_pTREEUSR _trx_pTreeUsr;
UINT         _trx_tree;

static BOOL	 bExit_Set = FALSE;

BOOL _trx_noErrNodRec;         /*Used by _trx_GetNode()*/

/*Each pointer in _trx_fileMap[] corresponds to a unique file, or
  handle returned by dos_Openfile(), etc. The pointers are maintained
  so that all valid (nonzero) elements are contiguous at the front
  of the array. */

TRX_pFILE    _trx_fileMap[TRX_MAX_FILES];

/*Each pointer in _trx_usrMap[] corresponds to a unique
  file/user connection. Nonzero elements are NOT necessarily
  contiguous. Index in this array is used to construct TRX_NO:
  TRX_NO = (index+1)<<16. */

TRX_pFILEUSR _trx_usrMap[TRX_MAX_USERS];

#ifndef _NO_TRXD
int *io_pidlst;
io_rectyp *io_prec = NULL;
#endif

/*=================================================================*/

apfcn_i _trx_TransferHdr(TRX_pFILE trxp, UINT typ)
{
	UINT len;
	UINT hdr_siz = TRX_HDR_SIZ(trxp->Fd.NumTrees, trxp->Fd.SizExtra);

	if (typ > 1) {
		typ = 1;
		len = 2;
	}
	else {
		len = hdr_siz;
		trxp->HdrChg = FALSE;
	}
	return dos_Transfer(typ, trxp->Cf.Handle,
		&trxp->FileID, trxp->Cf.Offset - hdr_siz, len) ?
		(trxp->Cf.Errno = TRX_ErrRead + typ) : 0;
}

apfcn_v _trx_FreeFile(TRX_pFILE trxp)
{
	int i;

	for (i = 0; i < TRX_MAX_FILES; i++)
		if (_trx_fileMap[i] == trxp) {
			while (++i < TRX_MAX_FILES)
				if ((_trx_fileMap[i - 1] = _trx_fileMap[i]) == 0) break;
			if (i == TRX_MAX_FILES) _trx_fileMap[TRX_MAX_FILES - 1] = 0;
			dos_Free(trxp);
			return;
		}
	/*Unreachable point!*/
}

apfcn_i _trx_AllocFile(TRX_FILEDEF FAR *pFd)
{
	TRX_pFILE trxp;

	if ((trxp = (TRX_pFILE)dos_Alloc(
		TRX_FILE_SIZ(pFd->NumTrees, pFd->SizExtra))) == 0)
		return TRX_ErrMem;
	trxp->Fd.NumTrees = pFd->NumTrees;
	trxp->Fd.SizExtra = pFd->SizExtra;
	trxp->MinPreWrite = 1;
	trxp->Cf.Offset = TRX_HDR_SIZ(pFd->NumTrees, pFd->SizExtra);
	trxp->Cf.Recsiz = pFd->SizNode;
	_trx_pFile = trxp;
	return 0;
}

static void _trx_Exit(void *f)
{
	FIX_EX(f);

	/*The above inserts f!=0 into chain and exits, or runs*/
	/*fcns below after more recently inserted fcns are executed with f=0.*/

	/*Close all open TRX files upon call to _dos_exit(0)*/
	trx_Clear();
	bExit_Set = FALSE;
}

apfcn_i _trx_AllocUsr(TRX_NO far *ptrx, TRX_pFILE trxp, UINT mode)
{
	int usrIndex;
	TRX_pFILEUSR usrp;

	for (usrIndex = 0; usrIndex < TRX_MAX_USERS; usrIndex++)
		if (_trx_usrMap[usrIndex] == 0) break;
	if (usrIndex == TRX_MAX_USERS) return TRX_ErrLimit;
	if ((usrp = (TRX_pFILEUSR)dos_Alloc(
		sizeof(TRX_FILEUSR) + trxp->Fd.NumTrees * sizeof(TRX_TREEUSR))) == 0)
		return TRX_ErrMem;
	usrp->pUsrFile = trxp;
	usrp->UsrIndex = usrIndex;
	usrp->UsrMode = mode &
		(TRX_Share + TRX_DenyWrite + TRX_ReadOnly + TRX_UserFlush + TRX_AutoFlush);
	usrp->pNextFileUsr = trxp->pFileUsr;
	_trx_usrMap[usrIndex] = trxp->pFileUsr = usrp;
	*ptrx = GET_TRX_NO(usrIndex);
	_trx_pFileUsr = usrp;

	if (!bExit_Set) {
		bExit_Set = TRUE;
		_dos_exit(_trx_Exit);
	}
	return 0;
}

apfcn_v _trx_FreeUsr(TRX_pFILEUSR usrp)
{
	TRX_pFILEUSR u = ((TRX_pFILE)usrp->pUsrFile)->pFileUsr;
	TRX_pFILEUSR uprev = 0;

	while (u && u != usrp) {
		uprev = u;
		u = u->pNextFileUsr;
	}
	if (u) {
		u = usrp->pNextFileUsr;
		if (uprev) uprev->pNextFileUsr = u;
		else ((TRX_pFILE)usrp->pUsrFile)->pFileUsr = u;
		_trx_usrMap[usrp->UsrIndex] = 0;
		dos_Free(usrp);
	}
}

apfcn_i _trx_FindFile(NPSTR name)
{
	int i;
	TRX_pFILE pf;

	for (i = 0; i < TRX_MAX_FILES; i++)
		if ((pf = _trx_fileMap[i]) == 0 || dos_MatchName(pf->Cf.Handle, name)) break;
	return i;
}

/*End of TRXALLOC.C*/
