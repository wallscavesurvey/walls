#include "stdafx.h"
#include <trx_str.h>
#include "XlsExportDlg.h"
#include "filecfg.h"
#include "crack.h"
#include "utility.h"

enum {SHP_CFG_FLD=1,SHP_CFG_FLDKEY,SHP_CFG_LOCDFLT,SHP_CFG_LOCERR,SHP_CFG_LOCTLBR,SHP_CFG_LOCZONE};

LPSTR shp_deftypes[]={"FLD","FLDKEY","LOCDFLT","LOCERR","LOCTLBR","LOCZONE"};
#define SHP_CFG_NUMTYPES (sizeof(shp_deftypes)/sizeof(LPSTR))

LPCSTR sLocflds[NUM_LOCFLDS]={"LATITUDE_","LONGITUDE_","EASTING_","NORTHING_","ZONE_","DATUM_","UPDATED_","CREATED_"};

int CShpDef::GetLocFldTyp(LPCSTR pNam) //static
{
	for(int i=0;i<NUM_LOCFLDS;i++) {
		if(!strcmp(pNam,sLocflds[i]))
			return i+1;
	}
	return 0;
}

CShpDef::~CShpDef(void)
{
}

/*
static LPCSTR pstrType[]={ "","Boolean","Byte","Integer","Long","Currency","Single","Double","Date","Binary",
                           "Text", "LongBinary", "Memo", "", "", "GUID", "BigInt", "VarBinary", "Char",
						   "Numeric", "Decimal", "Float", "Time", "TimeStamp" };
*/

static bool TypesCompatible(int srcTyp,char cDst)
{
	switch(srcTyp) {
	    case dbBoolean:
			return cDst=='L';
		case dbByte:
		case dbInteger:
		case dbLong:
		case dbSingle:
		case dbDouble:
		case dbDecimal:  ; //16-byte fixed point: VT_DECIMAL
			return cDst=='N' || cDst=='F';
		case dbText:
		case dbMemo:
			return cDst=='C' || cDst=='M';
	}
	return false;
}

BOOL CShpDef::Process(CDaoRecordset *pRS,LPCSTR pathName)
{
	CFileCfg fdef(shp_deftypes,SHP_CFG_NUMTYPES);
	if(!fdef.Open(pathName,CFile::modeRead)) {
		CMsgBox("Unable to open template file: %s.",pathName);
		return FALSE;
	}

	CString csFldKey,errMsg("Unsupported format");

	DBF_FLDDEF fld;
	fld.F_Off=0;
	fld.F_Nam[10]=0;
	int typ;

	while((typ=fdef.GetLine())>0 || typ==CFG_ERR_KEYWORD) {
		if(fdef.Argc()<2) goto _badExit;

		if(typ!=SHP_CFG_FLD) {
			LPCSTR pp=fdef.Argv(1);
			int n=fdef.GetArgv(fdef.Argv(1));
			switch(typ) {
				case SHP_CFG_FLDKEY:
				{
					if(n<1) goto _badExit;
					csFldKey=fdef.Argv(0);
					break;
				}
				case SHP_CFG_LOCDFLT:
				{
					if(n<2) goto _badExit;
					latDflt=atof(fdef.Argv(0));
					lonDflt=atof(fdef.Argv(1));
					break;
				}
				case SHP_CFG_LOCERR:
				{
					if(n<2) goto _badExit;
					latErr=atof(fdef.Argv(0));
					lonErr=atof(fdef.Argv(1));
					break;
				}
				case SHP_CFG_LOCTLBR:
				{
					if(n<4) goto _badExit;
					latMax=atof(fdef.Argv(0));
					lonMin=atof(fdef.Argv(1));
					latMin=atof(fdef.Argv(2));
					lonMax=atof(fdef.Argv(3));
					break;
				}
				case SHP_CFG_LOCZONE:
				{
					if(n<2) goto _badExit;
					zoneMin=atoi(fdef.Argv(0));
					zoneMax=atoi(fdef.Argv(1));
					break;
				}
			}
			continue;
		}

		char *pNam,*pDefault;

		if(pDefault=strstr(fdef.Argv(1),"?(")) {
			*pDefault++=0;
			if(pNam=strchr(++pDefault,')')) {
				*pNam=0;
				if(*pDefault++=='|' && (pNam=strchr(pDefault, '|'))) {
					*pNam=0;
					if(!*pDefault) pDefault=NULL;
				}
				else pDefault=NULL;
			}
			else pDefault=NULL;
		}

		double scale=1.0;
		int srcType=0;

#ifdef _DEBUG
		pNam=fdef.Argv(1);
#endif
		int n=fdef.GetArgv(fdef.Argv(1));

		if(n<3 || strlen(pNam=fdef.Argv(0))>10)
			goto _badExit;

		int srcIdx=-1;
		LPSTR srcFld=NULL;

		fld.F_Typ=toupper(*fdef.Argv(1));
		if(!(fld.F_Len=(BYTE)atoi(fdef.Argv(2))))
			goto _badExit;
		fld.F_Dec=0;
		if(fld.F_Typ=='N' || fld.F_Typ=='F') {
			if(fld.F_Len>20) {
				goto _badExit;
			}
			char *p=strchr(fdef.Argv(2),'.');
			if(p!=NULL) {
				BYTE dec=(BYTE)atoi(p+1);
				fld.F_Dec=dec;
				if(dec && (fld.F_Len<3 || dec>fld.F_Len-2)) {
					goto _badExit;
				}
			}
		}
		if(n>3) {
			srcFld=fdef.Argv(srcIdx=3);
			if(strlen(srcFld)<2 || memcmp(srcFld,"<<",2))
				goto _badExit;

			if(!srcFld[2]) {
				if(n<5)
					goto _badExit;
				srcFld=fdef.Argv(srcIdx=4);
			}
			else {
				srcFld+=2;
			}

			{
				LPSTR p=strchr(srcFld,'*');
				if(p && p>srcFld) {
					//will scale a numeric value --
					*p++=0;
					scale=atof(p);
				}
			}
		}

		memset(fld.F_Nam,0,11);
		pNam=_strupr(strcpy(fld.F_Nam,pNam));

		n=GetLocFldTyp(pNam)-1;

		if(n>=LF_LAT && n<=LF_DATUM) {
			if(srcFld || pDefault) {
				//coordinates require a src field or a default
				bool bConflict=false;
				if(pDefault && n!=LF_ZONE && n!=LF_DATUM) {
					errMsg="Default coordinates must be assigned with a .LOCDFLT directive";
					goto _badExit;
				}
				switch(n) {
					case LF_LAT:
					{
						if(iFldY>=0 || iTypXY==1) { bConflict=true; break; }
						iFldY=numFlds; iTypXY=0;
						break;
					}
					case LF_LON:
					{
						if(iFldX>=0 || iTypXY==1) { bConflict=true; break; }
						iFldX=numFlds; iTypXY=0;
						break;
					}
					case LF_NORTH:
					{
						if(iFldY>=0 || iTypXY==0) { bConflict=true; break; }
						iFldY=numFlds; iTypXY=1;
						uFlags|=SHPD_UTMNORTH;
						break;
					}
					case LF_EAST:
					{
						if(iFldX>=0 || iTypXY==0) { bConflict=true; break; }
						iFldX=numFlds; iTypXY=1;
						uFlags|=SHPD_UTMEAST;
						break;
					}
					case LF_ZONE:
					{
						iFldZone=numFlds;
						uFlags|=SHPD_UTMZONE;
						if(pDefault) {
							iZoneDflt=atoi(pDefault);
							if(!iZoneDflt || abs(iZoneDflt)>60) {
								errMsg="Bad default zone number specified";
								goto _badExit;
							}
						}
						break;
					}
					case LF_DATUM:
					{
						iFldDatum=numFlds;
						if(!pDefault || strcmp(pDefault, "WGS84") && strcmp(pDefault, "NAD83") && strcmp(pDefault, "NAD27")) {
							errMsg="Field DATUM_, if specified, must be assigned either WGS84, NAD83, or NAD27";
							goto _badExit;
						}
						break;
					}
				}
				if(bConflict) {
					errMsg="Either Lat/Lon or UTM coordinates (not both types) can have a source";
					goto _badExit;
				}
			}
			else if(n>=LF_EAST && n<=LF_ZONE) {
				if(n==LF_EAST) uFlags|=SHPD_UTMEAST;
				else if(n==LF_NORTH) uFlags|=SHPD_UTMNORTH;
				else uFlags|=SHPD_UTMZONE;
			}
		}
		else if(n>0) {
			//must be a timestamp fld
			fld.F_Len=45;
			fld.F_Typ='C';
		}

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

		if(srcFld) {
			//Will use data in a source file field -
			CDaoFieldInfo fi;
			try {
				if(*srcFld=='#') {
					n=atoi(srcFld+1);
					if(n<1|| n>numDbFlds) n=0;
					else if(pRS) {
						pRS->GetFieldInfo(n-1, fi, AFX_DAO_SECONDARY_INFO);
						ASSERT(fi.m_nOrdinalPosition==n-1);
					}
				}
				else if(pRS) {
					pRS->GetFieldInfo(srcFld, fi, AFX_DAO_SECONDARY_INFO);
					n=fi.m_nOrdinalPosition+1;
					if(n<0 || n>numDbFlds) n=0;
				}
				else {
					for(n=0; n<numDbFlds; n++) if(!v_srcNames[n].CompareNoCase(srcFld)) break;
					n=(n==numDbFlds)?0:(n+1);
				}
			}
			catch(...) {
				n=0;
			}
			if(!n) {
					errMsg.Format("Field \"%s\" for %s was not found in source file",srcFld,fld.F_Nam);
					goto _badExit;
			}

			srcIdx=n-1;
			if(pRS && !TypesCompatible(fi.m_nType,fld.F_Typ) && strcmp(fld.F_Nam,"ZONE_")) {
					errMsg.Format("Field %s - data type %s in source isn't compatible with type %c",
						fld.F_Nam,CCrack::strFieldType(fi.m_nType),fld.F_Typ);
					goto _badExit;
			}
			numSrcFlds++;
		}

		numFlds++;
		v_fldDef.push_back(fld);
		v_srcIdx.push_back(srcIdx);
		v_fldScale.push_back(scale);
		v_srcDflt.push_back(CString(pDefault?pDefault:""));
	}

	if(typ!=CFG_ERR_EOF) {
		errMsg="A corrupted or ill-formed line was encountered.";
		goto _badExit;
	}

	fdef.Close();

	if(!numFlds) {
		AfxMessageBox("No fields were defined in the template.");
		return FALSE;
	}

	if(!numSrcFlds) {
		AfxMessageBox("No source fields were specified or found in the template.");
		return FALSE;
	}

	if(iTypXY<0 || iFldX<0 || iFldY<0 || iTypXY==1 && iFldZone<0) {
		AfxMessageBox("Fields in template are insufficient for determining coordinates.");
		return FALSE;
	}

	if((uFlags&SHPD_UTMFLAGS) && uFlags!=SHPD_UTMFLAGS) {
		AfxMessageBox("Fields EASTING_, NORTHING_, and ZONE_ are all required if UTM is used.");
		return FALSE;
	}

	if(!csFldKey.IsEmpty()) {
		for(int f=0; f<numFlds; f++) {
			DBF_FLDDEF &fd=v_fldDef[f];
			if(!csFldKey.Compare(fd.F_Nam) && fd.F_Dec==0 && (fd.F_Typ=='C' || fd.F_Typ=='N')) {
				iFldKey=f;
				break;
			}
		}
	}

	return TRUE;

_badExit:
	CMsgBox("Template scan aborted. File %s, Line %u:\n%s.",trx_Stpnam(pathName),fdef.LineCount(),(LPCSTR)errMsg);
	fdef.Close();
	return FALSE;
}
