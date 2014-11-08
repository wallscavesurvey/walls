#include "stdafx.h"
#include "WallsMap.h"
#include "mainfrm.h"
#include "ShpLayer.h"
#include "GPSPortSettingDlg.h"

#define GPS_NUMFLDS 5

#pragma pack(1)
static DBF_FLDDEF gpsFldDef[GPS_NUMFLDS] = //15*8 = 120 bytes
{
	{"FR_TIME"	 ,'C',8,0,0},
	{"TO_TIME"   ,'C',8,0,0},
	{"FR_ELEV"   ,'N',8,1,0},
	{"TO_ELEV"   ,'N',8,1,0},
	{"PAUSED"    ,'L',1,0,0}
};

struct GPS_DBFREC {
	BYTE delflg;
	char fr_time[8];
	char to_time[8];
	char fr_elev[8];
	char to_elev[8];
	char paused;
};
#pragma pack()

static int GPSCreateShp(LPSTR ebuf,LPCSTR shpPathName)
{
	*ebuf=0;

	CSafeMirrorFile cfShp,cfShpx;

	CFltRect extent(DBL_MAX,-DBL_MAX,-DBL_MAX,DBL_MAX);
	CFltRect extentz(DBL_MAX,-DBL_MAX,0,0);

	int nOutRecs=vptGPS.size()-1;
	if(nOutRecs<=0) {
		ASSERT(0);
		return -1;
	}

	//Polylines --

	//Get UTM zone based on last point --
	int iZone=GetZone(vptGPS.back());
	
	UINT sizRecHdr=sizeof(SHP_POLY_REC)-8; //section before numParts

	//Calculate extent --
	for(it_gpsfix it=vptGPS.begin();it!=vptGPS.end();it++) {
		CFltPoint fp(*it);
		//Convert to UTM (forcing to zone iZone) --
		VERIFY(geo_LatLon2UTM(fp,&iZone,geo_WGS84_datum));
		CShpLayer::UpdateExtent(extent,fp);
		double e(it->elev);
		if(e<extentz.l) extentz.l=e;  //Z
		if(e>extentz.r) extentz.r=e;
	}

	SHP_MAIN_HDR hdr;

	CShpLayer::InitShpHdr(hdr,CShpLayer::SHP_POLYLINEZ,sizeof(SHP_POLYLINE1_REC)*nOutRecs,extent,&extentz);

	LPCSTR pMsg="Could not create SHP/SHX components";

	if(!CShpLayer::FileCreate(cfShp,ebuf,shpPathName,SHP_EXT))
		goto _fail;

	if(!CShpLayer::FileCreate(cfShpx,ebuf,shpPathName,SHP_EXT_SHX)) {
		cfShp.Abort();
		_unlink(shpPathName);
		goto _fail;
	}

	*ebuf=0;

	try {
		cfShp.Write(&hdr,sizeof(hdr));
		hdr.fileLength=CShpLayer::FlipBytes(4*nOutRecs+sizeof(SHP_MAIN_HDR)/2);
		cfShpx.Write(&hdr,sizeof(hdr));
		long offset=100;
		nOutRecs=0;

		SHX_REC shxRec;
		SHP_POLYLINE1_REC shpRec;
		shpRec.typ=CShpLayer::SHP_POLYLINEZ;
		shpRec.len=shxRec.reclen=CShpLayer::FlipBytes(SHP_POLYLINE1_SIZCONTENT/2);
		shpRec.numParts=1;
		shpRec.numPoints=2;
		shpRec.Parts[0]=0;
		shpRec.Mmin=shpRec.Mmax=shpRec.M[0]=shpRec.M[1]=0.0;

		it_gpsfix it0=vptGPS.begin();
		CFltPoint fp0(*it0);
		geo_LatLon2UTM(fp0,&iZone,geo_WGS84_datum);

		for(it_gpsfix it1=it0+1;it1!=vptGPS.end();it1++,it0++) {
			CFltPoint fp1(*it1);
			geo_LatLon2UTM(fp1,&iZone,geo_WGS84_datum);
			shpRec.Xmin=min(fp0.x,fp1.x);
			shpRec.Xmax=max(fp0.x,fp1.x);
			shpRec.Ymin=min(fp0.y,fp1.y);
			shpRec.Ymax=max(fp0.y,fp1.y);
			shpRec.Zmin=min(it0->elev,it1->elev);
			shpRec.Zmax=max(it0->elev,it1->elev);
			shpRec.Points[0]=fp0;
			shpRec.Points[1]=fp1;
			shpRec.Z[0]=it0->elev;
			shpRec.Z[1]=it1->elev;
			shpRec.recno=CShpLayer::FlipBytes(++nOutRecs);
			cfShp.Write(&shpRec,sizeof(SHP_POLYLINE1_REC));
			shxRec.offset=CShpLayer::FlipBytes(offset/2);
			cfShpx.Write(&shxRec,sizeof(shxRec));
			offset+=sizeof(SHP_POLYLINE1_REC);
			fp0=fp1;
		}
		ASSERT((UINT)cfShp.GetPosition()==100+sizeof(SHP_POLYLINE1_REC)*nOutRecs);
		cfShp.Close();
		ASSERT((UINT)cfShpx.GetPosition()==100+8*nOutRecs);
		cfShpx.Close();
	}
	catch(...) {
		//Write failure --
		cfShp.Abort();
		_unlink(cfShp.GetFilePath());
		cfShpx.Abort();
		_unlink(cfShpx.GetFilePath());
		pMsg="Failure writing SHP/SHX components";
		goto _fail;
	}

	strcpy(trx_Stpext(strcpy(ebuf,shpPathName)),SHP_EXT_PRJ);
	CShpLayer::WritePrjFile(ebuf,0,iZone); //UTM WGS84

	return nOutRecs;

_fail:
	if(*ebuf) CMsgBox("%s - %s",pMsg,ebuf);
	else AfxMessageBox(pMsg);
	return -1;
}

int GPSExportLog(LPCSTR shpName)
{
	char ebuf[MAX_PATH];
	int nOutRecs=GPSCreateShp(ebuf,shpName);

	ASSERT(nOutRecs==vptGPS.size()-1);
		
	if(nOutRecs<0) return 0;

	//AfxGetMainWnd()->BeginWaitCursor();

	//Create DBF and PRJ components --
	CString cs;
	it_gpsfix it0=vptGPS.begin();

	CShpDBF db;
	LPSTR pExt=trx_Stpext(strcpy(ebuf,shpName));
	strcpy(pExt,SHP_EXT_DBF);
	if(db.Create(ebuf,GPS_NUMFLDS,gpsFldDef))
		goto _failDBF;

	GPS_DBFREC rec;
	rec.delflg=' ';

	for(it_gpsfix it1=it0+1;it1!=vptGPS.end();it1++,it0++) {
		cs.Format("%02u:%02u:%02u",it0->h,it0->m,it0->s);
		memcpy(rec.fr_time,(LPCSTR)cs,8);
		cs.Format("%02u:%02u:%02u",it1->h,it1->m,it1->s);
		memcpy(rec.to_time,(LPCSTR)cs,8);
		cs.Format("%8.1f",it0->elev);
		memcpy(rec.fr_elev,(LPCSTR)cs,8);
		cs.Format("%8.1f",it1->elev);
		memcpy(rec.to_elev,(LPCSTR)cs,8);
		rec.paused=it0->flag?'Y':'N';
		if(db.AppendRec(&rec)) {
			goto _failDBF;
		}
	}

	if(!db.Close()) {
		return nOutRecs;
	}

_failDBF:
	db.CloseDel();
	CShpLayer::DeleteComponents(ebuf);
	//AfxGetMainWnd()->EndWaitCursor();
	AfxMessageBox("Unable save log - error writing DBF component.");
	return 0;
}
