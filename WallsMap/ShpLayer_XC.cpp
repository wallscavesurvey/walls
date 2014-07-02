//CShpLayer functions to handle .tmpshp shapefile component
#include "stdafx.h"
#include "resource.h"
#include "WallsMapDoc.h"
#include "ShowShapeDlg.h"
#include "NtiLayer.h"
#include "MsgCheck.h"
#include "shpdef.h"

int CShpLayer::XC_ParseDefinition(XCOMBODATA &xc,DBF_FLDDEF &Fld,LPCSTR p0, BOOL bDynamic /*=0*/)
{
	//static fcn
	LPCSTR p,pc;
	char ctyp=Fld.F_Typ;
	//first,look for combo list --
	if(pc=strchr(p0,'|')) {
		//count strings to reserve space for vector --
		UINT nSize=0;
		for(p=p0;p=strchr(p+1,'|');nSize++);
		if(!nSize) {
			return 1;
		}
		VEC_CSTR &sa=xc.sa;
		sa.reserve(nSize);

		int maxlen=0;
		while(p=strchr(++pc,'|')) {
			sa.push_back(CString());
			sa.back().SetString(pc,p-pc);
			if(p-pc>maxlen) maxlen=p-pc;
			pc=p;
		}
		ASSERT(nSize==(UINT)sa.size() && nSize==sa.capacity());

		if(p=strstr(sa[0],"::")) {
			if(p-(LPCSTR)sa[0]>3 && !_memicmp(p-4,NTI_EXT,4)) {
				xc.wFlgs|=XC_INITELEV;
				if(toupper(p[2])=='M') xc.wFlgs|=XC_METERS;
				if(ctyp!='N') return 8;
			}
			else {
				for(UINT i=1; i<nSize; i++) {
					if(!strstr(p=sa[i],"::") && *p && i<nSize-1)
					   return 12;
				}
				xc.wFlgs|=XC_INITPOLY;
				if(ctyp=='M' && bDynamic) return 8;
			}
		}
		else {
			if(maxlen>(int)Fld.F_Len && !(ctyp=='M' && nSize==1))
				return 3;

			while(*pc==',') {
				pc++;
				if(ctyp!='M' && isdigit(*pc)) {
					xc.nInitStr=(WORD)atoi(pc++)-1;
					if((UINT)xc.nInitStr>=nSize)
						return 4;
					xc.wFlgs|=XC_INITSTR;
					while(isdigit(*pc)) pc++;
				}
				else if(*pc=='R' && pc[1]=='O') {
					pc+=2;
					xc.wFlgs|=XC_LISTNOEDIT;
				}
				else
					return 5;
			}
			if(nSize==1) {
				xc.wFlgs|=XC_INITSTR;
			}
			else {
				xc.wFlgs|=XC_HASLIST;
			}
		}
	}
	else if((p=strchr(p0,'R')) && p[1]=='O') {
		if(ctyp=='L') {
			return 2;
		}
		xc.wFlgs|=XC_READONLY;
	}
	else
		return 7;

	return 0;
}

static bool ParseNextFloat(LPSTR &p0,float &f)
{
	LPSTR p=p0;
	if(!*p || *p!='-' && !isdigit(*p)) return false;
	for(;*p && !isspace(*p);p++); //skip to end of argument
	if(*p) *p++=0;
	f=(float)atof(p0);
	while(*p && isspace(*p)) p++;
	p0=p;
	return true;
}

UINT CShpLayer::ParseNextFieldName(LPSTR &p0)
{
	UINT f;
	LPSTR p=p0;
	for(;*p && !isspace(*p);p++); //skip to end of name
	if(*p) *p++=0;
	f=m_pdb->FldNum(p0);
	while(*p && isspace(*p)) p++;
	p0=p;
	return f;
}

int CShpLayer::ParseFldKey(UINT f,LPSTR p0)
{
	char ctyp=m_pdb->FldTyp(f);
	if(ctyp!='C' && (ctyp!='N' || m_pdb->FldDec(f)!=0)) return 8;

	int nArgs=cfg_GetArgv(p0,CFG_PARSE_ALL); //get remaining arguments
	if(nArgs>=1) {
		m_pdbfile->pfxfld=m_pdb->FldNum(cfg_argv[0]);
		if(nArgs>=4) {
			m_pdbfile->noloc_dir=(BYTE)atoi(cfg_argv[1]);
			m_pdbfile->noloc_lat=atof(cfg_argv[2]);
			m_pdbfile->noloc_lon=atof(cfg_argv[3]);
			if(nArgs>=5) {
				LPCSTR p=cfg_argv[4];
				if(strstr(p, "::")) {
					if(!m_pdbfile->pfxfld) return 9;
					m_pdbfile->wErrBoundary=10;
					if(nArgs>=7) {
						if(strlen(cfg_argv[6])+4>m_pdb->FldLen(f)) return 10;
						m_pUnkPfx=_strdup(cfg_argv[6]);
						m_pUnkName=_strdup(cfg_argv[5]);
						if(nArgs>=8) m_pdbfile->wErrBoundary=(WORD)atoi(cfg_argv[7]);
					}
					else if(nArgs==6) return 11;
				}
				else {
					if(strlen(p)+4>m_pdb->FldLen(f)) return 10;
				}
				m_pKeyGenerator=_strdup(p);
			}
		}
	}
	return 0;
}

static void InitError(LPCSTR fName,LPCSTR pathName,LPCSTR sIgnored,int e)
{
	LPCSTR p="bad format";
	switch(e) {
		case 2: p="RO disallowed for type L"; break;
		case 8: p="wrong field type"; break;
		case 9: p="2nd argument not a field"; break;
		case 10: p="prefix must allow a suffix of 4+ digits"; break;
		case 11: p="wrong number of arguments"; break;
		case 12: p="field name required"; break;
	}
	CMsgBox("File: %s\n\nCAUTION: Initialization specifier not usable for%s field %s (%s).",
		trx_Stpnam(pathName),sIgnored,fName,p);
}

void CShpLayer::XC_GetFldOptions(LPSTR pathName)
{
	//read .tmpshp component for special format args --

	ASSERT(!m_pdbfile->pvxc && !m_pdbfile->pvxd && !m_pdbfile->bHaveTmpshp);

	LPSTR pExt=trx_Stpext(pathName);
	strcpy(pExt,SHP_EXT_TMPSHP);
	if(_access(pathName,0)) goto _ret;

	{
		CFileBuffer fdef(2048);
		
		if(fdef.OpenFail(pathName,CFile::modeRead)) {
			CMsgBox("Unable to open template file: %s.",pathName);
			goto _ret;
		}

		m_pdbfile->bHaveTmpshp=true;

		try {
			ASSERT(!m_pdbfile->pvxc && !m_pdbfile->pvxc_ignore);

			char line[1088]; //TYPE uses 1066
			int len;

			while((len=fdef.ReadLine(line,1088))>=0) {
				if(*line!='.' || len>=1087 || _memicmp(line+1,"FLD",3))
					continue;

				UINT f;
				LPSTR p,p0;
				for(p=line+4;*p && !isspace(*p);p++); //skip past keyword
				while(isspace(*p)) p++;
				p0=p;

				if(!*p) continue;

				if(!(f=ParseNextFieldName(p0)))
					continue; //all valid lines have a field name as the first argument (ignore if non-existent)

				char ctyp=line[4];
				int e=0;

				if(isspace(ctyp)) {
					LPSTR pc=strchr(p0,';');
					if(pc) *pc=0;
					if(!(pc=strstr(p0,"?(")) || !(p=strchr(pc,')')))
						continue;

					if(!m_pdbfile->pvxc) m_pdbfile->pvxc=new VEC_XCOMBO;
					m_pdbfile->pvxc->push_back(XCOMBODATA());

					*p=0; pc+=2;
					int e=XC_ParseDefinition(m_pdbfile->pvxc->back(),m_pdb->m_pFld[f-1],pc,TRUE); //dynamic
					if(e) {
						InitError(m_pdb->FldNamPtr(f),pathName,"",e);
						m_pdbfile->pvxc->resize(m_pdbfile->pvxc->size()-1);
						continue;
					}
					m_pdbfile->pvxc->back().nFld=f;
				}
				else switch(toupper(ctyp)) {

					//FLDDESC --
					case 'D' : 	if(!m_pdbfile->pvxd) {
									m_pdbfile->pvxd=new VEC_XDESC;
									m_pdbfile->pvxd->reserve(m_pdb->NumFlds());
								}
								m_pdbfile->pvxd->push_back(XDESCDATA(f,p0));
								m_pdbfile->pvxd->back().sDesc.Replace("\\n","\r\n"); //necessary!
								//m_pdbfile->pvxd->back().sDesc.Replace("\\t","\t"); fails
								break;

					//FLDLBL --
					case 'L' :	ASSERT(!m_pdbfile->lblfld);
								m_pdbfile->lblfld=f;
							    break;

					//FLDSRCH --
					case 'S' :	m_pdbfile->vxsrch.push_back(f);
								while(*p0) {
								   if((f=ParseNextFieldName(p0)))
									   m_pdbfile->vxsrch.push_back((BYTE)f);
								}
								break;

					//FLDKEY --
					case 'K':   if(e=ParseFldKey(f, p0))
									InitError(m_pdb->FldNamPtr(f), pathName, " FLDKEY", e);
								else m_pdbfile->keyfld=f;
								break;

					//FLDIGNORE --
					case 'I':   if(!m_pdbfile->pvxc_ignore) m_pdbfile->pvxc_ignore=new VEC_XCOMBO;
								m_pdbfile->pvxc_ignore->push_back(XCOMBODATA());
								if(cfg_GetArgv(p0, CFG_PARSE_ALL)>=1 &&
										   (e=XC_ParseDefinition(m_pdbfile->pvxc_ignore->back(), m_pdb->m_pFld[f-1], cfg_argv[0]))) {
									InitError(m_pdb->FldNamPtr(f),pathName," FLDIGNORE",e);
									m_pdbfile->pvxc_ignore->resize(m_pdbfile->pvxc_ignore->size()-1);
								}
								else
								    m_pdbfile->pvxc_ignore->back().nFld=f;
							   break;
				}
			}
			if(m_pdbfile->pvxc_ignore && m_pdbfile->pvxc_ignore->empty()) {
				delete m_pdbfile->pvxc_ignore;
				m_pdbfile->pvxc_ignore=NULL;
			}
		}
		catch(...) {
			delete m_pdbfile->pvxc;
			m_pdbfile->pvxc=NULL;
			delete m_pdbfile->pvxc_ignore;
			m_pdbfile->pvxc_ignore=NULL;
			delete m_pdbfile->pvxd;
			m_pdbfile->pvxd=NULL;
			m_pdbfile->lblfld=m_pdbfile->keyfld=0;
			m_pdbfile->vxsrch.clear();
			m_pdbfile->bHaveTmpshp=false;
			CMsgBox("File: %s\n\nCAUTION: This template file was not processed due to lack of memory.",trx_Stpnam(pathName));
		}

		if(fdef.IsOpen()) fdef.Close();
	}

_ret:
	strcpy(pExt,SHP_EXT_DBF);
}

PXCOMBODATA CShpLayer::XC_GetXComboData(int nFld)
{
	PVEC_XCOMBO pvxc=m_pdbfile->pvxc;
	if(!pvxc || nFld<=0) return NULL;
	for(VEC_XCOMBO::iterator it=pvxc->begin();it!=pvxc->end();it++) {
		if(nFld==it->nFld) {
			return &*it;
		}
	}
	return NULL;
}

LPCSTR CShpLayer::XC_GetXDesc(int nFld)
{
	VEC_XDESC *pvxd=m_pdbfile->pvxd;
	if(!pvxd) return NULL;
	for(VEC_XDESC::iterator it=pvxd->begin();it!=pvxd->end();it++) {
		if(nFld==it->nFld) {
			return (LPCSTR)it->sDesc;
		}
	}
	return NULL;
}

void CShpLayer::XC_GetDefinition(CString &s,PXCOMBODATA pxc)
{
	s.Empty();
	if(!pxc) return;
	s=" ?(";
	if(pxc->sa.size()) {
		ASSERT(pxc->wFlgs&(XC_INITPOLY|XC_INITELEV|XC_INITSTR|XC_HASLIST));
		s.AppendChar('|');
		for(VEC_CSTR::const_iterator it=pxc->sa.begin();it!=pxc->sa.end();it++) {
			s+=*it;
			s.AppendChar('|');
		}
		if(pxc->wFlgs&XC_HASLIST) {
			char buf[12];
			sprintf(buf,",%u",pxc->nInitStr+1);
			if(pxc->wFlgs&XC_LISTNOEDIT)
				strcat(buf,",RO");
			s+=buf;
		}
	}
	else if(pxc->wFlgs&XC_READONLY)
		s+="RO";
	s.AppendChar(')');
}

static void GetVariableFldStr(CShpLayer *pShp,CString &s, UINT nFld)
{
	if(pShp->m_pdb->FldTyp(nFld)=='M') {
		DWORD dbtrec=CDBTFile::RecNo((LPCSTR)pShp->m_pdb->FldPtr(nFld));
		if(dbtrec) {
			UINT len=0;
			LPSTR p=pShp->m_pdbfile->dbt.GetText(&len,dbtrec);
			if(len) {
				s.SetString(p,len);
			}
			else s.Empty();
		}
	}
	else pShp->m_pdb->GetTrimmedFldStr(s,nFld);
}

LPCSTR CShpLayer::XC_GetInitStr(CString &s,XCOMBODATA &xc,const CFltPoint &fpt,LPBYTE pRec,CFltPoint *pOldPt/*=0*/)
{
	//pOldPt != NULL implies we are relocating an existing record. We need to compare with existing value
	//and avoid reinitialization if inappropriate.
	LPCSTR pstr=NULL;

	if(xc.wFlgs&(XC_INITPOLY|XC_INITELEV)) {
		CString s2;
		if(pOldPt) {
			ASSERT(pRec);
			//Will compare with current value upon successful search--
			m_pdb->GetTrimmedFldStr(pRec,s2,xc.nFld);
		}
		if(xc.wFlgs&XC_INITPOLY) {
			//see if fpt is contained in a loaded polygon shapefile --
			UINT fldNum;
			DWORD rec;
			CShpLayer *pShp;
			UINT iMax=xc.sa.size()-1;
			if(iMax>0 && !strstr(xc.sa[iMax], "::")) iMax--;
			if(!IsNotLocated(fpt)) {
				for(UINT i=0; i<=iMax; i++) {
					pShp=m_pDoc->FindShpLayer(xc.sa[i],&fldNum);
					if(pShp && pShp->ShpType()==CShpLayer::SHP_POLYGON) {
						if((rec=pShp->ShpPolyTest(fpt)) && !pShp->m_pdb->Go(rec)) {
							GetVariableFldStr(pShp,s,fldNum);
							return (!pOldPt || s.Compare(s2))?(LPCSTR)s:NULL;
						}
					}
				}
			}
			//New point is outside all polygons or defined as unlocated --
			if(pOldPt && !s2.IsEmpty() && (iMax+1==xc.sa.size() || s2.Compare(xc.sa[iMax+1]))) {
			    //Dynamic init -- Old value is nonempty and potential new value is the default value (possibly empty)
				if(!(xc.wFlgs&XC_NOPROMPT)) {
					CString msg;
					msg.Format("CAUTION: You're moving the point to a location outside the extents of the initialization layers. "
						"Since field %s is non-empty, however, it won't be cleared or reinitialized. "
						"You may want to edit it.",m_pdb->FldNamPtr(xc.nFld));
					CMsgCheck dlg(IDD_MSGEXTENT,msg);
					dlg.DoModal();
					if(dlg.m_bNotAgain) xc.wFlgs|=XC_NOPROMPT;
				}
				return NULL;
			}

			if(xc.sa.size()>iMax+1) {
				pstr=(LPCSTR)xc.sa[iMax+1];
			}
		}
		else if(xc.wFlgs&XC_INITELEV) {
			CLayerSet &ls=m_pDoc->LayerSet();
			CImageLayer *pL;
			if(ls.HasNteLayers()) {
				int elev=CNTERecord::NODATA;
				if(!IsNotLocated(fpt) && (pL=ls.GetTopNteLayer(fpt)) && (elev=((CNtiLayer *)pL)->GetBestElev(fpt))!=CNTERecord::NODATA) {
					if(xc.wFlgs&XC_METERS) {
						s.Format("%.1f",elev*0.3048);
						if(!pOldPt || atof(s)!=atof(s2))
							pstr=(LPCSTR)s;
					}
					else {
						if(!pOldPt || elev!=atoi(s2)) {
							s.Format("%d",elev);
							pstr=(LPCSTR)s;
						}
					}
				}
				if(pOldPt && !pstr && elev==CNTERecord::NODATA && !(xc.wFlgs&XC_NOPROMPT) && !s2.IsEmpty() && ls.GetTopNteLayer(*pOldPt)) {
					CString msg;
					msg.Format("CAUTION Although the previous location is covered by an NTI elevation layer, the new "
							"location is not. You may want to edit the unchanged value in field %s.",m_pdb->FldNamPtr(xc.nFld));
					CMsgCheck dlg(IDD_MSGEXTENT,msg);
					dlg.DoModal();
					if(dlg.m_bNotAgain) xc.wFlgs|=XC_NOPROMPT;
				}
			}
		}
	}
	else if(xc.wFlgs&XC_INITSTR) {
		if(!pOldPt) {
			pstr=(LPCSTR)xc.sa[xc.nInitStr];
			if(strstr(pstr,"\\n")) {
				s=pstr;
				s.Replace("\\n","\r\n");
				pstr=(LPCSTR)s;
			}
		}
	}
	return pstr;
}

BOOL CShpLayer::XC_PutInitStr(LPCSTR p, DBF_pFLDDEF pFld, LPBYTE pDst,CDBTFile *pdbt)
{
	if(pFld->F_Typ=='M') {
		UINT dbtrec;
		int len=strlen(p);
		if(len<=510) dbtrec=pdbt->PutTextField(p,len);
		else {
			EDITED_MEMO memo;
			memo.recNo=0;
			memo.pData=(LPSTR)p;
			dbtrec=pdbt->PutText(memo);
        }
		if(dbtrec) {
			if(dbtrec==(UINT)-1) return FALSE;
			CDBTFile::SetRecNo((LPSTR)pDst,dbtrec);
		}
	}
	else
		StoreText(p,pFld->F_Len,pDst);

	return TRUE;
}

void CShpLayer::XC_FlushFields(UINT nRec)
{
	if(m_pdb->Go(nRec)) {
		ASSERT(0);
		return;
	}
	PVEC_XCOMBO pvxc=m_pdbfile->pvxc;
	ASSERT(pvxc);
	UINT foff=0;
	for(VEC_XCOMBO::iterator it=pvxc->begin();it!=pvxc->end();it++) {
		if(it->wFlgs&(XC_INITELEV|XC_INITPOLY)) {
			foff=m_pdb->FldOffset(it->nFld);
			memcpy(m_pdb->RecPtr()+foff,m_pdbfile->pbuf+foff,m_pdb->FldLen(it->nFld));
		}
	}
	if(foff) {
		VERIFY(!m_pdb->FlushRec());
		if(!m_bEditFlags) { //***added
			m_pDoc->LayerSet().m_nShpsEdited++;
		}
		m_bEditFlags|=SHP_EDITDBF;
		RecEditFlag(nRec)|=SHP_EDITDBF;
	}
}

BOOL CShpLayer::XC_RefreshInitFlds(LPBYTE pRec,const CFltPoint &fpt,BOOL bReinit,CFltPoint *pOldPt /*=NULL*/)
{
	// bReinit = 0(init new), 1(init new or similar),
	// 2 (CShowShapeDlg::ApplyChangedPoint: init only poly or elev if avavailable and different)

	BOOL bRet=FALSE;
	CString s;
		
	PVEC_XCOMBO pvxc=m_pdbfile->pvxc;
	for(VEC_XCOMBO::iterator it=pvxc->begin();it!=pvxc->end();it++) {
		LPCSTR pstr=NULL;

		if(bReinit==1 && (it->wFlgs&XC_READONLY)) {
			memset(pRec+m_pdb->FldOffset(it->nFld),' ',m_pdb->FldLen(it->nFld));
		}
		else if(pstr=XC_GetInitStr(s,*it,fpt,pRec,pOldPt)) {
			ASSERT(bReinit<2 || app_pShowDlg);
			if(bReinit==2 && !bRet) {
				VERIFY(app_pShowDlg->LoadEditBuf());
			}
			bRet=TRUE;
			m_pdb->StoreTrimmedFldData(m_pdbfile->pbuf,pstr,it->nFld);
		}
	}
	return bRet; //actually used only if bReinit==2 to refreash field list, etc.
}

