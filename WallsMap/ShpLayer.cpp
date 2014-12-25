#include "stdafx.h"
#include "WallsMap.h"
#include "mainfrm.h"
#include "resource.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "ShpLayer.h"
#include "UpdatedDlg.h"
#include "DbtEditDlg.h"
#include "ColorPopup.h"
#include "TabCombo.h"
#include <georef.h>
#include "KmlDef.h"
#include "DlgOptionsGE.h"
#include "EditLabel.h"
#include "ImageViewDlg.h"
#include "ShowShapeDlg.h"
#include "ShpDef.h"
#include "ExportShpDlg.h"
#include "DBGridDlg.h"
#include "GEDefaultsDlg.h"
#include "LayerSetSheet.h"
#include "QuadTree.h"
//#include "modeless.h"

#ifdef _USE_TRX
#include <trxfile.h>
#endif

#ifdef _DEBUG
BYTE *dbg_ptr=0;
#endif

char * CShpLayer::new_point_str="<new point>";
//#define LEN_NEW_POINT_STR 11 //in header

const LPCSTR CShpLayer::shp_ext[CShpLayer::EXT_COUNT]={".shp",".shx",".dbf",".dbt",".dbe",".prj",".tmpshp",".qpj",".sbn",".sbx"};
const LPCSTR CShpLayer::lpstrFile_pfx="<file://";

bool CShpLayer::m_bSelectedMarkerChg=false;

SHP_MRK_STYLE CShpLayer::m_mstyle_sD(RGB(255,255,0),RGB(0,0,0),FSF_HOLLOW,FST_CIRCLES,15,1.4f); //hollow yellow circles
SHP_MRK_STYLE CShpLayer::m_mstyle_hD(RGB(255,255,0),RGB(0,0,0),FSF_HOLLOW,FST_PLUSES,27,1.0f); //yellow crosses
SHP_MRK_STYLE CShpLayer::m_mstyle_s=CShpLayer::m_mstyle_sD;
SHP_MRK_STYLE CShpLayer::m_mstyle_h=CShpLayer::m_mstyle_hD;

LPCSTR CShpLayer::shp_typname[]={"Point","Polyline","Polygon","Multipoint",
								 "PointZ","PolylineZ","PolygonZ","MultipointZ",
								 "PointM","PolyLineM","PolygonM","MultipointM","Multipatch"};

int CShpLayer::shp_typcode[]={SHP_POINT,SHP_POLYLINE,SHP_POLYGON,SHP_MULTIPOINT,
							  SHP_POINTZ,SHP_POLYLINEZ,SHP_POLYGONZ,SHP_MULTIPOINTZ,
							  SHP_POINTM,SHP_POLYLINEM,SHP_POLYGONM,SHP_MULTIPOINTM,SHP_MULTIPATCH};

CString CShpLayer::m_csIconSingle;
CString CShpLayer::m_csIconMultiple;
bool CShpLayer::m_bChangedGEPref=false;
LPCSTR CShpLayer::pIconSingle="http://maps.google.com/mapfiles/kml/paddle/W.png";
LPCSTR CShpLayer::pIconMultiple="http://maps.google.com/mapfiles/kml/paddle/ltblu-diamond.png";
static const TCHAR szOptionsKML[] = _T("OptionsKML");
static const TCHAR szShpArgs[] = _T("Preferences");
static const TCHAR szSelMrk[] = _T("SelMrk");
static const TCHAR szHighMrk[] = _T("HighMrk");
static const TCHAR szIconSingle[] = _T("IconSingle");
static const TCHAR szIconMultiple[] = _T("IconMultiple");

void SHP_MRK_STYLE::ReadProfileStr(CString &s)
{
	int i;
	if(!s.IsEmpty() && (i=cfg_GetArgv((LPSTR)(LPCSTR)s,CFG_PARSE_ALL))>=6) {
		crMrk=atol(cfg_argv[0]);
		crBkg=atol(cfg_argv[1]);
		wFilled=(WORD)atol(cfg_argv[2]);
		wShape=(WORD)atol(cfg_argv[3]);
		wSize=(WORD)atol(cfg_argv[4]);
		fLineWidth=(float)atof(cfg_argv[5]);
		if(i>=8) {
			iOpacitySym=(WORD)atol(cfg_argv[6]);
			iOpacityVec=(WORD)atol(cfg_argv[7]);
		}
		else {
			iOpacitySym=iOpacityVec=100;
			wSize=2*wSize+1;
		}
	}
}

//globals used by ZipDlg and CompareShp()
SEQ_DATA seq_data;
LPCSTR seq_pstrval;
VEC_CSTR seq_vlinks;
int CALLBACK seq_fcn(int i)
{
	return _stricmp(seq_pstrval,seq_vlinks[i]);
}

//==================================================
//SHP_DBFILE -- File specific operations -- applicable to all docs

LPCSTR SHP_DBFILE::sLocflds_old[NUM_LOCFLDS]={"_LATITUDE","_LONGITUDE","_EASTING","_NORTHING","_ZONE","_DATUM","_UPDATED","_CREATED"};
LPCSTR SHP_DBFILE::sLocflds[NUM_LOCFLDS]={"LATITUDE_","LONGITUDE_","EASTING_","NORTHING_","ZONE_","DATUM_","UPDATED_","CREATED_"};
bool SHP_DBFILE::bEditorPrompted=false;
bool SHP_DBFILE::bEditorPreserved=false;

CString SHP_DBFILE::csEditorName;

int SHP_DBFILE::GetLocFldTyp(LPCSTR pNam) //static
{
	bool bOld=*pNam=='_';
	for(int i=0;i<NUM_LOCFLDS;i++) {
		if(!strcmp(pNam,bOld?sLocflds_old[i]:sLocflds[i]))
			return i+1;
	}
	return 0;
}

bool SHP_DBFILE::InitLocFlds()
{
	UINT nFlds=db.NumFlds();
	UINT count=0;

	if(shpTypeOrg==CShpLayer::SHP_POINT) {

		memset(locFldNum,0,NUM_LOCFLDS);

		for(UINT f=nFlds;f;f--) {
			int i=GetLocFldTyp(db.FldNamPtr(f));
			if(i>0 && !locFldNum[i-1]) {
				locFldNum[i-1]=(BYTE)f;
				count++;
			}
			if(count==NUM_LOCFLDS) break;
		}
		if(count && count<NUM_LOCFLDS) {
			if((locFldNum[LF_LON]==0)!=(locFldNum[LF_LAT]==0)) {
				locFldNum[LF_LON]=locFldNum[LF_LAT]=0;
				count--;
			}
			if((locFldNum[LF_EAST]==0)!=(locFldNum[LF_NORTH]==0)) {
				locFldNum[LF_EAST]=locFldNum[LF_NORTH]=0;
				count--;
			}
			if(locFldNum[LF_ZONE]) {
				if(!locFldNum[LF_EAST]) {
					locFldNum[LF_ZONE]=0;
					count--;
				}
			}
			else if(locFldNum[LF_EAST]) {
				locFldNum[LF_EAST]=locFldNum[LF_NORTH]=0;
				count-=2;
			}
		}
	}

	if(count) {
		//Can have DATUM_, UPDATED_, or _CREATED --
		locFldTyp.assign(nFlds,0);
		for(int i=0;i<NUM_LOCFLDS;i++) {
			int f=locFldNum[i];
			if(f) {
				locFldTyp[f-1]=i+1;
				count--;
				if(!count) break;
			}
		}
	}
	else {
		memset(locFldNum,0,NUM_LOCFLDS);
		locFldTyp.clear();
	}

	return !HasLocFlds() || NADZON_OK(iNad,iZone);
}

bool SHP_DBFILE::HasLocFlds()
{
	return (locFldNum[LF_LAT]!=0 || locFldNum[LF_ZONE]!=0 || locFldNum[LF_DATUM]!=0);
}

bool SHP_DBFILE::HasTimestamp(BOOL bUpdated)
{
	//bUpdated==1 (editing)  : return true iff LF_UPDATED present
	//bUpdated==2 (adding)   : return true iff either LF_UPDATED or LF_CREATED is present
	return (locFldNum[LF_UPDATED]!=0 || (bUpdated==2) && locFldNum[LF_CREATED]!=0);
}

bool SHP_DBFILE::GetTimestampName(SHP_DBFILE *pdbfile,BOOL bUpdating)
{
	// 0 - menu pref
	// 1 - editing
	// 2 - adding

	CUpdatedDlg dlg(pdbfile,bUpdating);
	return dlg.DoModal()==IDOK;
}

bool SHP_DBFILE::InitRecbuf()
{
	ASSERT(!pbuf);
	ASSERT(shpTypeOrg==CShpLayer::SHP_POINT || shpTypeOrg==CShpLayer::SHP_POINTZ || shpTypeOrg==CShpLayer::SHP_POINTM);
	ASSERT(db.Opened());

	if(!(pbuf=(LPBYTE)malloc(db.SizRec()))) {
		CMsgBox("Unexpected error opening %s. Out of memory.",db.FileName());
		return false;
	}
	return true;
}

void SHP_DBFILE::Clear()
{
	dbt.Close();
	free(pbuf);
	pbuf=NULL;
	delete pvxc;
	pvxc=NULL;
	delete pvxc_ignore;
	pvxc_ignore=NULL;
	delete pvxd;
	pvxd=NULL;
	lblfld=keyfld=pfxfld=bTypeZ=bHaveTmpshp=0;
	noloc_dir=0;
	if(col.order.size()) {
		VEC_INT().swap(col.order);
		VEC_INT().swap(col.width);
	}
	if(shpType==CShpLayer::SHP_POINT) {
		ASSERT(ppoly==NULL);
		if(locFldTyp.size()) {
			VEC_BYTE().swap(locFldTyp);
		}
		if(vxsrch.size()) {
			VEC_BYTE().swap(vxsrch);
		}
		VEC_BYTE().swap(vdbe);
	}
	else {
		free(ppoly);
		ppoly=NULL;
		if(pfShp) {
			pfShp->Close();
			delete pfShp;
			pfShp=NULL;
		}
	}
}

int SHP_DBFILE::Reopen(bool bReadOnly)
{
	ASSERT(bReadOnly!=db.IsReadOnly());

	if(!bReadOnly && shpType==CShpLayer::SHP_POINT && !InitRecbuf())
		return CShpDBF::ErrMem;

	char path[MAX_PATH];
	strcpy(path,db.PathName());

	VERIFY(!db.Close());

	int iRet=db.Open(path,bReadOnly?CShpDBF::ReadOnly:CShpDBF::ReadWrite);

_retry:

	if(iRet && !bReadOnly && !db.Open(path,CShpDBF::ReadOnly)) {
		if(shpType==CShpLayer::SHP_POINT) {
			free(pbuf);
			pbuf=NULL;
		}
		return CShpDBF::ErrShare;    //file remains read_only
	}

	if(!iRet && dbt.IsOpen()) {
		//Also toggle R/W status of DBT --
		if(bReadOnly) dbt.AppendFree();
		dbt.Close();
		if(!dbt.Open(path,bReadOnly?(CFile::modeRead|CFile::shareDenyNone):
				(CFile::modeReadWrite|CFile::shareDenyWrite))) {
			iRet++;
			if(!bReadOnly && !dbt.Open(path,CFile::modeRead|CFile::shareDenyNone)) {
				strcpy(trx_Stpext(path),".dbf");
				VERIFY(!db.Close());
				goto _retry;
			}
		}
	}

	if(iRet) {
		AfxMessageBox("Error reopening layer as read-only!");
		return CShpDBF::ErrMem;
	}

	if(bReadOnly && shpType==CShpLayer::SHP_POINT) {
		free(pbuf);
		pbuf=NULL;
	}

	return 0;
}

//=====================================================================
//SHP File operations --

CPtNode *CShpLayer::m_pPtNode=NULL;
BYTE *CShpLayer::m_pRecBuf=NULL;
UINT CShpLayer::m_uRecBufLen=0;
SHP_DBFILE CShpLayer::dbfile[NUM_SHP_DBFILES];
HFONT CShpLayer::m_hfFieldList=NULL;
WORD CShpLayer::m_nFieldListCharWidth;
VEC_DWORD CShpLayer::m_vGridRecs;
CShpLayer *CShpLayer::m_pGridShp;
bool CShpLayer::m_bShpReeditMsg=false;

UINT CShpLayer::m_uKmlRange=1000;
UINT CShpLayer::m_uIconSize=10;
UINT CShpLayer::m_uLabelSize=8;

VEC_SHPOPT vec_shpopt; //global used for text searches only

static bool bFromZip=false; //affects AddMemoLink() operation when called from GetMemoLinks() (used only when creating zip)

void CShpLayer::CancelMemoMsg(EDITED_MEMO &memo)
{
	CString title;
	CMsgBox(MB_ICONINFORMATION,"%s\n\nNOTE: Although you've changed the content of this memo field, without an "
		"editor's name your only option is to Cancel and discard any changes.",
		GetMemoTitle(title,memo));
}

LPCSTR CShpLayer::ShpTypName(int code)
{
	for(int i=0;i<SHP_NUMTYPES;i++) {
		if(shp_typcode[i]==code) return shp_typname[i];
	}
	return NULL;
}

/*
void CShpLayer::AppendCoordSystemOrg(CString &cs) const
{
	ASSERT(m_bConverted && (m_iNad==1 || m_iNad==2) && NadOrg()<3);
	CString datum=(m_iNadOrg==2)?"NAD27":(m_bWGS84?"WGS84":"NAD83");
	if(!NadOrg() || ZoneOrg()==-99) datum+=" (assumed)";
	if(m_iZoneOrg) {
		ASSERT(abs(m_iZoneOrg)<=60);
		cs.AppendFormat("UTM Zone %d%c %s",m_iZoneOrg,(m_iZoneOrg<0)?'S':'N',datum);
	}
	else {
		cs.AppendFormat("Lat/Long %s",datum);
	}
}
*/

void CShpLayer::AppendInfo(CString &cs) const
{
	AppendFileTime(cs,PathName(),m_pdbfile->dbt.IsOpen()?2:1);

	cs.AppendFormat("Type: ESRI %s Shapefile (%u records)\r\nProjection: ",
		ShpTypName(ShpTypeOrg()),m_nNumRecs);

	if(m_bConverted) {
		//AppendCoordSystemOrg(cs); 
		//vs
		ASSERT((m_iNad==1 || m_iNad==2) && NadOrg()<3);
		CString datum=(m_iNadOrg==2)?"NAD27":(m_bWGS84?"WGS84":"NAD83");
		if(!NadOrg() || ZoneOrg()==-99) datum+=" (assumed)";
		if(m_iZoneOrg) {
			ASSERT(abs(m_iZoneOrg)<=60);
			cs.AppendFormat("UTM Zone %d%c %s",m_iZoneOrg,(m_iZoneOrg<0)?'S':'N',datum);
		}
		else {
			cs.AppendFormat("Lat/Long %s",datum);
		}
		//
		cs+=" converted to ";
	}

	cs+=GetCoordSystemStr();
	if(!m_bConverted && (m_iNad!=NadOrg() || m_iZone!=ZoneOrg())) cs+=" assumed";
	cs+="\r\n";
	if(!m_bConverted && (m_iNad==3 || abs(m_iZone==99)))
		AppendProjectionRefInfo(cs);
}

void CShpLayer::Init()
{
	LOGFONT lf;
	memset(&lf,0,sizeof(lf));
	lf.lfHeight=-12; //same as 9 pts
	//lf.lfWidth=0;
	//lf.lfEscapement=0;
	//lf.lfOrientation=0;
	lf.lfWeight=FW_NORMAL;
	//lf.lfItalic=0;
	//lf.lfUnderline=0;
	//lf.lfStrikeOut=0;
	//lf.lfCharSet=ANSI_CHARSET; //==0
	//lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
	//lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
	lf.lfQuality=DRAFT_QUALITY; //DRAFT_QUALITY=1, PROOF_QUALITY=2
	lf.lfPitchAndFamily=FIXED_PITCH | FF_MODERN; //== 1 + 48 == 1 + (3<<4)
	strcpy(lf.lfFaceName,"Lucida Console");
	VERIFY(m_hfFieldList=::CreateFontIndirect(&lf));
	if(m_hfFieldList) {
		HFONT hfOld;
		TEXTMETRIC tm;
		CDC dc;
		dc.CreateCompatibleDC(NULL);
		VERIFY(hfOld=(HFONT)::SelectObject(dc.GetSafeHdc(),m_hfFieldList));
		dc.GetTextMetrics(&tm);
		::SelectObject(dc.GetSafeHdc(),hfOld);
		m_nFieldListCharWidth=(WORD)tm.tmAveCharWidth;
	}

	SHP_DBFILE::csEditorName=theApp.GetProfileString(szPreferences,szEditorName,"");
	SHP_DBFILE::bEditorPrompted=SHP_DBFILE::bEditorPreserved=!SHP_DBFILE::csEditorName.IsEmpty();

	CString s=theApp.GetProfileString(szPreferences,szOptionsKML);
	if(!s.IsEmpty()) {
		int i=cfg_GetArgv((LPSTR)(LPCSTR)s,CFG_PARSE_ALL);
		if(i>=2) {
			m_uKmlRange=(UINT)atol(cfg_argv[0]);
			m_uIconSize=(UINT)atol(cfg_argv[1]);
			if(i>2) m_uLabelSize=(UINT)atol(cfg_argv[2]);
		}
	}
	m_csIconSingle=theApp.GetProfileString(szPreferences,szIconSingle,pIconSingle);
	m_csIconMultiple=theApp.GetProfileString(szPreferences,szIconMultiple,pIconMultiple);

	s = theApp.GetProfileString(szPreferences,szSelMrk,NULL);
	m_mstyle_s.ReadProfileStr(s);

	s = theApp.GetProfileString(szPreferences,szHighMrk,NULL);
	m_mstyle_h.ReadProfileStr(s);
}

void CShpLayer::UnInit()
{
	//CShpDBF::FreeDefaultCache();
	ASSERT(!dbf_defCache);
	free(m_pRecBuf);
	m_pRecBuf=NULL;
	m_uRecBufLen=0;
	::DeleteObject(m_hfFieldList);

	if(m_bSelectedMarkerChg) {
		CString s;
		theApp.WriteProfileString(szPreferences,szSelMrk,m_mstyle_s.GetProfileStr(s));
		theApp.WriteProfileString(szPreferences,szHighMrk,m_mstyle_h.GetProfileStr(s));
	}
	if(m_bChangedGEPref) {
		CString s;
		s.Format("%u %u %u",m_uKmlRange,m_uIconSize,m_uLabelSize);
		theApp.WriteProfileString(szPreferences,szOptionsKML,s);
		theApp.WriteProfileString(szPreferences,szIconSingle,m_csIconSingle.Compare(pIconSingle)?(LPCSTR)m_csIconSingle:NULL);
		theApp.WriteProfileString(szPreferences,szIconMultiple,m_csIconMultiple.Compare(pIconMultiple)?(LPCSTR)m_csIconMultiple:NULL);
	}
	SafeRelease(&d2d_pStrokeStyleRnd);
	//d2d_pFactory object released by MFC
}

void CShpLayer::SetGEDefaults()
{
	CGEDefaultsDlg dlg(m_uKmlRange,m_uLabelSize,m_uIconSize,m_csIconSingle,m_csIconMultiple);
	if(IDOK==dlg.DoModal()) {
		if(m_uKmlRange!=dlg.m_uKmlRange || m_uIconSize!=dlg.m_uIconSize || m_uLabelSize!=dlg.m_uLabelSize) {
			m_uLabelSize=dlg.m_uLabelSize;
			m_uKmlRange=dlg.m_uKmlRange;
			m_uIconSize=dlg.m_uIconSize;
			m_bChangedGEPref=true;
		}
		if(m_csIconSingle.Compare(dlg.m_csIconSingle)) {
			m_csIconSingle=dlg.m_csIconSingle;
			m_bChangedGEPref=true;
		}
		if(m_csIconMultiple.Compare(dlg.m_csIconMultiple)) {
			m_csIconMultiple=dlg.m_csIconMultiple;
			m_bChangedGEPref=true;
		}
	}
}

CShpLayer::CShpLayer()
{
	m_pdbfile=NULL;
	m_bTrans=m_bDragExportPrompt=m_bFillPrompt=true;
	m_bConverted=m_bSearchExcluded=m_bDupEditPrompt=false;
	m_pvSrchFlds=&m_vSrchFlds;
	m_bEditFlags=0;
	#ifdef _DEBUG
	  dbg_ptr=&m_bEditFlags;
	#endif
	m_pQT=NULL;
	m_bQTdepth=QT_DFLT_DEPTH;
	m_bQTusing=0;
	m_uQTnodes=0; 
	m_uDelCount=0;
	m_nLblFld=0;
	m_crLbl=RGB(255,255,255);
	m_fTransImg[5]=-1.0;
	m_pKeyGenerator=m_pUnkName=m_pUnkPfx=NULL;
	ComputeInverseTransform();
}

//Virtual fcns --

CShpLayer::~CShpLayer()
{
	if(m_pdbfile && m_pdb && m_pdb->Opened()) {
		ASSERT(m_pdbfile->nUsers);
		CloseDBF();
	}
	FreeDupedStrings();

	free(m_pKeyGenerator);
	free(m_pUnkName);
	free(m_pUnkPfx);

	if(m_pvSrchFlds!=&m_vSrchFlds)
		delete m_pvSrchFlds;
	if(m_pQT) delete m_pQT;
}

int CShpLayer::LayerType() const
{
	return TYP_SHP;
}

#define SHP_FILECODE 9994
#define SHP_EXT_BORDERRATIO 0.005
#define SHP_EXT_MINLENGTH_M 1.0
#define SHP_EXT_MINLENGTH_D 0.00001

static CFltRect &InflateExtent(CFltRect &ext,double m)
{
	double d=(ext.r-ext.l)*SHP_EXT_BORDERRATIO;
	d=max(m,d);
	ext.l-=d; ext.r+=d;
	d=(ext.t-ext.b)*SHP_EXT_BORDERRATIO;
	d=max(m,d);
	ext.b-=d; ext.t+=d;
	return ext;
}

void CShpLayer::CloseDBF()
{
	//Called only from destructor!

	ASSERT(m_pdbfile->nUsers);

	if(IsGridDlgOpen()) {
		m_pdbfile->pDBGridDlg->Destroy();
		ASSERT(!m_pdbfile->pDBGridDlg);
	}

	ASSERT(!IsEditable() || m_pdbfile->pShpEditor==this);

	if(IsEditable()) {
		if(m_bEditFlags) {
			//Database records were updated
			SaveShp();
		}
		ASSERT(HasMemos()==m_pdbfile->dbt.IsOpen());
		m_pdbfile->dbt.AppendFree();
		m_pdbfile->pShpEditor=NULL;
		if(m_pdbfile->nUsers>1) {
			ASSERT(!m_pdb->IsReadOnly());
			VERIFY(!m_pdbfile->Reopen(true)); //also frees pbuf if point shapefile
		}
	}
	if(!(--m_pdbfile->nUsers)) {
		ASSERT(!m_pdbfile->pDBGridDlg);
		m_pdb->Close();
		m_pdbfile->Clear();
		ASSERT(!m_vdbe->size());
		if(m_pdbfile->shpType==SHP_POINT) {
			VEC_FLTPOINT().swap(m_fpt);
		}
		else {
			ASSERT(!m_fpt.size());
		}
	}
}

void CShpLayer::ResourceAllocMsg(LPCSTR pathName)
{
	CMsgBox("%s not opened: Could not allocate resources for %u records.",trx_Stpnam(pathName),m_nNumRecs);
}

BOOL CShpLayer::OpenDBE(LPSTR pathName,UINT uFlags)
{
	CFile cf;
	CFileException ex;

	try {
		m_vdbe->assign(m_nNumRecs,0);
	}
	catch(...) {
		ResourceAllocMsg(pathName);
		return FALSE;
	}

	bool bAssigned=false;

	ASSERT(m_vdbe->size()==m_nNumRecs);

	strcpy(trx_Stpext(pathName),SHP_EXT_DBE);
	if(cf.Open(pathName,CFile::modeRead|CFile::shareDenyWrite,&ex)) {
		UINT len=cf.Read(&(*m_vdbe)[0],m_nNumRecs);
		if(len) {
			if(len!=m_nNumRecs) {

				CMsgBox("NOTE: The size of %s is no longer correct for the shapefile. "
					"The edit flags will be cleared.",trx_Stpnam(pathName));

				if((uFlags&NTL_FLG_EDITABLE)) {
					cf.Close();
					_unlink(pathName);
				}
			}
			else {
				bAssigned=true;
				for(VEC_BYTE::iterator it=m_vdbe->begin();it!=m_vdbe->end();it++)
					(*it)&=~(SHP_EDITDEL|SHP_SELECTED|SHP_EDITSHP); //will set delete flags based on shp compoment
			}
		}
	}
	else {
		if(ex.m_cause!=CFileException::fileNotFound) {
			//Extremely unlikely to ever happen --
			CMsgBox("NOTE: File %s exists but can't be opened. The edit flags will be cleared.",pathName);
		}
	}
	if(!bAssigned) {
		memset(&(*m_vdbe)[0],0,m_nNumRecs);
	}
	return TRUE;
}

void CShpLayer::ClearDBE()
{
	//Called only when user clears edit flags --
	ASSERT((m_bEditFlags&~SHP_EDITCLR)==0);
	ASSERT(IsEditable() && ShpTypeOrg()==SHP_POINT);

	char buf[MAX_PATH];
	strcpy(trx_Stpext(strcpy(buf,PathName())),SHP_EXT_DBE);
	_unlink(buf);
	{
		LPBYTE pdbe=&(*m_vdbe)[0];
		for(UINT i=0;i<m_nNumRecs;i++,pdbe++) *pdbe&=(SHP_EDITDEL|SHP_SELECTED);  //keep deleted and selected flags!
	}
	m_bEditFlags&=~SHP_EDITCLR; //No need to update dbe until there are more edits
}

int	CShpLayer::DbfileIndex(LPCSTR pathName)
{
	for(int i=0;i<NUM_SHP_DBFILES;i++) {
		if(dbfile[i].nUsers) {
			if(!_stricmp(pathName,dbfile[i].db.PathName())) return i;
		}
	}
	return -1;
}

void CShpLayer::InitSrchFlds(UINT nLblFld)
{
	m_vSrchFlds.clear();
	if(nLblFld) m_vSrchFlds.push_back((BYTE)nLblFld);
}

BOOL CShpLayer::OpenDBF(LPSTR pathName,int shpTyp,UINT uFlags)
{
	m_pdbfile=NULL;
	int i=DbfileIndex(pathName);

	if(i>=0) {
		m_pdbfile=&dbfile[i];
		ASSERT(m_pdbfile->nUsers && shpTyp==m_pdbfile->shpTypeOrg);
		m_pdb=&m_pdbfile->db;
		ASSERT(m_pdb->Opened());
		m_vdbe=&m_pdbfile->vdbe;
		m_nNumRecs=m_pdb->NumRecs();
		m_iNad=m_iNadOrg=m_pdbfile->iNad;
		m_iZone=m_iZoneOrg=m_pdbfile->iZone;
		//ASSERT(m_iZone!=-99 && m_iZone!=99); //could be zero
		m_bProjected=m_iZone!=0;

		if(!(m_nLblFld=m_pdbfile->lblfld))
			VERIFY(SetLblFld(1));

		if(m_pdbfile->vxsrch.empty()) InitSrchFlds(m_nLblFld);
		else m_vSrchFlds=m_pdbfile->vxsrch;

		if(uFlags&NTL_FLG_EDITABLE) {
			if(m_pdbfile->pShpEditor) {
				if(!(uFlags&NTL_FLG_LAYERSET) || !m_bShpReeditMsg) {
					CMsgBox(MB_ICONINFORMATION,IDS_ERR_SHPREEDIT,pathName);
					m_bShpReeditMsg=true;
				}
				uFlags&=~NTL_FLG_EDITABLE;
			}
			else {
				//The file was opened as read-only -- let's try changing the mode!

				//A memo-enabled shapefile will always have the DBT component open
				//if it exists. If it doesn't exist we'll still allow the viewing
				//(but not editing) of non-memo fields.
				if(HasMemos() && !m_pdbfile->dbt.IsOpen())
						uFlags&=~NTL_FLG_EDITABLE;
				else {
					ASSERT(m_pdb->IsReadOnly());
					//Let's make this file editable if we can!
					if(i=m_pdbfile->Reopen(false)) { //not read only
						uFlags&=~NTL_FLG_EDITABLE;
						if(i!=CShpDBF::ErrShare) {
							//Remove this shapefile from all documents!?
							return FALSE;
						}
					}
					else m_pdbfile->pShpEditor=this;
				}
			}
		}
		m_pdbfile->nUsers++;
		m_uFlags=uFlags;
		return TRUE;
	}

	//===================================================================
    //Files not already open!
	for(int i=0;i<NUM_SHP_DBFILES;i++) {
		if(!dbfile[i].nUsers) {
			m_pdbfile=&dbfile[i];
			ASSERT(!m_pdbfile->ppoly && !m_pdbfile->vdbe.size() && !m_pdbfile->db.Opened() && !m_pdbfile->pfShp);
			m_pdbfile->shpTypeOrg=shpTyp;
			break;
		}
	}

	if(!m_pdbfile) {
		CMsgBox("File %s: Open failure - Too many open shapefiles",trx_Stpnam(pathName));
		return FALSE;
	}

	/*
	if(!dbf_defCache && csh_Alloc((TRX_CSH far *)&dbf_defCache,4096,32)) {
		AfxMessageBox("Cannot allocate shapefile cache");
		return FALSE;
	}
	*/

	m_pdb=&m_pdbfile->db;
	ASSERT(!m_pdb->Opened());
	m_vdbe=&m_pdbfile->vdbe;
	ASSERT(!m_vdbe->size());

	bool bReadOnly=(uFlags&NTL_FLG_EDITABLE)==0;
	bool bPoly=(shpTyp!=SHP_POINT && shpTyp!=SHP_POINTZ && shpTyp!=SHP_POINTM);

	int e=m_pdb->Open(pathName,bReadOnly?(CShpDBF::ReadOnly|CShpDBF::ForceOpen):(CShpDBF::ReadWrite|CShpDBF::ForceOpen));
	if(e) {
		if(e!=DBF_ErrShare && e!=DBF_ErrReadOnly) {
			//bad format or no memory? --
			CMsgBox("File: %s\n\nOpen failure - %s.",trx_Stpnam(pathName),m_pdb->Errstr(e));
			if(uFlags&NTL_FLG_LAYERSET)
					CLayerSet::m_bOpenConflict=-1;
			m_pdbfile=NULL;
			return FALSE;
		}

		if(!bReadOnly && !m_pdb->Open(pathName,CShpDBF::ReadOnly|CShpDBF::ForceOpen)) {
			uFlags&=~NTL_FLG_EDITABLE;
			bReadOnly=true;
			if(uFlags&NTL_FLG_LAYERSET)
					CLayerSet::m_bReadWriteConflict=true; //examine this only after successful open
		}
		else {
			//ReadOnly open failed --
			if(uFlags&NTL_FLG_LAYERSET)
					CLayerSet::m_bOpenConflict=1; //share conflict
			else
				CMsgBox("File: %s\n\nOpen failure - %s.", trx_Stpnam(pathName), m_pdb->Errstr(e));
			m_pdbfile=NULL;
			return FALSE;
		}
	}
	m_nNumRecs=m_pdb->NumRecs();

	if(!CheckFldTypes(pathName,uFlags)) {
		goto _fail;
	}

	XC_GetFldOptions(pathName);

	ASSERT(!m_nLblFld);
	if(!(m_nLblFld=m_pdbfile->lblfld) && !SetLblFld(1)) {
		CMsgBox("File %s: Open failure - Table must have at least one non-memo field",trx_Stpnam(pathName));
		goto _fail;
	}

	if(!bReadOnly && !bPoly && !m_pdbfile->InitRecbuf())
		goto _fail;


	if(m_pdbfile->vxsrch.empty()) InitSrchFlds(m_nLblFld);
	else m_vSrchFlds=m_pdbfile->vxsrch;

	ASSERT(!m_pdbfile->ppoly);

	if(HasMemos()) {
		//Open dbt file --
		CDBTFile &dbt=m_pdbfile->dbt;
		ASSERT(!dbt.IsOpen());
		if(!dbt.Open(pathName,bReadOnly?(CFile::modeRead|CFile::shareDenyNone):(CFile::modeReadWrite|CFile::shareDenyWrite)))
			goto _fail;
	}

	if(!bPoly && m_nNumRecs && !OpenDBE(pathName,uFlags))
		goto _fail;

	//Can't fail now --

	m_pdbfile->pShpEditor=bReadOnly?NULL:this;

	m_uFlags=uFlags;
	m_pdbfile->nUsers=1;

	//We will determine this from .shp file --
	m_pdbfile->shpType=SHP_NULL;
	//And these from PRJ file or coordinates
	m_pdbfile->iNad=0;
	m_pdbfile->iZone=-99;

	return TRUE;

_fail:
	m_pdb->Close();
	m_pdbfile->dbt.Close();
	free(m_pdbfile->pbuf);
	m_pdbfile->pbuf=NULL;
	m_pdbfile=NULL;
	return FALSE;
}

void CShpLayer::AllocateQT()
{
	ASSERT(ShpType()==SHP_POLYGON);
	m_pQT = new CQuadTree();
	if(!m_pQT->Create(this)) {
		delete m_pQT; m_pQT=NULL;
		m_bQTusing=0;
	}
	#ifdef QT_TEST
	else {
		if(ShpType()==SHP_POLYGON)
			m_pQT->QT_TestPoints("TSS All Types_2014-04-18");
		#ifdef _USE_QTFLG
		else 
			CMsgBox("Layer %s --\n\nDepth=%u "
				"Nodes=%u of %u Parts=%u spcN=%uK of %uK spcP=%uK",
				trx_Stpnam(PathName()),QTNODE::maxlvl,
				m_pQT->m_numNodes,m_pQT->m_maxNodes,m_pQT->m_numParts,(m_pQT->m_numNodes*28+512)/1024,
				(m_pQT->m_maxNodes*28+512)/1024,(m_pQT->m_numParts*4+512)/1024);
		#endif
	}
	#endif
}

void CShpLayer::UpdatePolyExtent()
{
	//Called from DBGridDlg::DeleteRecords after delete operation and CShpLayer::Init_ppoly
	if(m_nNumRecs-m_uDelCount==0) {
		m_extent.l=m_extent.t=m_extent.b=m_extent.r=0.0;
		return;
	}

	ASSERT(m_nNumRecs && m_pdbfile->pfShp && m_pdbfile->ppoly && m_ext.size()==m_nNumRecs);

	CFltRect extent(DBL_MAX,-DBL_MAX,-DBL_MAX,DBL_MAX);
	UINT nPts=0;
	SHP_POLY_DATA *ppoly=m_pdbfile->ppoly;

	for(it_rect it=m_ext.begin();it!=m_ext.end();it++,ppoly++) {
		if(ppoly->IsDeleted()) continue;
		nPts++;
		extent.l=min(extent.l,it->l);
		extent.r=max(extent.r,it->r);
		extent.t=max(extent.t,it->t);
		extent.b=min(extent.b,it->b);
	}
	ASSERT(nPts==m_nNumRecs-m_uDelCount);
	m_extent=extent;
	ASSERT(!m_pQT && !m_bQTusing || ShpType()==SHP_POLYGON );
	if(ShpType()==SHP_POLYGON) {
		if(m_pQT) {
			ASSERT(m_bQTusing);
			delete m_pQT; m_pQT=NULL;
		}
		if(m_bQTusing) {
			AllocateQT();
		}
	}
}

void CShpLayer::UpdateShpExtent(bool bPrj /*=true*/)
{
	// if bPrj, prjection status is already known --
	CFltRect extent(DBL_MAX,-DBL_MAX,-DBL_MAX,DBL_MAX);
	m_bNullExtent=false;
	UINT nPts=0;

	if(m_nNumRecs) {
		if(ShpType()==SHP_POINT) {
			ASSERT(m_vdbe->size()==m_nNumRecs);
			LPBYTE pdbe=&m_pdbfile->vdbe[0];
			for(it_fltpt it=m_fpt.begin();it!=m_fpt.end();it++,pdbe++) {
				if((*pdbe)&SHP_EDITDEL) continue;
				nPts++;
				extent.l=min(extent.l,it->x);
				extent.r=max(extent.r,it->x);
				extent.t=max(extent.t,it->y);
				extent.b=min(extent.b,it->y);
			}
		}
		else {
			extent=m_extent;
			nPts=m_nNumRecs;
		}
	}

	if(!nPts) {
		// if(!bPrj) we are opening a shapefile with unknown projection --
		if(!bPrj) m_bProjected=m_pDoc->IsProjected();
		//The extent is irrelevant unless this is the only shapefile in the layerset,
		//in which case it matters to CWallsMapView::InitFullViewWindow --
		m_bNullExtent=true;
		extent.l=extent.t=extent.b=extent.r=0;
	}

	m_extent=extent;

	if(!bPrj) {
		if(nPts) m_bProjected=!IsLatLongExtent();
		m_iZone=m_bProjected?-99:0;
	}

	if(m_bNullExtent || ShpType()==SHP_POINT) {
		InflateExtent(m_extent,m_bProjected?SHP_EXT_MINLENGTH_M:SHP_EXT_MINLENGTH_D);
	}

	if(!(m_uFlags&NTL_FLG_LAYERSET))
		m_pDoc->LayerSet().UpdateExtent();
}

BOOL CShpLayer::CheckFldTypes(LPCSTR pathName,UINT uFlags)
{
	DBF_pFLDDEF pFld=m_pdb->m_pFld;
	UINT nFlds=m_pdb->NumFlds();
	ASSERT(pFld);

	for(;nFlds;nFlds--,pFld++) {
		char typ=pFld->F_Typ;
		BYTE len=pFld->F_Len;
		BYTE dec=pFld->F_Dec;
		bool errorFlg=false,warnFlg=false,warnEdit=false;

		switch(typ) {
			//case 'C' : if(len>254) return FALSE;
			case 'C' : if(!len)
						   errorFlg=true; //limit of 254 has been documented, but we'll ignore it
					   break;

			case 'D' : if(len!=8)
						   errorFlg=true;
					   break;
			case 'L' : if(len!=1)
						   errorFlg=true;
					   break;
			case 'M' : if(len!=10)
						   errorFlg=true;
					   break;

			case 'N' :
			case 'F' : if(len<1 || (dec && dec>len-2))
						   errorFlg=true;
					   break;

			default  : errorFlg=true;;
		}

		if(errorFlg) {
			CMsgBox("File %s: Open failure - The format specification for attribute field %s (%c %u.%u) is incompatible "
				"with dBase conventions and not supported by this program",trx_Stpnam(pathName),pFld->F_Nam,typ,pFld->F_Len,pFld->F_Dec);
			return FALSE;
		}
		/*
		if(warnFlg && !(uFlags&NTL_FLG_LAYERSET)) {
			CString s;
			s.Format("File %s, field %s (%c %u.%u):\n\nNOTE: The type/length/decimals specification for this attribute field "
				"is outside the standard limits for dBase-III tables. (This message will not appear if the shapefile "
				"is opened as a layerset member.)",
				trx_Stpnam(pathName),pFld->F_Nam,typ,pFld->F_Len,pFld->F_Dec);
			if(warnEdit) {
				pFld->F_Dec=dec;
				s+="\nCAUTION: If you edit the field's contents, numbers will be formatted with a reduced precision.";
			}
			AfxMessageBox(s);
		}
		*/
	}
	return TRUE;
}

BOOL CShpLayer::SetLblFld(UINT nFld)
{
	UINT nFlds=m_pdb->NumFlds();
	if(!nFld || nFld>nFlds || m_pdb->FldTyp(nFld)=='M') {
		for(nFld=1;nFld<=nFlds;nFld++) {
			if(m_pdb->FldTyp(nFld)!='M') break;
		}
		if(nFld>nFlds) return FALSE;
	}
	m_nLblFld=nFld;
	return TRUE;
}

void CShpLayer::SetSrchFlds(const std::vector<CString> &vcs)
{
	//called only from CWallsMapDoc::OpenLayerSet()
	m_vSrchFlds.clear();
	if(m_bSearchable && vcs.size()) {
		for(std::vector<CString>::const_iterator it=vcs.begin();it!=vcs.end();it++) {
			UINT f=m_pdb->FldNum(*it);
			if(f) {
				m_vSrchFlds.push_back((BYTE)f);
			}
		}
	}
}

BOOL CShpLayer::GetSrchFlds(CString &fldnam) const
{
	UINT sz=m_vSrchFlds.size();
	if(!sz) return TRUE;
	if(sz==1 && m_nLblFld==(UINT)m_vSrchFlds[0]) return FALSE;
	for(UINT i=0;i<sz;i++) {
		LPCSTR p=m_pdb->FldNamPtr(m_vSrchFlds[i]);
		if(p) {
			fldnam+=" \"";
			fldnam+=p;
			fldnam+='\"';
		}
	}
	return TRUE;
}

BOOL CShpLayer::Open(LPCTSTR lpszPathName,UINT uFlags)
{
	char pathbuf[MAX_PATH];
	char eMsg[MAX_PATH];

	LPSTR fullpath=dos_FullPath(lpszPathName,dbf_ext);
	if(!fullpath) {
		ASSERT(FALSE);
		return FALSE;
	}

	char *pExt=trx_Stpext(strcpy(pathbuf,fullpath));
	ASSERT(strlen(pExt)==4);

	CFileMap *pfShp=new CFileMap(); //Same as CFile for now 
	CFileException ex;

	if(!pfShp->Open(pathbuf,CFile::modeRead|CFile::shareDenyWrite|CFile::osSequentialScan,&ex)) {
		if(uFlags&NTL_FLG_LAYERSET) {
			delete pfShp;
			CLayerSet::m_bOpenConflict=-1; //not found or could not be opened
			return FALSE;
		}
		ex.GetErrorMessage(pExt=eMsg,MAX_PATH);
		goto _failMsg;
	}

	SHP_MAIN_HDR hdr;
	if(pfShp->Read(&hdr,sizeof(hdr))<sizeof(hdr) || FlipBytes(hdr.fileCode)!=SHP_FILECODE) {
		pExt="Bad shapefile header";
		goto _failMsg;
	}

	bool bPoly=false;
	UINT contLenOrg=0;

	switch(hdr.shpType) {
		case SHP_POINT :
			contLenOrg=20;
			break;
		case SHP_POINTM :
			contLenOrg=28;
			break;
		case SHP_POINTZ :
			contLenOrg=36;
			break;
		case SHP_POLYGON :
		case SHP_POLYGONZ :
		case SHP_POLYGONM :
		case SHP_POLYLINE:
		case SHP_POLYLINEZ :
		case SHP_POLYLINEM :
			bPoly=true;
			if(!(uFlags&NTL_FLG_LAYERSET))
				uFlags&=~NTL_FLG_EDITABLE;
			uFlags|=(NTL_FLG_NONSELECTABLE+NTL_FLG_SEARCHEXCLUDED);
			break;
		default :
			goto _failMsgTyp;
	}

	if((uFlags&(NTL_FLG_EDITABLE|NTL_FLG_LAYERSET))==NTL_FLG_EDITABLE) {
		//Opened editable but not as part of an existing layerset --
		CFileStatus stat;
		try {
			pfShp->GetStatus(stat);
			if(stat.m_attribute&1) //1==CFile::readOnly)
				uFlags&=~NTL_FLG_EDITABLE;
		}
		catch(...) {
			uFlags&=~NTL_FLG_EDITABLE;
		}
	}

	strcpy(pExt,SHP_EXT_DBF);

	if(!OpenDBF(pathbuf,hdr.shpType,uFlags)) {
		goto _failClose;
	}

	//hdr.Ymin,hdr.Ymax are reversed wrt CFltRect order!
	if(m_nNumRecs) m_extent=CFltRect(hdr.Xmin,hdr.Ymax,hdr.Xmax,hdr.Ymin);

	if(!bPoly) {
		//point shapefile --

		m_pdbfile->shpType=SHP_POINT;

		try {
			m_fpt.reserve(IsEditable()?(m_nNumRecs+20):m_nNumRecs);
		}
		catch(...) {
			pExt="Not enough memory";
			goto _failMsg;
		}

		ASSERT(!m_uDelCount);
#ifdef _DEBUG
		UINT extraShpDels=0,extraDbfDels=0;
		CString sBadDeletes;
#endif
		UINT dataLen=(2*FlipBytes(hdr.fileLength)-sizeof(hdr));
		UINT nShpRecs=0;
		
		while(dataLen>=sizeof(SHP_POINT_REC)) {
			SHP_POINTZ_REC rec;
			if(pfShp->Read(&rec,sizeof(SHP_POINT_REC))!=sizeof(SHP_POINT_REC))
				goto _failMsgFmt;

			UINT contLen=FlipBytes(rec.length)*2; //record's content length in bytes

			if(contLen!=contLenOrg) {
#ifdef _DEBUG
				if(!nShpRecs) {
					*trx_Stpext(pathbuf)=0;
					CMsgBox("%s.shp --\n\nNOTE: Record content length of first record is %u. In a Point%s shapefile it should be %u.",
						pathbuf,contLen,(contLenOrg==20)?"":((contLenOrg==36)?"Z":"M"),contLenOrg);
				}
#endif
				if(nShpRecs || contLenOrg-contLen!=8 && contLenOrg-contLen!=16)
					goto _failMsgFmt;
				ASSERT(!m_pdbfile->bTypeZ || contLen==20+8*m_pdbfile->bTypeZ);
				m_pdbfile->bTypeZ=(BYTE)((contLen-20)/8);
				contLenOrg=contLen;
			}
			else if(!nShpRecs) m_pdbfile->bTypeZ=(BYTE)((contLenOrg-20)/8);

			if((contLen-=20) && pfShp->Read(&rec.z,contLen)!=contLen)
					goto _failMsgFmt;

			if(nShpRecs==m_nNumRecs) {
				*trx_Stpext(pathbuf)=0;
				CMsgBox("%s.shp--\n\nCAUTION: There are more records in the .shp component than there are attribute records. "
					"The additional records will be ignored.",pathbuf);
				break;
			}
			++nShpRecs;
			m_fpt.push_back(rec.fpt);
			dataLen-=(sizeof(SHP_POINT_REC)+contLen);

			if(rec.shpType!=hdr.shpType || rec.fpt.x<=-1.0e+39 || rec.fpt.y<=-1.0e+39) {
				#ifdef _DEBUG
					if(rec.shpType) {
						int xxx=0;
					}
					if(*m_pdb->RecPtr(nShpRecs)!='*') {
						extraShpDels++;
					}
				#endif
				(*m_vdbe)[nShpRecs-1]|=SHP_EDITDEL;
				m_uDelCount++;
			}
			#ifdef _DEBUG
			else {
				LPBYTE pDel=m_pdb->RecPtr(nShpRecs);
				if(*pDel=='*') {
					if(extraDbfDels<5 && !m_pdb->Go(nShpRecs)) {
						CString tssid;
						tssid.Format("%u",nShpRecs);
						if(extraDbfDels) sBadDeletes+=", ";
						sBadDeletes+=tssid;
					}
					extraDbfDels++;
				}
			}
			#endif
		}

		pfShp->Close();
		delete pfShp;
		pfShp=NULL;

		if(nShpRecs<m_nNumRecs) {
			goto _failMsgFmt;
		}

#ifdef _DEBUG
		if(extraShpDels || extraDbfDels) {
			*trx_Stpext(pathbuf)=0;
			CString msg;
			msg.Format("%s.shp\n\nNOTE: There were %u unmatched .shp deletions and %u unmatched .dbf deletions.\n"
				"In cases of mismatch the status of the delete flag in the .dbf will be ignored.",
				pathbuf,extraShpDels,extraDbfDels);

			if(extraDbfDels) msg.AppendFormat("\n\nFlagged recs: %s",(LPCSTR)sBadDeletes);
			MsgInfo(0, msg, "Format Note (Debug Only)");
		}
#endif

		if(!(m_uFlags&(NTL_FLG_MARKERS|NTL_FLG_LABELS))) {
			//Walls shapefile?
			if(m_pdb->NumFlds()>=6 && !strcmp(m_pdb->FldNamPtr(1),"PREFIX") &&	!strcmp(m_pdb->FldNamPtr(6),"NOTE")) {
					m_nLblFld=6;
			}
			m_uFlags|=NTL_FLG_MARKERS;
		}
		if(!m_nLblFld || m_nLblFld>m_pdb->NumFlds()) m_nLblFld=1;

		if(!(m_uFlags&NTL_FLG_LAYERSET)) {
			//This file was not opended as an existing NTL component.
			//Let's revise the marker style color --
			SetShapeClr(); //Use next available unused color
		}
	}
	else {
		m_pdbfile->pfShp=pfShp;
		pfShp=NULL;
		m_pdbfile->shpType=ShpTypeOrg();
		if(m_pdbfile->shpType==SHP_POLYLINEZ || m_pdbfile->shpType==SHP_POLYLINEM) m_pdbfile->shpType=SHP_POLYLINE;
		else if(m_pdbfile->shpType==SHP_POLYGONZ || m_pdbfile->shpType==SHP_POLYGONM) m_pdbfile->shpType=SHP_POLYGON;

		if(!(m_uFlags&NTL_FLG_LAYERSET)) {
			//This file was not opened as an existing NTL component.
			//Let's revise the polygon/polyline style defaults --
			SetShapeClr();
			m_mstyle.crBkg=RGB(127,127,127);
			m_mstyle.wFilled=0;
		}
	}

	bool bProj=true; //will indicate m_iNad and m_iZone has been calculated

	if(m_pdbfile->nUsers==1) {
		//Get projection info if it exists --
		strcpy(pExt,SHP_EXT_PRJ);
		bProj=GetPrjProjection(pathbuf);
	}

	//bPrj==true if m_iZone and m_iNad has been computed

	m_uFlags|=NTL_FLG_LAYERSET; //Do not refresh layerset's extent after update
	UpdateShpExtent(bProj); //if !bPrj we'll also need to initialize m_bProjected, m_iZone, and m_iNad
	m_uFlags&=~NTL_FLG_OPENONLY;

	if(m_pdbfile->nUsers==1) {
		m_pdbfile->iNad=m_iNad;
		m_pdbfile->iZone=m_iZone;
		if(!m_pdbfile->InitLocFlds()) {
			pExt="A shapefile with special location fields (e.g., LATITUDE_) requires a .prj component "
				"identifying a supported coordinate system";
			goto _failMsg;
		}
	}

	m_iNadOrg=m_iNad;
	m_iZoneOrg=m_iZone;

	m_pathName=_strdup(lpszPathName);

	UpdateSizeStr();

	if(m_bSearchable=(ShpType()==SHP_POINT)) {
		m_bSearchExcluded=IsSearchExcluded();
	}

	return TRUE;

_failMsgFmt:
	pExt="shp component is corrupt or truncated";
	goto _failMsg;

_failMsgTyp:
	switch(hdr.shpType) {
		case SHP_MULTIPOINT: pExt="Multipoint"; break;
		case SHP_MULTIPOINTM: pExt="MultipointM"; break;
		case SHP_MULTIPOINTZ: pExt="MultipointZ"; break;
		case SHP_MULTIPATCH: pExt="Multipatch"; break;
		default:
			pExt=GetIntStr(hdr.shpType);
	}
	sprintf(eMsg,"Shapefile type %s is unsupported in this version",pExt);
	pExt=eMsg;

_failMsg:
	*trx_Stpext(pathbuf)=0;
	{
		CString errstr;
		errstr.Format("Can't open %s.shp --\n\n%s.",pathbuf,pExt);
		AfxMessageBox(errstr);
	}

_failClose:
	if(pfShp) {
		if(pfShp->m_hFile!=INVALID_HANDLE_VALUE) pfShp->Close();
		delete pfShp;
	}
	return FALSE;
}

BOOL CShpLayer::FileCreate(CSafeMirrorFile &cf,LPSTR ebuf,LPCSTR pathName,LPCSTR pExt)
{
	CFileException ex;
	strcpy(trx_Stpext(strcpy(ebuf,pathName)),pExt);
	if(!cf.Open(ebuf,CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive,&ex)) {
		ex.GetErrorMessage(ebuf,MAX_PATH);
		return FALSE;
	}
	*ebuf=0;
	return TRUE;
}

void CShpLayer::InitShpHdr(SHP_MAIN_HDR &hdr,int typ,UINT sizData,const CFltRect &extent,const CFltRect *pextz)
{
	memset(&hdr,0,sizeof(SHP_MAIN_HDR));
	hdr.fileCode=SWAPLONG(SHP_FILECODE);
	hdr.version=1000;
	hdr.shpType=typ;
	//must revise the following before close if not SHP_POINT!!
	hdr.fileLength=FlipBytes((sizData+sizeof(SHP_MAIN_HDR))/2);
	hdr.Xmin=extent.l;
	hdr.Ymin=extent.b;
	hdr.Xmax=extent.r;
	hdr.Ymax=extent.t;
	if(pextz) {
		hdr.Zmin=pextz->l;
		hdr.Zmax=pextz->r;
		hdr.Mmin=pextz->b;
		hdr.Mmax=pextz->t;
	}
}

bool CShpLayer::EditFlagsCleared()
{
	//we've only cleared editflags for a few individual records
	LPBYTE pdbe=&(*m_vdbe)[0];
	for(UINT i=0; i<m_nNumRecs; i++, pdbe++) 
		if(*pdbe&(SHP_EDITDEL|SHP_EDITDBF|SHP_EDITLOC|SHP_EDITADD)) return false;
	return true;
}


BOOL CShpLayer::SaveShp()
{
	ASSERT(IsEditable() && m_bEditFlags && m_nNumRecs==m_pdb->NumRecs());

	char eMsg[MAX_PATH];
	char *pMsg=eMsg;
	*pMsg=0;


	if(m_bEditFlags&(SHP_EDITSHP|SHP_EDITADD|SHP_EDITDEL)) {
		ASSERT(m_nNumRecs && IsShpEditable());

		SHP_MAIN_HDR hdr;

		//modify only changed locations to avoid changes due only to CRS conversions
		CFile cfShp;
		CFileException ex;
		if(!cfShp.Open(PathName(),CFile::modeWrite,&ex)) { //CFile::shareExclusive is default
			ex.GetErrorMessage(eMsg,MAX_PATH);
			goto _fail;
		}

		{
			CFltRect ext(m_extent);
			if(m_bConverted) {
				ConvertPointsTo(&ext.tl,m_iNadOrg,m_iZoneOrg,2);
			}
			InitShpHdr(hdr,SHP_POINT,m_nNumRecs*sizeof(SHP_POINT_REC),ext,NULL);
		}

		try {
			cfShp.Write(&hdr,sizeof(hdr));

			SHP_POINT_REC rec;
			rec.length=FlipBytes(sizeof(SHP_POINT_XY)/2);
			LPBYTE pdbe=&m_pdbfile->vdbe[0];
			UINT nOutRecs=1;
			UINT offset=sizeof(hdr);

			for(it_fltpt it=m_fpt.begin();it!=m_fpt.end();it++,pdbe++,nOutRecs++,offset+=sizeof(rec)) {
				if(*pdbe&SHP_EDITSHP) {
					rec.recNumber=FlipBytes(nOutRecs);
					if(*pdbe&SHP_EDITDEL) {
						rec.shpType=SHP_NULL;
					}
					else {
						rec.shpType=SHP_POINT;
					}
					rec.fpt=*it;
					if(m_bConverted)
						ConvertPointsTo(&rec.fpt,m_iNadOrg,m_iZoneOrg,1);

					cfShp.Seek(offset,CFile::begin);
					cfShp.Write(&rec,sizeof(rec));
					*pdbe&=~SHP_EDITSHP;
				}
			}
			cfShp.Close();
		}
		catch(...) {
			cfShp.Abort();
			pMsg="Error updating SHP component";
			goto _fail;
		}

		if(m_bEditFlags&SHP_EDITADD) {
			CSafeMirrorFile cfShp;
			if(!FileCreate(cfShp,eMsg,m_pathName,SHP_EXT_SHX))
				goto _fail;
			SHX_REC shxRec;
			long offset=50;
			shxRec.reclen=FlipBytes(sizeof(SHP_POINT_XY)/2);

			hdr.fileLength=FlipBytes(4*m_nNumRecs+sizeof(SHP_MAIN_HDR)/2);

			try {
				cfShp.Write(&hdr,sizeof(hdr));
				for(UINT n=0;n<m_nNumRecs;n++) {
					shxRec.offset=FlipBytes(offset);
					offset+=sizeof(SHP_POINT_REC)/2;
					cfShp.Write(&shxRec,sizeof(shxRec));
				}
				cfShp.Close();
			}
			catch(...) {
				cfShp.Abort();
				pMsg="Error writing SHX component";
				goto _fail;
			}
		}
	}

	if(m_bEditFlags&(SHP_EDITSHP|SHP_EDITDBF|SHP_EDITADD|SHP_EDITDEL|SHP_EDITCLR)) {
		ASSERT(m_nNumRecs && m_pdbfile->pbuf);

		if(m_bEditFlags==SHP_EDITCLR && EditFlagsCleared()) {
			ClearDBE();
		}
		else {

			CSafeMirrorFile cfShp;

			if(!FileCreate(cfShp,eMsg,PathName(),SHP_EXT_DBE))
				goto _fail;

			ASSERT(m_nNumRecs); //Any edits require there be records

			try {
				cfShp.Write(&m_pdbfile->vdbe[0],m_nNumRecs);
				cfShp.Close();
			}
			catch(...) {
				cfShp.Abort();
				CFile::Remove(eMsg);
				pMsg="Error writing DBE component (edit flags)";
				goto _fail;
			}
		}
	}

	m_bEditFlags&=~(SHP_EDITDBF|SHP_EDITSHP|SHP_EDITADD|SHP_EDITDEL|SHP_EDITCLR);
	return TRUE;

_fail:
	CString errstr;
	errstr.Format("Error updating %s --\n\n%s.",FileName(),pMsg);
	AfxMessageBox(errstr);
	return FALSE;
}

void CShpLayer::UnInit_ppoly()
{
	if(m_pQT) {
		delete m_pQT; m_pQT=NULL;
	}
	if(m_ext.size()) {
		VEC_FLTRECT().swap(m_ext);
	}
	if(m_pdbfile->nUsers==1 && m_pdbfile->pfShp && m_pdbfile->ppoly) {
		free(m_pdbfile->ppoly);
		m_pdbfile->ppoly=NULL;
	}
}

SHP_POLY_DATA * CShpLayer::Init_ppoly()
{
	if(ShpType()==SHP_POINT) {
		ASSERT(0);
		return NULL;
	}
	if(m_ext.size()==m_nNumRecs) {
		ASSERT(!m_nNumRecs || m_pdbfile->ppoly);
		return m_pdbfile->ppoly;
	}

	ASSERT(!m_ext.size() && m_nNumRecs>0);

	SHP_POLY_DATA *ppoly=m_pdbfile->ppoly;

	bool bNeedInit=(ppoly==NULL);

	CFileMap &cf=*m_pdbfile->pfShp;

	if(bNeedInit) {
		ASSERT(cf.m_hFile!=INVALID_HANDLE_VALUE);

		if(!(ppoly=(SHP_POLY_DATA *)calloc(m_nNumRecs,sizeof(SHP_POLY_DATA)))) {
			ResourceAllocMsg(PathName());
			return NULL;
		}
		m_pdbfile->ppoly=ppoly;
	}

	UINT nextOff=100;
	m_uDelCount=0;

	try {
		m_ext.reserve(m_nNumRecs);

		for(UINT rec=0;rec<m_nNumRecs;rec++,ppoly++) {

			SHP_POLY_REC sRec;

			if(bNeedInit) ppoly->off=nextOff+sizeof(SHP_POLY_REC)-8;

			cf.Seek(nextOff,CFile::begin);
			if(cf.Read(&sRec,sizeof(SHP_POLY_REC))<sizeof(SHP_POLY_REC)) {
				if(m_nNumRecs==m_pdbfile->db.NumRecs()) //message not shown earlier
					CMsgBox("CAUTION: File %s is truncated. Only %u of %u poly%ss could be read.",FileName(),
						rec,m_nNumRecs,(ShpType()==SHP_POLYLINE)?"line":"gon");
				m_nNumRecs=rec;
				rec=m_pdbfile->db.NumRecs();
				break;
			}

			nextOff+=2*FlipBytes(sRec.len)+8;
			ASSERT(bNeedInit || ppoly->MaskedLen()==8+4*sRec.numParts+16*sRec.numPoints);
			if(bNeedInit) ppoly->SetLen(8+4*sRec.numParts+16*sRec.numPoints); //length of data needed for subsequent passes
			//File extent's Y coords are reversed wrt CFltRect order!
			m_ext.push_back(CFltRect(sRec.Xmin,sRec.Ymax,sRec.Xmax,sRec.Ymin));
			if(!sRec.typ || !sRec.numParts) { //Test for sRec.numParts needed for CAPCOG polylines
				ppoly->Delete();
				m_uDelCount++;
			}
			else {
				ASSERT(sRec.typ==ShpTypeOrg() && FlipBytes(sRec.recno)==rec+1);
			}
		}

		//update layer stats --
		UpdateSizeStr();
		if(hPropHook && m_pDoc==pLayerSheet->GetDoc())
				pLayerSheet->RefreshListItem(this);
						
		if(m_bConverted)
			ConvertPointsFromOrg(&m_ext[0].tl,2*m_ext.size());

		//Get layer's true extent and reallocate m_pQT if required--
		UpdatePolyExtent();
	}
	catch (...) {
		ASSERT(0);
		m_uDelCount=0;
		VEC_FLTRECT().swap(m_ext);
		if(bNeedInit) {
			free(m_pdbfile->ppoly);
			m_pdbfile->ppoly=NULL;
		}
	}

	return m_pdbfile->ppoly;
}

int CShpLayer::ConvertPointsTo(CFltPoint *pfpt,int iNad,int iZone,UINT iCount)
{
		ASSERT((iZone==199 || NADZON_OK(iNad,iZone)) && NADZON_OK(m_iNad,m_iZone));

	    int dst_datum=(iNad==2)?geo_NAD27_datum:geo_WGS84_datum;
		int src_datum=(m_iNad==2)?geo_NAD27_datum:geo_WGS84_datum;

		bool bFrom84=(dst_datum==geo_NAD27_datum); //if datum conversion necessary

		CFltPoint *pf=pfpt;

		if(iZone) {
			//The specified projection is UTM, convert from lat/long or from UTM in a different zone -- --
			if(!m_iZone) {
				//Convert Lat/Long to layer's datum if required --
				if(src_datum!=dst_datum) {
					//Convert from WGS84 to NAD27 if layerset's datum is geo_NAD27_datum --
					ASSERT(iNad*m_iNad==2);
					for(UINT i=iCount;i--;pf++)
						geo_FromToWGS84(bFrom84,&pf->y,&pf->x,bFrom84?dst_datum:src_datum);
				}
				//Convert Lat/Lon to UTM, while forcing zone to specified zone, but only if iZone!=99 --
				if(iZone==199) iZone=0;
			}
			else {
				//Both documents UTM, either zones or datums different.
				ASSERT(iZone!=199);
				ASSERT(m_iZone!=iZone || iNad*m_iNad==2);
				for(UINT i=iCount;i--;pf++)
					geo_UTM2LatLon(*pf,m_iZone,src_datum); //bug fixed 10/21/09! was iZone
				if(src_datum!=dst_datum) {
					pf=pfpt;
					for(UINT i=iCount;i--;pf++)
						geo_FromToWGS84(bFrom84,&pf->y,&pf->x,bFrom84?dst_datum:src_datum);
				}
				//Determine appropriate zone or force back to UTM with destination's zone --
			}
			pf=pfpt;
			for(UINT i=iCount;i--;pf++)
				geo_LatLon2UTM(*pf,&iZone,dst_datum);
		}
		else {
			//The specified projection is Lat/Long --
			if(m_iZone) {
				//Convert UTM to Lat/Long --
				for(UINT i=iCount;i--;pf++)
					geo_UTM2LatLon(*pf,m_iZone,src_datum);
				//and switch datums if required --
				if(src_datum==dst_datum) return iZone;
			}
			//else both systems are lat/lon
			pf=pfpt;
			for(UINT i=iCount;i--;pf++)
				geo_FromToWGS84(bFrom84,&pf->y,&pf->x,bFrom84?dst_datum:src_datum);
		}
		return iZone;
}

/*
void CShpLayer::ConvertPointFrom(CFltPoint &fpt,int iNad,int iZone)
{
	int zone=m_iZone;
	GetConvertedPoint(fpt,m_iNad,zone?&zone:NULL,iNad,iZone);
	ASSERT(m_iZone==(INT8)zone);

	m_iNad=iNad;
	m_iZone=iZone;
	ConvertPointsTo(&fpt,iNadSave,iZoneSave,1);
	m_iNad=iNadSave;
	m_iZone=iZoneSave;
}
*/

void CShpLayer::ConvertPointsFromOrg(CFltPoint *pt,int iCnt)
{
	int iNadSave=m_iNad;
	m_iNad=m_iNadOrg;
	int iZoneSave=m_iZone;
	m_iZone=m_iZoneOrg;
	ConvertPointsTo(pt,iNadSave,iZoneSave,iCnt);
	m_iNad=iNadSave;
	m_iZone=iZoneSave;
}

BOOL CShpLayer::ConvertProjection(int iNad,int iZone,BOOL bUpdateViews)
{
	//Convert datum to/from WGS84 alone
	//Convert projection to/from Lat/long alone
	//Convert datum and projection

	ASSERT(NADZON_OK(m_iNad,m_iZone));
	ASSERT(NADZON_OK(iNad,iZone));
	/*
	if(!m_iNad) {
		m_iNad=iNad;
		if(m_iZone==iZone) return bUpdateViews;
	}
	*/

	if(m_nNumRecs) {
		if(ShpType()==SHP_POINT) {
			//m_extent=CFltRect(DBL_MAX,-DBL_MAX,-DBL_MAX,DBL_MAX); //why?
			ConvertPointsTo(&m_fpt[0],iNad,iZone,m_fpt.size());

		}
		else {
			ConvertPointsTo(&m_extent.tl,iNad,iZone,2);
			if(m_ext.size())
				//convert extents
				ConvertPointsTo(&m_ext[0].tl,iNad,iZone,2*m_ext.size());
		}
		
		if(bUpdateViews) {
			bUpdateViews=FALSE;
			CConvertHint hint(this,iNad,iZone);
			m_pDoc->UpdateAllViews(NULL,LHINT_CONVERT,&hint);
		}
	}

	m_iNad=(INT8)iNad;
	m_iZone=(INT8)iZone;
	m_bProjected=iZone!=0;

	m_uFlags|=NTL_FLG_LAYERSET; //Do not refresh layerset's extent after update
	UpdateShpExtent();
	m_uFlags&=~NTL_FLG_LAYERSET; //Do not refresh layerset's extent after update
	m_bConverted=true;
	return bUpdateViews;
}

void CShpLayer::GetTooltip(UINT rec,LPSTR text)
{
	ASSERT(m_pdb);
	if(!m_pdb->Go(rec)) {
		LPCSTR pFld=(LPCSTR)m_pdb->FldPtr(m_nLblFld);
		if(pFld) {
			int len=m_pdb->FldLen(m_nLblFld);
			if(len>SHP_TOOLTIP_SIZ) {
				len=SHP_TOOLTIP_SIZ;
			}
			memcpy(text,pFld,len);
			while(len && xisspace(text[len-1])) len--;
			if(len==SHP_TOOLTIP_SIZ) {
				memcpy(text+(len-4),"...",4);
			}
			else {
				text[len]=0;
			}
			return;
		}
	}
	*text=0;
}

static bool vec_ptnode_pred(const MAP_PTNODE_PTR &pn1,const MAP_PTNODE_PTR &pn2)
{
	if(pn1->pLayer==pn2->pLayer) {
		//compare labels --
		BYTE buf1[256];
		CShpDBF *pdb=pn1->pLayer->m_pdb;
		UINT fld=pn1->pLayer->m_nLblFld;
		UINT len=pdb->FldLen(fld);
		PBYTE pLbl1=pdb->FldPtr(pn1->rec,fld);
		if(!pdb->IsMapped()) {
			pLbl1=(PBYTE)memcpy(buf1,pLbl1,len);
		}
		return _memicmp(pLbl1,pdb->FldPtr(pn2->rec,fld),len)<0;
	}
	//compare order in layerset --
	return pn1->pLayer->m_nLayerIndex>pn2->pLayer->m_nLayerIndex;
}

void CShpLayer::Sort_Vec_ptNode(VEC_PTNODE &vec_pn)
{
	std::sort(vec_pn.begin(),vec_pn.end(),vec_ptnode_pred);
}

static bool vec_shprec_pred(const SHPREC &pShp1,const SHPREC &pShp2)
{
	if(pShp1.pShp==pShp2.pShp) {
		//compare labels --
		BYTE buf1[256];
		CShpDBF *pdb=pShp1.pShp->m_pdb;
		UINT fld=pShp1.pShp->m_nLblFld;
		UINT len=pdb->FldLen(fld);
		PBYTE pLbl1=pdb->FldPtr(pShp1.rec,fld);
		if(!pdb->IsMapped()) {
			pLbl1=(PBYTE)memcpy(buf1,pLbl1,len);
		}
		return _memicmp(pLbl1,pdb->FldPtr(pShp2.rec,fld),len)<0;
	}

	return pShp1.pShp->m_nLayerIndex>pShp2.pShp->m_nLayerIndex;
}

void CShpLayer::Sort_Vec_ShpRec(VEC_SHPREC &vec_shprec)
{
	std::sort(vec_shprec.begin(),vec_shprec.end(),vec_shprec_pred);
}

int CShpLayer::FindInRecord(VEC_BYTE *pvSrchFlds,LPCSTR target,int lenTarget,WORD wFlags)
{
	//returns number of first field containing target, or zero.

	char buf[256];
	LPSTR pBuf;
	//VEC_SHPREC &vec_shprec=m_pDoc->m_vec_shprec;

	UINT ftyp=GET_FTYP(wFlags);

	for(VEC_BYTE::const_iterator it=pvSrchFlds->begin();it!=pvSrchFlds->end();it++) {
		if(m_pdb->FldTyp(*it)=='M') {
			UINT dbtrec=CDBTFile::RecNo((LPCSTR)m_pdb->FldPtr(*it));
			if(!lenTarget) {
				if(!dbtrec) {
					return 1; // field no. ignored except in calls from CDBGridDlg where lenTarget>0
				}
				continue;
			}
			if(!dbtrec) continue;

			UINT len=0; //no length limit!
			pBuf=m_pdbfile->dbt.GetText(&len,dbtrec);

			if(len && CDBTData::IsTextRTF(pBuf)) {
				len=CDBTFile::StripRTF(pBuf,FALSE); //emiminate '{' prefix also
			}
			if(!pBuf || len<(UINT)lenTarget) continue;
		}
		else {
			int len=m_pdb->GetRecordFld(*it,buf,256);
			if(len<lenTarget) continue;
			if(!lenTarget) {
				if(!len) {
					return 1;
				}
				continue;
			}
			pBuf=buf;
		}

		if(!(wFlags&F_MATCHCASE)) SetLowerNoAccents(pBuf);

		LPCSTR p=pBuf;
		while(p=strstr(p,target)) {
			if(wFlags&F_MATCHWHOLEWORD) {
				if(p>pBuf && !strchr(WORD_SEPARATORS,p[-1]) || !strchr(WORD_SEPARATORS,*(p+lenTarget))) {
					p+=lenTarget;
					while(*p && strchr(WORD_SEPARATORS,*p)) p++;
					if(!*p) break;
					continue;
				}
			}
			return *it;
		}
	}
	return 0;
}

BOOL CShpLayer::GetMatches(VEC_BYTE *pvSrchFlds,LPCSTR target,WORD wFlags,const CFltRect &frRect)
{
	if(!m_nNumRecs) return 0;

	LPBYTE pdbe=&m_pdbfile->vdbe[0];
	int lenTarget=strlen(target);
	ASSERT(lenTarget>=0 && lenTarget<=128);
	BOOL bRet=FALSE;

	UINT fTyp=GET_FTYP(wFlags);
	VEC_SHPREC &vec_shprec=CLayerSet::vec_shprec_srch;
	SHPREC sRec(this,0);

	if(fTyp>=FTYP_KEEPMATCHED) {
		//examine only records already selected --
		for(UINT rec=1;rec<=m_nNumRecs;rec++,pdbe++) {
			if(!(*pdbe&SHP_SELECTED)) continue;

			bool bRemove=false;
			if((wFlags&F_VIEWONLY) && !frRect.IsPtInside(m_fpt[rec-1])) {
				bRemove=(fTyp==FTYP_KEEPMATCHED);
			}

			if(!bRemove) {
				if(m_pdb->Go(rec)) {
					ASSERT(0);
					continue;
				}

				//bRemove=(0==FindInRecord(pvSrchFlds,target,lenTarget,wFlags));

				if(FindInRecord(pvSrchFlds,target,lenTarget,wFlags)) {
					bRemove=(fTyp==FTYP_KEEPUNMATCHED);
				}
				else {
					bRemove=(fTyp==FTYP_KEEPMATCHED);
				}
			}

			if(!bRemove) {
				sRec.rec=rec;
				vec_shprec.push_back(sRec);
				//no need for capacity check!
			}
			else bRet=TRUE; //we'll modify the selection
		}
	}
	else {
		//FTYP_SELMATCHED,FTYP_SELUNMATCHED,FTYP_ADDMATCHED,FTYP_ADDUNMATCHED -- not FTYP_KEEPMATCHED,FTYP_KEEPUNMATCHED
		UINT size=vec_shprec.size();

		for(UINT rec=1;rec<=m_nNumRecs;rec++,pdbe++) {
			if((*pdbe&SHP_EDITDEL) || fTyp>=FTYP_ADDMATCHED && (*pdbe&SHP_SELECTED) || 
				((wFlags&F_VIEWONLY) && !frRect.IsPtInside(m_fpt[rec-1]))) continue;

			if(m_pdb->Go(rec)) {
				ASSERT(0);
				continue;
			}
			bool bInsert=(fTyp==FTYP_SELUNMATCHED || fTyp==FTYP_ADDUNMATCHED);
			if(FindInRecord(pvSrchFlds,target,lenTarget,wFlags)) {
				bInsert=!bInsert;
			}

			if(bInsert) {
				sRec.rec=rec;
				vec_shprec.push_back(sRec);
				bRet=TRUE;
				if(++size>MAX_VEC_SHPREC_SIZE) break;
			}
		}
	}
	return bRet;
}

int CShpLayer::SelectEditedShapes(UINT &nMatches,UINT uFlags,BOOL bAddToSel,CFltRect *pRectView)
{
	if(!m_nNumRecs) return 0;

	SHPREC sRec(this,0);
	LPBYTE pdbe=&m_pdbfile->vdbe[0];

	UINT maxMatches=MAX_VEC_SHPREC_SIZE;
	if(bAddToSel) maxMatches-=app_pShowDlg->NumSelected();
    
	for(UINT rec=0;rec<m_nNumRecs;rec++) {
		BYTE flag=*pdbe++;
		if(flag&SHP_EDITDEL) {
			//If this record was deleted, include it only
			//if deleted records were requested
			flag&=~(SHP_EDITLOC|SHP_EDITADD|SHP_EDITDBF);
		}
		if(flag&SHP_EDITADD) {
			//Same for added records
			flag&=~(SHP_EDITLOC|SHP_EDITDBF);
		}
		if(!((BYTE)uFlags&flag) || (bAddToSel && (flag&SHP_SELECTED))) continue;

		if(!pRectView || pRectView->IsPtInside(m_fpt[rec])) {
			sRec.rec=rec+1;
			if(nMatches>=maxMatches) {
				return -1;
			}
			nMatches++;
			CLayerSet::vec_shprec_srch.push_back(sRec);
		}
	}
	return nMatches;
}

void CShpLayer::SetShapeClr()
{
	int idx=MainColorStartIdx();
	int nLayers=m_pDoc->NumLayers();
	COLORREF clr=CColorPopup::m_crColors[idx].crColor;
	for(int tries=CColorPopup::m_nNumColors;tries;tries--) {
		int n=0;
		for(;n<nLayers;n++) {
			PTL pLayer=m_pDoc->LayerPtr(n);
			if(pLayer->LayerType()==TYP_SHP && ((CShpLayer *)pLayer)->ShpType()==ShpType() &&
				((CShpLayer *)pLayer)->MainColor()==clr) break;
		}
		if(n==nLayers) break;
		if(++idx>=CColorPopup::m_nNumColors) idx=0;
		clr=CColorPopup::m_crColors[idx].crColor;
	}
	MainColor()=clr;
}

void CShpLayer::UpdateLayerWithPoint(UINT iPoint,CFltPoint &fpt0)
{

	CFltPoint fpt(fpt0);

	ASSERT(!IsEditable());

	if(m_bConverted) {
		int zone=m_iZone;
		GetConvertedPoint(fpt,m_iNad,zone?&zone:NULL,m_iNadOrg,m_iZoneOrg);
		//ConvertPointFrom(fpt,m_iNadOrg,m_iZoneOrg);
	}

	if(iPoint>=m_nNumRecs) {
		if(iPoint>m_nNumRecs) {
			ASSERT(0);
			return;
		}
		try {
			m_fpt.push_back(fpt);
			m_nNumRecs++;
			UpdateExtent(m_extent,fpt);
			m_pDoc->LayerSet().UpdateExtent();
		}
		catch(...) {
			ASSERT(0);
			return;
		}
	}
	else {
		m_fpt[iPoint]=fpt;
		UpdateShpExtent(); //Also updates layerset's extent
	}

	if(IsVisible()) {
		//For now let's assume the views need updating, otherwise we would have to
		//test both the original and new point's location via LHINT_TESTPOINT --
		m_pDoc->RefreshViews();
	}
}

void CShpLayer::UpdateDocsWithPoint(UINT iPoint)
{
	CFltPoint fpt=m_fpt[--iPoint];

	if(m_bConverted) {
		ConvertPointsTo(&fpt,m_iNadOrg,m_iZoneOrg,1);
	}

	CMultiDocTemplate *pTmp=CWallsMapApp::m_pDocTemplate;
	POSITION pos = pTmp->GetFirstDocPosition();
	while(pos) {
		CWallsMapDoc *pDoc=(CWallsMapDoc *)pTmp->GetNextDoc(pos);
		if(pDoc!=m_pDoc) {
			for(int n=pDoc->NumLayers();n--;) {
				PTL pLayer=pDoc->LayerPtr(n);
				if(pLayer->LayerType()==TYP_SHP	&& ((CShpLayer *)pLayer)->m_pdbfile==m_pdbfile) {
					((CShpLayer *)pLayer)->UpdateLayerWithPoint(iPoint,fpt);
					break;
				}
			}
		}
	}
}

DWORD CShpLayer::InitEditBuf(CFltPoint &fpt,DWORD rec)
{
	//Initialize a new ahapefile record in m_pdbfile->pbuf for potentially
	//appending to DBF. m_fpt[] and m_pdbfile->vdbe[] are extended by one.
	//m_nNumRecs is incremented to equal m_pdb->NumRecs()+1.

	ASSERT(IsEditable() && ShpTypeOrg()==SHP_POINT);
	if(!rec || m_pdb->Go(rec)) {
		ASSERT(!rec);
		rec=0;
		memset(m_pdbfile->pbuf,' ',m_pdb->SizRec());
	}
	else {
		//creating similar record --
		m_pdb->GetRec(m_pdbfile->pbuf);
		*m_pdbfile->pbuf=' '; //in case source is a selected deleted record
		for(UINT f=m_pdb->NumFlds();f;f--) {
			if(m_pdb->FldTyp(f)=='M' || GetLocFldTyp(f))
				memset(m_pdbfile->pbuf+m_pdb->FldOffset(f),' ',m_pdb->FldLen(f));
		}
	}

	if(HasXCFlds()) {
		XC_RefreshInitFlds(m_pdbfile->pbuf,fpt,rec!=0);
	}

	try {
		m_fpt.push_back(fpt);
		m_vdbe->push_back((BYTE)(SHP_EDITSHP|SHP_EDITDBF|SHP_EDITADD|SHP_SELECTED)); 
	}
	catch(...) {
		//unlikely lack of memory
		if((UINT)m_fpt.size()!=m_nNumRecs) {
			ASSERT((UINT)m_fpt.size()==m_nNumRecs+1);
			m_fpt.resize(m_nNumRecs);
		}
		ASSERT(m_fpt.size()==m_nNumRecs && m_vdbe->size()==m_nNumRecs);
		return 0;
	}
	m_nNumRecs++;
	ASSERT(m_fpt.size()==m_nNumRecs && m_vdbe->size()==m_nNumRecs);
	return m_nNumRecs;
}

bool CShpLayer::HasOpenRecord()
{
	return app_pShowDlg && app_pShowDlg->IsRecOpen(this);
}

bool CShpLayer::HasEditedText(CMemoEditDlg *pDlg)
{
	return pDlg && pDlg->m_pShp==this && pDlg->HasEditedText();
}

bool CShpLayer::HasEditedMemo()
{
	return HasEditedText(app_pDbtEditDlg) || HasEditedText(app_pImageDlg);
}

bool CShpLayer::HasPendingEdits()
{
	return (app_pShowDlg && app_pShowDlg->HasEditedRec(this)) || HasEditedMemo();
}

void CShpLayer::SetDlgEditable(CMemoEditDlg *pDlg,bool bEditable)
{
	if(pDlg && pDlg->m_pShp==this)
		pDlg->SetEditable(bEditable);
}

BOOL CShpLayer::SetEditable()
{
	//ASSERT(ShpType()==SHP_POINT);
	if(IsEditable()) {
		ASSERT(m_pdbfile->pShpEditor==this);
		if(HasOpenRecord() || HasEditedMemo())
		{
			ASSERT(0);
			//CMsgBox("%s\n\nThis shapefile cannot be set read-only while a record is currently being edited.",FileName());
			return FALSE;
		}
		if(m_bEditFlags) {
			//Database records were updated
			ASSERT(ShpTypeOrg()==SHP_POINT);
			SaveShp();
		}
		VERIFY(!m_pdbfile->Reopen(true)); //read only
		m_pdbfile->pShpEditor=NULL;
		m_uFlags&=~NTL_FLG_EDITABLE;
	}
	else {
		ASSERT(m_pdbfile->pShpEditor!=this);
		if(m_pdbfile->pShpEditor) {
			ASSERT(!m_pdb->IsReadOnly());
			CMsgBox("Layer %s cannot be set editable in %s while it's already editable in %s.",
				Title(),(LPCSTR)m_pDoc->GetTitle(),(LPCSTR)m_pdbfile->pShpEditor->m_pDoc->GetTitle());
			return FALSE;
		}

		if(HasMemos() && !m_pdbfile->dbt.IsOpen()) {
			CMsgBox("Shapefile %s is not editable because the associated .DBT component could not be opened.",FileName());
			return FALSE;
		}

		int i=m_pdbfile->Reopen(false); //not read only

		if(i) {
			ASSERT(i==CShpDBF::ErrShare);
			CMsgBox("Shapefile %s is not editable because it's either read-only or protected by another process.",FileName());
			free(m_pdbfile->pbuf);
			m_pdbfile->pbuf=NULL;
			return FALSE;
		}
		m_uFlags|=NTL_FLG_EDITABLE;
		m_pdbfile->pShpEditor=this;
	}

	SetDlgEditable(app_pDbtEditDlg,(m_uFlags&NTL_FLG_EDITABLE)!=0);
	SetDlgEditable(app_pImageDlg,(m_uFlags&NTL_FLG_EDITABLE)!=0);

	if(m_pdbfile->pDBGridDlg)
		m_pdbfile->pDBGridDlg->EnableAllowEdits();
	if(app_pShowDlg)
		app_pShowDlg->RefreshBranchTitle(this);

	return TRUE;
}

double CShpLayer::GetDouble(LPCSTR pSrc,UINT len)
{
	char buf[80];
	ASSERT(len<80);
	if(len>=80) return 0.0;
	memcpy(buf,pSrc,len);
	buf[len]=0;
	return atof(buf);
}

bool CShpLayer::StoreDouble(double x,int len,int dec,LPBYTE pDest)
{
	char buf[80];
	if(len<80 && _snprintf(buf, 80, "%*.*f", len, dec, x)==len) {
		memcpy(pDest,buf,len);
		return true;
	}
	return false;
}

bool CShpLayer::StoreExponential(double x,int len,int dec,LPBYTE pDest)
{
	char buf[80];
	if(len<80 && _snprintf(buf,80,"%*.*e",len,dec,x)==len) {
		memcpy(pDest,buf,len);
		return true;
	}
	return false;
}

bool CShpLayer::StoreText(LPCSTR s,int lenf,LPBYTE pDest)
{
	int lens=strlen(s);
	memcpy(pDest,s,min(lens,lenf));
	if(lens<lenf) memset(pDest+lens,' ',lenf-lens);
	return lenf>=lens;
}

void CShpLayer::StoreZone(int i,char typ,int len,LPBYTE pDest)
{
	//Allow character field?
	char buf[24];
	if(len>=2 && len<=8 && (typ=='C' || typ=='N')) {
		if(typ=='N') {
			if(len==2 && i<0) i=-i;
			if(_snprintf(buf,23,"%*d",len,i)!=len) return;
		}
		else {
			LPSTR s;
			if(len==2 && i<0) {
				i=-i;
				s="";
			}
			else s=(i>=0)?"N":"S";
			if((i=_snprintf(buf,len+1,"%d%s",i,s))<0) return;
			if(i<len) memset(buf+i,' ',len-i);
		}
		memcpy(pDest,buf,len);
	}
}

void CShpLayer::UpdateLocFlds(DWORD rec,LPBYTE pRecBuf)
{
	CFltPoint fpt(m_fpt[rec-1]);
	UINT f;

	//First, convert fpt to degrees in iNadOrg
	if(m_iZone || m_iNadOrg!=m_iNad) {
		ConvertPointsTo(&fpt,m_iNadOrg,0,1);
	}

	if((f=m_pdbfile->locFldNum[LF_LON])!=0) {
		StoreRecDouble(fpt.x,pRecBuf,f);
		StoreRecDouble(fpt.y,pRecBuf,m_pdbfile->locFldNum[LF_LAT]);
	}

	if((f=m_pdbfile->locFldNum[LF_ZONE])!=0) {
		//Convert to UTM with correct zone --
		int iNadSav=m_iNad;
		INT8 iZoneSav=m_iZone;
		m_iZone=0;
		INT8 iZone=ConvertPointsTo(&fpt,m_iNad=m_iNadOrg,199,1);
		m_iNad=iNadSav;
		m_iZone=iZoneSav;
		StoreRecZone(iZone,pRecBuf,f);
		StoreRecDouble(fpt.x,pRecBuf,m_pdbfile->locFldNum[LF_EAST]);
		StoreRecDouble(fpt.y,pRecBuf,m_pdbfile->locFldNum[LF_NORTH]);
	}
	if((f=m_pdbfile->locFldNum[LF_DATUM])!=0) {
		StoreRecText(DatumStr(m_iNadOrg),pRecBuf,f);
	}
}

BOOL CShpLayer::OpenLinkedDoc(LPCSTR pPath)
{
	char buf[MAX_PATH];
	//Assumes *p!=0 --
	if(!(pPath=GetFullFromRelative(buf,pPath))) return FALSE;

	CWallsMapApp::m_bNoAddRecent=TRUE;
	BOOL bRet=theApp.OpenDocumentFile(pPath)!=NULL;
	CWallsMapApp::m_bNoAddRecent=FALSE;
	return bRet;
}


void CShpLayer::DeleteComponents(LPCSTR pathName)
{
	//Delete all shapefile components --
	char pathBuf[MAX_PATH];

	LPSTR pExt=trx_Stpext(strcpy(pathBuf,pathName));
	for(int i=0;i<CShpLayer::EXT_COUNT;i++) {
		strcpy(pExt,CShpLayer::shp_ext[i]);
		_unlink(pathBuf);
	}
}

void CShpLayer::WritePrjFile(LPCSTR pathName,int iNad,int iZone)
{
	try {
		CFile file(pathName,CFile::modeCreate|CFile::modeWrite);
		CString s;
		if(!iZone) {
			s.LoadString(iNad?((iNad==1)?IDS_PRJ_NAD83:IDS_PRJ_NAD27):IDS_PRJ_WGS84GEO);
		}
		else {
			int iaZone=abs(iZone);
			ASSERT(iaZone<=60);
			s.Format(iNad?((iNad==1)?IDS_PRJ_NAD83UTM:IDS_PRJ_NAD27UTM):IDS_PRJ_WGS84UTM,iaZone,(iZone<0)?'S':'N',(iaZone<0)?10000000:0,-183+iaZone*6);
		}
		file.Write((LPCSTR)s,s.GetLength());
		file.Close();
	}
	catch(...) {
		AfxMessageBox("Note: A PRJ file could not be created.");
	}
}

BOOL CShpLayer::InitShpDef(CShpDef &shpdef,BOOL bUseLayout,bool bXLS/*=false*/)
{
	int nFlds=m_pdb->NumFlds();
	shpdef.numSrcFlds=nFlds;
	try {
		shpdef.v_fldIdx.resize(nFlds);
		shpdef.v_fldDef.resize(nFlds);
	}
	catch(...) {
		//Unlikely, but why not...
		AfxMessageBox("Out of memory.");
		return FALSE;
	}
	DBF_FLDDEF *pFld=&shpdef.v_fldDef[0];
	DBF_FLDDEF *pFld0=m_pdb->m_pFld;

	if(bUseLayout) {
		ASSERT(m_pdbfile->pDBGridDlg);
		m_pdbfile->pDBGridDlg->SaveShpColumns();
		for(int f=1;f<=nFlds;f++) {
			pFld[f-1]=pFld0[m_pdbfile->col.order[f]-1];
		}
	}
	else {
		memcpy(pFld,pFld0,nFlds*sizeof(DBF_FLDDEF));
	}

	int nfDst=0;
	int *pIdx=&shpdef.v_fldIdx[0];
	pFld0=pFld;
	
	for(int nF=1;nF<=nFlds;nF++,pFld++) {

		int nfSrc=nF;

		if(bUseLayout) {
			nfSrc=m_pdbfile->col.order[nF];
			if(!m_pdbfile->col.width[nfSrc])
				continue;
		}

		if(pFld->F_Typ=='M') {
			if(shpdef.uFlags&SHPD_EXCLUDEMEMOS) continue;
			shpdef.numMemoFlds++;
		}

		int n=bXLS?0:GetLocFldTyp(nfSrc);
		if(n) {
			n--;
			if(*pFld->F_Nam=='_')
				strcpy(pFld->F_Nam,SHP_DBFILE::sLocflds[n]); //use new form!

			if(n>=LF_LAT && n<=LF_DATUM) {
				n=-1-n;
				shpdef.numLocFlds++;
			}
			else {
				ASSERT(n==LF_UPDATED || n==LF_CREATED);
				n=nfSrc;
			}
		}
		else {
			n=nfSrc;
			if(!bXLS && XC_GetXComboData(n))
				shpdef.uFlags|=SHPD_COMBOFLDS;
		}
		*pIdx++=n;
		*pFld0++=*pFld;
		nfDst++;
	}
	if(shpdef.numMemoFlds==nfDst) {
		AfxMessageBox("An exported shapefie must contain at least one non-memo field.");
		return FALSE;
	}
	shpdef.numFlds=nfDst;
	return TRUE;
}

void CShpLayer::UpdateExtent(CFltRect &extent, const CFltPoint &fpt)
{
	if(fpt.x<extent.l) extent.l=fpt.x;
	if(fpt.x>extent.r) extent.r=fpt.x;
	if(fpt.y>extent.t) extent.t=fpt.y;
	if(fpt.y<extent.b) extent.b=fpt.y;
}

void CShpLayer::UpdateExtent(CFltRect &extent, const CFltRect &ext)
{
	if(ext.l<extent.l) extent.l=ext.l;
	if(ext.r>extent.r) extent.r=ext.r;
	if(ext.t>extent.t) extent.t=ext.t;
	if(ext.b<extent.b) extent.b=ext.b;
}

static void ComputeExtentFromPoints(CFltRect &ext,CFltPoint *pf,int count)
{
	ext.t=ext.r=-DBL_MAX;
	ext.b=ext.l=DBL_MAX;
	for(;count--;pf++) {
		if(pf->x<ext.l) ext.l=pf->x;
		if(pf->x>ext.r) ext.r=pf->x;
		if(pf->y<ext.b) ext.b=pf->y;
		if(pf->y>ext.t) ext.t=pf->y;
	}
}

#ifdef _DEBUG
#undef _USE_XFORM
#endif

#ifdef _USE_XFORM
static CFltPoint xf_p0(534196.34,3290656.21); //initial position of origen of tranformation
static CFltPoint xf_p1(534195.93,3290656.66); //final position of origen of tranformation
static double xf_scaleX=0.53022269353;
static double xf_scaleY=0.53022269353;
static double xf_sa=0.0; //sin of CCW rotation about origin
static double xf_ca=1.0; //cos of CCW rotation about origin
static void xf_trans(CFltPoint *pt,UINT cnt)
{
	double x,y;
	for(;cnt--;pt++) {
		x = pt->x - xf_p0.x;
		y = pt->y - xf_p0.y;
		pt->x = x * xf_scaleX * xf_ca - y *  xf_scaleY * xf_sa + xf_p1.x;
		pt->y = x * xf_scaleX * xf_sa + y *  xf_scaleY * xf_ca + xf_p1.y;
	}
}
#endif

#undef _USE_SHFT

#ifdef _USE_SHFT
static void xf_shft(CFltPoint *pt,UINT cnt)
{
	//Fix INEGI quad boundary longitudes at nearest 1/3 degree --
	for(; cnt--; pt++) pt->x=(int)(pt->x*3-0.5)/3.0;
}
#endif

int CShpLayer::CreateShp(LPSTR ebuf,LPCSTR shpPathName,VEC_DWORD &vRec,VEC_FLTPOINT &vFpt,const CShpDef &shpdef)
{
	LPCSTR pMsg="Error reading source SHP component!";
	*ebuf=0;

	CSafeMirrorFile cfShp,cfShpx;

	VEC_DOUBLE vFptz;
	CFltRect extent(DBL_MAX,-DBL_MAX,-DBL_MAX,DBL_MAX);
	CFltRect extentz(DBL_MAX,-DBL_MAX,-DBL_MAX,DBL_MAX);

	UINT sizRecData=0;
	UINT sizRecHdr=0;

	BOOL bTypeZ=0;
	switch(ShpTypeOrg()) {
		case SHP_POINTM :
		case SHP_POLYLINEM :
		case SHP_POLYGONM :
			bTypeZ=1; break;
		case SHP_POINTZ :
		case SHP_POLYLINEZ :
		case SHP_POLYGONZ :
			bTypeZ=2; break;
	}

	UINT sizRec;
	bool bUseViewCoord=(shpdef.uFlags&SHPD_USEVIEWCOORD)!=0;
	bool bWrite3D=(bTypeZ && !(shpdef.uFlags&SHPD_MAKE2D));

	UINT nRecs=vRec.size();
	int nOutRecs=0;

	if(nRecs) {
		try {
			if(ShpType()==SHP_POINT) {
				vFpt.reserve(nRecs);
				if(bWrite3D && m_pdbfile->bTypeZ) vFptz.reserve(m_pdbfile->bTypeZ*nRecs);
			}
			else {
				if(!Init_ppoly()) throw 0;
				if(shpdef.numLocFlds) vFpt.reserve(nRecs);
			}
		}
		catch(...) {
			pMsg="Out of memory";
			goto _fail;
		}

		if(ShpType()==SHP_POINT) {
			if(bWrite3D && m_pdbfile->bTypeZ) {
			   //Open original shp to obtain Z and M values!
			   if(!cfShp.Open(PathName(),CFile::modeRead|CFile::shareDenyWrite)) {
				   goto _fail;
			   }
			}
			
			//Original size for calculating Z or M offset --
			sizRec=sizeof(SHP_POINT_REC)+m_pdbfile->bTypeZ*sizeof(double);
			//actual record size written --
			sizRecHdr=sizeof(SHP_POINT_REC);
			if(bWrite3D) sizRecHdr+=bTypeZ*sizeof(double);

			LPBYTE pdbe=&m_pdbfile->vdbe[0];
			ASSERT(pdbe);

			for(UINT n=0;n<nRecs;n++) {
				int rec=vRec[n]-1;
				ASSERT(rec>=0);
				bool bDeleted=(pdbe[rec]&SHP_EDITDEL)!=0;
				if(!bDeleted || (shpdef.uFlags&SHPD_RETAINDELETED)) {
					if(bDeleted) {
						vRec[n]|=0x80000000;
					}
					CFltPoint fpt=m_fpt[rec];
					if(!bUseViewCoord && m_bConverted)
						ConvertPointsTo(&fpt,m_iNadOrg,m_iZoneOrg,1);
#if 0
					if(IsNotLocated(fpt)) {
						fpt.y=m_pdbfile->noloc_lat;
						fpt.x=m_pdbfile->noloc_lon;
					}
#endif
					vFpt.push_back(fpt);
					if(!bDeleted) UpdateExtent(extent,fpt);
					if(bWrite3D && m_pdbfile->bTypeZ) {
						//retrieve z and/or m
						//ASSERT(!bDeleted); ??? need an example
						try {
							UINT off=100+(rec+1)*sizRec-m_pdbfile->bTypeZ*sizeof(double);
							cfShp.Seek(off,CFile::begin);
							cfShp.Read(&fpt,m_pdbfile->bTypeZ*sizeof(double)); //M or Z or Z,M 
							vFptz.push_back(fpt.x); //Z or M
							if(m_pdbfile->bTypeZ==2)
								vFptz.push_back(fpt.y); //M
							if(!bDeleted) {
								if(bTypeZ==2) {
									if(fpt.x>extentz.r) extentz.r=fpt.x;
									if(fpt.x<extentz.l) extentz.l=fpt.x;
									if(m_pdbfile->bTypeZ==2) {
										if(fpt.y>extentz.t) extentz.t=fpt.y;
										if(fpt.y<extentz.b) extentz.b=fpt.y;
									}
								}
								else {
									if(fpt.x>extentz.t) extentz.t=fpt.x;
									if(fpt.x<extentz.b) extentz.b=fpt.x;
								}
							}
						}
						catch(...) {
						   cfShp.Close();
						   goto _fail;
						}
					}
					nOutRecs++;
				}
				else {
					CFltPoint fpt;
					vFpt.push_back(fpt); //bug fixed (was missing!) 11/5/2009
					if(bWrite3D && m_pdbfile->bTypeZ) {
						vFptz.push_back(fpt.x);
						if(m_pdbfile->bTypeZ==2) vFptz.push_back(fpt.y);
					}
					vRec[n]=0; //skip writing record
				}

			}
			if(bWrite3D && m_pdbfile->bTypeZ) cfShp.Close();
		}
		else {
			//Polylines or polygons --
			CFltRect zext;
			CFltPoint *pzfpt=NULL;
			if(bWrite3D)
				pzfpt=(bTypeZ==2)?&zext.tl:&zext.br;

			sizRecHdr=sizeof(SHP_POLY_REC)-8; //section before numParts
			ASSERT(m_pdbfile->ppoly && m_pdbfile->pfShp);
			CFileMap &cf(*m_pdbfile->pfShp);

			for(UINT n=0;n<nRecs;n++) {
				SHP_POLY_DATA &poly(m_pdbfile->ppoly[vRec[n]-1]);
				//poly.off == file offset to NumParts
				//poly.Len() == length in bytes of data to read starting at NumParts (high bit set if deleted)
				bool bDel=poly.IsDeleted();
				if(bDel) {
					if(!(shpdef.uFlags&SHPD_RETAINDELETED)) {
						vRec[n]=0;
						continue;
					}
					vRec[n]|=0x80000000;
				}
				sizRecData+=poly.MaskedLen(); //mask high bit if deleted! 
				nOutRecs++;
				CFltRect ext(m_ext[(vRec[n]&~0x80000000)-1]);
				if(!bUseViewCoord && m_bConverted) {
					ConvertPointsTo(&ext.tl,m_iNadOrg,m_iZoneOrg,2);
				}
#ifdef _USE_XFORM
				if(!m_bConverted && bUseViewCoord) { //handle this particular UTM case
					xf_trans(&ext.tl,2);
				}
#endif

#ifdef _USE_SHFT
				xf_shft(&ext.tl,2);
#endif
				if(!bDel) UpdateExtent(extent,ext);

				try {
					if(shpdef.numLocFlds) vFpt.push_back(ext.Center());
					if(bWrite3D) {
						UINT off=poly.off;
						SHP_POLY_NUMP nump;
						cf.Seek(off,CFile::begin);
						cf.Read(&nump,sizeof(SHP_POLY_NUMP));
						sizRecData+=bTypeZ*(nump.numPoints+2)*sizeof(double); //space for Z/M
						if(!bDel) {
							off+=(8+4*nump.numParts+16*nump.numPoints); //offset of Zmin,Zmax or Mmin,Mmax if bTypeZ==1
							//CFltPoint fpt;
							cf.Seek(off,CFile::begin);
							cf.Read(pzfpt,sizeof(CFltPoint));  //Z min,max in zext.tl OR  M min,max in zext.br

							//Initially: extentz(l=DBL_MAX,t=-DBL_MAX,r=-DBL_MAX,b-DBL_MAX);

							if(bTypeZ==2) {
								off+=(2+nump.numPoints)*sizeof(double);
								cf.Seek(off,CFile::begin);
								cf.Read(&zext.br,sizeof(CFltPoint)); //M
								if(zext.l<extentz.l) extentz.l=zext.l;  //Z
								if(zext.t>extentz.r) extentz.r=zext.t;
							}
							if(zext.r>extent.t) extentz.t=zext.r; //M
							if(zext.b<extent.b) extentz.b=zext.b;
						}
					}
				}
				catch(...) {
					goto _fail;
				}
			}
		}
 		sizRecData+=sizRecHdr*nOutRecs;
    }

	/*
	if(!nOutRecs) {
		ASSERT(!sizRecData);
		memset(&extent,0,sizeof(CFltRect));
		if(bWrite3D) memset(&extentz,0,sizeof(CFltRect));
	}
	*/

	SHP_MAIN_HDR hdr;
	int typFinal=ShpTypeOrg();
	if(bTypeZ) {
		if(!bWrite3D) {
			switch(typFinal) {
				case SHP_POINTZ :
				case SHP_POINTM :
					typFinal=SHP_POINT;
					break;
				case SHP_POLYLINEZ :
				case SHP_POLYLINEM :
					typFinal=SHP_POLYLINE;
					break;
				case SHP_POLYGONZ :
				case SHP_POLYGONM :
					typFinal=SHP_POLYGON;
					break;
			}
		}
		else if(bTypeZ==1) {
			ASSERT(m_pdbfile->bTypeZ==1);
			//M type only so clear Z extent --
			extentz.l=extentz.r=0.0;
		}
		else if(m_pdbfile->bTypeZ==1) {
			//Missing M type so clear M extent --
			extentz.t=extentz.b=0.0;
		}
	}

	InitShpHdr(hdr,typFinal,sizRecData,extent,(bWrite3D && m_pdbfile->bTypeZ)?&extentz:NULL);

	pMsg="Could not create SHP/SHX components";

	if(!FileCreate(cfShp,ebuf,shpPathName,SHP_EXT))
		goto _fail;

	if(!FileCreate(cfShpx,ebuf,shpPathName,SHP_EXT_SHX)) {
		cfShp.Abort();
		_unlink(shpPathName);
		goto _fail;
	}

	*ebuf=0;

	try {
		cfShp.Write(&hdr,sizeof(hdr));
		hdr.fileLength=FlipBytes(4*nOutRecs+sizeof(SHP_MAIN_HDR)/2);
		cfShpx.Write(&hdr,sizeof(hdr));
		SHX_REC shxRec;
		long offset=100;
		nOutRecs=0;
		if(ShpType()==SHP_POINT) {
			SHP_POINTZ_REC shpRec;
			shpRec.m=shpRec.z=0.0;
			shpRec.length=shxRec.reclen=FlipBytes((sizRecHdr-8)/2); //content length in words
			for(UINT n=0;n<nRecs;n++) {
				if(!vRec[n]) continue;
				shpRec.shpType=(vRec[n]&0x80000000)?NULL:typFinal;
				shxRec.offset=FlipBytes(offset/2);
				cfShpx.Write(&shxRec,sizeof(shxRec));
				shpRec.recNumber=FlipBytes(nOutRecs+1);
				if(bWrite3D && m_pdbfile->bTypeZ) {
					if(bTypeZ==1 || m_pdbfile->bTypeZ==1) {
						shpRec.z=vFptz[n];
					}
					else {
						shpRec.z=vFptz[2*n];
						shpRec.m=vFptz[2*n+1];
					}
				}
				shpRec.fpt=vFpt[n]; //was nOutRecs++
#if 0
					if(IsNotLocated(shpRec.fpt)) {
						shpRec.fpt.y=m_pdbfile->noloc_lat;
						shpRec.fpt.x=m_pdbfile->noloc_lon;
					}
#endif
				cfShp.Write(&shpRec,sizRecHdr);
				offset+=sizRecHdr;
				nOutRecs++;
			}
			ASSERT(vFpt.size()==nRecs);
			ASSERT((!bWrite3D || vFptz.size()==nRecs*m_pdbfile->bTypeZ) && (bWrite3D || !vFptz.size()));
		}
		else {
			SHP_POLY_REC shpRec;
			CFileMap &cf(*m_pdbfile->pfShp);
			ASSERT(m_pdbfile->ppoly && cf.m_hFile!=INVALID_HANDLE_VALUE);

			for(UINT n=0;n<nRecs;n++) {
				if(!vRec[n]) continue;
				SHP_POLY_DATA &poly(m_pdbfile->ppoly[(vRec[n]&~0x80000000)-1]);
				UINT polyLen=poly.MaskedLen();
				ASSERT(polyLen);
				int *parts=(int *)m_pdbfile->pbuf;
				if(!parts || polyLen>m_pdbfile->pbuflen) {
					parts=(int *)Realloc_pbuf(polyLen);
					if(!parts) {
						throw 0;
					}
				}
				cf.Seek(poly.off,CFile::begin);
				if(cf.Read(parts,polyLen)!=polyLen) {
					throw 0; //error
				}
				int numParts=*parts;
				int numPoints=*(parts+1);

				CFltRect ext(m_ext[(vRec[n]&~0x80000000)-1]);
				if(m_bConverted) {
					if(bUseViewCoord) {
						CFltPoint *pf=(CFltPoint *)&parts[numParts+2]; //first double past numParts+2 integers starting at part[0]
						ConvertPointsFromOrg(pf,numPoints);
						ComputeExtentFromPoints(ext,pf,numPoints); //extent may be different due to reprojection!
					}
					else {
						ConvertPointsTo(&ext.tl,m_iNadOrg,m_iZoneOrg,2);
					}
				}
#ifdef _USE_XFORM
				else if(bUseViewCoord) { //handle this particular UTM case
					CFltPoint *pf=(CFltPoint *)&parts[parts[0]+2];
					xf_trans(pf,parts[1]);
					ComputeExtentFromPoints(ext,pf,parts[1]); //extent may be different due to reprojection!
				}
#endif

#ifdef _USE_SHFT
				{ //handle this particular UTM case
					CFltPoint *pf=(CFltPoint *)&parts[parts[0]+2];
					xf_shft(pf,parts[1]);
					ComputeExtentFromPoints(ext,pf,parts[1]); //extent may be different due to reprojection!
				}

#endif

				//Shape extent's Y coords are reversed wrt CFltRect order!
				shpRec.typ=poly.IsDeleted()?0:typFinal;
				shpRec.Xmin=ext.l;
				shpRec.Ymin=ext.b;
				shpRec.Xmax=ext.r;
				shpRec.Ymax=ext.t;
				shpRec.recno=FlipBytes(++nOutRecs);

				UINT cont_len3D=0;
				UINT cont_len=polyLen + 4 /*typ*/ + 4*8 /*box*/;

				ASSERT(cont_len==polyLen+sizeof(SHP_POLY_REC)-4*sizeof(int));
				if(bWrite3D) {
					cont_len+=(cont_len3D=bTypeZ*(numPoints+2)*8);
				}

				shpRec.len=shxRec.reclen=FlipBytes(cont_len/2);
				cfShp.Write(&shpRec,sizeof(SHP_POLY_REC)-8); //write leading portion up to &shpRec.numParts
				cfShp.Write(parts,polyLen);
				if(bWrite3D) {
					ASSERT(parts==(int *)m_pdbfile->pbuf);
					if(cont_len3D>m_pdbfile->pbuflen) {
						parts=(int *)Realloc_pbuf(cont_len3D);
					}
					if(cf.Read(parts,cont_len3D)!=cont_len3D) {
						throw 0; //error
					}
					cfShp.Write(parts,cont_len3D);
				}
				shxRec.offset=FlipBytes(offset/2);
				cfShpx.Write(&shxRec,sizeof(shxRec));
				offset+=cont_len+2*sizeof(int);
			}
		}
		ASSERT((UINT)cfShp.GetPosition()==100+sizRecData);
		cfShp.Close();
		ASSERT((UINT)cfShpx.GetPosition()==100+8*nOutRecs);
		cfShpx.Close();
	}
	catch(...) {
		//Write faulure --
		cfShp.Abort();
		_unlink(cfShp.GetFilePath());
		cfShpx.Abort();
		_unlink(cfShpx.GetFilePath());
		pMsg="Failure writing SHP/SHX components";
		goto _fail;
	}
	return nOutRecs;

_fail:
	if(*ebuf) CMsgBox("%s - %s",pMsg,ebuf);
	else AfxMessageBox(pMsg);
	return -1;
}


#ifdef _DEBUG
#ifdef _FIX_TNC
static void GetTNCString(CShpDBF *pdb,CString &s,LPCSTR pNam)
{
	int i=pdb->FldNum(pNam);
	ASSERT(i);
	CString s0,sData;
	s.Empty();
	pdb->GetTrimmedFldStr(sData,i);
	if(sData.IsEmpty())
		return;
	if(!strcmp(pNam,"ACRES"))
		s0.Format("ACRES: %.2f\r\n",atof(sData));
	else if(!strcmp(pNam,"MGT_"))
		s0.Format("REC: %u (TNC, 2011-11-07)\r\n",atoi(sData)-1);
	else 
		s0.Format("%s: %s\r\n",pNam,(LPCSTR)sData);
	s+=s0;
}
#endif
#endif

BOOL CShpLayer::ExportShapes(LPCSTR shpName,CShpDef &shpdef,VEC_DWORD &vRec)
{
	if(m_bEditFlags && !SaveShp())
		return FALSE;

	char ebuf[MAX_PATH];
	VEC_FLTPOINT vFpt;
	int nOutRecs=CreateShp(ebuf,shpName,vRec,vFpt,shpdef);
		
	if(nOutRecs<0) return FALSE;

	AfxGetMainWnd()->BeginWaitCursor();

#ifdef _USE_TRX
	CTRXFile trx;
#endif

	CShpDBF db;
	CDBTFile dbt;
	LPCSTR pMsg=NULL;
	CString csCreatedTime[2]; // used if new CREATED_ and UPDATED_ fields are being added!
	bool bSwitched=false;

	if(shpdef.uFlags&SHPD_GENTEMPLATE) {
		CSafeMirrorFile cf;
		if(!FileCreate(cf,ebuf,shpName,SHP_EXT_TMPSHP) || !shpdef.Write(this,cf)) {
			//shpdef.Write will have called cf.Abort() and _unlink()
			pMsg="Error writing associated template component";
			goto _fail;
		}
	}

	//Create DBF,DBT, and PRJ components --

	UINT nFlds=shpdef.numFlds;
	UINT memoCnt=0;

	LPSTR pExt=trx_Stpext(strcpy(ebuf,shpName));

	bool bMemo=shpdef.numMemoFlds!=0;

	if(bMemo) {
		strcpy(pExt,SHP_EXT_DBT);
		if(!dbt.Create(ebuf)) {
			goto _failDBT;
		}
	}

	strcpy(pExt,SHP_EXT_DBF);
	if(db.Create(ebuf,nFlds,(DBF_FLDDEF *)&shpdef.v_fldDef[0]))
		goto _failDBF;

	bool  bUseViewCoord=(shpdef.uFlags&SHPD_USEVIEWCOORD)!=0;

	INT8 iNad=bUseViewCoord?m_iNad:m_iNadOrg;
	INT8 iZone=bUseViewCoord?m_iZone:m_iZoneOrg;
	INT8 iNadSav,iZoneSav;
	LPCSTR pDatumStr=DatumStr(iNad);

	if(nOutRecs) {
		if(shpdef.numLocFlds) {
			bSwitched=(m_iNad!=iNad || m_iZone!=iZone);
			if(bSwitched) {
				iZoneSav=m_iZone;
				m_iZone=iZone;
				iNadSav=m_iNad;
				m_iNad=iNad;
			}
		}
	}

    UINT nTruncRec,nTruncCnt=0;
	LPCSTR pTruncFld;

#ifdef _USE_TRX
	int fTSSID=0;
	char keyIndex[8];
	if(bUsingIndex) {
		fTSSID=m_pdb->FldNum("TSSID");
		if(!fTSSID || trx.Open("TSS_Protected") || trx.AllocCache(10)) {
			if(trx.Opened()) trx.Close();
			bUsingIndex=false;
		}
		else trx.SetExact();
	}
#endif

	for(UINT nR=0;nR<vRec.size();nR++) {
		UINT rec=vRec[nR];
		if(!rec) continue;
		if(m_pdb->Go(rec&0x7FFFFFFF) || db.AppendBlank()) 
			goto _failDBF;

#ifdef _USE_TRX
		if(bUsingIndex) {
			CString s;
			m_pdb->GetTrimmedFldStr(s,fTSSID);
			trx_Stccp(keyIndex,s);
			if(trx.Find(keyIndex)) *keyIndex=0;
		}
#endif
		ASSERT(!(rec&0x80000000) || IsRecDeleted(rec&0x7FFFFFFF)); //check polygons where IsRecDeleted()==false!!

		CString csPrefix;
		if(shpdef.iPrefixFld) {
			csPrefix.Format(shpdef.csPrefixFmt,nR+1); //will append srcfld value to this if non-empty
		}

		LPCSTR pSrc=(LPCSTR)m_pdb->RecPtr()+1;
		LPBYTE pDst=(LPBYTE)db.RecPtr();
		if(rec&0x80000000) *pDst++='*';
		else pDst++;

		CFltPoint latlon,utm;
		INT8 zone=iZone;
		if(shpdef.numLocFlds) {
			latlon=utm=vFpt[nR];
			if(iZone) {
				ConvertPointsTo(&latlon,iNad,0,1);
			}
			else {
				zone=ConvertPointsTo(&utm,iNad,199,1);
			}
		}

		DBF_FLDDEF *pFld=(DBF_FLDDEF *)&shpdef.v_fldDef[0];
		int *pIdx=(int *)&shpdef.v_fldIdx[0];
		for(UINT nF=0;nF<nFlds;nF++,pIdx++,pDst+=pFld->F_Len,pFld++) {
			bool bPrefix=(nF+1==shpdef.iPrefixFld);
			if(*pIdx>0) {
				//Copying from source database --
				int fSrc=*pIdx;
				VERIFY(pSrc=(LPCSTR)m_pdb->FldPtr(fSrc));
				int lenSrc=m_pdb->FldLen(fSrc);

#ifdef _DEBUG
#ifdef _FIX_TNC
				if(!strcmp(pFld->F_Nam,"TNCMEMO_")) {
					ASSERT(pFld->F_Typ=='M');
					CString s0,s;
					GetTNCString(m_pdb,s0,"NAME");
					GetTNCString(m_pdb,s,"OWNERSHIP"); s0+=s;
					GetTNCString(m_pdb,s,"SOURCE"); s0+=s;
					GetTNCString(m_pdb,s,"CE_HOLDER"); s0+=s;
					GetTNCString(m_pdb,s,"ACRES"); s0+=s;
					GetTNCString(m_pdb,s,"MGT_"); s0+=s;
					ASSERT(!s0.IsEmpty());
					UINT dbtRec=dbt.PutTextField(s0,s0.GetLength());
					if(dbtRec==(UINT)-1) goto _failDBT;
					CDBTFile::SetRecNo((LPSTR)pDst,dbtRec);
					memoCnt++;
					continue;
				}
#endif
#endif
				if(CShpDBF::IsFldBlank((LPBYTE)pSrc,lenSrc)) continue;

				if(pFld->F_Typ=='M') {
					UINT dbtRec;
					if(m_pdb->FldTyp(fSrc)=='M') {
						if((dbtRec=CDBTFile::RecNo(pSrc))) {
							if((dbtRec=dbt.AppendCopy(m_pdbfile->dbt,dbtRec))==(UINT)-1)
								goto _failDBT;
						}
					}
					else {
 						ASSERT(m_pdb->FldTyp(fSrc)=='C');
						if((dbtRec=dbt.PutTextField(pSrc,m_pdb->FldLen(fSrc)))==(UINT)-1)
							goto _failDBT;
					}
					if(dbtRec) {
						CDBTFile::SetRecNo((LPSTR)pDst,dbtRec);
						memoCnt++;
					}
				}
				else {
					if(!bPrefix && pFld->F_Len==lenSrc && pFld->F_Dec==m_pdb->FldDec(fSrc) && pFld->F_Typ==m_pdb->FldTyp(fSrc)) {
						memcpy(pDst,pSrc,lenSrc);
					}
					else {
						double dScaled;
						if(CShpDBF::IsNumericType(pFld->F_Typ)) {
							dScaled=GetDouble(pSrc,lenSrc);
							if(shpdef.uFlags&SHPD_ISCONVERTING)	dScaled*=shpdef.v_fldScale[nF];
						}
						switch(pFld->F_Typ) {
							case 'F' :
								StoreExponential(dScaled,pFld->F_Len,pFld->F_Dec,pDst);
								break;
							case 'N' :
								StoreDouble(dScaled,pFld->F_Len,pFld->F_Dec,pDst);
								break;

							case 'C' :
								if(m_pdb->FldTyp(fSrc)=='M') {
									ASSERT(lenSrc==10);
									lenSrc=0;
									pSrc=m_pdbfile->dbt.GetText((UINT *)&lenSrc,CDBTFile::RecNo((LPCSTR)pSrc));
								}
								else {
									if(bPrefix) {
										CString sTemp;
										sTemp.SetString(pSrc,lenSrc);
										sTemp.Trim();
										csPrefix+=sTemp;
										pSrc=(LPCSTR)csPrefix;
										lenSrc=csPrefix.GetLength();
									}
									else {
										if(m_pdb->FldTyp(fSrc)=='N') {
											for(;lenSrc && *pSrc==' ';pSrc++,lenSrc--);
										}
										else if(lenSrc>pFld->F_Len) {
											LPCSTR p=pSrc+pFld->F_Len;
											while(p>pSrc && xisspace(p[-1])) p--;
											lenSrc=p-pSrc;
										}
									}
								}
								if(!lenSrc) break;
								if(lenSrc>pFld->F_Len) {
									if(!nTruncCnt) {
										nTruncRec=nR+1;
										pTruncFld=pFld->F_Nam;
									}
									nTruncCnt++;
									lenSrc=pFld->F_Len;
								}
								memcpy(pDst,pSrc,lenSrc);

								break;

							case 'L' :
							case 'D' : ASSERT(FALSE);
								break;
						}
					}
				}
			}
			else if(*pIdx<FLDIDX_NOSRC) {
#ifdef _USE_TRX
				if(*pIdx==FLDIDX_INDEX) {
					if(bUsingIndex && *keyIndex) {
						*pDst='Y';
					}
				}
				else
#endif
				if(*pIdx<=FLDIDX_XCOMBO) {
					CString s;
					CFltPoint fpt=m_fpt[(rec&0x7FFFFFFF)-1];
					XCOMBODATA &xc=shpdef.v_xc[FLDIDX_XCOMBO-*pIdx];
					LPCSTR p=XC_GetInitStr(s,xc,fpt);
					if(p && *p) {
						if(xc.wFlgs&XC_INITELEV)
							StoreDouble(atof(p),pFld->F_Len,pFld->F_Dec,pDst);
						else {
							if(!XC_PutInitStr(p,pFld,pDst,&dbt))
								goto _failDBT;
							memoCnt++;
						}
					}
				}
				else if(*pIdx<=FLDIDX_TIMESTAMP) {
					int j=(FLDIDX_TIMESTAMP-*pIdx);
					ASSERT(FLDIDX_TIMESTAMP-*pIdx>=0);
					StoreText(shpdef.csTimestamps[FLDIDX_TIMESTAMP-*pIdx],pFld->F_Len,pDst);
				}
				#ifdef _DEBUG
				else ASSERT(0);
				#endif
			}
			else if(*pIdx==FLDIDX_NOSRC) {
				if(bPrefix) {
					//Create field containing record number only--
					StoreText(csPrefix,pFld->F_Len,pDst);
					pSrc=(LPCSTR)csPrefix;
				}
			}
			else if(*pIdx) {
				int typ=-*pIdx-1;
				if(typ==LF_DATUM) {
					StoreText(pDatumStr,pFld->F_Len,pDst);
				}
				else if(typ==LF_ZONE) {
					StoreZone(zone,pFld->F_Typ,pFld->F_Len,pDst);
				}
				else {
					double d=0.0;
					switch(typ) {
						case LF_LAT: d=latlon.y; break;
						case LF_LON: d=latlon.x; break;
						case LF_EAST: d=utm.x; break;
						case LF_NORTH: d=utm.y; break;
						default:
							ASSERT(0);
					}
					StoreDouble(d,pFld->F_Len,pFld->F_Dec,pDst);
				}
			}
		} //field loop
	} //record loop

#ifdef _USE_TRX
	if(bUsingIndex)
		trx.Close();
#endif

	if(bMemo) {
		if(!dbt.FlushHdr()) goto _failDBT;
		else dbt.Close();
	}

	ASSERT(nOutRecs==db.NumRecs());

	if(!db.Close()) {
		CString msg,s;
		*trx_Stpext(ebuf)=0;
		msg.Format("A shapefile set, %s, with %u records ",trx_Stpnam(ebuf),nOutRecs);
		if(bMemo) {
			s.Format("and %u non-empty memo fields ",memoCnt);
			msg+=s;
		}
		msg+="was successfully created.";
		if(shpdef.uFlags&SHPD_GENTEMPLATE) {
			msg+="\n\nA template component (extension .tmpshp) was also created ";
			if(shpdef.uFlags&(SHPD_COMBOFLDS|SHPD_DESCFLDS))
				msg+="since some of the fields have special editing options that WallsMap will recognize.";
			else
				msg+="as requested, even though none of the exported fields have special editing options.";
		}

		if(nTruncCnt) {
			s.Format("\n\nNOTE: %u fields have truncated text, the first being %s of record %u.",
				nTruncCnt,pTruncFld,nTruncRec);
			msg+=s;
		}

		AfxGetMainWnd()->EndWaitCursor();

		AfxMessageBox(msg);

		if(NADZON_OK(iNad,iZone)) {
			strcpy(ebuf+strlen(ebuf),SHP_EXT_PRJ);
			WritePrjFile(ebuf,(iNad==1 && m_bWGS84)?0:iNad,iZone);
		}
		if(bSwitched) {
			m_iNad=iNadSav;
			m_iZone=iZoneSav;
		}

		return TRUE;
	}

_failDBF:
	pMsg="Error writing DBF component";
	goto _fail;
_failDBT:
	pMsg="Error writing DBT component";

_fail:

#ifdef _USE_TRX
	if(bUsingIndex && trx.Opened()) trx.Close();
#endif

	if(bSwitched) {
		m_iNad=iNadSav;
		m_iZone=iZoneSav;
	}
	if(bMemo && dbt.IsOpen()) dbt.Close();
	db.CloseDel();

	AfxGetMainWnd()->EndWaitCursor();
	CMsgBox("Unable to create %s - %s.",trx_Stpnam(shpName),pMsg);
	DeleteComponents(strcpy(ebuf,shpName));
	return FALSE;
}

BOOL CShpLayer::ConfirmAppendShp(CShpLayer *pShpSrc,BYTE *pFldSrc)
{
	//Appending records from pShpSrc, where fields non't match.
	//Fill pFldSrc[NumFlds()] with source field numbers.

	CString sNoCopy,sInit,sTypDiff;
	BOOL bRet=1;
	bool bTrunc=false;
	//Construct pFldSrc[] plus list of source fields that won't be copied --
	int nFldDst=m_pdb->NumFlds(); //fields in this (destination) shapefile
	memset(pFldSrc,0,nFldDst+1);
	for(BYTE fsrc=1;fsrc<=pShpSrc->m_pdb->NumFlds();fsrc++) {
		BYTE fdst=m_pdb->FldNum(pShpSrc->m_pdb->FldNamPtr(fsrc));
		if(fdst) {
			ASSERT(!pFldSrc[fdst]);
			pFldSrc[fdst]=fsrc;
		}
		else {
			if(!sNoCopy.IsEmpty()) sNoCopy+=", ";
			sNoCopy+=pShpSrc->m_pdb->FldNamPtr(fsrc);
		}
	}
	//Construct list of destination fields that will be default initialized (including loc fields) --
	for(BYTE fdst=1;fdst<=nFldDst;fdst++) {
		if(!pFldSrc[fdst]) {
			int n=GetLocFldTyp(fdst);
			if(n>=LF_LAT && n<=LF_DATUM) bRet=2;
			if(!sInit.IsEmpty()) sInit+=", ";
			sInit+=m_pdb->FldNamPtr(fdst);
		}
		else if(memcmp(&pShpSrc->m_pdb->m_pFld[pFldSrc[fdst]-1],&m_pdb->m_pFld[fdst-1],sizeof(DBF_FLDDEF)-sizeof(WORD))) {
			if(!sTypDiff.IsEmpty()) sTypDiff+=", ";
			sTypDiff+=m_pdb->FldNamPtr(fdst);
			//Check for possible truncation --
			if(m_pdb->FldTyp(fdst)=='C' && (pShpSrc->m_pdb->FldTyp(pFldSrc[fdst])=='M' || m_pdb->FldLen(fdst)<pShpSrc->m_pdb->FldLen(pFldSrc[fdst])))
				bTrunc=true;
		}
	}
	if(!sInit.IsEmpty() || !sNoCopy.IsEmpty() || !sTypDiff.IsEmpty()) {
		CString msg,s;
		msg.Format("NOTE: The source and destination field structures are different.\n"
			"Source: %s\nDestination: %s\n\n",pShpSrc->Title(),Title());
		if(!sNoCopy.IsEmpty()) {
			s.Format("Fields missing in destination (not copied): %s\n\n",(LPCSTR)sNoCopy);
			msg+=s;
		}
		if(!sInit.IsEmpty()) {
			s.Format("Fields missing in source (default initialized): %s\n\n",(LPCSTR)sInit);
			msg+=s;
		}
		if(!sTypDiff.IsEmpty()) {
			s.Format("Fields requiring type conversion: %s\n",(LPCSTR)sTypDiff);
			if(bTrunc)
				s+="CAUTION: Truncation of field data might occur!\n\n";
			else s+='\n';
			msg+=s;
		}
		msg+="Proceed with copy operation?";

		if(IDYES!=AfxGetMainWnd()->MessageBox(msg,"CAUTION: Attribute Structure Mismatch",
			MB_ICONQUESTION|MB_YESNO))
			return 0;
	}
	return bRet;
}

UINT CShpLayer::CopyShpRec(CShpLayer *pSrcLayer,UINT uSrcRec,BYTE *pFldSrc,bool bUpdateLocFlds,TRUNC_DATA &td)
{
	//Called from CShowShapeDlg::CopySelectedItem() when dragging and dropping items
	//between different tree branches or when exporting a selected item via the context menu.

	ASSERT(CanAppendShpLayer(pSrcLayer));

	bool bDeleted=((*pSrcLayer->m_vdbe)[uSrcRec-1]&SHP_EDITDEL)!=0;

	try {
		m_fpt.push_back(pSrcLayer->m_fpt[uSrcRec-1]);
		ASSERT(m_fpt.size()==m_nNumRecs+1);

		m_vdbe->push_back(bDeleted?(SHP_EDITSHP+SHP_EDITDEL):(SHP_EDITSHP)); //shp revision needed
		if(bDeleted) m_uDelCount++;

		ASSERT(m_vdbe->size()==m_nNumRecs+1);

		pSrcLayer->m_pdb->Go(uSrcRec);

		ASSERT((*pSrcLayer->m_pdb->RecPtr()=='*')==bDeleted);

		bool bHasMemos;

		if(!pFldSrc) {
			//fields match --
			bHasMemos=HasMemos();
			if(m_pdb->AppendRec(pSrcLayer->m_pdb->RecPtr())) //pbuf not needed
				throw 0;
		}
		else {
			//else append blank record and copy identically named fields, posibly changing type and length
			bHasMemos=false;
			VERIFY(!m_pdb->AppendBlank());
			if(bDeleted) *m_pdb->RecPtr()='*';
			for(int f=1;f<=(int)m_pdb->NumFlds();f++) {
				BYTE fSrc=pFldSrc[f];
				if(fSrc) {
					BYTE *pSrcPtr=pSrcLayer->m_pdb->FldPtr(fSrc);
					BYTE *pDstPtr=m_pdb->FldPtr(f);
					DBF_pFLDDEF pDstDef=&m_pdb->m_pFld[f-1];
					DBF_pFLDDEF pSrcDef=&pSrcLayer->m_pdb->m_pFld[fSrc-1];
					UINT lenSrc=pSrcDef->F_Len;
					if(CShpDBF::IsFldBlank(pSrcPtr,lenSrc)) continue;
					//Some type changes are allowed --
					ASSERT(CShpDBF::TypesCompatible(pSrcDef->F_Typ,pDstDef->F_Typ));
					//ASSERT((pSrcDef->F_Typ==pDstDef->F_Typ) || (pSrcDef->F_Typ=='C' && pDstDef->F_Typ=='M') || (pSrcDef->F_Typ=='M' && pDstDef->F_Typ=='C'));
					if((pSrcDef->F_Typ==pDstDef->F_Typ)) {
						if(pSrcDef->F_Typ=='M') {
							bHasMemos=true;
						}
						else {
							//non-memo flds match wrt type --
							if(pSrcDef->F_Len==pDstDef->F_Len && pSrcDef->F_Dec==pDstDef->F_Dec) {
								//identical field format --
								memcpy(pDstPtr,pSrcPtr,pSrcDef->F_Len);
							}
							else {
								//either lengths or decimals differ --
								CString s;
								pSrcLayer->m_pdb->GetTrimmedFldStr(s,fSrc);
								if(s.GetLength()) {
									if(m_pdb->StoreTrimmedFldData(m_pdb->RecPtr(),s,f)) {
										if(!td.count++) {
											//store first truncation
											td.rec=m_pdb->NumRecs();
											td.fld=f;
										}
									}
								}
							}
						}
					}
					else {
						if(pSrcDef->F_Typ=='M') {
							//Converting 'M' to 'C' -- store first truncation
							UINT dbtRec=CDBTFile::RecNo((LPCSTR)pSrcPtr);
							if(dbtRec) {
								lenSrc=pDstDef->F_Len+1;
								LPSTR pStr=pSrcLayer->m_pdbfile->dbt.GetText(&lenSrc,dbtRec);
								lenSrc=strlen(pStr);
								if(lenSrc>pDstDef->F_Len) {
									lenSrc=pDstDef->F_Len;
									if(!td.count++) {
										td.rec=m_pdb->NumRecs();
										td.fld=f;
									}
								}
								memcpy(pDstPtr,pStr,lenSrc);
							}
						}
						else if(pDstDef->F_Typ=='M') {
							//Converting 'C' to 'M'
							CString s;
							s.SetString((LPSTR)pSrcPtr,pSrcDef->F_Len);
							s.Trim();
							if(s.GetLength()) {
								bHasMemos=true; //causes dbt hdr flush in next secion
								UINT dbtRec=m_pdbfile->dbt.PutTextField(s,s.GetLength());
								if(dbtRec==(UINT)-1) {
									//treat this as truncation
									if(!td.count++) {
										td.rec=m_pdb->NumRecs();
										td.fld=f;
									}
								}
								else if(dbtRec) {
									CDBTFile::SetRecNo((LPSTR)pDstPtr,dbtRec);
								}
							}
						}
						else {
							//Converting between 'C' and numeric --
							double dScaled;
							bool bTrunc=false;
							if(CShpDBF::IsNumericType(pDstDef->F_Typ)) {
								ASSERT(pSrcDef->F_Typ=='C');
								dScaled=GetDouble((LPCSTR)pSrcPtr,lenSrc);
							}
							switch(pDstDef->F_Typ) {
								case 'F' :
									if(!StoreExponential(dScaled,pDstDef->F_Len,pDstDef->F_Dec,pDstPtr))
										bTrunc=true;
									break;
								case 'N' :
									if(!StoreDouble(dScaled,pDstDef->F_Len,pDstDef->F_Dec,pDstPtr))
									   bTrunc=true;
									break;

								case 'C' :
									ASSERT(CShpDBF::IsNumericType(pSrcDef->F_Typ));
									for(;lenSrc && *pSrcPtr==' ';pSrcPtr++,lenSrc--);
									if(!lenSrc) break;
									if(lenSrc>pDstDef->F_Len) {
										lenSrc=pDstDef->F_Len;
										bTrunc=true;
									}
									memcpy(pDstPtr,pSrcPtr,lenSrc);
									break;

								default: ASSERT(0);
							}
							if(bTrunc && !td.count++) {
								td.rec=m_pdb->NumRecs();
								td.fld=f;
							}
						} //Converting between 'C' and numeric --
					} //types not same
				} //fSrc!=0 -- src fld present
			} //field loop
		} //fields don't match

		ASSERT(m_pdb->NumRecs()==m_nNumRecs+1);
		
		ASSERT(*m_pdb->RecPtr()==(bDeleted?'*':' ')); // 2012-11-13 removed

		m_nNumRecs=m_pdb->NumRecs();

		//fixed 2011-01-30 (adding to empty shp)
		if(!bDeleted) {
			if(m_nNumRecs-m_uDelCount>1)
				UpdateExtent(m_extent,m_fpt[m_nNumRecs-1]);
			else UpdateShpExtent();
		}

		if(bUpdateLocFlds) {
			UpdateLocFlds(m_nNumRecs,(LPBYTE)m_pdb->RecPtr());
		}

		if(bHasMemos) {
			CDBTFile &dbt=m_pdbfile->dbt;
			ASSERT(dbt.IsOpen());
			for(int e=1;e<=(int)m_pdb->NumFlds();e++) {
				BYTE fldSrc=pFldSrc?pFldSrc[e]:e;
				if(fldSrc && m_pdb->FldTyp(e)=='M' && pSrcLayer->m_pdb->FldTyp(fldSrc)=='M') {
					UINT dbtRec=CDBTFile::RecNo((LPCSTR)pSrcLayer->m_pdb->FldPtr(fldSrc));
					if(dbtRec && (dbtRec=dbt.AppendCopy(pSrcLayer->m_pdbfile->dbt,dbtRec))==(UINT)-1) {
						CMsgBox("Failed to copy data for memo field %s!",m_pdb->FldNamPtr(e)); 
						dbtRec=0;
					}
					CDBTFile::SetRecNo((LPSTR)m_pdb->FldPtr(e),dbtRec);
				}
			}
		}
	}
	catch(...) {
		AfxMessageBox("Copy failed: Could not append data!"); 
		m_fpt.resize(m_nNumRecs);
		m_vdbe->resize(m_nNumRecs);
		return 0;
	
	}

	return m_nNumRecs;
}

bool CShpLayer::SwapSrchFlds(VEC_BYTE &vSrchFlds)
{
	int n=vSrchFlds.size();
	if(n!=m_vSrchFlds.size() || n && memcmp(&vSrchFlds[0],&m_vSrchFlds[0],n)) {
		m_vSrchFlds.swap(vSrchFlds);
		return true;
	}
	return false;
}

void CShpLayer::GetTimestampString(CString &s,LPCSTR pName)
{
	//generate UTC timestamp --
	char ts_buf[64];
	struct tm tm_utc;
	__time64_t ltime;
	_time64(&ltime);
	if(ltime==-1 || _gmtime64_s(&tm_utc, &ltime)) *ts_buf=0;
	else GetTimestamp(ts_buf,tm_utc);

	ASSERT(strlen(ts_buf)==19);
	if(*ts_buf && pName && *pName) {
		ts_buf[19]=' ';
		int len=strlen(pName)+20;
		if(len>=sizeof(ts_buf)) len=sizeof(ts_buf)-1;
		memcpy(ts_buf+20,pName,len-20);
		ts_buf[len]=0;
	}
	s=ts_buf;
}

void CShpLayer::UpdateTimestamp(LPBYTE pRecBuf,BOOL bCreating)
{
	// bUpdating:
	// 0 - updating
	// 1 - creating

	if(!bCreating && bIgnoreEditTS) return;

	BYTE fc=m_pdbfile->locFldNum[LF_CREATED];
	BYTE fu=m_pdbfile->locFldNum[LF_UPDATED];
	if(!fc && !fu) return;

	CString s;
	GetTimestampString(s,SHP_DBFILE::csEditorName);
	
	if(fc && bCreating) StoreRecText(s,pRecBuf,fc);
	if(fu) StoreRecText(s,pRecBuf,fu); 
}

int CShpLayer::ConfirmSaveMemo(HWND hWnd, const EDITED_MEMO &memo,BOOL bForceClose)
{
	CString title,s;
	
	s.Format("%s\n\nYou've edited the content of this memo field.  Save changes before exiting?",
			  GetMemoTitle(title,memo));

	return MsgYesNoCancelDlg(hWnd,s,m_csTitle,"Save", "Discard Changes", bForceClose?NULL:"Continue Editing");
}

bool CShpLayer::IsMemoOpen(DWORD nRec,UINT nFld)
{
	//if the particular memo is already open, set focus to the appopriate dialog and return true
	CMemoEditDlg *pDlg=NULL;
	if(app_pDbtEditDlg && app_pDbtEditDlg->IsMemoOpen(this,nRec,nFld)) {
		pDlg=app_pDbtEditDlg;
		ASSERT(!app_pImageDlg || !app_pImageDlg->IsMemoOpen(this,nRec,nFld));
	}
	else if(app_pImageDlg && app_pImageDlg->IsMemoOpen(this,nRec,nFld)) {
		pDlg=app_pImageDlg;
	}
	if(pDlg) {
		pDlg->BringWindowToTop();
		return true;
	}
	return false;
}

bool CShpLayer::OpenMemo(CDialog *pParentDlg,EDITED_MEMO &memo,BOOL bViewMode)
{
	//assumes dbf is positioned!
	//Memo dialogs may be open but not with same record and field.

	CMemoEditDlg *pDlg;
	
	if(bViewMode) pDlg=app_pImageDlg;
	else pDlg=app_pDbtEditDlg;

	if(pDlg && !memo.bChg && pDlg->DenyClose())
		return false;

	if(!memo.pData) {
		if(memo.recNo) {
			UINT uDataLen=0;
			memo.pData=m_pdbfile->dbt.GetText(&uDataLen,memo.recNo);
			memo.recCnt=EDITED_MEMO::GetRecCnt(uDataLen); //original length
		}
		else {
			memo.pData="";
			memo.recCnt=0;
		}
	}

	if(bViewMode) {
		//Open or initialize image viewer --
		if(!pDlg) {
			pDlg=new CImageViewDlg(pParentDlg,this,memo);
			//The dialog will delete itself
			ASSERT(pDlg==app_pImageDlg || !app_pImageDlg);
			return app_pImageDlg!=NULL;
		}
	}
	else {
		//Open or initialize text editor --
		if(!pDlg) {
			pDlg=new CDbtEditDlg(pParentDlg,this,memo);
			ASSERT(pDlg && pDlg==app_pDbtEditDlg); //can't fail?
			return app_pDbtEditDlg!=NULL;
		}
	}

	pDlg->ReInit(this,memo);

	return true;
}

bool CShpLayer::IsGridDlgOpen()
{
	return  m_pdbfile->pDBGridDlg && m_pdbfile->pDBGridDlg->m_pShp==this;
}

void CShpLayer::UpdateTitleDisplay()
{
	if(app_pShowDlg && ShpType()==SHP_POINT)
		app_pShowDlg->UpdateLayerTitle(this);
	if(IsGridDlgOpen())
		InitDlgTitle(m_pdbfile->pDBGridDlg);
}

void CShpLayer::OpenTable(VEC_DWORD *pVec)
{
	CDBGridDlg *pDlg=m_pdbfile->pDBGridDlg;
	if(pDlg) {
		if(pDlg->m_pShp==this) {
			pDlg->m_bRestoreLayout=false;
			pDlg->ReInitGrid(pVec);
		}
		else {
			CMsgBox("The table for %s is already open as a member of %s. "
				"You must close it before reopening it as a member of %s.",
				Title(),pDlg->m_pShp->m_pDoc->GetTitle(),m_pDoc->GetTitle());
		}
		return;
	}
	
	m_pdbfile->pDBGridDlg = new CDBGridDlg(this,pVec,theApp.m_pMainWnd);
}

bool CShpLayer::ExportRecords(VEC_DWORD &vRec,bool bSelected)
{
	CExportShpDlg dlg(this,vRec,bSelected);
	//return IDOK==dlg.DoModal() && dlg.m_bAddLayer==TRUE; //rtn value ignored for now
	if(IDOK==dlg.DoModal()) {
		//if(dlg.m_bGenTemplate) {
		// Get app associated with txt
		//SHP_EXT_TMPSHP
		//}
		return true;
	}

	return false;
}

bool CShpLayer::FitExtent(VEC_DWORD &vRec,BOOL bZoom)
{
	POSITION pos = m_pDoc->GetFirstViewPosition();
	CWallsMapView *pView=(CWallsMapView *)m_pDoc->GetNextView(pos);
	ASSERT(pView);

	CFltRect extent(DBL_MAX,-DBL_MAX,-DBL_MAX,DBL_MAX);

	if(ShpType()==SHP_POINT) {
		for(VEC_DWORD::iterator it=vRec.begin();it!=vRec.end();it++) {
			UpdateExtent(extent,m_fpt[*it-1]);
		}
		if(!extent.IsValid()) goto _fail;
	}
	else {
		SHP_POLY_DATA *ppoly=Init_ppoly();
		if(!ppoly) {
			//ASSERT(0);
			goto _fail;
		}

		for(VEC_DWORD::iterator it=vRec.begin();it!=vRec.end();it++) {
			DWORD rec=*it-1;
			ASSERT(rec<m_nNumRecs && rec<m_ext.size());
			if(!ppoly[rec].IsDeleted()) {
				UpdateExtent(extent,m_ext[rec]);
			}
		}
		if(!extent.IsValid()) goto _fail;
	}


	if(!bZoom) {
		ASSERT(m_pGridShp);
		pView->CenterOnGeoPoint(extent.Center(),1.0);
	}
	else pView->FitShpExtent(extent);
	return true;

_fail:
	CMsgBox(MB_ICONQUESTION,"The view is unchanged since the selected item(s) are deleted.");
	return false;
}

LPCSTR CShpLayer::GetRecordLabel(LPSTR pLabel,UINT sizLabel,DWORD rec)
{
	if(rec>m_pdb->NumRecs()) {
		ASSERT(rec==m_pdb->NumRecs()+1 && sizLabel>LEN_NEW_POINT_STR);
		return strcpy(pLabel,new_point_str);
	}
	*pLabel=0;
	if(!m_pdb->Go(rec)) {
		m_pdb->GetRecordFld(m_nLblFld,pLabel,sizLabel);
	}
	if(!*pLabel) strcpy(pLabel,"unnamed");
	
	return pLabel;
}

UINT CShpLayer::GetPolyRange(UINT rec)
{
	SHP_POLY_DATA *ppoly=Init_ppoly();
	if(ppoly) {
		CFltRect ext(m_ext[rec-1]);
		CFltPoint fpt=ext.Center();
		rec=(UINT)(IsProjected()?ext.Width():(1000*MetricDistance(fpt.y,ext.l,fpt.y,ext.r)));
		rec=(rec+9)/10*10;
		if(rec<100) rec=100;
		else if(rec>100000) rec=100000;
	}
	else rec=0;
	return rec;
}

void CShpLayer::InitFlyToPt(CFltPoint &fpt,DWORD rec)
{
	if(rec) {
		if(ShpType()==SHP_POINT) {
			fpt=m_fpt[rec-1];
		}
		else {
			ASSERT(m_ext.size());
			fpt=m_ext[rec-1].Center();
		}
	}
	if(m_iNad!=1 || m_iZone) {
		ConvertPointsTo(&fpt,1,0,1);
	}
}

void CShpLayer::OnLaunchWebMap(DWORD rec,CFltPoint *pfpt/*=NULL*/)
{
	char label[80];
	GetRecordLabel(label,80,rec);
	CFltPoint fpt;
	if(pfpt) {
		fpt=*pfpt;
		rec=0;
	}
	InitFlyToPt(fpt,rec);
	GetMF()->Launch_WebMap(fpt,label);
}

bool CShpLayer::GetFieldTextGE(CString &text,DWORD rec,UINT nFld,BOOL bSkipEmpty)
{
	UINT len;
	CString s;	//Set s to field value --
	if(CShowShapeDlg::m_bLaunchGE_from_Selelected)
		len=app_pShowDlg->GetSelectedFieldValue(s,nFld);
	else {
		VERIFY(!m_pdb->Go(rec));
		len=GetFieldValue(s,nFld);
	}

	if(m_pdb->FldTyp(nFld)=='M') {
		if(len && CDBTData::IsTextRTF(s)) {
			try {
				LPSTR p=s.GetBuffer();
				len=CDBTFile::StripRTF(p,FALSE); //emiminate '{' prefix also
				s.ReleaseBuffer(len);
			}
			catch(...) {
				s.Empty();
				len=0;
			}
		}
		//trim trailing crlf --
		while(len>=2 && !memcmp((LPCSTR)s+len-2,"\r\n",2)) {
			len-=2;
			s.Truncate(len);
		}
	}

	if(!len && bSkipEmpty) return false;

	try {
		text.Format(bSkipEmpty?"<b>%s</b>: %s\r\n\r\n":"%s: %s\r\n\r\n",m_pdb->FldNamPtr(nFld),(LPCSTR)s);
	}
	catch(...) {
		text.Empty();
		len=0;
	}
	return len>0;
}

bool CShpLayer::InitDescGE(CString &text,DWORD rec,VEC_BYTE &vFld,BOOL bSkipEmpty)
{
	ASSERT(text.IsEmpty());
	CString s;
	for(VEC_BYTE::iterator it=vFld.begin();it!=vFld.end(); it++) {
		if(!GetFieldTextGE(s,rec,(UINT)*it,bSkipEmpty) && bSkipEmpty) continue;
		text+=s;
	}
	return !text.IsEmpty();
}

void CShpLayer::LaunchGE(CString &kmlPath,VEC_DWORD &vRec,BOOL bFly,CFltPoint *pfpt /*=NULL*/)
{
	//Called from OnOptionsGE() when bFly is 0 or 2
	//else called from OnLaunchGE() for a fly direct (bFly=1)
	char label[80];
	UINT len=vRec.size();
	CFltPoint fpt;
	if(pfpt) {
		ASSERT(len==1);
		fpt=*pfpt;
	}
	VEC_GE_POINT vpt;
	vpt.reserve(len);
	for(VEC_DWORD::iterator it=vRec.begin();it!=vRec.end();it++) {
		GetRecordLabel(label,80,*it);
		InitFlyToPt(fpt,pfpt?0:*it);
		vpt.push_back(GE_POINT(label,*it,fpt.y,fpt.x));
	}
	ASSERT(len==vpt.size());
	GE_Launch(this,kmlPath,&vpt[0],len,bFly);
}

static UINT CheckLenGE(VEC_DWORD &vRec)
{
	UINT len=vRec.size();
	if(len>GE_MAX_POINTS) {
		if(IDOK!=CMsgBox(MB_OKCANCEL,"CAUTION: You've chosen to export %u points to KML, which requires confirmation since this "
			"number is greater than %u.\n\nSelect OK to proceed.",len,GE_MAX_POINTS))
				len=0;
	}
	return len;
}

void CShpLayer::OnLaunchGE(VEC_DWORD &vRec,CFltPoint *pfpt /*=NULL*/)
{
	//Called from fly direct only --
	UINT len= CheckLenGE(vRec);
	if(!len) return;
		
	char label[100];
	CString kmlPath;

	GetRecordLabel(label,80,vRec[0]);
	if(len>1) {
		ASSERT(ShpType()==SHP_POINT);
		CString sfx;
		sfx.Format(" (%u pts)",len);
		strcat(label,sfx);
	}
	/*
	if(len==1) GetRecordLabel(label,80,vRec[0]);
	else {
		ASSERT(ShpType()==SHP_POINT);
		trx_Stncc(label,Title(),80);
	}
	*/
	if(!GetKmlPath(kmlPath,label)) return; //shouldn't happen
	LaunchGE(kmlPath,vRec,TRUE,pfpt); // fly direct
}

void CShpLayer::OnOptionsGE(VEC_DWORD &vRec,CFltPoint *pfpt /*=NULL*/)
{
	UINT len= CheckLenGE(vRec);
	if(!len) return;

	CDlgOptionsGE dlg(this,vRec);
	char label[80];
	if(len==1) GetRecordLabel(label,80,vRec[0]);
	else trx_Stncc(label,Title(),80);
	GetKmlPath(dlg.m_pathName,label);

	if(IDOK==dlg.DoModal()) {
		LaunchGE(dlg.m_pathName,vRec,dlg.m_bFlyEnabled?2:0,pfpt);
	}
}

UINT CShpLayer::GetFieldValue(CString &s,int nFld)
{
	//Called from modal dialog CDlgOptionsGE. Assumes m_pdb is positioned.
	UINT len=0;
	LPCSTR pRet="";

	if(m_pdb->FldTyp(nFld)=='M') {
		pRet=m_pdbfile->dbt.GetText(&len,CDBTFile::RecNo((LPCSTR)m_pdb->FldPtr(nFld)));
	}
	else {
		pRet=(LPCSTR)m_pdb->FldPtr(nFld);
		LPCSTR p=pRet+m_pdb->FldLen(nFld);
		while(p>pRet && xisspace(p[-1])) p--;
		len=p-pRet;
	}
	s.SetString(pRet,len);
	return len;
}

void CShpLayer::ChkEditDBF(UINT rec)
{
	BYTE &eflg=RecEditFlag(rec);
	if(!(eflg&SHP_EDITDBF)) {
		eflg|=SHP_EDITDBF;
		m_bEditFlags|=SHP_EDITDBF; //will at least need to rewrite DBE file when shapefile is closed
	}
}

LPCSTR CShpLayer::GetMemoTitle(CString &title,const EDITED_MEMO &memo)
{
	CString label;
	LPCSTR p;
	UINT uRec=memo.uRec;
	ASSERT(uRec<=m_nNumRecs && uRec<=m_pdb->NumRecs()+1);
	if(uRec<=m_pdb->NumRecs()) {
		VERIFY(!m_pdb->Go(uRec));
		m_pdb->GetTrimmedFldStr(label,m_nLblFld);
		p=(LPCSTR)label;
	}
	else p=new_point_str;
	title.Format("%s: %s",m_pdb->FldNamPtr(memo.fld),p);
	return (LPCSTR)title;
}

void CShpLayer::SaveEditedMemo(EDITED_MEMO &memo,CString &csData)
{
	free(memo.pData);
	memo.pData=_strdup((LPCSTR)csData);

	ASSERT(memo.pData);

	if(!app_pShowDlg || !app_pShowDlg->UpdateMemo(this,memo)) {
		//Not a selected record --
		ASSERT(memo.uRec && memo.recNo!=INT_MAX);
		ASSERT(m_pdbfile->dbt.IsOpen());
		//ModelessOpen(m_csTitle,1000,"Saving memo %s of record %u ...", m_pdb->FldNamPtr(memo.fld),memo.uRec);

		VERIFY(!m_pdb->Go(memo.uRec)); //Prepare for flushRec()!
		CDBTFile::SetRecNo((LPSTR)m_pdb->FldPtr(memo.fld),m_pdbfile->dbt.PutText(memo));
		m_pdbfile->dbt.FlushHdr();

		if(m_pdbfile->HasTimestamp(1)) { //UPDATE_ timestamp
			ASSERT(SHP_DBFILE::bEditorPrompted);
			UpdateTimestamp(m_pdb->RecPtr(),FALSE); //not creating
		}
		
		VERIFY(!m_pdb->FlushRec());
		//ModelessClose();

		if(IsGridDlgOpen()) 
			m_pdbfile->pDBGridDlg->RefreshTable(memo.uRec);

		ChkEditDBF(memo.uRec);
		//Above may NOT have set m_pSelLayer->m_bEditFlags!
	}
}

void CShpLayer::UpdateSizeStr()
{
	if(m_uDelCount)
		m_csSizeStr.Format("%u (%u)",m_nNumRecs-m_uDelCount,m_uDelCount);
	else
		m_csSizeStr.Format("%u",m_nNumRecs);
}

int CShpLayer::AddMemoLink(LPCSTR fpath,VEC_CSTR &vlinks,SEQ_DATA &s,LPCSTR pRootDir,int rootLen)
{
	int iRet=0;
	if((int)strlen(fpath)<rootLen || _memicmp(fpath, pRootDir, rootLen)) {
		if(bFromZip) return 1;
		iRet=1;
	}
	ASSERT(vlinks.size()==s.seq_len);
	if(s.seq_siz<=s.seq_len) {
		ASSERT((s.seq_siz==0)==(s.seq_buf==0));
		s.seq_buf=(PDWORD)realloc(s.seq_buf,(s.seq_siz+=500)*sizeof(DWORD));
		trx_Bininit(s.seq_buf,&s.seq_len,s.seq_fcn);
	}
	trx_Binsert(vlinks.size(),TRX_DUP_NONE);
	if(!trx_binMatch) {
		vlinks.push_back(CString(fpath+rootLen));
	}
	return iRet;
}

int CShpLayer::GetFieldMemoLinks(LPSTR pStart,LPSTR fpath,VEC_CSTR &vlinks,SEQ_DATA &s,LPCSTR pRootDir,int rootLen,VEC_CSTR *pvBadLinks /*=NULL*/)
{
	LPSTR pEnd,pStart0=pStart;
	UINT nBadDirCount=0;

	if(CDBTData::IsTextIMG(pStart)) {
		while((pStart=(LPSTR)CDBTData::NextImage(pStart)) && (pStart=strstr(pStart+3,"\r\n"))) {
			bool bBadFound=false;
			pStart+=2;
			while(xisspace(*pStart)) pStart++; //start of relative path name
			pEnd=strstr(pStart,"\r\n");
			if(!pEnd) pEnd=pStart+strlen(pStart);
			if(pEnd-pStart<_MAX_PATH) {
				char cTerm=*pEnd; //avoid copying string!
				*pEnd=0;
				if(::GetFullFromRelative(fpath,pStart,PathName())) {
					if(AddMemoLink(fpath,vlinks,s,pRootDir,rootLen)) {
						nBadDirCount++; bBadFound=true;
					}
					else {
						*trx_Stpext(fpath)=0;
						if(GetMediaTyp(fpath) && !_access(fpath,0) && AddMemoLink(fpath,vlinks,s,pRootDir,rootLen)) {
							nBadDirCount++; bBadFound=true;
						}
					}
				}
				if(bBadFound && pvBadLinks) {
					pvBadLinks->push_back(fpath);
				}
				*pEnd=cTerm;
			}
			if(!*pEnd) break;
			pStart=pEnd+2;
		}
	}
	else {
		CString path;
		while(pStart=strstr(pStart,lpstrFile_pfx)) {
			if(!(pEnd=strchr(pStart+=8,'>'))) {
				break;
			}
			path.SetString(pStart,pEnd-pStart);
			path.Replace('/','\\');
			if(::GetFullFromRelative(fpath,path,PathName())) {
				if(AddMemoLink(fpath,vlinks,s,pRootDir,rootLen)) {
					nBadDirCount++;
					if(pvBadLinks)
						pvBadLinks->push_back(fpath);
				}
			}
			pStart=pEnd+1;
		}
	}
	return nBadDirCount;
}

int CShpLayer::GetMemoLinks(VEC_CSTR &vlinks,SEQ_DATA &s,LPCSTR pRootDir,int rootLen)
{
	ASSERT(HasMemos());
	UINT nBadDirCount=0;

	char fpath[_MAX_PATH];

	*(LPCSTR *)s.seq_pval=fpath+rootLen;

	try {
		for(UINT r=1;r<=m_nNumRecs;r++) {
			VERIFY(!m_pdb->Go(r));
			for(UINT f=m_pdb->NumFlds();f;f--) {
				if(m_pdb->FldTyp(f)!='M') continue;
				int dbtrec=CDBTFile::RecNo((LPCSTR)m_pdb->FldPtr(f));
				if(!dbtrec) continue;
				
				UINT uDataLen=0;
				LPSTR pStart=m_pdbfile->dbt.GetText(&uDataLen,dbtrec);
				if(!pStart) {
					ASSERT(0);
					continue;
				}
				bFromZip=true;
				nBadDirCount+=GetFieldMemoLinks(pStart,fpath,vlinks,s,pRootDir,rootLen);
				bFromZip=false;
			}
		}
	}
	catch (...) {
		return -1;
	}
	return nBadDirCount;
}

bool CShpLayer::ToggleDelete(DWORD rec)
{
	if(m_pdb->Go(rec)) {
		ASSERT(0);
		return false;
	}

	bool bDelete;

	if(ShpType()!=SHP_POINT) {
		ASSERT(m_pdbfile->ppoly && m_pdbfile->pfShp);
		SHP_POLY_DATA &poly=m_pdbfile->ppoly[rec-1];
		bDelete=!poly.IsDeleted();
		int typ=bDelete?0:ShpTypeOrg();
		try {
			m_pdbfile->pfShp->Seek(poly.off-(4*8+4),CFile::begin);
			m_pdbfile->pfShp->Write(&typ,4);
			*m_pdb->RecPtr()=bDelete?'*':' ';
			if(bDelete) {
				m_uDelCount++;
				poly.Delete();
			}
			else {
				m_uDelCount--;
				poly.UnDelete();
			}
		}
		catch(...) {
			return false;
		}
	}
	else {

		BYTE &eflg = RecEditFlag(rec); //(*m_vdbe)[uRec-1]

		bDelete=(eflg&SHP_EDITDEL)==0;
		//ASSERT(*m_pdb->RecPtr()==(bDelete?' ':'*'));

		*m_pdb->RecPtr()=bDelete?'*':' ';

		eflg |= SHP_EDITSHP; //shp record needs revision
		if(bDelete) {
			m_uDelCount++;
			eflg|=SHP_EDITDEL;
		}
		else {
			ASSERT(m_uDelCount);
			m_uDelCount--;
			eflg&=~SHP_EDITDEL;
		}

		//Refresh other document views that contain this layer unconditionally. Can be slow if many
		//records are being deleted/undeleted --
		if(m_pdbfile->nUsers>1)
			UpdateDocsWithPoint(rec);
	}

	return true;
}

bool CShpLayer::IsAdditionPending()
{
	return app_pShowDlg && app_pShowDlg->IsAddingRec() && app_pShowDlg->SelectedLayer()==this;
}

void CShpLayer::OutOfZoneMsg(int iZone)
{
		CMsgBox("The proposed location in UTM zone %d is "
			"outside the shapefile's allowable range, which is in or near zone %d.",iZone,m_iZoneOrg);
}

bool CShpLayer::check_overwrite(LPCSTR pathName)
{
	//static fcn called only prior to an export to pathName and after processing any template --
	char pathBuf[MAX_PATH];

	strcpy(pathBuf,pathName);
	LPSTR pExt=trx_Stpext(pathBuf);

	strcpy(pExt,SHP_EXT_DBF);
	if(CShpLayer::DbfileIndex(pathBuf)>=0) {
		strcpy(pExt,SHP_EXT);
		CMsgBox("File %s is currently in use and can't be overwritten.",trx_Stpnam(pathBuf));
		return false;
	}

	//Check for accessibility and existence to confirm overwrite --
	strcpy(pExt,SHP_EXT);
	if(!_access(pathBuf,0)) {
		if(IDYES!=CMsgBox(MB_YESNO,"Shapefile %s already exists. Do you want to overwrite it?",trx_Stpnam(pathBuf)))
			return false;
	}
	return true;
}

void CShpLayer::TestMemos(BOOL bOnLoad /*=FALSE*/)
{
	CDBTFile &dbt=m_pdbfile->dbt;
	ASSERT(dbt.IsOpen());

	bool bFreeInit=(dbt.m_uFlags&CDBTFile::F_INITFREE)!=0;
	bool bWasteSaved=false,bCorrupt=false;

	if(!bFreeInit) {
		//Only initialize dbt.m_uNextRec and dbt.m_vFree, don't truncate file and write m_uNextRec to header!
		if(!dbt.InitFree(TRUE)) return;
	}

	if(!bOnLoad) AfxGetMainWnd()->BeginWaitCursor();

	UINT nFreeBlks=0,nBadFld=0,nBadRec=0,nBadFreeRec=0,nDelBlks=0,nDeleted=0,nNewFree=0,nWasted=0;

	VEC_FREE vFree; //for recovered free blocks
	VEC_BYTE vBlkFlgs;
	VEC_BYTE vF;
	VEC_INT vBadRec;
	CString s;

	for(BYTE f=m_pdb->NumFlds(); f>=1; f--) {
		if(m_pdb->FldTyp(f)=='M') vF.push_back(f);
	}
	vBlkFlgs.assign(dbt.m_uNextRec-1,0);
	if(dbt.m_vFree.size()) {
		//flag all free blocks in dbt with 255 --
		for(it_free it=dbt.m_vFree.begin(); it!=dbt.m_vFree.end(); it++) {
			memset(&vBlkFlgs[0]+it->recNo-1,255,it->recCnt);
			ASSERT(it->recCnt>0);
			nFreeBlks+=it->recCnt;
		}
		ASSERT(nFreeBlks<=vBlkFlgs.size());
	}

	//flag all used blocks on dbt with 1 while counting conflicts --
	for(DWORD rec=1; rec<=m_pdb->NumRecs(); rec++) {
		if(m_pdb->Go(rec)) {
			s.AppendFormat("\nFailure accessing DBF record %u.",rec);
			goto _ret;
		}
		bool bBadRec=false,bBadFreeRec=false;
		UINT recBlks=0; //total blocks used
		for(it_byte it=vF.begin(); it<vF.end(); it++) {
			int dbtrec=CDBTFile::RecNo((LPCSTR)m_pdb->FldPtr(*it));
			if(!dbtrec) continue;
			UINT len=0;
			dbt.GetText(&len,dbtrec);
			ASSERT(len); //length of data, not counting 0x1A terminator
			//convert to block count --
			len=(len+512)/512; //blocks used by memo, allow for 0x1A at end of data
			recBlks+=len;
			LPBYTE pBlk=&vBlkFlgs[0]+dbtrec-1; //flag pos of first block of this memo
			BYTE bConflict=0;
			for(UINT b=0; b<len; b++) {
				if(pBlk[b]==1) {
					//block is in use!
					bConflict =1;
					break;
				}
				if(pBlk[b]==255) bConflict=255;
			}
			if(bConflict==1) {
				//fatal conflict with block used by another memo, either in this rec or another rec
				nBadFld++;
				bBadRec=true;
			}
			else if(bConflict==255) {
				//at least some blocks of this memo are on free list, rest used only by this memo (so far)
				bBadFreeRec=true;
			}
			memset(pBlk,1,len); //assign ownership if blocks to this memo
		} //fld loop

		if(m_pdb->IsDeleted()) {
			nDeleted++;
			nDelBlks+=recBlks;
		}

		if(bBadRec) {
			nBadRec++;
			vBadRec.push_back(rec);
		}
		else if(bBadFreeRec) {
			//only conflicts with free block enountered with this record
			nBadFreeRec++;
		}
	} //rec loop

	bCorrupt= nBadFreeRec || nBadRec;

	if(nBadRec) {
		s.AppendFormat("\nThere are records (%u) whose ownership of DBT file space conflicts with that\n"
			             "of earlier numbered records. Although you can repair the format with an export,\n"
						 "it's likely that some memo fields will have wrong or duplicated data.\n"
					     "Record numbers:", nBadRec);
		UINT nListed=0;
		for(it_int it=vBadRec.begin(); it!=vBadRec.end(); it++) {
			if(nListed==10) {
				s.AppendFormat(" (%u more)", vBadRec.size()-nListed);
				break;
			}
			nListed++;
			s.AppendFormat(" %u",*it);
		}
		s+=".\n";
	}
	else if(nBadFreeRec)
		s.AppendFormat("\nThere are no conflicts, but %u records are using memory on the free block list.\n"
		                 "Prior to any edits you should repair the format with an export, then compare\n"
						 "the exported shapefile with a backup if one exists.\n",nBadFreeRec);

	if(bCorrupt) s+=   "\nYou shouldn't be seeing this. If you think the file corruption was caused by the\n"
		               "program, please report it as a bug!";

	else {

		//Check for permanently wasted space --

		bWasteSaved=IsEditable();
		for(it_byte it=vBlkFlgs.begin(); it!=vBlkFlgs.end(); it++) {
			if(!*it) {
				if(bWasteSaved) {
					it_byte ite=it+1;
					while(ite!=vBlkFlgs.end() && !*ite) ite++;
					vFree.push_back(DBT_FREEREC(1+it-vBlkFlgs.begin(),ite-it));
					nWasted+=ite-it;
					it=ite-1;
				}
				else nWasted++;
			}
		}
		if(!nWasted) bWasteSaved=false;

		if(!bOnLoad || nWasted) {

			if(nWasted)
				s.AppendFormat("\nAlthough harmless, there are %u 512-byte blocks of unusable file space, likely due to\nan abnormal program shutdown. "
				"To make this space available, %s.\n",nWasted,bWasteSaved?"check the box below":"run this test with\nthe shapefile editable");

			if(nFreeBlks)
				s.AppendFormat("\nThere are%s %u %sblocks of reusable file space on the free list.",nWasted?" also":"",nFreeBlks,nWasted?"":"512-byte ");
			else s+="\nThe free block list is empty.";

			if(nDelBlks)
				s.AppendFormat("\nThe %u deleted records are using %u blocks of file space.",nDeleted,nDelBlks);

			UINT nUnused=nWasted+nFreeBlks+nDelBlks; //reduction in size due to an export assuming no conflicts
			if(nUnused)
				s.AppendFormat("\nIf exported, DBT file size, now %.1f KB, will be %u x 512b = %.1f KB smaller.",
					(double)(dbt.m_uNextRec*512)/1024, nUnused, (double)(nUnused*512)/1024);
		}
	}

	if(bOnLoad) s+="\n\nNOTE: To avoid this integrity test with each project load, you can uncheck that\n"
			            "option in the shapefile's Symbols dialog.";
	else if(!(m_uFlags&NTL_FLG_TESTMEMOS))
			    s+= "\n\nNOTE: To check integrity with each project load, you can enable that option in\n"
						"the shapefile's Symbols dialog. You'll be notified only if a repair is needed.";

	if(!bOnLoad) AfxGetMainWnd()->EndWaitCursor();

	if(!bOnLoad || bCorrupt || nWasted) {
		CString msg,title;

		title=bCorrupt?"corrupted":"internally consistent";
		if(!bCorrupt && nWasted) title+=", but contains wasted space"; 

		msg.Format("Layer: %s\n\nThe DBT component is %s!\n%s",Title(),(LPCSTR)title,(LPCSTR)s);
		title.Format("Status of %s",(LPCSTR)dbt.GetFileName());

		if(!bWasteSaved) {
			MsgInfo(NULL,msg,title,bCorrupt?MB_ICONEXCLAMATION:0);
		}
		else {
			if(MsgCheckDlg(NULL, MB_OK, msg, title, "Recover wasted DBT file space")>1) {
				//Let's add the vFree[] items to the free list --
				for(it_free it=vFree.begin(); it!=vFree.end();it++)
						dbt.AddToFree(*it); //alters m_vFree, might reduce m_uNextRec and set F_TRUNCATE (4), but doesn't write to file

				//let's insure file is flushed and in F_INITFREE state --
				dbt.SeekToBegin();
				dbt.Write(&dbt.m_uNextRec,sizeof(UINT));
				dbt.SetLength(dbt.m_uNextRec*512);
				dbt.Flush();
				dbt.m_uFlags=CDBTFile::F_INITFREE;
				bFreeInit=true;
			}
		}
	}

_ret:
	if(!bFreeInit) {
		//file has not been modified
		ASSERT(!(dbt.m_uFlags&CDBTFile::F_INITFREE));
		dbt.m_uNextRec=0;
		if(dbt.m_vFree.size()) {
			//delete it since it was used for this function only (could be R/O)
			std::vector<DBT_FREEREC>().swap(dbt.m_vFree);
		}
	}
}

