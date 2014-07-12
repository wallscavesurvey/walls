//CShpLayer functions to handle .tmpshp shapefile component
#include "stdafx.h"
#include "resource.h"
#include "WallsMapDoc.h"
#include "ShpLayer.h"
#include "CompareDlg.h"

#define MAX_LOGSTR 63
#define LEN_PFX 13
#define LEN_MAXBAR 91

//=====================================================================
CTRXFile c_Trx,c_Trxref,c_Trxlinks,c_Trxkey;

//Often-used variables local to this file --
static LPCSTR pErrIdx;

enum {N_NONE=0,N_NOTE=1,N_CAUTION=2,N_FLAGGED=3};
static char *notepfx[4]={"","NOTE: ","CAUTION: ","FLAGGED for removal. "};
static LPCSTR pNO="N",pYES="Y";
static char pfx[LEN_PFX+1];
static char bar[LEN_MAXBAR+1];

//Values the same for all shapefiles being examined, initialized in CompareShp()
static VEC_COMPSHP   *pvCompShp;
static CFile flog;
static LPSTR  pKeybuf;        //pounter to keybuf[256] on CompareShp() stack
static LPSTR  pPathbuf;		  //pointer to pathbuf[MAX_PATH+4] on CompareShp() stack
static LPCSTR pPathName;	  //points to CString in CCompareDlg(), used by FixPath(ext) for intitialing pathbuf
static LPCSTR pKeyName;		  //"TSSID", for example
static UINT   nCautions,nNotes,nBadLinks,nGoodLinks;
static WORD   fKey,fLabel,fPrefix,fUpdated,fCreated;
static WORD   fcKey,fcLabel,fcPrefix,fcUpdated,fcCreated;
static WORD   fLat,fLon,fcLat,fcLon;
static WORD	  fDatum,fEast,fNorth,fZone;
static WORD	  fcDatum,fcEast,fcNorth,fcZone;
static int    lenKey,lenPfx;
static bool   bLogFailed;
static UINT	  nLinkedFiles,nMissingFiles,nNewRecs,nRefBlank,nChangedPfx;
static UINT   nNotFound,nFound,nUnchanged,nOutdated,nChanged,nRevised,nRemoved;

static __int64 iSizeLinkedFiles;

//Status of record being scanned --
static bool   bRecHeaderListed;
static bool   bDeleted,bOutdated,bPrefixKeys;

//Info for two records being compared --
static CShpLayer *pShpR,*pShpL,*pShpPfx;
static WORD *pcFld; //index to matching fields in compared chapefile: fR=pcFld[fL];
static CShpDBF *pdbL,*pdbR;
static WORD   recLenL,recLenR;
static WORD   numFldsL,numFldsR;

//Info for two fields being compared --
static LPSTR pBufL,pBufR;
static LPCSTR pfL,pfR;        //field pointers
static LPCSTR pfNam;         // and field name
static DBF_pFLDDEF pfdL,pfdR; //field type info
static LPSTR psL,psR;        //field data strings (can point to csL and csR)
static UINT	fLenL,fLenR;    //field data lengths (after trim)
static char fTypL, fTypR;   //field types

//Allocated space for second memo being compared --
static LPSTR pMemoBuf;
static UINT lenMemoBuf;
static bool bMemosL,bMemosR;

//for link checking --
static LPCSTR pRootPathL,pRootPathR;
static WORD rootPathLenL,rootPathLenR;
static LPCSTR seq_pstrvalL,seq_pstrvalR;
static VEC_CSTR seq_vlinksL,seq_vlinksR;

static int CALLBACK seq_fcnL(int i)
{
	return _stricmp(seq_pstrvalL,seq_vlinksL[i]);
}

static int CALLBACK seq_fcnR(int i)
{
	return _stricmp(seq_pstrvalR,seq_vlinksR[i]);
}

void CShpLayer::GetCompareStats(UINT &nLinks,UINT &nNew,UINT &nFiles,__int64 &sz)
{
	nFiles=nLinkedFiles; sz=iSizeLinkedFiles; nLinks=nGoodLinks; nNew=nNewRecs;
}

LPCSTR CShpLayer::InitRootPath(CString &path,WORD &rootPathLen)
{
		path=PathName();
		int i=path.ReverseFind('\\');
		ASSERT(i>0);
		path.Truncate(i);
		i=path.ReverseFind('\\');
		if(i>=0) path.Truncate(i+1);
		else path+='\\';
		rootPathLen=path.GetLength();
		return path;
}

//=====================================================================

static void clrpfx(int len)
{
	memset(pfx,' ',LEN_PFX);
	pfx[LEN_PFX-len]=0;
}

static bool IsLogOpen()
{
	return flog.m_hFile != CFile::hFileNull;
}

static LPSTR FixPath(LPCSTR pExt)
{
	strcpy(pPathbuf,pPathName);
	strcpy(trx_Stpext(pPathbuf),pExt);
	return pPathbuf;
}

static void __inline writeln()
{
	if(!bLogFailed)	flog.Write("\r\n",2);
}

static void writeln(LPCSTR p)
{
	if(bLogFailed) return;
	ASSERT(IsLogOpen());
	flog.Write(p,strlen(p)); writeln();
}

static int CDECL writelog(const char *format,...)
{
  char buf[1024];
  int len;

  if(bLogFailed) return 0;
  ASSERT(IsLogOpen());

  va_list marker;
  va_start(marker,format);
  //NOTE: Does the 2nd argument include the terminating NULL?
  //      The return value definitely does NOT.
  if((len=_vsnprintf(buf,1023,format,marker))<0) buf[len=1023]=0;
  try {
	flog.Write(buf,len);
  }
  catch(...) {
	  AfxMessageBox("Error writing to log - truncated file closed.");
	  flog.Close();
	  bLogFailed=true;
	  len=0;
  }
  va_end(marker);
  return len;
}

static void writelognote(int typ,const char *format,...)
{
	if(!bRecHeaderListed && !pShpL->ListRecHeader()) return;
	char buf[1024]; buf[1023]=0;
	if(typ==N_CAUTION) nCautions++;
	else if(typ==N_NOTE) nNotes++;
	clrpfx(3);
	if(!writelog("%s*** %s",pfx,notepfx[typ])) return;
	va_list marker;
	va_start(marker,format);
	int len=_vsnprintf(buf,1023,format,marker);
	if(len>0) flog.Write(buf,len);
	writeln();
	va_end(marker);
}

static void barln()
{
	writelog("\r\n%s\r\n",bar);
}

static LPCSTR GetLogStr(LPCSTR s,LPSTR buf)
{
	if(!s) {
		*buf=0;
		return buf;
	}
	if(strlen(s)>MAX_LOGSTR) {
		s=trx_Stncc(buf,s,MAX_LOGSTR+1);
		ASSERT(strlen(buf)==MAX_LOGSTR);
		LPSTR p=buf+MAX_LOGSTR-4;
		if(*p=='\n') p--;
		while(p>=buf && (*p==' ' || *p=='\t')) p--;
		strcpy(p,"...");
	}
	if(s!=buf) s=strcpy(buf,s);
	for(LPSTR p=buf; *p; p++) {
		if(*p=='\r') {
			if(p[1]=='\n') {
				*p++='\\';
				*p='n';
			}
			else *p='.';
		}
		else if(*p=='\n') {
			*p='.';
		}
	}
	return s;
}

bool CShpLayer::OpenCompareLog()
{
	if(bLogFailed) return false;
	CFileException Error;

	if(!flog.Open(FixPath(".txt"),CFile::modeCreate|CFile::modeReadWrite|CFile::shareExclusive,&Error )) {
		TCHAR szError[1024];
		Error.GetErrorMessage(szError, 1024);
		CMsgBox("Aborted: Log %s could not be created: %s",trx_Stpnam(pPathbuf),szError);
		bLogFailed=true;
	    return false;
	}
	CString s,timestr(GetTimeStr(GetLocalFileTime(PathName())));
	timestr.Truncate(10);
	writelog("Compare Log: %s\r\n"
			 "Date: %s\r\n\r\n"
			 "Reference Shapefile: %s (Date: %s, Key Field: %s)\r\n",
 			 trx_Stpnam(pPathbuf),GetLocalTimeStr(),LayerPath(s),(LPCSTR)timestr,pKeyName);

	int lnsize=writelog("Compared with: ");
	*pfx=0;
	for(it_compshp it=pvCompShp->begin();it!=pvCompShp->end();it++) {
		if(lnsize>=100) {
			writeln(); *pfx=0; lnsize=0;
		}
		lnsize+=writelog("%s%s",pfx,it->pShp->LayerPath(s));
		strcpy(pfx,", ");
	}
	writeln();
	return true;
}

static bool __inline isBlank(LPCSTR p,int len)
{
	while(len-- && *p++==' ');
	return len<0;
}

static bool IsWGS84(LPCSTR p)
{
	return !memcmp(p,"WGS84",5) || !memcmp(p,"NAD83",5);
}

bool CShpLayer::ListRecHeader(TRXREC *pTrxrec /*=0*/,bool bCrlf/*=true*/)
{
	CShpLayer *pShp=pTrxrec?pTrxrec->pShp:this;
	CShpDBF &db=*pShp->m_pdb;

	bRecHeaderListed=true;
	if(bLogFailed) return true;

	ASSERT(!pTrxrec || IsLogOpen());
	if(!pTrxrec && !IsLogOpen()) {
		if(!OpenCompareLog()) return false;
		barln();
		UINT n=pvCompShp->size();
		writelog("Records and fields with detected differences are listed below, with reference\r\n"
			  "shapefile values at left and values from the selected shapefiles at right.");
		barln();
	}

	char buf[256];
	LPSTR p=buf+db.GetRecordFld(pTrxrec?pTrxrec->pcFld[fKey]:fKey,buf,20);
	if(p>buf) *p++=' ';
	if(pShp->m_pdbfile->pfxfld) {
		p+=db.GetRecordFld(pShp->m_pdbfile->pfxfld,p,80);
	}
	if(p>buf) {
		*p++=':'; *p++=' ';
	}
	if(!db.GetRecordFld(pShp->m_nLblFld,p,120)) strcpy(p,"<no label>");
	writelog("%s%s\r\n",bCrlf?"\r\n============= ":"",buf);
	return true;
}

void CShpLayer::ListFieldContent(LPCSTR pL,LPCSTR pR)
{
	if(!bRecHeaderListed && !ListRecHeader()) return;
	clrpfx(strlen(pfNam));
    writelog("%s%s|%s| --> |%s|\r\n",pfNam,pfx,pL,pR);
}

static bool ContainedIn(const CString &s,const VEC_CSTR &vec)
{
	for(cit_cstr it=vec.begin();it!=vec.end();it++) {
		if(!it->CompareNoCase(s)) return true;
	}
    return false;
}

static void ListLinks(const VEC_CSTR &v1,const VEC_CSTR &v2,LPCSTR ptyp)
{
	bool bTest=!bDeleted && !bOutdated && *ptyp=='A';
	bool nNew=false;
	int sz2=v2.size();

	if(bTest)
		strcpy(pPathbuf,pRootPathR);

	for(cit_cstr it=v1.begin();it!=v1.end();it++) {
		if(!sz2 || !ContainedIn(*it,v2)) {
			clrpfx(0);
			if(!nNew) {
				writelog("%s|LINKS %s --\r\n",pfx,ptyp);
				nNew=true;
			}
			bool bna=false;
			if(bTest) {
				strcpy(pPathbuf+rootPathLenR,*it);
				if(bna=(_access(pPathbuf,2)!=0)) {
					nBadLinks++;
					clrpfx(8);
				}
				else {
					nGoodLinks++;
				}

				//If !bna The file is is accessible wrt pShpR, but it may not be *currently* accessible wrt the reference.
				//We can still open it and store it in the zip with pathname *it assuming we use pShpR->InitRootPath() --
				{
					CShpLayer *pShp=bna?NULL:pShpR;
					ASSERT(c_Trxlinks.Opened());
					int e=c_Trxlinks.InsertKey(&pShp,trx_Stccp(pKeybuf,*it));
					ASSERT(!e || e==CTRXFile::ErrDup);
					if(!e) {
						if(bna) nMissingFiles++;
						else {
							nLinkedFiles++;
							VERIFY(AddFileSize(pPathbuf,iSizeLinkedFiles));
						}
					}
				}
			}
			writelog("%s%s|%s|\r\n",pfx,bna?"*** N/A ":"",(LPCSTR)*it);
		}
	}
}

static void ListOutOfRange(VEC_CSTR &vLinks)
{
	ASSERT(!vLinks.empty());
	nCautions++;
	clrpfx(4);
	writelog("%s*** |LINKS OUT-OF-RANGE --\r\n",pfx);
	clrpfx(0);
	for(it_cstr it=vLinks.begin();it!=vLinks.end();it++) {
		writelog("%s|%s|%s\r\n",pfx,(LPCSTR)*it,(_access(*it,0))?" N/A":"");
	}
	vLinks.clear();
}

void CShpLayer::ListMemoFields()
{
	if(!bRecHeaderListed && !ListRecHeader()) return;
	clrpfx(strlen(pfNam));

	if(fLenL && fTypL=='M' && CDBTData::IsTextRTF(psL))
		CDBTFile::StripRTF((LPSTR)psL,TRUE);
	if(fLenR && fTypR=='M' && CDBTData::IsTextRTF(psR))
		CDBTFile::StripRTF((LPSTR)psR,TRUE);

	char bufOld[MAX_LOGSTR+1],bufNew[MAX_LOGSTR+1];

	int n=0;
	if(fLenL && fLenR) {
		while(psL[n] && (psL[n]==psR[n])) n++;
		if(n>(3*MAX_LOGSTR)/4) {
			n-=10;
		}
		else n=0;
	}
	LPCSTR dots=n?"...":"";
	writelog("%s%s|%s%s| --> |%s%s|\r\n",pfNam,pfx,dots,GetLogStr(psL+n,bufOld),dots,GetLogStr(psR+n,bufNew));

	SEQ_DATA sData;

	ASSERT(seq_vlinksR.empty() && seq_vlinksL.empty());

	if(fLenL && fTypL=='M') {
		seq_pstrvalL=pRootPathL+rootPathLenL;
		seq_vlinksL.clear();
		sData.Init((TRXFCN_IF)seq_fcnL,(LPVOID *)&seq_pstrvalL);

		GetFieldMemoLinks(psL,pPathbuf,seq_vlinksL,sData,pRootPathL,rootPathLenL); //ignores paths out-of-range paths of reference (for now)
		fLenL=seq_vlinksL.size();
	}
	else fLenL=0;

	int e=0;
	VEC_CSTR vBadLinks;

	if(fLenR && fTypR=='M') {
		seq_pstrvalR=pRootPathR+rootPathLenR;
		seq_vlinksR.clear();
		sData.Init((TRXFCN_IF)seq_fcnR,(LPVOID *)&seq_pstrvalR);

		e=pShpR->GetFieldMemoLinks(psR,pPathbuf,seq_vlinksR,sData,pRootPathR,rootPathLenR,&vBadLinks); //save paths out-of-range of mShpR in vBadlinks
		fLenR=seq_vlinksR.size();
	}
	else fLenR=0;

	if(fLenL) {
		ListLinks(seq_vlinksL,seq_vlinksR,"REMOVED");
	}
	if(fLenR) {
		ListLinks(seq_vlinksR,seq_vlinksL,"ADDED");
	}
	if(e) {
		ListOutOfRange(vBadLinks);
	}
	seq_vlinksR.clear();
	seq_vlinksL.clear();
}

bool CShpLayer::IsNotLocated(double lat,double lon)
{
	bool b=false;
	double latx=m_pdbfile->noloc_lat;
	double lonx=m_pdbfile->noloc_lon;
	switch(m_pdbfile->noloc_dir) {
		case 0 : break;
		case 1 : b=(lat>=latx && lon<=lonx);
			     break;
		case 2 : b=(lat>=latx && lon>=lonx);
			     break;
		case 3 : b=(lat<=latx && lon<=lonx);
			     break;
		case 4 : b=(lat<=latx && lon>=lonx);
			     break;
	}
	return b;
}

static void ListNewTS(WORD f)
{
	CString s;
	pfNam=pdbR->FldNamPtr(f);
	clrpfx(strlen(pfNam));
	pdbR->GetTrimmedFldStr(s,f);
	writelog("%s%s|%s|\r\n",pfNam,pfx,(LPCSTR)s);
}

static void GetUtmStr(CString &sUTM,CFltPoint &fpt,int lenDec)
{
	int iZone=0;
	VERIFY(geo_LatLon2UTM(fpt, &iZone,geo_WGS84_datum));
	sUTM.Format("%.*f %.*f %u%c", lenDec, fpt.x, lenDec, fpt.y, abs(iZone), (iZone<0)?'S':'N');
}

static void GetPrefixKey(CShpDBF *pdb,UINT fKeyToPfx)
{
	int lenKeyToPfx=pdb->FldLen(fKeyToPfx);
	memcpy(pKeybuf+1,pdb->FldPtr(fKeyToPfx),lenKeyToPfx);
	LPSTR p=pKeybuf+lenKeyToPfx; //pts to last field chr
	while(p>pKeybuf && *p==' ') p--;
	//(p==pKeybuf) implies empty (COUNTY) field
	*++p=0;
	_strupr(pKeybuf+1);
	*pKeybuf=(BYTE)(p-pKeybuf); //at least 1
}

static void ShowLocError(double errDist)
{
	CString sPfx;
	pdbR->GetTrimmedFldStr(sPfx,fcPrefix);
	LPCSTR p=pdbR->FldNamPtr(fcPrefix);
	
	if(errDist>=pShpL->m_pdbfile->wErrBoundary) {
		if(errDist==DBL_MAX) 
			writelognote(N_CAUTION, "The %s value, \"%s\" was not found in the prefix shapefile, preventing new key assignemt.",
			   p, (LPCSTR)sPfx);
		else {
			CString s(p+1);
			s.MakeLower();
			writelognote(N_CAUTION, "Location is outside the boundary of %s %c%s. Distance: %s m",
			   (LPCSTR)sPfx, *p, (LPCSTR)s, CommaSep(pKeybuf,(__int64)errDist));
		}
	}
	else if(errDist<-0.5) {
		writelognote(N_CAUTION, "Updated coordinates are in the valid range but field %s contains \"%s.\"",
		   pdbR->FldNamPtr(fcPrefix), (LPCSTR)sPfx);
	}
}

bool CShpLayer::CompareLocs(bool bChangedPfx)
{
	CString sLocL,sLocR;
	CFltPoint fptL=m_fpt[pdbL->Position()-1];
	CFltPoint fptR=pShpR->m_fpt[pdbR->Position()-1];

	bool bNolocL=IsNotLocated(fptL);
	bool bNolocR=IsNotLocated(fptR);
	bool bLocChanged=true;

	if(bNolocL==bNolocR) {
		if(bNolocL || (fptL.y==fptR.y && fptL.x==fptR.x) || (fabs(fptR.y-fptL.y)<0.000001 && fabs(fptR.x-fptL.x)<0.000001))
			bLocChanged=false; //close enough?
	}

	double errDist=0.0;
	if(bPrefixKeys && !bNolocR && (bLocChanged || bChangedPfx)) {
		//Check if location is in county
		DWORD recNo=0;
		GetPrefixKey(pdbR, fcPrefix);
		if(!c_Trxkey.Find(pKeybuf) && !c_Trxkey.GetRec(&recNo)) {
			if(recNo) pShpPfx->ShpPolyTest(fptR, recNo, &errDist);
			else errDist=-1.0; //County is "Unknown" while location is in nrange!
		}
		else errDist=DBL_MAX;
	}

	if(!bLocChanged && errDist==0.0) return false;

	//coordinates are different, so list coordinate pairs --
	if(!bRecHeaderListed && !ListRecHeader()) return false;

	int lenDec=fLat?pdbL->FldDec(fLat):6;

	if(!bNolocL) {
		sLocL.Format("%.*f %.*f", lenDec, fptL.y, lenDec, fptL.x);
	}
	if(!bNolocR) {
		sLocR.Format("%.*f %.*f", lenDec, fptR.y, lenDec, fptR.x);
	}

    clrpfx(8);
	writelog("LAT/LONG%s|%s| --> |%s|\r\n",pfx,(LPCSTR)sLocL,(LPCSTR)sLocR);

	if(bLocChanged) {
		sLocL.Empty(); sLocR.Empty();
		if(!bNolocL) {
			GetUtmStr(sLocL,fptL,2);
		}
		if(!bNolocR) {
			GetUtmStr(sLocR,fptR,2);
		}
		CString sdif;
		if(!bNolocL && !bNolocR ) {
			double de=fptR.x-fptL.x;
			double dn=fptR.y-fptL.y;
			double az=atan2(de,dn)*(180.0/3.141592653589793);
			if(az<0) az+=360.0;
			if(fabs(az-360.0)<=0.5) az=0.0;
			dn=sqrt(de*de+dn*dn);
			sdif.Format(" (%.2f m, %.0f deg)",dn,az);
			if(dn>50) {
				sdif+=(dn>1000)?"!!!":((dn>250)?"!!":"!");
			}
		}
		clrpfx(7);
		writelog("UTM E/N%s|%s| --> |%s|%s\r\n",pfx,(LPCSTR)sLocL,(LPCSTR)sLocR,(LPCSTR)sdif);
	}

	if(errDist!=0.0) ShowLocError(errDist);
	return bLocChanged;
}

int CShpLayer::CheckTS(WORD f,WORD fc,bool bRevised)
{
	if(!fc) return 0;
	int e=memcmp(pdbL->FldPtr(f), pdbR->FldPtr(fc),LEN_TIMESTAMP);
	if(e || bRevised) {
		pdbL->GetRecordFld(f,pBufL,80);
		pdbR->GetRecordFld(fc,pBufR,80);
		pfNam=m_pdb->FldNamPtr(f);
		ListFieldContent(pBufL,pBufR);
	}
	return e;
}

static LPSTR GetSortKey(CShpLayer *pShp,WORD fLbl,WORD fPfx)
{
	BYTE flen=0;
	LPSTR p=pKeybuf+1;
	if(fPfx) {
		flen=pShp->m_pdb->GetRecordFld(fPfx,p,80)+1;
		p+=flen; //keep trailing null as separator
	}
	ASSERT(fLbl==pShp->m_nLblFld);
	flen+=pShp->m_pdb->GetRecordFld(fLbl,p,120); //label field of pShp
	*pKeybuf=flen;
	return pKeybuf;
}

//*************************************************************************************************
//*************************************************************************************************

static void GetTrimmedMemo(LPSTR *pps, UINT *pfLen, CShpLayer *pShp, UINT dbtrec)
{
	UINT len=*pfLen;
	LPSTR p=pShp->m_pdbfile->dbt.GetText(&len,dbtrec);
	ASSERT(p);
	while(len && p[len-1]==' ') len--;
	p[len]=0;
	*pps=p;
	*pfLen=len;
}

static void GetFieldStr(LPSTR *pps, UINT *pfLen, CShpLayer *pShp, LPCSTR pf, DBF_pFLDDEF pfd, LPSTR pBuf)
{
	static LPCSTR nullstr="";

	if(pfd->F_Typ=='M') {
		UINT rec=CDBTFile::RecNo(pf);
		if(!rec) {
			*pps=(LPSTR)nullstr;
			*pfLen=0;
			return;
		}
		if(fLenL && fTypL=='M' && pShp!=pShpL) {
			//CDBTFile has only a static buffer, so move data from psL (the likely smaller one)
			//to a separate dynamically allocated buffer --
			if(fLenL>=lenMemoBuf) {
				free(pMemoBuf);
				pMemoBuf=(LPSTR)malloc(lenMemoBuf=(fLenL/512+1)*512);
			}
			psL=(LPSTR)memcpy(pMemoBuf,psL,fLenL+1);
		}
		*pfLen=0;
		GetTrimmedMemo(pps, pfLen, pShp, rec);
	}
	else {
		if(pfd->F_Typ=='L') {
			ASSERT(pfd->F_Len==1);
			*pBuf=(*pf=='Y'||*pf=='T')?'Y':'N';
			pBuf[1]=0;
		}
		else {
			CString s(pf,pfd->F_Len);
			if(pfd->F_Typ=='N' || pfd->F_Typ=='F') {
				s.TrimLeft();
			}
			else s.TrimRight();
			strcpy(pBuf,s);
		}
		*pps=pBuf;
		*pfLen=strlen(pBuf);
	}
}

static void InitR(TRXREC &trxrec)
{
	pShpR=trxrec.pShp;
	pdbR=pShpR->m_pdb;
	bMemosR=pdbR->HasMemos();
	recLenR=pdbR->SizRec();
	fcLabel=pShpR->m_nLblFld;
	pcFld=trxrec.pcFld;
	fcPrefix=fPrefix?pcFld[fPrefix]:0;
	fcLat=fLat?pcFld[fLat]:0;
	fcLon=fLon?pcFld[fLon]:0;
	fcEast=fEast?pcFld[fEast]:0;
	fcNorth=fNorth?pcFld[fNorth]:0;
	fcZone=fZone?pcFld[fZone]:0;
	fcDatum=fDatum?pcFld[fDatum]:0;
	fcUpdated=fUpdated?pcFld[fUpdated]:0;
	fcCreated=fCreated?pcFld[fCreated]:0;
}

int CShpLayer::CompareUpdates(BOOL bIncludeAll)
{
	//Perform comparison by scanning reference shapefile using index --

	ASSERT(!IsLogOpen());
	nCautions=nNotes=nBadLinks=nLinkedFiles=nMissingFiles=nGoodLinks=0;
    iSizeLinkedFiles=0;
	nNotFound=nFound=nUnchanged=nOutdated=nChanged=nRevised=nRemoved=0;

	pShpR=NULL;
	TRXREC trxrec;
	CString pathR;

	int e=c_Trxref.First();
	while(!e) {
		DWORD r;
		VERIFY(!c_Trxref.GetRec(&r,sizeof(DWORD)));
		if(e=m_pdb->Go(r)) {
			ASSERT(0);
			break;
		}
		*pKeybuf=(BYTE)lenKey;
		memcpy(pKeybuf+1,m_pdb->FldPtr(fKey),lenKey);
		if(isBlank((LPCSTR)pKeybuf+1,lenKey) || c_Trx.Find(pKeybuf)) {
			nNotFound++;
		}
		else {
			nFound++;
			VERIFY(!c_Trx.GetRec(&trxrec,sizeof(TRXREC)));
			if(pShpR!=trxrec.pShp) {
				InitR(trxrec);
				if(bMemosR)
					pRootPathR=pShpR->InitRootPath(pathR, rootPathLenR);
			}

			if(trxrec.rec&0xE0000000) {
				//same record accessed earlier in loop, implying duplicate key in reference!
				sprintf(pPathbuf, "Aborted: Key %s in reference is duplicated!",trx_Stxpc(pKeybuf));
				pErrIdx=pPathbuf;
				return -1;
			}

			if(e=pShpR->m_pdb->Go(trxrec.rec)) {
				ASSERT(0);
				pErrIdx="Aborted: Unexpected indexing error.";
				return -1;
			}

			if(!bMemosL && !bMemosR && pdbL->FieldsMatch(pdbR) && !memcmp(pdbL->RecPtr(),pdbR->RecPtr(),recLenL)) {
			    trxrec.rec|=0x80000000; //flag as found only
				nUnchanged++;
				goto _putRec;
			}

			int iUpdated=0;
			//Used when testing memo file links --
			bDeleted=*pdbR->FldPtr(fcLabel)=='*';
			bOutdated=fUpdated && fcUpdated && (iUpdated=memcmp(pdbR->FldPtr(fcUpdated),pdbL->FldPtr(fUpdated),LEN_TIMESTAMP))<=0;

			if(bOutdated) {
			    //record has the same or older timestamp than the reference timestamp --
				if (!CCompareDlg::bLogOutdated) {
					if(iUpdated<0) nOutdated++;
					trxrec.rec |= 0x80000000; //flag as found only
					goto _putRec;
				}
			}

			bool bChangedPfx=false;
			UINT nFieldsListed=0;

			bRecHeaderListed=false;

			//Scan all but location and timestamp fields --
			for(WORD f=1; f<=numFldsL; f++) {

				WORD fR=pcFld[f];
				if(!fR || f==fKey || f==fLat || f==fLon || f==fEast || f==fNorth ||
					f==fZone || f==fDatum || f==fUpdated || f==fCreated) continue;

				pfdL=pdbL->FldDef(f);
				pfdR=pdbR->FldDef(fR);
				fTypL=pfdL->F_Typ;
				fTypR=pfdR->F_Typ;
				pfL=(LPSTR)pdbL->FldPtr(f);
				pfR=(LPSTR)pdbR->FldPtr(fR);

				//Get trimmed text version of each version
				fLenL=0; //leave left memo in dbt buffer
				GetFieldStr(&psL,&fLenL,pShpL,pfL,pfdL,pBufL);
				//if fLen>0 move left memo to allocated buffer and use dbt buffer for right memo (if nec)
				GetFieldStr(&psR,&fLenR,pShpR,pfR,pfdR,pBufR);

				if(fLenR==fLenL && !memcmp(psL,psR,fLenL)) continue;

				pfNam=(LPCSTR)m_pdb->FldNamPtr(f);

				//list differences --
				if(pfdL->F_Typ=='M' || pfdR->F_Typ=='M') ListMemoFields();
				else {
					ListFieldContent(psL, psR);
					if(f==fPrefix && !bOutdated && !bDeleted && bPrefixKeys) {
					   nChangedPfx++;
					   bChangedPfx=true;
					   writelognote(N_NOTE,"Changing the %s value will cause a new key to be assigned.",pdbL->FldNamPtr(fPrefix));
					}
				}
				nFieldsListed++;
			}

			if(!bDeleted && CompareLocs(bChangedPfx))
				nFieldsListed++;

			int iCreated=CheckTS(fCreated,fcCreated,false);
			iUpdated=CheckTS(fUpdated,fcUpdated,nFieldsListed!=0);
			
			if(iCreated) {
				nFieldsListed++;
				writelognote(N_CAUTION,"The CREATED timestamps should match!"); 
			}

			if(bOutdated) {
				ASSERT(iUpdated>=0);
				if(nFieldsListed) {
					nOutdated++;
					writelognote(N_CAUTION,"The compared record has an UPDATED time %s that of the reference and will be ignored (not included).",
					   iUpdated?"older than":"identical to");
				}
				else nUnchanged++;
				//We'll not archive the record
				trxrec.rec |= 0x80000000; //flag as found only
				goto _putRec;
			}

			if(bDeleted) {
				writelognote(N_FLAGGED,"The %s field has an asterisk prefix.",pdbR->FldNamPtr(fcLabel));
				if(*pdbL->FldPtr(fLabel)=='*')
					writelognote(N_CAUTION,"The reference's label field also has an asterisk prefix!");
				nRemoved++;
				trxrec.rec|=0x40000000;
				if(bIncludeAll) trxrec.rec|=0x20000000; //if updating reference, skip this record
				goto _putRec;
			}

			if(nFieldsListed) {
				nChanged++;
				nRevised++;
				trxrec.rec|=0x40000000; //add modified record to exported shapefile
			}
			else {
				nUnchanged++;
				trxrec.rec|=0x80000000; //found only
			}
_putRec:
			VERIFY(!c_Trx.PutRec(&trxrec));
		}
		e=c_Trxref.Next();
	}
	if(e==CTRXFile::ErrEOF) e=0;
	return e;
}

int CShpLayer::CompareNewRecs()
{
	//Process new records --
	CString sTemp,pathR;
	TRXREC trxrec;
	char buf[MAX_LOGSTR+1];

	c_Trx.UseTree(1);
	ASSERT(c_Trx.NumRecs()==nNewRecs);

	pShpR=NULL;
	bOutdated=bDeleted=false;

	int e=c_Trx.First();

	while(!e) {
		VERIFY(!c_Trx.Get(&trxrec,pKeybuf));
		bool bKeyConflict=!c_Trxref.Find(pKeybuf);

		if(pShpR!=trxrec.pShp) {
			InitR(trxrec);
			if(bMemosR)
				pRootPathR=pShpR->InitRootPath(pathR, rootPathLenR);
		}

		if(e=pdbR->Go(trxrec.rec)) {
			ASSERT(0);
			e=-1;
			goto _fail;
		}

		if(!ListRecHeader(&trxrec,true)) {
			e=-1;
			goto _fail;
		}

		fLenL=0; //GetFieldStr will not use pMemoBuf

		//Scan all but location and timestamp fields --
		for(UINT f=1;f<=numFldsL;f++) {
			WORD fR=pcFld[f];
			if(!fR || f==fKey || f==fLat || f==fLon || f==fEast || f==fNorth || f==fZone || f==fDatum || f==fCreated || f==fUpdated)
				continue;

			pfNam=pdbL->FldNamPtr(f);
			clrpfx(strlen(pfNam));

			pfdR=pdbR->FldDef(fR);
			fTypR=pfdR->F_Typ;
			fLenR=pfdR->F_Len;
			pfR=(LPCSTR)pdbR->FldPtr(fR);

			UINT lenMemo=0;
			psR=NULL;

			if(fTypR=='M') {
				UINT recR=CDBTFile::RecNo(pfR);
				if(!recR) continue;

				psR=pShpR->m_pdbfile->dbt.GetText(&lenMemo,recR);
				while(lenMemo && psR[lenMemo-1]==' ') lenMemo--;
				psR[lenMemo]=0;

				if(lenMemo && CDBTData::IsTextRTF(psR))
					CDBTFile::StripRTF(psR,TRUE);

				pfR=GetLogStr(psR,buf);
			}
			else {
				if(fTypR=='L') {
					if(!(*pfR=='Y' || *pfR=='T')) continue;
					pfR=pYES;
				}
				else {
					if(isBlank(pfR,fLenR)) continue;
					sTemp.SetString(pfR,fLenR);
					if(fTypR=='N' || fTypR=='F') {
						sTemp.TrimLeft();
					}
					else sTemp.TrimRight();
					if(sTemp.IsEmpty()) continue;
					pfR=sTemp;
				}
			}
			writelog("%s%s|%s|\r\n",pfNam,pfx,pfR);

			if(lenMemo) {
				SEQ_DATA sData;

				ASSERT(seq_vlinksR.empty() && seq_vlinksL.empty());

				seq_vlinksL.clear(); //use for bad paths
				seq_vlinksR.clear();
				seq_pstrvalR=pRootPathR+rootPathLenR;
				sData.Init((TRXFCN_IF)seq_fcnR,(LPVOID *)&seq_pstrvalR);

				e=pShpR->GetFieldMemoLinks(psR,pPathbuf,seq_vlinksR,sData,pRootPathR,rootPathLenR,&seq_vlinksL);

				if(seq_vlinksR.size()) {
					ListLinks(seq_vlinksR,seq_vlinksL,"ADDED");
				}
				if(e) {
					ListOutOfRange(seq_vlinksL);
				}
				seq_vlinksR.clear();
			}
		}

		//List location data last --
		{
			clrpfx(8);
			CFltPoint fptR=pShpR->m_fpt[pdbR->Position()-1];

			if(IsNotLocated(fptR)) {
				writelog("LAT/LONG%s||\r\n", pfx);
			}
			else {
				int lenDec=pdbL->FldDec(fLat);
				writelog("LAT/LONG%s|%.*f %.*f|\r\n",pfx, lenDec, fptR.y, lenDec, fptR.x);
				GetUtmStr(sTemp,CFltPoint(fptR),pdbL->FldDec(fEast));
				clrpfx(7);
				writelog("UTM E/N%s|%s|\r\n",pfx,(LPCSTR)sTemp);
				if(bPrefixKeys) {
					//Check if location is in county
					double errDist=0.0;
					DWORD recNo=0;
					GetPrefixKey(pdbR, fcPrefix);
					if(!c_Trxkey.Find(pKeybuf) && !c_Trxkey.GetRec(&recNo)) {
						if(recNo) pShpPfx->ShpPolyTest(fptR, recNo, &errDist);
						else errDist=-1.0; //County is "Unknown" while location is in range!
					}
					else errDist=DBL_MAX;
					if(errDist!=0.0) {
						ShowLocError(errDist);
						trxrec.rec|=0x80000000;
						c_Trx.PutRec(&trxrec);
					}
				}
			}
		}
		if(fcCreated) {
			ListNewTS(fcCreated);
		}
		if(fcUpdated) {
			ListNewTS(fcUpdated);
		}

		if(*pdbR->FldPtr(fcLabel)=='*') {
			writelognote(N_CAUTION,"The label field (%s) of a new record shouldn't have an asterisk prefix.",pdbR->FldNamPtr(fcLabel));
		}

		if(bKeyConflict) {
			//County|Name found in c_Trxref index
			DWORD rec;
			c_Trxref.GetRec(&rec);
			if(!m_pdb->Go(rec)) {
				sTemp.SetString((LPCSTR)m_pdb->FldPtr(fKey),lenKey);
				sTemp.Trim(); //could be empty!
				writelognote(N_CAUTION,"The %s%s%s field value%s that of a record in the reference: %s.",
					fPrefix?m_pdb->FldNamPtr(fPrefix):"",
					fPrefix?" and ":"",
					m_pdb->FldNamPtr(fLabel),
					fPrefix?"s match":" matches",
					(LPCSTR)sTemp);
			}
			else {
				ASSERT(0);
			}
		}
		e=c_Trx.Next();
	}

	if(e==CTRXFile::ErrEOF) e=0;

_fail:
	c_Trx.UseTree(0);
	return e;
}

int CShpLayer::CompareShp(CString &csPathName, CString &summary, VEC_COMPSHP &vCompShp, WORD *pwFld, VEC_WORD &vFldsIgnored, BOOL bIncludeAll)
{
	char keybuf[256],pathbuf[MAX_PATH+4]; //declare some buffers on stack to access globally in CompareShp --
	char bufL[260],bufR[260];
	CString sTemp,pathL,pathR;
	VEC_DWORD v_noKey;

	int e; //fcn returns

	//Values for the reference file used for listing, etc --
	pvCompShp=&vCompShp;
	pShpL=this;
	pShpPfx=NULL;
	pPathbuf=pathbuf;
	pKeybuf=keybuf;
	pBufL=bufL; pBufR=bufR;
	pdbL=m_pdb;
	pKeyName=(LPCSTR)pdbL->FldNamPtr(KeyFld());
	fKey=pdbL->FldNum(pKeyName);
	lenKey=pdbL->FldLen(fKey);
	lenPfx=-1;
	fLabel=m_nLblFld;
	fPrefix=m_pdbfile->pfxfld;
	numFldsL=pdbL->NumFlds();
	recLenL=pdbL->SizRec();
	numFldsL=pdbL->NumFlds();
	bLogFailed=false;
	nRefBlank=nChangedPfx=0;
	bPrefixKeys=m_pKeyGenerator && strstr(m_pKeyGenerator,"::");

	pErrIdx="Compare aborted: Failure creating temporary index.";

	//Create index with two trees: one for existing recs (TRXREC,key=TSSID) and
	//one for new recs(TRXREC,key=PREFIX|LABEL) --
	if(!GetTempFilePathWithExtension(sTemp,"trx") || c_Trx.Create(sTemp,sizeof(TRXREC),0,2) || c_Trx.AllocCache(64,TRUE)) {
		goto _errExit;
	}
	if(bPrefixKeys) {
		if(InitTrxkey(&pErrIdx)) {
		   goto _errExit;
		}
		ASSERT(lenPfx>0);
	}

	c_Trx.UseTree(1);
	c_Trx.InitTree(sizeof(TRXREC));
	c_Trx.UseTree(0);
	c_Trx.SetUnique();
	c_Trx.SetExact();

	UINT trxRecs=0,wFldOff=0;
	nNewRecs=0;
	TRXREC trxrec;
	pcFld=pwFld-1;

	for(it_compshp it=pvCompShp->begin();it!=pvCompShp->end();it++,pcFld+=numFldsL) {
		ASSERT(!it->bSel);
		pShpR=trxrec.pShp=it->pShp;
		pdbR=pShpR->m_pdb;
		trxrec.pcFld=pcFld;
		for(WORD f=1; f<=numFldsL; f++) {
			pcFld[f]=(WORD)pdbR->FldNum(m_pdb->FldNamPtr(f));
		}
		if(!vFldsIgnored.empty()) {
			for(it_word it=vFldsIgnored.begin(); it!=vFldsIgnored.end(); it++)
				pcFld[*it]=0;
		}

		fcPrefix=pcFld[fPrefix];
		ASSERT(fcPrefix || !bPrefixKeys);
		fcKey=pcFld[fKey];
		fcLabel=pShpR->m_nLblFld;
		ASSERT(fcKey && fcLabel);
		DWORD nrecs=pdbR->NumRecs();

		for(DWORD r=1;r<=nrecs;r++) {
			if(!pdbR->Go(r) && *pdbR->RecPtr()!='*') {
				ASSERT(!pShpR->IsPtRecDeleted(r));
				memcpy(keybuf+1,pdbR->FldPtr(fcKey),lenKey); //length will be same
				trxrec.rec=r;
				if(isBlank(keybuf+1,lenKey)) {
					//Add key for new record -- key is COUNTY|NAME
					c_Trx.UseTree(1);
					if(c_Trx.InsertKey(&trxrec,GetSortKey(pShpR,fcLabel,fcPrefix))) {
						goto _errExit;
					}
					c_Trx.UseTree(0);
					nNewRecs++;
				}
				else {
					//Add key for existing record -- key is TSSID
					*keybuf=lenKey;
				    e=c_Trx.InsertKey(&trxrec,keybuf);
					if(e) {
						if(e==CTRXFile::ErrDup) {
							TRXREC tr;
							VERIFY(!c_Trx.GetRec(&tr,sizeof(TRXREC)));
							LPCSTR p="";
							sTemp=tr.pShp->LayerPath(pathL);
							if(tr.pShp!=pShpR) {
								p="s";
								sTemp+='\n';
								sTemp+=pShpR->LayerPath(pathL);
							}
							pathR.Format("Compare aborted - A duplicated key value, \"%s\", was found in the following shapefile%s:\n\n"
								"%s\n\nThe selected set must collectively have unique (or empty) key values.",
								trx_Stxpc(keybuf),p,(LPCSTR)sTemp);
							pErrIdx=pathR;
						}
						goto _errExit;
					}
					trxRecs++;
				}
			}
		}
	}

	ASSERT(trxRecs==c_Trx.NumRecs());

	//Create index for traversing reference --
	if(!GetTempFilePathWithExtension(sTemp,"trx") || c_Trxref.Create(sTemp,sizeof(DWORD))) {
		goto _errExit;
	}
	c_Trxref.SetExact(TRUE); //for checking county names in new records

	for(DWORD r=1;r<=m_pdb->NumRecs();r++) {
		if(pdbL->Go(r)) goto _errExit;
		if(*pdbL->RecPtr()=='*') {
			ASSERT(IsPtRecDeleted(r));
			continue;
		}
		if(isBlank((LPCSTR)m_pdb->FldPtr(fKey),lenKey)) {
			nRefBlank++;
			v_noKey.push_back(r);
		}
		if(c_Trxref.InsertKey(&r,GetSortKey(this,fLabel,fPrefix))) {
			goto _errExit;
		}
	}

	UINT trxrefRecs=c_Trxref.NumRecs();

	if(!trxrefRecs) {
		pErrIdx="Compare aborted: Reference contains no records with keys.";
		goto _errExit;
	}

	if(!GetTempFilePathWithExtension(sTemp,"trx") || c_Trxlinks.Create(sTemp,sizeof(CShpLayer *))) {
		goto _errExit;
	}
	c_Trxlinks.SetUnique(TRUE);

	pErrIdx="Compare aborted: Failure writing to log.";

	//Initialize globals --
	pPathName=csPathName;
	memset(bar,'=',LEN_MAXBAR);

	fDatum=m_pdb->FldNum("DATUM_");
	fLat=m_pdb->FldNum("LATITUDE_");
	fLon=m_pdb->FldNum("LONGITUDE_");
	fEast=m_pdb->FldNum("EASTING_");
	fNorth=m_pdb->FldNum("NORTHING_");
	fZone=m_pdb->FldNum("ZONE_");
	fUpdated=m_pdb->FldNum("UPDATED_");
	if(fUpdated && m_pdb->FldLen(fUpdated)<LEN_TIMESTAMP) fUpdated=0;
	fCreated=m_pdb->FldNum("CREATED_");
	if(fCreated && m_pdb->FldLen(fCreated)<LEN_TIMESTAMP) fCreated=0;

	bMemosL=m_pdb->HasMemos();
	pMemoBuf=NULL;
	lenMemoBuf=0;

	if(bMemosL) {
		//Set up pRootPath and compute rootPathLen for root path checking --
		pRootPathL=InitRootPath(pathL,rootPathLenL);
	}

	//==============================================================================
	if(CompareUpdates(bIncludeAll)) goto _errExit;
	//==============================================================================

	ASSERT(nNotFound+nFound==trxrefRecs);

	free(pMemoBuf);
	pMemoBuf=NULL; //might as well

	if(!nNewRecs) goto _getOrphans;

	if(!bLogFailed) {
		if(!IsLogOpen() && !OpenCompareLog()) goto _errExit;
		barln();
		writelog("The %u new record%s found in the selected set %s listed below with empty fields omitted.",
			nNewRecs,(nNewRecs>1)?"s":"",(nNewRecs>1)?"are":"is");
		barln();
	}

	//==============================================================================
	if(CompareNewRecs()) goto _errExit;;
	//==============================================================================

//Get orphan count --
_getOrphans:

	ASSERT(c_Trx.GetTree()==0);
	#define MAX_ORPHANS_LISTED 200
	UINT nOrphans=0;
	e=c_Trx.First();
	while(!e) {
		VERIFY(!c_Trx.GetRec(&trxrec,sizeof(TRXREC)));
		if((trxrec.rec&~0x1FFFFFFF)==0) {
			if(!nOrphans) {
				nCautions++;
				if(!IsLogOpen() && !OpenCompareLog()) break;
				barln();
				writeln("*** CAUTION: The following records have non-blank keys not found in the reference:\r\n");
			}
			if(nOrphans<=MAX_ORPHANS_LISTED && !trxrec.pShp->m_pdb->Go(trxrec.rec&0x3FFFFFFF)) {
				ListRecHeader(&trxrec,false); //no crlf prefix
			}
			nOrphans++;
		}
		e=c_Trx.Next();
	}
	if(nOrphans>MAX_ORPHANS_LISTED)
		writelog("(%u more)\r\n",nOrphans-MAX_ORPHANS_LISTED);

	if(nRefBlank) {
		nCautions++;
		if(IsLogOpen() || OpenCompareLog()) {
			barln();
			writeln("*** CAUTION: The following records in the reference have blank keys:\r\n");
			for(it_dword it=v_noKey.begin(); it!=v_noKey.end(); it++) {
				if(pdbL->Go(*it)) goto _errExit;
				LPCSTR p=GetSortKey(this,fLabel,fPrefix)+1;
				if(fPrefix) {
					writelog("%s: ", p);
					p+=strlen(p)+1;
				}
				writeln(p);
			}
		}
	}

	if(seq_vlinksR.size()) {
		VEC_CSTR().swap(seq_vlinksR);
	}
	if(seq_vlinksL.size()) {
		VEC_CSTR().swap(seq_vlinksL);
	}
	ASSERT(!bLogFailed);

    UINT nOutRecs=nRevised+nNewRecs+nRemoved;
	
	e=(IsLogOpen() && !bLogFailed)?0x40000000:0;

	if(e) {
		ASSERT(c_Trxlinks.NumRecs()==nLinkedFiles+nMissingFiles);
		if(nLinkedFiles+nMissingFiles) { 
			barln();
			writelog("These new links to local files were found in memo fields:\r\n",nLinkedFiles,nMissingFiles+nLinkedFiles);
			if(nLinkedFiles) {
				writeln("Accessible files --");
				for(int i=c_Trxlinks.First();!i;i=c_Trxlinks.Next()) {
					if(!c_Trxlinks.GetRec(&trxrec,sizeof(TRXREC)) && trxrec.pShp && !c_Trxlinks.GetKey(keybuf)) {
						writelog("%s\r\n",trx_Stxpc(keybuf));
					}
				}
			}
			if(nMissingFiles) { 
				writeln("*** Inaccessible files (links broken) --");
				for(int i=c_Trxlinks.First();!i;i=c_Trxlinks.Next()) {
					if(!c_Trxlinks.GetRec(&trxrec,sizeof(TRXREC)) && !trxrec.pShp && !c_Trxlinks.GetKey(keybuf)) {
						writelog("%s\r\n",trx_Stxpc(keybuf));
					}
				}
			}
		}
		barln();
		clrpfx(4); //(PFX_LEN-9) = (13-9)
		writelog("TOTALS - Reference records: %u, Records in selected set: %u, Records compared: %u\r\n"
				        "%sUnchanged: %u, Revised: %u, Outdated: %u, New: %u, Flagged for removal: %u\r\n",
			trxrefRecs,trxRecs+nNewRecs,nFound,
			pfx,nUnchanged,nRevised,nOutdated,nNewRecs,nRemoved);

		if(nOutRecs) {
			 e|=1;
			 writelog("%sUpdated shapefile records: %u (revised, new, or removed)\r\n",pfx,nOutRecs);
		}

		if(nLinkedFiles|nMissingFiles) {
			 if(nLinkedFiles) e|=2;
			 writelog("%sNew linked files: %u (%u accessible)\r\n",pfx,(nLinkedFiles+nMissingFiles),nLinkedFiles);
		}

		flog.Close();

		sTemp.Empty();

		if(nLinkedFiles|nMissingFiles) {
			 sTemp.Format("New linked files: %u (%u accessible)\n",(nLinkedFiles+nMissingFiles),nLinkedFiles);
		}

		if(nOrphans) {
			pathR.Format("\n%u records have keys missing from the reference!\n",nOrphans);
			sTemp+=pathR;
		}

		if(nRefBlank) {
			pathR.Format("%s%u records in the reference have blank keys!%s\n",nOrphans?"":"\n",nRefBlank,
			             bIncludeAll?" Key assignment disabled.":"");
			sTemp+=pathR;
		}
		
		pathR.Format("\nA log was generated. Cautions: %u, Noted items: %u\n",nCautions,nNotes);
		sTemp+=pathR;
	}
	else sTemp="\nNo log was created.";

	if(pvCompShp->size()>1)
		pathR.Format("%u shapefiles",pvCompShp->size());
	else
		pathR=(*pvCompShp)[0].pShp->Title();

	memset(keybuf,' ',80); keybuf[80]=0;

	summary.Format(
		"Comparison of reference %s with %s:\n\n"
		"Record Totals --%s\n"
        "Reference: %u, Selected set: %u, Compared: %u\n"
		"Revised: %u, Outdated: %u, New: %u, Removed: %u\n%s",
		LayerPath(pathL),(LPCSTR)pathR,keybuf,trxrefRecs,trxRecs+nNewRecs,nFound,
		nRevised,nOutdated,nNewRecs,nRemoved,(LPCSTR)sTemp);

	return e;

_errExit:
	if(IsLogOpen()) flog.Close();
	AfxMessageBox(pErrIdx);

	if(seq_vlinksR.size()) {
		VEC_CSTR().swap(seq_vlinksR);
	}
	if(seq_vlinksL.size()) {
		VEC_CSTR().swap(seq_vlinksL);
	}
	return -1;
}

int CShpLayer::ExportUpdateShp(LPSTR pathBuf,CString &pathName,int iFlags)
{
	int nRecs=0;
	TRXREC trxrec;
	DWORD rec;
	char keyBuf[256]={lenKey};

	//First get record count and extent
	CFltRect extent(DBL_MAX,-DBL_MAX,-DBL_MAX,DBL_MAX);

	c_Trx.UseTree(0);

	for(int e=c_Trxref.First(); !e; e=c_Trxref.Next()) {
		VERIFY(!c_Trxref.GetRec(&rec,sizeof(DWORD)));
		VERIFY(!m_pdb->Go(rec));
		memcpy(keyBuf+1,m_pdb->FldPtr(fKey),lenKey);

		if(isBlank((LPCSTR)keyBuf+1,lenKey) || c_Trx.Find(keyBuf)) {
			if(!(iFlags&4)) continue;
			trxrec.pShp=this;
			trxrec.rec=rec;
		}
		else {
			VERIFY(!c_Trx.GetRec(&trxrec, sizeof(TRXREC)));
			if(trxrec.rec&0x20000000) {
				//remove from reference
				ASSERT(iFlags&4);
				continue;
			}

			if(!(trxrec.rec&0x40000000)) {
			    if(!(iFlags&4)) continue;
				trxrec.pShp=this;
				trxrec.rec=rec;
			}
			ASSERT(trxrec.rec&0x1FFFFFFF);
		}
		nRecs++;
		CFltPoint fpt=trxrec.pShp->m_fpt[(trxrec.rec&0x1FFFFFFF)-1];
		if(IsNotLocated(fpt)) {
			fpt.y=m_pdbfile->noloc_lat;
			fpt.x=m_pdbfile->noloc_lon;
		}
		UpdateExtent(extent,fpt);
	}

	c_Trx.UseTree(1);
	for(int i=c_Trx.First();!i;i=c_Trx.Next()) {
		VERIFY(!c_Trx.GetRec(&trxrec,sizeof(TRXREC)));
		//ASSERT(!(trxrec.rec&~0x1FFFFFFF));
		nRecs++;
		UpdateExtent(extent,trxrec.pShp->m_fpt[(trxrec.rec&0x1FFFFFFF)-1]);
	}
	c_Trx.UseTree(0);

	CSafeMirrorFile cfShp,cfShpx;
	SHP_MAIN_HDR hdr;

	InitShpHdr(hdr,SHP_POINT,nRecs*sizeof(SHP_POINT_REC),extent,NULL);

	if(!FileCreate(cfShp,pathBuf,pathName,SHP_EXT))
		goto _fail;


	if(!FileCreate(cfShpx,pathBuf,pathName,SHP_EXT_SHX)) {
		cfShp.Abort();
		goto _fail;
	}

	try {
		cfShp.Write(&hdr,sizeof(hdr));
		hdr.fileLength=FlipBytes(4*nRecs+sizeof(SHP_MAIN_HDR)/2);
		cfShpx.Write(&hdr,sizeof(hdr));
		SHX_REC shxRec;
		long offset=100;
		UINT nOutRecs=0;
		SHP_POINT_REC shpRec;
		shpRec.shpType=SHP_POINT;
		shpRec.length=shxRec.reclen=FlipBytes((sizeof(SHP_POINT_REC)-8)/2); //content length in words

		c_Trx.UseTree(0);

		for(int e=c_Trxref.First(); !e; e=c_Trxref.Next()) {
			VERIFY(!c_Trxref.GetRec(&rec,sizeof(DWORD)));
			VERIFY(!m_pdb->Go(rec));
			memcpy(keyBuf+1,m_pdb->FldPtr(fKey),lenKey);

			if(isBlank((LPCSTR)keyBuf+1,lenKey) || c_Trx.Find(keyBuf)) {
				if(!(iFlags&4)) continue;
				trxrec.pShp=this;
				trxrec.rec=rec;
			}
			else {
				VERIFY(!c_Trx.GetRec(&trxrec, sizeof(TRXREC)));
				if(trxrec.rec&0x20000000) continue;
				if(!(trxrec.rec&0x40000000)) {
					if(!(iFlags&4)) continue;
					trxrec.pShp=this;
					trxrec.rec=rec;
				}
				ASSERT(trxrec.rec&0x1FFFFFFF);
			}
			shxRec.offset=FlipBytes(offset/2);
			cfShpx.Write(&shxRec,sizeof(shxRec));
			shpRec.recNumber=FlipBytes(nOutRecs+1);
			shpRec.fpt=trxrec.pShp->m_fpt[(trxrec.rec&0x1FFFFFFF)-1];
			if(IsNotLocated(shpRec.fpt)) {
				shpRec.fpt.y=m_pdbfile->noloc_lat;
				shpRec.fpt.x=m_pdbfile->noloc_lon;
			}
			cfShp.Write(&shpRec,sizeof(SHP_POINT_REC));
			offset+=sizeof(SHP_POINT_REC);
			nOutRecs++;
		}

		c_Trx.UseTree(1);
		for(int i=c_Trx.First();!i;i=c_Trx.Next()) {
			VERIFY(!c_Trx.GetRec(&trxrec,sizeof(TRXREC)));
			shxRec.offset=FlipBytes(offset/2);
			cfShpx.Write(&shxRec,sizeof(shxRec));
			shpRec.recNumber=FlipBytes(nOutRecs+1);
			shpRec.fpt=trxrec.pShp->m_fpt[(trxrec.rec&0x1FFFFFFF)-1];
			if(IsNotLocated(shpRec.fpt)) {
				shpRec.fpt.y=m_pdbfile->noloc_lat;
				shpRec.fpt.x=m_pdbfile->noloc_lon;
			}
			cfShp.Write(&shpRec,sizeof(SHP_POINT_REC));
			offset+=sizeof(SHP_POINT_REC);
			nOutRecs++;
		}
		c_Trx.UseTree(0);

		ASSERT(nRecs==nOutRecs);

		ASSERT((UINT)cfShp.GetPosition()==100+nRecs*sizeof(SHP_POINT_REC));
		cfShp.Close();
		ASSERT((UINT)cfShpx.GetPosition()==100+8*nRecs);
		cfShpx.Close();
	}
	catch(...) {
		//Write failure --
		cfShp.Abort();
		cfShpx.Abort();
		goto _fail;
	}

	return nRecs;

_fail:
	AfxMessageBox("Export aborted - Could not create SHP/SHX components");
    return -1;
}

static void __inline FillNumericFld(double d, DBF_FLDDEF &fd, void *pFld)
{
	VERIFY(fd.F_Len==_snprintf((LPSTR)pFld,fd.F_Len,"%*.*f",fd.F_Len,fd.F_Dec,d));
}

static BOOL __inline LocationsDiffer(CFltPoint &fpt1, CFltPoint &fpt2)
{
	return fabs(fpt1.x-fpt2.x)>0.00001 || fabs(fpt1.x-fpt2.x)>0.00001;
}

int CShpLayer::AppendUpdateRec(CShpDBF &db,CDBTFile *pdbt,TRXREC &trxrec,int iFlags)
{
	bool bNew=c_Trx.GetTree()==1;

	if(!bNew) {
		if(db.AppendRec(m_pdb->RecPtr())) return -1;
		//will need to copy memo fields if not replaced with versions in source record
	}
	else if(db.AppendBlank()) return -1;
		
	CShpLayer *pShp=trxrec.pShp;
	CFltPoint fpt=pShp->m_fpt[(trxrec.rec&0x1FFFFFFF)-1];

	WORD *pcFld=trxrec.pcFld;
	CShpDBF &srcDb=*pShp->m_pdb;
	CString csSrc;
	LPSTR pSrc;
	LPSTR pDst;
	UINT dbtrec;

	bool bInit=pShp!=this && (iFlags&4)!=0 && m_pdbfile->pvxc_ignore; //may need to generate new values
	int iLocDif=-1;

	for(UINT fD=1; fD<=db.NumFlds(); fD++) {
		if(fD==fLat || fD==fLon || fD==fEast || fD==fNorth || fD==fZone)
			continue;

		if(fD==fKey) {
			//key already copied
			ASSERT(bNew || !memcmp(db.FldPtr(fD),srcDb.FldPtr(pcFld?pcFld[fD]:fD),lenKey));
			continue;
		}

		DBF_FLDDEF &dstFd=db.m_pFld[fD-1];
		pDst=(LPSTR)db.FldPtr(fD);

		if(!pcFld || !pcFld[fD]) {
			//no copy from update required for this field, but a reference memo will need need fixing,
			//or the field may need initialization --
			if(bInit) {
				bool bInitialized=false;
				for(it_vec_xcombo it=m_pdbfile->pvxc_ignore->begin(); it!=m_pdbfile->pvxc_ignore->end();it++) {
					if(it->nFld==fD) {
						if((it->wFlgs&XC_INITPOLY)) {
							if(IsNotLocated(fpt)) {
								bInitialized=true;
								memset(pDst,' ',dstFd.F_Len);
								break;
							}
							if(!bNew && (!iLocDif || iLocDif<0 && !(iLocDif=LocationsDiffer(fpt, m_fpt[m_pdb->Position()-1]))))
							    break; //fix if memo field
						}
						CString s;
						LPCSTR p=XC_GetInitStr(s, *it, fpt);
						if(p && *p) {
							VERIFY(XC_PutInitStr(p,&dstFd,(LPBYTE)pDst,pdbt));
						}
						else memset(pDst,' ',dstFd.F_Len);
						bInitialized=true;
						break;
					}
				}
				if(bInitialized) continue;
			}

		    //copy memo field from reference, or skip --
			if(!bNew && dstFd.F_Typ=='M' && (dbtrec=CDBTFile::RecNo((LPCSTR)m_pdb->FldPtr(fD)))) {
				if((dbtrec=pdbt->AppendCopy(m_pdbfile->dbt,dbtrec))==(UINT)-1)
					return -1;
				CDBTFile::SetRecNo((LPSTR)pDst,dbtrec);
			}
			//Copied an unrevised field --
			continue;
		}

		UINT fS=pcFld[fD];
		DBF_FLDDEF &srcFd=srcDb.m_pFld[fS-1];
		pSrc=(LPSTR)srcDb.FldPtr(fS);

		ASSERT(!strcmp(dstFd.F_Nam,srcFd.F_Nam));

		memset(pDst,' ',dstFd.F_Len);
		if(isBlank(pSrc,srcFd.F_Len))
			continue;

		if(srcFd.F_Typ=='M') {
			if(!(dbtrec=CDBTFile::RecNo(pSrc)))
				continue;

			if(dstFd.F_Typ=='M') {
				if((dbtrec=pdbt->AppendCopy(pShp->m_pdbfile->dbt,dbtrec))==(UINT)-1)
					return -1;
				CDBTFile::SetRecNo((LPSTR)pDst,dbtrec);
			}
			else if(dstFd.F_Typ=='C') {
				UINT len=dstFd.F_Len;
				GetTrimmedMemo(&pSrc, &len, pShp, dbtrec);
				if(len) {
					memcpy(pDst,pSrc,len);
				}
			}
			//no conversions from 'M' to other types
		}
		else if(srcFd.F_Typ==dstFd.F_Typ && srcFd.F_Len==dstFd.F_Len && srcFd.F_Dec==dstFd.F_Dec) {
			memcpy(pDst,pSrc,dstFd.F_Len);
		}
		else {
			//field types or formats don't match --
			csSrc.SetString(pSrc,srcFd.F_Len);
			csSrc.Trim();
			UINT len=csSrc.GetLength();
			ASSERT(len);
			if(!len) continue; //sanity chk

			switch(dstFd.F_Typ) {
				case 'C':
					if(len>dstFd.F_Len) len=dstFd.F_Len;
					memcpy(pDst, csSrc, len);
					break;

				case 'M':
					if((dbtrec=pdbt->PutTextField(csSrc, len))==(UINT)-1)
						return -1;
					ASSERT(dbtrec);
					if(dbtrec) CDBTFile::SetRecNo(pDst, dbtrec);
					break;

			    case 'N':
				   StoreDouble(atof(csSrc),dstFd.F_Len,dstFd.F_Dec,(LPBYTE)pDst);
				   break;

			    case 'F':
				   StoreExponential(atof(csSrc),dstFd.F_Len,dstFd.F_Dec,(LPBYTE)pDst);
				   break;

				default: break; //No conversions to 'L' and 'D' from a different type
			}
		}
	}

	//Normalize coordinates -- for now, done for all records, including if pShp==this
	if(fLat && fLon) {
		CFltPoint fpt(pShp->m_fpt[srcDb.Position()-1]);
		if(IsNotLocated(fpt)) {
			fpt.y=m_pdbfile->noloc_lat;
			fpt.x=m_pdbfile->noloc_lon;
		}
		FillNumericFld(fpt.y, db.m_pFld[fLat-1], db.FldPtr(fLat));
		FillNumericFld(fpt.x, db.m_pFld[fLon-1], db.FldPtr(fLon));
		if(fEast && fNorth && fZone) {
			int iZone=0;
			VERIFY(geo_LatLon2UTM(fpt, &iZone, geo_WGS84_datum));
			FillNumericFld(fpt.y, db.m_pFld[fNorth-1], db.FldPtr(fNorth));
			FillNumericFld(fpt.x, db.m_pFld[fEast-1], db.FldPtr(fEast));
			int fZoneLen=db.m_pFld[fZone-1].F_Len;
			LPBYTE p=(LPBYTE)db.FldPtr(fZone);
			memset(p,' ', fZoneLen);
			csSrc.Format("%u%c",abs(iZone),(iZone<0)?'S':'N');
			if(csSrc.GetLength()<=fZoneLen) memcpy(p,csSrc,csSrc.GetLength());
		}
	}
	return 0;
}

int CShpLayer::InitTrxkey(LPCSTR *ppMsg)
{
	ASSERT(fPrefix && (!m_pUnkName)==(!m_pUnkPfx));

	UINT nFldKey,nFldPfx;
	pShpPfx=m_pDoc->FindShpLayer(m_pKeyGenerator,&nFldKey,&nFldPfx);
	if(!pShpPfx || pShpPfx->m_pdb->FldLen(nFldPfx)+4 >lenKey) {
		*ppMsg="Key prefix shapefile is missing or doesn't qualify.";
		return -1;
	}
	ASSERT(nFldKey && nFldPfx);
	CShpDBF *pdb=pShpPfx->m_pdb;
	int nLenKey=pdb->FldLen(nFldKey);
	lenPfx=pdb->FldLen(nFldPfx);

	if(m_pUnkPfx && (lenPfx!=strlen(m_pUnkPfx))) {
		*ppMsg="Key prefix length in template doesn't match length in prefix shapefile.";
		return -1;
	}

	CString sTemp;
	if(!GetTempFilePathWithExtension(sTemp,"trx") || c_Trxkey.Create(sTemp,sizeof(DWORD))) {
		goto _fail0;
	}
	c_Trxkey.SetUnique();

	DWORD count=0;
	
	if(m_pUnkPfx) {
		//First, insert |pUnkName|0|pUnkPfx|
		int len=strlen(m_pUnkName);
		_strupr(strcpy(pKeybuf+1,m_pUnkName));
		memcpy(pKeybuf+len+2,m_pUnkPfx,lenPfx);
		*pKeybuf=(BYTE)len+lenPfx+1;
		if(c_Trxkey.InsertKey(&count, pKeybuf)) goto _fail0;
	}

	count++;
	int e=pdb->First();
	for(;!e; e=pdb->Next(),count++) {
		if(pdb->IsDeleted()) continue;
		memcpy(pKeybuf+1,pdb->FldPtr(nFldKey),nLenKey);
		LPSTR p=pKeybuf+nLenKey; //pts to last field chr
		while(p>pKeybuf && *p==' ') p--;
		if(p==pKeybuf) {
			*ppMsg="Prefix shapefile has a blank key.";
			goto _fail;
	    }
		*++p=0;
		_strupr(pKeybuf+1);
		memcpy(p+1,pdb->FldPtr(nFldPfx),lenPfx);
		*pKeybuf=(BYTE)((p-pKeybuf)+lenPfx);

		if(c_Trxkey.InsertKey(&count, pKeybuf)) {
			if(c_Trxkey.Errno()==CTRXFile::ErrDup) {
				*ppMsg="Prefix shapefile has a duplicate key.";
				goto _fail;
			}
		    goto _fail0;
		}
	}

	if(e==CShpDBF::ErrEOF) {
		return 0;
	}
	
_fail0:
	*ppMsg="Failure creating temorary index.";

_fail:
	c_Trxkey.CloseDel();
	return -1;
}

int CShpLayer::ResetTrxkey(int *pnLenPfx)
{

	if(!strstr(m_pKeyGenerator,"::")) {
		*pnLenPfx=strlen(m_pKeyGenerator);
		return 0;
	}

	if(!bPrefixKeys || (!nChangedPfx && !nNewRecs)) {
		*pnLenPfx=-1; //no key generation
		return 0;
	}

	ASSERT(fPrefix && (!m_pUnkName)==(!m_pUnkPfx));
	ASSERT(c_Trxkey.Opened());

	DWORD count=0;
	int e=c_Trxkey.First();
	while(!e) {
		if(c_Trxkey.PutRec(&count)) break;
		e=c_Trxkey.Next();
	}

	if(e==CTRXFile::ErrEOF) {
		*pnLenPfx=lenPfx;
		return fPrefix;
	}
	
	c_Trxkey.CloseDel();
	return -1;
}

int CShpLayer::ExportUpdateDbf(LPSTR pathBuf,CString &pathName,int iFlags)
{
	CShpDBF db;
	CDBTFile dbt;
	CDBTFile *pdbt=NULL;
	LPCSTR pMsg=NULL;
	TRXREC trxrec;
	DWORD rec;

	int fKeyToPfx=0;
	__int64 fNextKeyNum=0;
	CString sTemp;
	LPCSTR pFmt=0;

	char keybuf[256];
	pKeybuf=keybuf;
	UINT lenCnt;

	ASSERT(nNewRecs==c_Trx.NumKeys(1));
	
	int nOutRecs=nNewRecs+(iFlags&4)?m_pdb->NumRecs():c_Trx.NumKeys();

	//Create DBF,DBT components --

	UINT nFlds=m_pdb->NumFlds();
	UINT sizRec=m_pdb->SizRec();

	LPSTR pExt=trx_Stpext(strcpy(pathBuf,pathName));

	if(HasMemos()) {
		strcpy(pExt,SHP_EXT_DBT);
		if(!dbt.Create(pathBuf)) {
			goto _failDBT;
		}
		pdbt=&dbt;
	}

	strcpy(pExt,SHP_EXT_DBF);
	if(db.Create(pathBuf,nFlds,m_pdb->m_pFld,nOutRecs))
		goto _failDBF;

	nOutRecs=0;

	if((iFlags&4) && !nRefBlank && m_pKeyGenerator) {

		if((fKeyToPfx=ResetTrxkey(&lenPfx))<0) { //sets records of c_Trxkey tree to 0
			pMsg="Failure accessing temporary index.";
			goto _fail;
		}

		if(lenPfx>=0) {
			// new keys will need to be generated --
			lenCnt=lenKey-lenPfx;
			if(!fKeyToPfx) {
				ASSERT(lenCnt>=4);
				if(lenPfx>0 || m_pdb->FldTyp(fKey)=='C') pFmt="%s%0*I64u";
				else pFmt="%s%*I64u";
				fNextKeyNum=1;
			}
		}
	}
	else lenPfx=-1;

	//Process revised records --
	c_Trx.UseTree(0);

#ifdef _DEBUG
	UINT nCounties=0; //count counties with fearures
#endif

	for(int e=c_Trxref.First(); !e; e=c_Trxref.Next()) {
		VERIFY(!c_Trxref.GetRec(&rec,sizeof(DWORD)));
		VERIFY(!m_pdb->Go(rec));

		*keybuf=lenKey;
		memcpy(keybuf+1,m_pdb->FldPtr(fKey),lenKey);
		keybuf[lenKey+1]=0;

		bool bBlankRefKey=isBlank((LPCSTR)keybuf+1,lenKey);

		if(bBlankRefKey || c_Trx.Find(keybuf)) {
			//key not found in revised set --
			if(!(iFlags&4)) continue;
			//updating reference so copy reference record while updating memo fields --
			trxrec.pShp=this;
			trxrec.rec=m_pdb->Position();
			trxrec.pcFld=NULL;
			//no need to initialize ignored fields found in m_pdbfile->pvxc_ignore --
			if(AppendUpdateRec(db, pdbt, trxrec,0))
				goto _failDBF;
			nOutRecs++;
			if(bBlankRefKey) continue;
			goto _keyIdx;
		}

		VERIFY(!c_Trx.GetRec(&trxrec, sizeof(TRXREC)));
		if(trxrec.rec&0x20000000) continue; //removed
		if(!(trxrec.rec&0x40000000)) {
			if(!(iFlags&4)) continue;
		}
		ASSERT(trxrec.rec&0x1FFFFFFF);

		if(trxrec.pShp->m_pdb->Go(trxrec.rec&0x1FFFFFFF)) {
			goto _failDBF;
		}

		fcLabel=trxrec.pShp->m_nLblFld;
		fcPrefix=fPrefix?trxrec.pcFld[fPrefix]:0;
		ASSERT(!fKeyToPfx || fcPrefix);

		if(fKeyToPfx && fcPrefix) {
			ASSERT(fPrefix==fKeyToPfx);
		    //If keys that access the prefix shapefile don't match, we'll need to create a new record --
			CString sn;
			trxrec.pShp->m_pdb->GetTrimmedFldStr(sn,fcPrefix);
			m_pdb->GetTrimmedFldStr(sTemp,fPrefix);
			if(sn.CompareNoCase(sTemp)) {
				trxrec.rec|=0x20000000; //remove from edited set
				c_Trx.PutRec(&trxrec);
				c_Trx.UseTree(1);
				//add to new records --
				if(c_Trx.InsertKey(&trxrec, GetSortKey(trxrec.pShp, fcLabel, fcPrefix))) {
					pMsg="Indexing error";
					goto _fail;
				}
				c_Trx.UseTree(0);
				continue;
			}
		}

		//This is a revised record, initialize ignored fields if location changes, but only if updating reference
		if(AppendUpdateRec(db, pdbt, trxrec, iFlags))
			goto _failDBF;
		nOutRecs++;

_keyIdx:
		//record has been copied --
		if(lenPfx>=0) {
			//Update counter for matched key --
			if(fKeyToPfx) {
				//prefixes depend on COUNTY field, so obtain associated counter from index record
				DWORD oldCnt,newCnt=atoi(keybuf+lenPfx+1);
				GetPrefixKey(&db,fPrefix); //if changed from m_pdf (reference), will need new key
				if(c_Trxkey.Find(keybuf)) {
					goto _failKey;
				}
				VERIFY(!c_Trxkey.GetRec(&oldCnt));
				if(newCnt>oldCnt) {
#ifdef _DEBUG
					if(!oldCnt) nCounties++;
#endif
					VERIFY(!c_Trxkey.PutRec(&newCnt));
				}
			}
			else if(lenPfx>=0) {
				__int64 f=_atoi64(keybuf+lenPfx+1);
				if(f>=fNextKeyNum) fNextKeyNum=f+1;
			}
		}
	}

	c_Trx.UseTree(1);

	for(int i=c_Trx.First();!i;i=c_Trx.Next()) {
		VERIFY(!c_Trx.GetRec(&trxrec,sizeof(TRXREC)));
		if(trxrec.pShp->m_pdb->Go(trxrec.rec&0x1FFFFFFF))
			goto _failDBF;

		//This is a new record, initialize ignored fields if location exists, but only if updating reference
		if(AppendUpdateRec(db, pdbt, trxrec, iFlags))
			goto _failDBF;

		nOutRecs++;

		if(trxrec.rec&0x80000000) {
			continue;
		}

		//fcLabel=trxrec.pShp->m_nLblFld;

		if(fKeyToPfx) {
			//fcPrefix=trxrec.pcFld[fPrefix];
			//ASSERT(fcPrefix);
			DWORD count;

			GetPrefixKey(&db, fPrefix);

			if(c_Trxkey.Find(keybuf)) {
				goto _failKey;
			}

			VERIFY(!c_Trxkey.Get(&count, keybuf));
			++count;
			VERIFY(!c_Trxkey.PutRec(&count));

			LPSTR p=keybuf+1;
			p+=strlen(p)+1; //points to key prefix

			if(_snprintf(p+lenPfx,lenCnt,"%0*u",lenCnt,count) != lenCnt)
				goto _failKeylen;
			memcpy(db.FldPtr(fKey),p,lenKey);
		}
		else if(lenPfx>=0) {
		    int e;
			if((e=_snprintf(keybuf,lenKey,pFmt,m_pKeyGenerator,lenKey-lenPfx,(__int64)fNextKeyNum))!=lenKey)
				goto _failKeylen;
			memcpy(db.FldPtr(fKey),keybuf,lenKey);
		}
	}

	c_Trx.UseTree(0);

	if(pdbt) {
		if(!dbt.FlushHdr()) goto _failDBT;
		dbt.Close();
	}

	ASSERT(nOutRecs==db.NumRecs());

	if(db.Close()) goto _failDBF;

	return nOutRecs;

_failKey:
	db.GetRecordFld(fPrefix,keybuf,100);
	db.GetTrimmedFldStr(sTemp,fLabel);
	CMsgBox("Key prefix for %s: %s not found in prefix shapefile.",keybuf,(LPCSTR)sTemp);
	goto _fail0;

_failKeylen:
	db.GetRecordFld(m_nLblFld,keybuf,256);
	CMsgBox("Key capacity exceeded for new record: %s.", keybuf);
	goto _fail0;

_failDBF:
	pMsg="Error writing DBF component";
	goto _fail;
_failDBT:
	pMsg="Error writing DBT component";
_fail:
	AfxMessageBox(pMsg);
_fail0:
	db.CloseDel();
	if(pdbt) dbt.CloseDel();
	return -1;
}