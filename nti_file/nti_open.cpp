#include <nti_file.h>
#include <trx_str.h>
#include <sys/stat.h>
#include <assert.h>

apfcn_i nti_Open(NTI_NO *pnti,LPCSTR pszPathName,BOOL bReadOnly)
{
	//if srcTime!=0, require that srcTime matches SrcTime in file header
	int e;
	NTI_NO pNTI=NULL;
	NTI_HDR hdr;
	NTI_LVL lvl;
	int szFullHdr;
	CSF_XFRFCN xfrFcn=NULL;

	*pnti=NULL;

	LPSTR pathName=_nti_getPathname(pszPathName);

	if(!pathName) return nti_errno;

	int handle;
	if(e=_dos_OpenFile(&handle,pathName,bReadOnly?O_RDONLY:O_RDWR,bReadOnly?DOS_Share:DOS_Exclusive)) {
		return nti_errno=e;
	}

	if(!(e=dos_Transfer(0,handle,&hdr,0L,SZ_NTI_HDR))) {

		if(hdr.Version>NTI_VERSION) {
			e=NTI_ErrVersion;
			goto _fail;
		}
		if(e=(hdr.fType&NTI_FCNMASK)) {
			if(!bReadOnly) e|=NTI_FCNENCODE;
			if(!(xfrFcn=nti_XfrFcn(e))) {
				e=NTI_ErrCompressTyp;
				goto _fail;
			}
			e=0;
		}
		_nti_InitLvl(&lvl,hdr.nWidth,hdr.nHeight,hdr.nColors,hdr.nBands);
		szFullHdr=SZ_NTI_HDR+hdr.nSzExtra+hdr.nColors*sizeof(RGBQUAD);
		if(xfrFcn) szFullHdr+=lvl.nRecTotal*sizeof(DWORD); /*record index*/

		if(!(pNTI=(NTI_NO)dos_Alloc(sizeof(NTI_FILE_PFX)+szFullHdr))) e=DOS_ErrMem;
		else {
			memcpy(&pNTI->Lvl,&lvl,sizeof(lvl));
			memcpy(&pNTI->H,&hdr,SZ_NTI_HDR);
			e=dos_Transfer(0,handle,pNTI->Pal,SZ_NTI_HDR,szFullHdr-SZ_NTI_HDR);
		}
	}

	if(!e) {
		pNTI->Cf.Recsiz=NTI_BLKSIZE;
		pNTI->Cf.Offset=szFullHdr;
		pNTI->Cf.Handle=handle;
		pNTI->Cf.XfrFcn=xfrFcn;
		pNTI->pszExtra=(char *)(pNTI->Pal+pNTI->H.nColors);
		pNTI->pRecIndex=(DWORD *)(pNTI->pszExtra+pNTI->H.nSzExtra);
	}

	//if(!e && nti_defCache) e=nti_AttachCache(pNTI,nti_defCache); 
	
	if(!e && bReadOnly) {
		struct __stat64 status;
		if(_stat64(pathName,&status)) e=NTI_ErrFind; //impossible at this stage
		else {
			assert(status.st_size<0x100000000 || *pNTI->H.dwOffsetRec);
			__int64 szData=status.st_size-szFullHdr;
			if(szData!=nti_SizeEncoded(pNTI)) {
				__int64 szLvl0=nti_SizeEncodedLvl(pNTI,0);
				e=(szLvl0<=0 || szData<szLvl0)?NTI_ErrFormat:NTI_ErrNoOverviews;
			}
		}
	}

_fail:
	if(e) {
		dos_Free(pNTI);
		dos_CloseFile(handle);
	}
	else *pnti=pNTI; 
	return nti_errno=e;
}
