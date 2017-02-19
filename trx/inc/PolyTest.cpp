#include "stdafx.h"
#include "PolyTest.h"
//#include <dbfile.h>
//#include <trx_file.h>

#define POLY_FLD_NAME 1

BOOL ReadPolyShp(LPCSTR pathShp,)
{
	char pathbuf[MAX_PATH];
	it_stats pStats;

	strcpy(pathbuf,pathShp);
	strcpy(trx_Stpext(pathbuf),".dbf");

	CDBFile db;
	CFile cfShp;
	CFileException ex;
	SHP_POLY_REC pRec;
	SHP_PT_POLY ptPoly;

	bool bAllocCache=(dbf_defCache==NULL);

	if(bAllocCache && csh_Alloc((TRX_CSH far *)&dbf_defCache,4096,32)) {
		return FALSE;
	}

	if(db.Open(pathbuf,CDBFile::ReadOnly)) {
		goto _fail;
	}

	int lenFld=db.FldLen(POLY_FLD_NAME);
	if(!lenFld || lenFld>80)
		goto _fail;

	char *pName=(char *)db.FldPtr(POLY_FLD_NAME);
	UINT numRecs=db.NumRecs();
	UINT count=0;
	DWORD *co_idx;

	strcpy(trx_Stpext(pathbuf),".shp");
	if(!cfShp.Open(pathbuf,CFile::modeRead|CFile::shareDenyWrite,&ex)) {
		goto _fail;
	}

	SHP_POLY_HDR hdr;
	if(cfShp.Read(&hdr,sizeof(hdr))<sizeof(hdr) || FlipBytes(hdr.filecode)!= 9994 || hdr.shapetype!=5) {
		goto _fail;
	}

	int len;
#ifdef _DEBUG
	UINT ptTotal=0;
#endif

	try {
		m_ptPoly.reserve(numRecs);

		for(UINT i=1;i<=numRecs;i++) {
			if(cfShp.Read(&pRec,sizeof(SHP_POLY_REC))!=sizeof(SHP_POLY_REC))
				throw "";
			ASSERT(pRec.numParts==1);
#ifdef _DEBUG
			ptTotal+=pRec.numPoints;
#endif

			len=sizeof(int)+pRec.numPoints*sizeof(CFltPoint);
			ASSERT(FlipBytes(pRec.recno)==i);
			ASSERT(pRec.typ==5);

			if(!(ptPoly.poly=(SHP_POLY *)malloc(len)) ||
					cfShp.Read(ptPoly.poly,len)!=len)
				throw "";

			ptPoly.numPoints=pRec.numPoints;

			//Now see if county has features --
			if(db.Go(i)) throw "";
			for(len=lenFld;len && pName[len-1]==' ';len--);
#ifdef _FIXNAMES
			if(e=FixName(pName,len)) {
				db.Mark();
				if(e==2) len--;
			}
#endif
			m_csCounty.SetString(pName,len);
			ptPoly.pszCounty=strdup(m_csCounty);
			m_ptPoly.push_back(ptPoly);
			if(co_idx=trx_Blookup(TRX_DUP_NONE)) {
				pStats=co_stats.begin()+*co_idx;
				count++;
				ASSERT(!pStats->wShpRec);
				pStats->wShpRec=m_ptPoly.size();
			}
		}
	}
	catch(...) {
		ASSERT(FALSE);
		goto _fail;
	}

#ifdef _DEBUG
	ASSERT(count<=m_uCountysTTL);
	if(count!=m_uCountysTTL) {
		for(pStats=co_stats.begin();pStats!=co_stats.end();pStats++) {
			if(pStats->wShpRec==0) {
				count++;
			}
		}
	}
	ASSERT(count==m_uCountysTTL);
#endif

	return m_bCountyInit=TRUE;

_fail:
	if(bAllocCache)
		CDBFile::FreeDefaultCache();
	return FALSE;
}

