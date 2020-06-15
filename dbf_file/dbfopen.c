/* DBFOPEN.C -- DBF File Open Function*/

#include <string.h>
#include "a__dbf.h"

/*First 16-bytes of file read initially to check validity --*/
#pragma pack(1)
typedef struct {
	BYTE FileID;
	DBF_HDRDATE Date;
	DWORD NumRecs;
	WORD SizHdr;
	WORD SizRec;
	WORD Res1;
	BYTE Incomplete;
	BYTE Encrypted;
} DBF_HDR16;
#pragma pack()

/*=================================================================*/

#ifdef _DBF2
static void NEAR PASCAL _dbf_from_dbf2(DBF2_HDR *pH2)
{
	UINT numRecs = pH2->wNumRecs;
	UINT sizRec = pH2->SizRec;
	DBF_HDRDATE Date = pH2->Date;
	DBF_HDR16 *pH = (DBF_HDR16 *)pH2;

	pH->Date.Year = Date.Day;
	pH->Date.Month = Date.Year;
	pH->Date.Day = Date.Month;
	pH->NumRecs = numRecs;
	pH->SizHdr = DBF2_SIZHDR;
	pH->SizRec = sizRec;
	pH->Res1 = 0;
	if (pH->FileID & 0x80) {
		pH->Incomplete = TRUE;
		pH->FileID &= 0x7F;
	}
	else pH->Incomplete = FALSE;
	pH->Encrypted = 0;
}

static apfcn_i _dbf2_GetNumFlds(DBF_pFILE dbfp)
{
	int i;
	UINT lenTTL = 1;
	DWORD dwOffset = sizeof(DBF2_HDR);
	DBF2_FILEFLD f;

	for (i = 0; i < DBF2_MAXFLD; i++) {
		if (dos_Transfer(0, dbfp->Cf.Handle, &f, dwOffset, sizeof(DBF2_FILEFLD))) return FORMAT_ERROR();
		if (f.F_Nam[0] == 0x0D) break;
		dwOffset += sizeof(DBF2_FILEFLD);
		lenTTL += f.F_Len;
	}
	if (lenTTL != dbfp->H.SizRec) return FORMAT_ERROR();
	dbfp->NumFlds = i;
	return 0;
}
#endif

apfcn_i _dbf_WriteHdr(DBF_pFILE dbfp)
{
	DBF_HDR *pHdr = &dbfp->H; /*Must work in DLL!*/
	int e;
	int sizHdr = sizeof(DBF_HDR);
#ifdef _DBF2
	DBF2_HDR hdr2;
#endif

	/*Read or write the 32-byte header from/to disk --*/
	dbfp->HdrChg = FALSE;
	dbfp->FileChg = TRUE;
#ifdef _DBF2
	if (pHdr->FileID == DBF2_FILEID) {
		hdr2.FileID = DBF2_FILEID;
		if (pHdr->Incomplete) hdr2.FileID |= 0x80;
		hdr2.wNumRecs = (WORD)pHdr->NumRecs;
		hdr2.Date.Year = pHdr->Date.Month;
		hdr2.Date.Month = pHdr->Date.Day;
		hdr2.Date.Day = pHdr->Date.Year;
		hdr2.SizRec = pHdr->SizRec;
		pHdr = (DBF_HDR *)&hdr2;
		sizHdr = sizeof(DBF2_HDR);
	}
#endif
	e = dos_Transfer(1, dbfp->Cf.Handle, pHdr, 0L, sizHdr);
	if (_PDBFU->U_Mode&DBF_Share) _dbf_LockHdr(dbfp, DOS_UnLock);
	return dbfp->Cf.Errno = dbf_errno = e;
}

apfcn_i _dbf_RefreshHdr(DBF_pFILE dbfp, int typlock)
{
	DBF_HDR hdr;
	int e;

	if (typlock && (e = _dbf_LockHdr(dbfp, typlock))) return e;
	e = dos_Transfer(0, dbfp->Cf.Handle, &hdr, 0L, sizeof(hdr));
	if (!e) {
#ifdef _DBF2
		if ((dbfp->H.FileID & 0x7F) == DBF2_FILEID) {
			_dbf_from_dbf2((DBF2_HDR *)&hdr);
			memcpy(&dbfp->H, &hdr, sizeof(DBF_HDR16));
		}
		else
#endif
			memcpy(&dbfp->H, &hdr, sizeof(DBF_HDR));
		//if(hdr.Incomplete) e=DBF_ErrNotClosed; /*Should be impossible!*/
	}
	if (typlock && (e || typlock != DOS_ExclusiveLock)) _dbf_LockHdr(dbfp, DOS_UnLock);
	return dbf_errno = e;
}

int DBFAPI dbf_Open(DBF_NO *pdbf, LPSTR pathName, UINT mode)
{
	int e, handle;
	DBF_pFILE dbfp;
	DBF_pFILEUSR usrp;
	NPSTR fullpath;
	int mapIndex;
	DBF_HDR16 hdr;
#ifdef _DBF2
	BOOL dbf2_type = FALSE;
#endif

	*pdbf = 0;

	if ((fullpath = dos_FullPath(pathName, dbf_ext)) == 0) {
		e = DBF_ErrName;
		goto _return_e;
	}
	if ((mapIndex = _dbf_FindFile(fullpath)) == DBF_MAX_FILES) {
		e = DBF_ErrLimit;
		goto _return_e;
	}

	/*To simplify flag testing, we can consider read only mode
	  as a type of manual flush mode --*/
	if (mode&DBF_Share) {
		mode &= ~(DBF_UserFlush + DBF_DenyWrite);
		mode |= DBF_AutoFlush;
	}
	else {
		if (mode&DBF_ReadOnly) mode |= DBF_UserFlush;
		if (mode&DBF_UserFlush) mode &= ~DBF_AutoFlush;
	}

	if ((dbfp = _dbf_fileMap[mapIndex]) != 0) {

		/*File is already in use by this process. Check share and flush modes.
		  Also, deny open if a fatal error condition is already set!*/

		  //***Fixed to allow read-only sharing (5/24/06) --
		if (((e = dbfp->Cf.Errno) != 0) || (!(mode&DBF_ReadOnly) && !(mode&DBF_Share) && (e = DBF_ErrShare) != 0))
			goto _return_e;

		for (usrp = dbfp->pFileUsr; usrp; usrp = usrp->U_pNext) {
			if (!(usrp->U_Mode&DBF_ReadOnly) && !(usrp->U_Mode&DBF_Share)
				|| (!(mode&DBF_ReadOnly) && (usrp->U_Mode&DBF_DenyWrite)) ||
				((mode&DBF_DenyWrite) && !(usrp->U_Mode&DBF_ReadOnly))) {
				e = DBF_ErrShare;
				goto _return_e;
			}
		}

		e = _dbf_AllocUsr(pdbf, dbfp, mode);

		goto _return_e;
	} /*File in use*/

	//***Fixed to allow read-only (5/24/06) --
	if ((e = _dos_OpenFile(&handle, fullpath, (mode&DBF_ReadOnly) ? O_RDONLY : O_RDWR,
		mode&(DOS_Sequential + DOS_Share))) != 0) goto _return_e;

	/*Share lock the second byte (first byte already locked) --*/
	if ((mode&DBF_Share) &&
		(e = dos_Lock(DOS_ShareLock, handle, 1, 1))) goto _errc;

	/*Read the first 16 file bytes --*/
	if (dos_Transfer(0, handle, &hdr, 0L, sizeof(hdr))) {
#ifdef _DBF_TEST
		_dbf_errLine = __LINE__; /*FORMAT_ERROR() assumes _dbf_pFile exists*/
#endif
		e = DBF_ErrFormat;
		goto _errc;
	}

	if ((e = (hdr.FileID&DBF_FILEID_MASK)) != DBF_FILEID) {
#ifdef _DBF2
		if (e == DBF2_FILEID) {
			dbf2_type = TRUE;
			_dbf_from_dbf2((DBF2_HDR *)&hdr);
		}
		else
#endif
		{
			e = DBF_ErrFileType;
			goto _errc;
		}
	}

	if (hdr.Incomplete) {
		if ((mode&(DBF_ForceOpen + DBF_Share)) != DBF_ForceOpen) {
			e = DBF_ErrNotClosed;
			goto _errc;
		}
	}

	if ((e = _dbf_AllocFile()) != 0) goto _errc;
	_dbf_fileMap[mapIndex] = dbfp = _dbf_pFile;
	dbfp->Cf.Handle = handle;
	dbfp->Cf.Offset = hdr.SizHdr;

	/*Now read the 32-byte header --*/
#ifdef _DBF2
	if (dbf2_type) {
		memcpy(&dbfp->H, &hdr, sizeof(hdr));
		if ((e = _dbf2_GetNumFlds(dbfp))) goto _errf;
	}
	else
#endif
	{
		if (_dbf_RefreshHdr(dbfp, 0)) {
			e = FORMAT_ERROR();
			goto _errf;
		}
		dbfp->NumFlds = (dbfp->H.SizHdr - sizeof(DBF_HDR) - 1) >> 5;
	}

	/*dbfp->pFldDef initialized only when required*/

	if ((e = _dbf_AllocUsr(pdbf, dbfp, mode)) == 0) {
		if (mode&DBF_Share) {
			_dbf_LockHdr(dbfp, DOS_UnLock);
			/*Now records can be appended*/
			goto _return_e;
		}

		if (!dbf_defCache || (e = dbf_AttachCache(*pdbf, dbf_defCache)) == 0)
			goto _return_e;
		_dbf_FreeUsr(dbfp->pFileUsr);
		*pdbf = 0;
	}

_errf:
	_dbf_FreeFile(dbfp);

_errc:
	dos_CloseFile(handle); /*removes any file locks*/

_return_e:
	return dbf_errno = e;
}
/*End of DBFOPEN.C*/
