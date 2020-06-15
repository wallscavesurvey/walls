/* TRXOPEN.C -- TRX File Open Function*/

#include <a__trx.h>

/*=================================================================*/

int TRXAPI trx_Open(TRX_NO FAR *ptrx, LPSTR pathName, UINT mode)
{
	int e, handle;
	TRX_pFILE trxp;
	TRX_pFILEUSR usrp;
	LPSTR fullpath;
	UINT mapIndex;
	TRX_FILEHDR hdr;

	*ptrx = 0;

	if ((fullpath = dos_FullPath(pathName, trx_ext)) == NULL) {
		e = TRX_ErrName;
		goto _return_e;
	}
	if ((mapIndex = _trx_FindFile(fullpath)) == _trx_max_files) {
		e = TRX_ErrLimit;
		goto _return_e;
	}

	/*To simplify flag testing, we can consider read only mode
	  as a type of manual flush mode --*/

	if (mode&TRX_ReadOnly) mode |= TRX_UserFlush;
	if (mode&TRX_UserFlush) mode &= ~TRX_AutoFlush;

	if ((trxp = _trx_fileMap[mapIndex]) != 0) {

		/*File is already in use. Check share and flush modes. Also,
		  deny open if a fatal error condition is already set!*/

		if ((e = trxp->Cf.Errno) != 0 || ((mode&TRX_Share) == 0 && (e = TRX_ErrShare) != 0))
			goto _return_e;

		for (usrp = trxp->pFileUsr; usrp; usrp = usrp->pNextFileUsr) {
			if ((usrp->UsrMode&TRX_Share) == 0 ||
				((mode&TRX_ReadOnly) == 0 && (usrp->UsrMode&TRX_DenyWrite) != 0) ||
				((mode&TRX_DenyWrite) != 0 && (usrp->UsrMode&TRX_ReadOnly) == 0)) {
				e = TRX_ErrShare;
				goto _return_e;
			}

			/*If we are trying to open the file in a non-flushed writeable
			mode and an existing user already has it in a flushed writeable
			mode, then we will instead open it in auto-flush mode. If this
			would be undesirable, the user should examine the resulting mode
			with trx_FileMode().*/

			if (!(mode&(TRX_UserFlush + TRX_AutoFlush)) &&
				!(usrp->UsrMode&TRX_ReadOnly) &&
				(usrp->UsrMode&(TRX_UserFlush + TRX_AutoFlush))) mode |= TRX_AutoFlush;
		}

		e = _trx_AllocUsr(ptrx, trxp, mode);

		if (!e && (mode&(TRX_UserFlush + TRX_AutoFlush)) && !(mode&TRX_ReadOnly)) {

			/*We are opening the file in a writeable flushed mode. If any
			other user has it open in a writeable non-flushed mode, we must
			change that user's mode to auto-flush! We should also make sure
			that MinPreWrite is positive to guard against the possibility
			that a disk-full condition will corrupt the file.*/

			if (!trxp->MinPreWrite) ++trxp->MinPreWrite;

			for (usrp = trxp->pFileUsr->pNextFileUsr; usrp; usrp = usrp->pNextFileUsr)
				if (!(usrp->UsrMode&(TRX_UserFlush + TRX_AutoFlush)))
					usrp->UsrMode |= TRX_AutoFlush;

			if (trxp->FileID != TRX_CLOSED_FILE_ID) {

				/*The file was previously open in a non-flushed mode. We must
				change its status to flushed --*/

				trxp->FileID = TRX_CLOSED_FILE_ID;
				if (!trxp->HdrChg) _trx_TransferHdr(trxp, 2);
				if ((e = _trx_Flush(trxp)) != 0) {
					/*Note:_trx_Flush() will have reflagged file as "not closed"*/
					_trx_FreeUsr(trxp->pFileUsr);
					*ptrx = 0;
				}
			}
		}
		goto _return_e;
	}

	if ((e = dos_OpenFile(&handle, fullpath, 0)) != 0) goto _return_e;

	/*Read the first 10 file bytes --*/

	if (dos_Transfer(0, handle, &hdr, 0, sizeof(hdr))) {
#ifdef _TRX_TEST
		_trx_errLine = __LINE__; /*FORMAT_ERROR() assumes _trx_pFile exists*/
#endif
		e = TRX_ErrFormat;
		goto _errc;
	}

	if (hdr.Id != TRX_CLOSED_FILE_ID) {
		if (hdr.Id == TRX_OPENED_FILE_ID) {
			if ((mode&(TRX_ForceOpen + TRX_Share)) != TRX_ForceOpen) {
				e = TRX_ErrNotClosed;
				goto _errc;
			}
		}
		else {
			e = TRX_ErrFileType;
			goto _errc;
		}
	}

	if (_trx_ErrRange(hdr.Fd.NumTrees, 1, TRX_MAX_NUMTREES) ||
		hdr.Fd.SizExtra > TRX_MAX_SIZEXTRA) {
#ifdef _TRX_TEST
		_trx_errLine = __LINE__;
#endif
		e = TRX_ErrFormat;
		goto _errc;
	}

	if ((e = _trx_AllocFile(&hdr.Fd)) != 0) goto _errc;
	_trx_fileMap[mapIndex] = trxp = _trx_pFile;
	trxp->Cf.Handle = handle;

	/*Now read the complete header --*/
	if (_trx_TransferHdr(trxp, 0)) {
		e = FORMAT_ERROR();
		goto _errf;
	}

	if ((e = _trx_AllocUsr(ptrx, trxp, mode)) == 0) {
		if (!trx_defCache || (e = trx_AttachCache(*ptrx, trx_defCache)) == 0)
			goto _return_e;
		_trx_FreeUsr(trxp->pFileUsr);
		*ptrx = 0;
	}

_errf:
	_trx_FreeFile(trxp);

_errc:
	dos_CloseFile(handle);

_return_e:
	return trx_errno = e;
}

/*End of TRXOPEN.C*/
