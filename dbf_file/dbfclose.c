/*DBFCLOSE.C -- dbf_Close() and low-level flush finctions --*/

#include <string.h>
#include <stdlib.h>
#include "a__dbf.h"

static BYTE *_dbf_buf = NULL;
static UINT _dbf_buf_size = 0;

static apfcn_i _dbf_PutCshRec(DBF_pFILE dbfp)
{
	DBF_pFILEUSR usrp = _PDBFU;
	CSH_HDRP hp;
	DWORD blkno;

	if ((hp = _csf_GetRecord((CSF_NO)dbfp, blkno = usrp->U_Recoff / dbfp->Cf.Recsiz, 0)) == 0)
		return dbfp->Cf.Errno;
	memcpy(hp->Buffer + (usrp->U_Recoff - (blkno*dbfp->Cf.Recsiz)),
		usrp->U_Rec, dbfp->H.SizRec);
	_csh_MarkHdr(hp);
	return (usrp->U_Mode&DBF_AutoFlush) ? _csh_FlushHdr(hp) : 0;
}

apfcn_i _dbf_LockFilRec(DBF_pFILE dbfp, int typ)
{
	DBF_pFILEUSR usrp = _PDBFU;

	return dos_Lock(typ, dbfp->Cf.Handle, dbfp->Cf.Offset + usrp->U_Recoff,
		dbfp->H.SizRec);
}

static apfcn_i _dbf_PutFilRec(DBF_pFILE dbfp)
{
	DBF_pFILEUSR usrp = _PDBFU;
	int e;

	if (usrp->U_Mode&DBF_Share) {
		/*Allocate compare buffer --*/
		if (_dbf_buf_size < dbfp->H.SizRec &&
			!(_dbf_buf = (BYTE *)realloc(_dbf_buf, _dbf_buf_size = dbfp->H.SizRec))) {
			_dbf_buf_size = 0;
			return DBF_ErrMem;
		}
		/*Lock file record for reading, comparing, and then updating --*/
		if ((e = _dbf_LockFilRec(dbfp, DOS_ExclusiveLock))) return e;

		/*Read and compare with original version --*/
		if ((e = dos_Transfer(0, dbfp->Cf.Handle, _dbf_buf,
			dbfp->Cf.Offset + usrp->U_Recoff, dbfp->H.SizRec)) ||
			(memcmp(_dbf_buf, usrp->U_Rec + dbfp->H.SizRec, dbfp->H.SizRec) &&
			(e = DBF_ErrRecChanged))) goto _unlck;
	}

	if ((e = dos_Transfer(1, dbfp->Cf.Handle, usrp->U_Rec, dbfp->Cf.Offset +
		usrp->U_Recoff, dbfp->H.SizRec))) dbfp->Cf.Errno = e;

_unlck:

	if (usrp->U_Mode&DBF_Share) {
		_dbf_LockFilRec(dbfp, DOS_UnLock);
		if (!e) memcpy(usrp->U_Rec + dbfp->H.SizRec, usrp->U_Rec, dbfp->H.SizRec);
		/*If the record was modified by another user, we refresh this user's
		  record buffer with the current version --*/
		else if (e == DBF_ErrRecChanged) {
			memcpy(usrp->U_Rec, _dbf_buf, dbfp->H.SizRec);
			memcpy(usrp->U_Rec + dbfp->H.SizRec, _dbf_buf, dbfp->H.SizRec);
		}
	}

	return e;
}

apfcn_i _dbf_CommitUsrRec(void)
{
	/*Commits current record to disk if changed*/
	/*Always leaves current record unlocked and marked unchanged*/
	DBF_pFILE dbfp = _PDBFF;
	DBF_pFILEUSR usrp = _PDBFU;
	int e;

	if (!usrp->U_RecChg) return 0;
	if (!(e = dbfp->Cf.Errno)) {
		dbfp->FileChg = TRUE;
		e = (dbfp->Cf.Csh ? _dbf_PutCshRec : _dbf_PutFilRec)(dbfp);
	}
	if (e && e != DBF_ErrRecChanged) usrp->U_Recno = 0L;
	usrp->U_RecChg = 0;
	return dbf_errno = e;
}

apfcn_i _dbf_ErrReadOnly(void)
{
	/*This function is called at the start of any file update operation.
	It checks for read only status. It also makes sure that if no flush
	modes are in effect (user or auto) the disk version of the file
	is marked as "non-closed".*/

	DWORD m = _PDBFU->U_Mode&(DBF_ReadOnly + DBF_AutoFlush + DBF_UserFlush);

	if (m&DBF_ReadOnly) return dbf_errno = DBF_ErrReadOnly;
	if (!m && _PDBFF->H.Incomplete == FALSE) {
		/*This can't happen for shared file --*/
		_PDBFF->H.Incomplete = TRUE;
		return _dbf_WriteHdr(_PDBFF);
	}
	return 0;
}

apfcn_i _dbf_LockHdr(DBF_pFILE dbfp, int typ)
{
	/*Lock or unlock the second file byte on disk --*/
	return dos_Lock(typ, dbfp->Cf.Handle, 1, 1);
}

apfcn_i _dbf_RefreshNumRecs(void)
{
	int e = _dbf_RefreshHdr(_PDBFF, DOS_ShareLock);
	return e ? -e : _PDBFF->H.NumRecs;
}

apfcn_i _dbf_FlushHdr(void)
{
	int e = 0;
	if (_PDBFF->HdrChg) {
		e = _dbf_WriteHdr(_PDBFF);
		_PDBFF->HdrChg = 0;
	}
	return e;
}

apfcn_i _dbf_Flush(void)
{
	DBF_pFILE dbfp = _PDBFF;

	/*Any flush failure is non-recoverable (file is corrupted).
	  _csf_Flush() may set Cf.Errno but returns it in any case. If an
	  error occurs we must try to insure that the file on disk is flagged
	  "non-closed" (if we have not already done so).*/

	if (dbfp->Cf.Csh && (dbf_errno = _csf_Flush((CSF_NO)dbfp)) != 0 && !dbfp->H.Incomplete) {
		dbfp->H.Incomplete = TRUE;
		dbfp->HdrChg = TRUE;
	}
	_dbf_FlushHdr(); /*Can set dbf_errno=Cf.Errno=DBF_ErrWrite*/

	return dbf_errno;
}

static apfcn_i _dbf_AppendEOF(DBF_pFILE dbfp)
{
	/*Append an EOF mark and truncate the file.*/

	DWORD offset = dbfp->Cf.Offset + dbfp->H.NumRecs*dbfp->H.SizRec;
	char eof = DBF_EOFMARK;

	if (dos_Transfer(1, dbfp->Cf.Handle, &eof, offset, 1)) return DBF_ErrSpace;
	dos_Transfer(1, dbfp->Cf.Handle, 0, offset + 1, 0);
	return 0;
}

int DBFAPI dbf_MarkHdr(DBF_NO dbf)
{
	/*Called prior to a header date or reserved area revision --*/
	if (!_GETDBFU) return DBF_ErrArg;
	if ((_DBFU->U_Mode&DBF_ReadOnly) != 0) return DBF_ErrReadOnly;
	_PDBFF->HdrChg = TRUE;
	return 0;
}

int DBFAPI dbf_Close(DBF_NO dbf)
{
	int e;

	if (!_GETDBFP) return DBF_ErrArg;

	/*Even if we are not the last user, a file close still forces
	  a complete header update and flush if we are in a writeable mode --*/

	if ((_PDBFU->U_Mode&DBF_ReadOnly) == 0) {

		/*Commit user's record copy to cache (if updated) --*/
		e = _dbf_CommitUsrRec();

		/*We can restore the file integrity flag if no errors have occurred --*/
		if (!e && _DBFP->H.Incomplete) {
			_DBFP->H.Incomplete = 0;
			_DBFP->FileChg = TRUE;
			_DBFP->HdrChg = TRUE;
		}

		if (!e && _DBFP->FileChg && !(_PDBFU->U_Mode&DBF_NoCloseDate)) {
			if (!(_PDBFU->U_Mode&DBF_Share) || !(e = _dbf_RefreshHdr(_DBFP, DOS_ExclusiveLock))) {
				dos_GetSysDate(&_DBFP->H.Date);
				_DBFP->HdrChg = TRUE;
			}
		}
		/*This shouldn't happen --*/
		else if (_DBFP->HdrChg && (_PDBFU->U_Mode&DBF_Share)) e = _dbf_RefreshHdr(_DBFP, DOS_ExclusiveLock);

		/*Flush cache and/or file header --*/
		if (_dbf_Flush() && !e) e = dbf_errno;

		/*For now, failure to append an EOF mark is not considered fatal
		  to file integrity. It is returned as DBF_ErrSpace.*/

		if (!(_PDBFU->U_Mode&DBF_Share) && _DBFP->FileSizChg && _dbf_AppendEOF(_DBFP) && !e) e = DBF_ErrSpace;
	}
	else e = _DBFP->Cf.Errno;

	if ((_DBFP->pFileUsr)->U_pNext) _dbf_FreeUsr(_PDBFU);
	else {
		/*Close the file since we are the last user*/

		/*This will also free the cache, if it is the default and we are the last file --*/
		if (_csf_DeleteCache((CSF_NO)_DBFP, (CSH_NO *)&dbf_defCache) && !e)
			e = _DBFP->Cf.Errno;

		if (dos_CloseFile(_DBFP->Cf.Handle) && !e) e = DBF_ErrClose;
		_dbf_FreeUsr(_PDBFU);
		_dbf_FreeFile(_DBFP);
	}
	return dbf_errno = e;
}

int DBFAPI dbf_Clear(void)
{
	/*Close all DBF's opened by this process*/
	int usrIndex;
	DBF_pFILEUSR usrp;
	int e = 0;

	for (usrIndex = 0; usrIndex < DBF_MAX_USERS; usrIndex++)
		if ((usrp = _dbf_usrMap[usrIndex])) {
			dbf_Close((DBF_NO)(usrIndex + 1));
			if (dbf_errno) e = dbf_errno;
		}

	return e;
}
