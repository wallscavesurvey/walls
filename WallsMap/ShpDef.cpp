#include "Stdafx.h"
#include "resource.h"
#include "WallsMapDoc.h"
#include "filecfg.h"
#include "shpdef.h"

#ifdef _USE_TRX
bool bUsingIndex;
#endif

LPSTR shp_deftypes[]={"FLD","FLDDESC","FLDIGNORE","FLDKEY","FLDLBL","FLDSRCH"};

#define SHP_CFG_NUMTYPES (sizeof(shp_deftypes)/sizeof(LPSTR))

enum {
	SHP_CFG_FLD=1,SHP_CFG_FLDDESC,SHP_FLDIGNORE,SHP_CFG_FLDKEY,SHP_CFG_FLDLBL,SHP_CFG_FLDSRCH
};

CShpDef::~CShpDef(void)
{
	for(it_lpstr it=v_fldCombo.begin(); it!=v_fldCombo.end(); it++) free(*it);
	for(it_filter it=v_filter.begin(); it!=v_filter.end(); it++) free(it->pVals);
	if(m_pFdef) {
		if(m_pFdef->IsOpen()) m_pFdef->Close();
		delete m_pFdef;
	}
}

BOOL CShpDef::Process(CShpLayer *pShp,LPCSTR pathName)
{
	m_pFdef=new CFileCfg(shp_deftypes,SHP_CFG_NUMTYPES,46,2048);
	if(!m_pFdef->Open(pathName,CFile::modeRead)) {
		CMsgBox("Unable to open template file: %s.",pathName);
		delete m_pFdef;
		m_pFdef=NULL;
		return FALSE;
	}
	CString errMsg("Unsupported field format");
	DBF_FLDDEF fld;
#ifdef _USE_TRX
	bUsingIndex=false;
#endif
	fld.F_Off=0;
	fld.F_Nam[10]=0;
	CShpDBF *pdb=pShp->m_pdb;
	int typ;
	while((typ=m_pFdef->GetLine())>0 || typ==CFG_ERR_KEYWORD) {
		if(typ!=SHP_CFG_FLD) {
			if(typ==CFG_ERR_KEYWORD) {
				errMsg="Invalid directive";
				goto _badExit;
			}
			uFlags|=SHPD_DESCFLDS;
			break;
		}


		if(m_pFdef->Argc()<2) goto _badExit;

		LPSTR p,pParams=m_pFdef->Argv(1);
		LPSTR pNam,pSrcFld=strstr(pParams,"<<");
		LPSTR pCombo=strstr(pParams,"?(");
		LPSTR pInit=NULL,pTSName=NULL;

		bool bTS=!memcmp(pParams, "CREATED_",8) || !memcmp(pParams, "UPDATED_",8);

		if(pCombo) {
			if(pSrcFld>=pCombo) goto _badExit;
			*pCombo++=0; pCombo++;
			if(!pSrcFld && (pCombo=strstr(pInit=pCombo,"?("))) pCombo+=2;
			LPSTR p;
			if(pCombo) {
				if(!(p=strchr(pCombo, ')'))) goto _badExit;
				*p=0;
				if(bTS) pCombo=NULL;
				else uFlags|=SHPD_COMBOFLDS;
			}
			if(pInit) {
				pSrcFld=NULL;
				if(!(p=strchr(pInit,')'))) goto _badExit;
				*p=0;
				uFlags|=SHPD_INITFLDS;
			}
		}

		v_fldCombo.push_back(pCombo?_strdup(pCombo):NULL);

		if(bTS) {
			pSrcFld=NULL;
			if(p=strchr(pParams, '[')) {
				if(pInit)  goto _badExit;
				*p++=0;
				pTSName=p;
				if(!(p=strchr(p,']'))) goto _badExit;
				*p=0;
			}
		}

		if(pSrcFld) {
			pSrcFld+=2;
			while(isspace(*pSrcFld)) pSrcFld++;
			for(p=pSrcFld; *p && !isspace(*p) && *p!='='; p++);
			if(*p==' ') {
				*p++=0;
				while(isspace(*p)) p++;
			}
			if(*p=='=') {
				WORD bNeg=0;
				*p++=0;
				while(isspace(*p)) p++;
				if(*p=='!') {
					while(isspace(*++p));
					bNeg++;
				}
				if(*p!='(' || !(pNam=strchr(p,')'))) goto _badExit;
				*p++=0; *pNam++=0; *pNam=0;
				UINT f=pShp->m_pdb->FldNum(pSrcFld);
				if(!f) {
					errMsg="Filtered field not in source.";
					goto _badExit;
				};
				if(!*p) pNam=NULL;
				else {
					pNam=(LPSTR)malloc(pNam-p+1);
					memcpy(pNam,p,strlen(p)+2);
					for(p=pNam; *p; p++) if(*p=='|') *p=0;
				}
				v_filter.push_back(FILTER(f,bNeg,pNam));
				uFlags|=SHPD_FILTER;
			}
		}


		double scale=1.0;
		int n=m_pFdef->GetArgv(pParams);

		if(n<3 || strlen(pNam=m_pFdef->Argv(0))>10)
			goto _badExit;

#ifdef _USE_TRX
		bool bFromIdx=false;
#endif

		fld.F_Typ=toupper(*m_pFdef->Argv(1));
		if((uFlags&SHPD_EXCLUDEMEMOS) && fld.F_Typ=='M') continue;
		if(!(fld.F_Len=(BYTE)atoi(m_pFdef->Argv(2))))
			goto _badExit;

		fld.F_Dec=0;
		if(fld.F_Typ=='N' || fld.F_Typ=='F') {
			if(fld.F_Len>20) {
				goto _badExit;
			}
			p=strchr(m_pFdef->Argv(2),'.');
			if(p!=NULL) {
				BYTE dec=(BYTE)atoi(p+1);
				fld.F_Dec=dec;
				if(dec && (fld.F_Len<3 || dec>fld.F_Len-2)) {
					goto _badExit;
				}
			}
		}
		if(!(uFlags&SHPD_INITEMPTY) && pSrcFld) {

			if(*pSrcFld=='|') {
				if(iPrefixFld) {
					errMsg="Only one source field format specification allowed in template";
					goto _badExit;
				}
				p=strchr(pSrcFld+1,'|');
				if(!p) goto _badExit;
				csPrefixFmt.SetString(pSrcFld+1,p-pSrcFld-1);
				pSrcFld=p+1;
				if(!*pSrcFld) pSrcFld=NULL;
				iPrefixFld=numFlds+1;
			}

#ifdef _USE_TRX
			if(*pSrcFld=='&' && pSrcFld[1]=='&') {
				bFromIdx=true;
				bUsingIndex=true;
				pSrcFld=NULL;
			}
#endif
		}

		memset(fld.F_Nam,0,11);
		pNam=_strupr(strcpy(fld.F_Nam,pNam));

		n=SHP_DBFILE::GetLocFldTyp(pNam);

		if(n) {
			n--;
			if(*pNam=='_')
				strcpy(pNam,SHP_DBFILE::sLocflds[n]); //use new form!

			if(n>=LF_LAT && n<=LF_DATUM) {
				n=-1-n;
				numLocFlds++;
				numSrcFlds++;
			}
			else {
				ASSERT(bTS && (n==LF_UPDATED || n==LF_CREATED));
				fld.F_Typ='C';
				if(fld.F_Len<25) fld.F_Len=25;
				else if(fld.F_Len>50) fld.F_Len=50;
				if(pTSName) { //we'll generate them
					ASSERT(!pInit);
					CShpLayer::GetTimestampString(csTimestamps[n==LF_UPDATED],pTSName);
					n=FLDIDX_TIMESTAMP-(n==LF_UPDATED);
				}
				else if(!pInit) {
				    BYTE fn=pShp->m_pdbfile->locFldNum[n];
					if(fn) pSrcFld=(LPSTR)pdb->FldNamPtr(fn); //We'll keep the same timestamps!
				}
			}
		}

		if(n>=0) {
			if(fld.F_Len<1 || fld.F_Len<=fld.F_Dec || fld.F_Dec>15)
				goto _badExit;

			if(fld.F_Typ!='N') fld.F_Dec=0;

			switch(fld.F_Typ) {
				case 'C': if(fld.F_Len>255)
							  goto _badExit;
						break;
				case 'M': fld.F_Len=10;
						numMemoFlds++;
						break;
				case 'N': 
				case 'F': if(fld.F_Len>20)
							  goto _badExit;
						break;
				case 'L': fld.F_Len=1;
						break;
				case 'D': fld.F_Len=8;
						break;
				default :
					goto _badExit;
			}

			n=pSrcFld?0:FLDIDX_NOSRC;

#ifdef _USE_TRX
			if(bFromIdx) {
				n=FLDIDX_INDEX;
			}
#endif
			if(!n) {
				p=strchr(pSrcFld,'*');
				if(p) {
					*p++=0;
					scale=atof(p);
					if(!(uFlags&SHPD_ISCONVERTING)) {
						uFlags|=SHPD_ISCONVERTING;
						v_fldScale.resize(numFlds,1.0);
					}
				}
				n=pdb->NumFlds();
				for(;n;n--) {
					if(!_stricmp(pdb->FldNamPtr(n),pSrcFld)) break;
				}
				if(!n || !CShpDBF::TypesCompatible(pdb->FldTyp(n),fld.F_Typ)) {
						errMsg.Format("Source field %s %s",pSrcFld,n?"has the wrong type":"was not found");
						goto _badExit;
				}
				numSrcFlds++;
				/*
				if(!cUnitsConv) {
					if(fld.F_Typ!='N' || !strchr("MmFf",cUnitsConv)) {
						errMsg="Converted fields must be of type N and have units feet or meters.";
						goto _badExit;
					}
					cUnitsConv=toupper(cUnitsConv);
				}
				*/
			}

			if(pInit) {
				v_xc.push_back(XCOMBODATA());
				XCOMBODATA &xc=v_xc.back();
				if(CShpLayer::XC_ParseDefinition(xc,fld,pInit)) {
					errMsg.Format("Field %s has an invalid option specifier: ?(%s)",pNam,pInit);
					goto _badExit;
				}
				if(n==FLDIDX_NOSRC && !(uFlags&SHPD_INITEMPTY)) {
					WORD wFlgs=(xc.wFlgs&(XC_INITPOLY|XC_INITELEV|XC_INITSTR));
					if(wFlgs&XC_INITPOLY) {
						UINT fldnum;
						CShpLayer *pShpPoly;
						int iMax=xc.sa.size()-1;
						if(iMax>0 && !strstr(xc.sa[iMax], "::")) iMax--;
						for(int i=0; i<=iMax; i++) {
							pShpPoly=pShp->m_pDoc->FindShpLayer(xc.sa[i],&fldnum);
							if(!pShpPoly || pShpPoly->ShpType()!=CShpLayer::SHP_POLYGON) {
								errMsg.Format("Source %s missing or unsuitable for field %s initialization",(LPCSTR)xc.sa[i],pNam);
								goto _badExit;
							}
						}
					}
					else if(wFlgs&XC_INITELEV) {
						if(!pShp->m_pDoc->HasNteLayers()) {
							errMsg.Format("%s -- No suitable layers are present for obtaining elevation",pNam);
							goto _badExit;
						}
					}
					if(wFlgs) {
						v_xc.back().nFld=numFlds+1;
						n=FLDIDX_XCOMBO-(v_xc.size()-1);
					}
				}
				if(n==FLDIDX_NOSRC)
					v_xc.resize(v_xc.size()-1);
			}
		}

		numFlds++;
		v_fldDef.push_back(fld);
		v_fldIdx.push_back(n);
		if(uFlags&SHPD_ISCONVERTING)
			v_fldScale.push_back(scale);
	}

	ASSERT(!(uFlags&SHPD_ISCONVERTING) || v_fldScale.size()==numFlds);

	if(!numFlds) {
		CMsgBox("No fields were defined in file %s.",trx_Stpnam(pathName));
		goto _badExit1;
	}

	if(!(uFlags&SHPD_DESCFLDS)) {
		m_pFdef->Close();
	}

	if(!(uFlags&SHPD_INITEMPTY) && !numSrcFlds) {
		if(!IDOK!=AfxMessageBox("This operation will initialize an empty shapefile since no source fields "
				"were specified or found in the template. Select OK to proceed.",MB_OKCANCEL)) {
			return FALSE;
		}
		uFlags|=SHPD_INITEMPTY;
	}
	return TRUE;

_badExit:
	CMsgBox("File %s, Line %u: %s.",trx_Stpnam(pathName),m_pFdef->LineCount(),(LPCSTR)errMsg);

_badExit1:
	m_pFdef->Close();
	delete m_pFdef;
	m_pFdef=NULL;
	return FALSE;
}

DBF_pFLDDEF CShpDef::FindDstFld(UINT srcFld) const
{
	if(!srcFld) return NULL;
	const int *pSrcFldNum=&v_fldIdx[0];
	DBF_pFLDDEF pFldDef=(DBF_pFLDDEF)&v_fldDef[0];
	for(int i=0;i<numFlds;i++,pFldDef++,pSrcFldNum++) {
		if(*pSrcFldNum>0 && srcFld==*pSrcFldNum) {
		    return pFldDef;
		}
	}
	return NULL;
}

BOOL CShpDef::Write(CShpLayer *pShp,CSafeMirrorFile &cf) const
{
	LPCSTR hdr=NULL;
	HGLOBAL hg;
	HRSRC hr;

	if(!(hr=FindResource(NULL,MAKEINTRESOURCE(IDR_DATA1),RT_RCDATA)) ||
	   !(hg=LoadResource(NULL,hr)) ||
	   !(hdr=(LPCSTR)LockResource(hg))) {
			cf.Abort();
			return FALSE;
	}

	try {
		cf.Write(hdr,SizeofResource(NULL, hr));
		CString fld,flen,finit;
		DBF_pFLDDEF pFld=(DBF_pFLDDEF)&v_fldDef[0];
		const int *pSrcFldNum=(!m_pFdef && pShp->HasXFlds())?&v_fldIdx[0]:NULL;
		const LPCSTR pEnd="=================================\r\n";
		#define pEND_LEN 35

		for(int i=0;i<numFlds;i++,pFld++) {
			if(pFld->F_Typ=='N') flen.Format("%u.%u",pFld->F_Len,pFld->F_Dec);
			else flen.Format("%u",pFld->F_Len);
			if(SHP_DBFILE::GetLocFldTyp(pFld->F_Nam))
				fld.Format(".FLD %-11s%c %-5s\r\n",pFld->F_Nam,pFld->F_Typ,(LPCSTR)flen);
			else {
				finit.Empty();
				if(m_pFdef) {
					if(v_fldCombo[i]) finit.Format(" ?(%s)",v_fldCombo[i]);
				}
				else if(pSrcFldNum) {
					pShp->XC_GetDefinition(finit,pShp->XC_GetXComboData(*pSrcFldNum));
					pSrcFldNum++;
				}
				fld.Format(".FLD %-11s%c %-5s << %s%s\r\n",pFld->F_Nam,pFld->F_Typ,(LPCSTR)flen,pFld->F_Nam,(LPCSTR)finit);
			}
			cf.Write((LPCSTR)fld,fld.GetLength());
		}
		cf.Write(pEnd,pEND_LEN);

		if(pSrcFldNum) {
			pFld=FindDstFld(pShp->m_nLblFld);
			if(pFld && pFld->F_Typ!='M') {
				fld.Format(".FLDLBL %s\r\n",pFld->F_Nam);
				cf.Write((LPCSTR)fld,fld.GetLength());
			}
			SHP_DBFILE *pdf=pShp->m_pdbfile;
			if((pFld=FindDstFld(pdf->keyfld)) && pFld->F_Typ=='C') {
				fld.Format(".FLDKEY %s",pFld->F_Nam);
				cf.Write((LPCSTR)fld,fld.GetLength());
				if((pFld=FindDstFld(pdf->pfxfld)) && pFld->F_Typ=='C') {
					fld.Format(" %s", pFld->F_Nam);
					cf.Write((LPCSTR)fld,fld.GetLength());
					if(pdf->noloc_dir>0) {
						fld.Format(" %u %.2f %.2f",pdf->noloc_dir,pdf->noloc_lat,pdf->noloc_lon);
						cf.Write((LPCSTR)fld,fld.GetLength());
						if(pShp->m_pKeyGenerator) {
							fld.Format(" \"%s\"",pShp->m_pKeyGenerator);
							cf.Write((LPCSTR)fld, fld.GetLength());
							if(pShp->m_pUnkName && pShp->m_pUnkPfx) {
								fld.Format(" \"%s\" \"%s\"", pShp->m_pUnkName, pShp->m_pUnkPfx);
								cf.Write((LPCSTR)fld, fld.GetLength());
							}
						}
					}
				}
				cf.Write("\r\n",2);
				if(pShp->m_pdbfile->pvxc_ignore) {
					for(it_vec_xcombo it=pShp->m_pdbfile->pvxc_ignore->begin(); it!=pShp->m_pdbfile->pvxc_ignore->end();it++) {
						pShp->XC_GetDefinition(finit,&*it);
						if(!finit.IsEmpty()) { // ?(|
							finit.Replace("?(","\"");
							finit.Replace(')','"');
						}
						fld.Format(".FLDIGNORE %s%s\r\n",pShp->m_pdb->FldNamPtr(it->nFld),(LPCSTR)finit);
						cf.Write((LPCSTR)fld,fld.GetLength());
					}
				}
			}

			if(!pShp->m_vSrchFlds.empty()) {
				fld=".FLDSRCH";
				for(it_byte it=pShp->m_vSrchFlds.begin();it!=pShp->m_vSrchFlds.end();it++) {
					pSrcFldNum=&v_fldIdx[0];
					pFld=(DBF_pFLDDEF)&v_fldDef[0];
					for(int i=0;i<numFlds;i++,pFld++,pSrcFldNum++) {
						if(*pSrcFldNum==*it) {
							fld+=' '; fld+=pFld->F_Nam;
							break;
						}
					}
				}
				if(fld.GetLength()>8) {
					fld+="\r\n";
					cf.Write((LPCSTR)fld,fld.GetLength());
				}
			}

			pSrcFldNum=&v_fldIdx[0];
			pFld=(DBF_pFLDDEF)&v_fldDef[0];
			bool bDesc=false;
			for(int i=0;i<numFlds;i++,pFld++,pSrcFldNum++) {
				LPCSTR p;
				if(*pSrcFldNum>0 && (p=pShp->XC_GetXDesc(*pSrcFldNum))) {
					CString s(p);
					s.Replace("\r\n","\\n");
					fld.Format(".FLDDESC %-11s%s\r\n",pFld->F_Nam,(LPCSTR)s);
					cf.Write((LPCSTR)fld,fld.GetLength());
					bDesc=true;
				}
			}
			if(bDesc) cf.Write(pEnd,pEND_LEN);
		}
		else if(m_pFdef && m_pFdef->IsOpen()) {
			int e=0; LPCSTR p0,p1;
			do {
				p0=m_pFdef->Argv(0);
				if(m_pFdef->Argc()==2) {
					p1=m_pFdef->Argv(1);
					cf.Write(".", 1);
					cf.Write(m_pFdef->Argv(0),strlen(m_pFdef->Argv(0)));
					cf.Write(" ",1);
					cf.Write(m_pFdef->Argv(1),strlen(m_pFdef->Argv(1)));
					cf.Write("\r\n",2);
				}
			}
			while((e=m_pFdef->GetLine())>0);
			cf.Write(pEnd,pEND_LEN);
			m_pFdef->Close();
		}
		cf.Close();
	}
	catch(...) {
		cf.Abort();
		_unlink(cf.GetFilePath());
		return FALSE;
	}
	return TRUE;
}

