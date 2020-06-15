/* TRXCREAT.C -- Index File Creation*/

#include <a__trx.h>

int TRXAPI trx_Create(
	TRX_NO FAR *ptrx,
	LPSTR pathName,
	UINT mode,
	UINT numTrees,
	UINT sizExtra,
	UINT sizNode,
	UINT sizNodePtr)
{
	TRX_FILEDEF fd;
	if (!numTrees) numTrees = 1;
	if (!sizNode) sizNode = trx_defCache ? ((CSH_NO)trx_defCache)->Bufsiz : 1024;
	if (!sizNodePtr) sizNodePtr = 4;
	fd.NumTrees = (WORD)numTrees;
	fd.SizExtra = (WORD)sizExtra;
	fd.SizNode = (WORD)sizNode;
	fd.SizNodePtr = (WORD)sizNodePtr;
	return trx_CreateInd(ptrx, pathName, mode, &fd);
}

int TRXAPI trx_CreateInd(TRX_NO FAR *ptrx, LPSTR pathName,
	UINT mode, LPVOID fileDef)
{
	int e;
	TRX_pFILE trxp;
	NPSTR fullpath;
	UINT mapIndex;
#define _FDP ((TRX_FILEDEF FAR *)fileDef)

	*ptrx = 0;

	if (((fullpath = dos_FullPath(pathName, trx_ext)) == 0 && (e = TRX_ErrName) != 0) ||
		((mapIndex = _trx_FindFile(fullpath)) == _trx_max_files && (e = TRX_ErrLimit) != 0) ||
		(_trx_fileMap[mapIndex] != 0 && (e = TRX_ErrShare) != 0) ||
		((mode&TRX_ReadOnly) && (e = TRX_ErrReadOnly) != 0)) goto _return_e;

	if (_trx_ErrRange(_FDP->SizNodePtr, 2, 4) ||
		_trx_ErrRange(_FDP->SizNode, TRX_MIN_SIZNODE, TRX_MAX_SIZNODE) ||
		_trx_ErrRange(_FDP->NumTrees, 1, TRX_MAX_NUMTREES) ||
		_FDP->SizExtra > TRX_MAX_SIZEXTRA) {
		e = TRX_ErrArg;
		goto _return_e;
	}

	if ((e = _trx_AllocFile(_FDP)) == 0) {
		_trx_fileMap[mapIndex] = trxp = _trx_pFile;
		trxp->Fd.SizNodePtr = _FDP->SizNodePtr;
		trxp->Fd.SizNode = _FDP->SizNode;
		if ((e = _trx_AllocUsr(ptrx, trxp, mode)) == 0) {
			if (!trx_defCache || (e = trx_AttachCache(*ptrx, trx_defCache)) == 0) {
				if ((e = dos_CreateFile(&trxp->Cf.Handle, fullpath, 0)) == 0) {
					trxp->FileID = (mode&(TRX_UserFlush + TRX_AutoFlush)) ?
						TRX_CLOSED_FILE_ID : TRX_OPENED_FILE_ID;

					/*Finish by writing out the complete header --*/
					if ((e = _trx_TransferHdr(trxp, 1)) == 0) goto _return_e;

					/*If we cannot write the header, close and delete the file --*/
					dos_CloseDelete(trxp->Cf.Handle);
				}
				if (trx_defCache) _csf_DeleteCache((CSF_NO)trxp, (CSH_NO *)&trx_defCache);
			}
			_trx_FreeUsr(_trx_pFileUsr);
			*ptrx = 0;
		}
		_trx_FreeFile(trxp);
	}

_return_e:
	return trx_errno = e;
#undef _FDP
}

/*End of TRXCREAT.C*/
