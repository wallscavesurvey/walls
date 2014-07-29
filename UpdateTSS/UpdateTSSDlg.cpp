// UpdateTSSDlg.cpp : implementation file
//

#include "stdafx.h"
#include "utility.h"
#include "UpdateTSS.h"
#include "UpdateTSSDlg.h"
#include "PromptPath.h"
#include "filecfg.h"
#include <trx_str.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static LPCSTR dbf_fldname[] =
{
    "TSSID",
	"NAME",
	"OTHER_NAME",
	"TYPE",
	"COUNTY",
	"QUADRANGLE",
	"DEPTH_SRV",
	"LENGTH_SRV",
	"AESTHETIC",
	"ARCHAEO",
	"BAD_AIR",
	"BATS",
	"BIOLOGICAL",
	"ENDANG_SP",
	"GEOLOGICAL",
	"HISTORICAL",
	"HYDRO",
	"PALEO",
	"RUMORED",
	"HAS_SPRING",
	"MAPS",
	"PHOTOS",
	"LATITUDE_",
	"LONGITUDE_"
};

#define NUM_DBF_FIELDS (sizeof(dbf_fldname)/sizeof(LPCSTR)) //21
static int dbf_fldnum[NUM_DBF_FIELDS];
static char dbf_fldtyp[NUM_DBF_FIELDS];

enum {
    f_tssid,
	f_name,
	f_other_name,
	f_type,
	f_county,
	f_quadrangle,
	f_depth,
	f_length,
	f_aesthetic,
	f_archeological,
	f_bad_air,
	f_bats,
	f_biological,
	f_endangered_sp,
	f_geological,
	f_historical,
	f_hydrological,
	f_paleontological,
	f_rumored,
	f_has_spring,
	f_maps,
	f_photos,
	f_latitude,
	f_longitude
};


//MDB All Types Fields --
enum {
	F_TSSID_A,
	F_TSS_NAME_A,
	F_OTHER_NAME_A,
	F_TYPE_A,
	F_COUNTY_A,
	F_QUADRANGLE_A,
	F_DEPTH_A,
	F_LENGTH_A,
	F_LENGTH_TYPE_A,
	F_AESTHETIC_A,
	F_ARCHEOLOGICAL_A,
	F_BAD_AIR_A,
	F_BIOLOGICAL_A,
	F_ENDANGERED_SP_A,
	F_GEOLOGICAL_A,
	F_HISTORICAL_A,
	F_HYDROLOGICAL_A,
	F_PALEONTOLOGICAL_A,
	F_RUMORED_A,
	F_MAPPED_A,
	F_PHOTOGRAPHED_A,
	F_LOCATED_A,
	F_LATITUDE_A,
	F_LONGITUDE_A
};

static LPCSTR mdb_fldnameA[]={
	"TSSID",
	"TSS_Name",
	"Other_Name",
	"Type",
	"County",
	"Quadrangle",
	"Depth",
	"Length",
	"Length_Type",
	"Aesthetic",
	"Archeological",
	"Bad_Air",
	"Bats",
	"Biological",
	"Endangered_Sp",
	"Geological",
	"Historical",
	"Hydrological",
	"Paleontological",
	"Rumored",
	"Mapped",
	"Photographed",
	"Located",
	"Latitude",
	"Longitude"
};

#define NUM_MDB_FIELDS_A (sizeof(mdb_fldnameA)/sizeof(LPCSTR))
static int mdb_fldnumA[NUM_MDB_FIELDS_A];
static int mdb_fldsrcA[NUM_MDB_FIELDS_A] = {
	f_tssid,
	f_name,
	f_other_name,
	f_type,
	f_county,
	f_quadrangle,
	f_depth,
	f_length,
	-1,
	f_aesthetic,
	f_archeological,
	f_bad_air,
	f_bats,
	f_biological,
	f_endangered_sp,
	f_geological,
	f_historical,
	f_hydrological,
	f_paleontological,
	f_rumored,
	f_maps,
	f_photos,
	-2,
	f_latitude,
	f_longitude
};

//Statistics fields --
enum {
	F_DB_DATE_S,
	F_DB_RECORDS_S,
	F_CAVES_S,
	F_SINKS_CAVITIES_S,
	F_SHELTERS_S,
	F_SPRINGS_S,
	F_UNDEFINED_S,
	F_RUMORED_S,
	F_AESTHETIC_S,
	F_ARCHEOLOGICAL_S,
	F_BAD_AIR_S,
	F_BATS_S,
	F_BIOLOGICAL_S,
	F_ENDANGERED_SP_S,
	F_GEOLOGICAL_S,
	F_HISTORICAL_S,
	F_HYDROLOGICAL_S,
	F_PALEONTOLOGICAL_S,
	F_COUNTIES_S,
	F_LOCATED_S,
	F_TOPO_ONLY_S,
	F_UNLOCATED_S,
	F_MAPPED_S,
	F_PHOTOGRAPHED_S
};

static LPCSTR mdb_fldnameS[]={
	"DB_Date",
	"DB_Records",
	"Caves",
	"Sinks_Cavities",
	"Shelters",
	"Springs",
	"Undefined",
	"Rumored",
	"Aesthetic",
	"Archeological",
	"Bad_Air",
	"Bats",
	"Biological",
	"Endangered_Sp",
	"Geological",
	"Historical",
	"Hydrological",
	"Paleontological",
	"Counties",
	"Located",
	"Topo_Only",
	"Unlocated",
	"Mapped",
	"Photographed"
};

#define NUM_MDB_FIELDS_S (sizeof(mdb_fldnameS)/sizeof(LPCSTR))
static int mdb_fldnumS[NUM_MDB_FIELDS_S];
static long mdb_stats[NUM_MDB_FIELDS_S];

//Deep_caves and long_caves fields --
enum {
	F_TSSID_D,
	F_TSS_NAME_D,
	F_COUNTY_D,
	F_DEPTH_D,
	F_LENGTH_D,
	F_LENGTH_TYPE_D
};

static LPCSTR mdb_fldnameD[]={
	"TSSID",
	"TSS_Name",
	"County",
	"Depth",
	"Length",
	"Length_Type"
};

#define NUM_MDB_FIELDS_D (sizeof(mdb_fldnameD)/sizeof(LPCSTR))
static int mdb_fldnumD[NUM_MDB_FIELDS_D];
static int mdb_fldsrcD[NUM_MDB_FIELDS_D]={f_tssid,f_name,f_county,f_depth,f_length,-1};

static int num_mdb_flds;
static LPCSTR *mdb_fldname;
static int *mdb_fldnum;
static int *mdb_fldsrc;
static char pDateStr[]="yyyy_mm_dd";

struct TYP_DREC {
	TYP_DREC(float id0, float id1, UINT trec) : d0(id0), d1(id1), rec(trec) {}
	float d0,d1;
	UINT rec;
};

/*
struct TYP_DREC {
	TYP_DREC(int id0, int id1, UINT trec) : d0(id0), d1(id1), rec(trec) {}
	int d0,d1;
	UINT rec;
};
*/

typedef std::vector<TYP_DREC> VEC_DREC;
typedef VEC_DREC::iterator IT_VEC_DREC;
static VEC_DREC vDepth,vLength;
static DWORD bufCounty[256],numCounties;
static LPCSTR pCounty;
static VEC_CSTR vCounty;
static int nAll_types=0,nDeep=0,nLong=0,nDeleted=0;

static int CALLBACK seq_fcn(int i)
{
	return _stricmp(pCounty,vCounty[i]);
}

static bool comp_drec(TYP_DREC &dr0,TYP_DREC &dr1)
{
	return dr1.d0<dr0.d0 || (dr1.d0==dr0.d0 && dr1.d1<dr0.d1);
}

#ifdef _USE_TEMPLATE
#define MAX_TEMPLATES	30
static char szAppPath[MAX_PATH];
static LPSTR pNamTemplate;
static int nTemplates=0,nMemoTrunc=0;
static LPCSTR spaces=":                                                           ";
#define LEFT_MGN 25

static char keybuf[256];

struct MDB_FLD {
	CString fnam;
	long fsiz;
	short ftyp;
};

#define MDB_FLD_SIZ 75
static MDB_FLD mdb_fld[MDB_FLD_SIZ];
static int mdb_fld_len=0;

static DBF_pFLDDEF pFlddef;

struct FLD_DAT {
	CString name;
	int isrc;
	int idst;
	UINT flg;
};

typedef std::vector<FLD_DAT> V_FLD_DAT;
static V_FLD_DAT vFldDat;

static CString csLogPath;

static __inline bool isspc(char c)
{
	return c==' ' || c=='\\';
}

static bool IsRTF(LPCSTR p)
{
	return *p=='{' && *++p=='\\' && *++p=='r' && *++p=='t' && *++p=='f';
}

static double roundf(double d)
{
	double fint,frac;

	frac=modf(d,&fint);
	if(d>=0)
		return (frac<0.5)?fint:(fint+1.0);
	return (frac>-0.5)?fint:(fint-1.0);
}
#endif

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// CUpdateTSSDlg dialog

CUpdateTSSDlg::CUpdateTSSDlg(CString &csPathName,CWnd* pParent /*=NULL*/)
	: CDialog(CUpdateTSSDlg::IDD, pParent)
	, m_bSaveStats(FALSE)
{
	char path[_MAX_PATH];
	VERIFY(GetModuleFileName(NULL,path,_MAX_PATH));
	strcpy(trx_Stpext(path), ".ini");
	m_pathINI.SetString(path);
	strcpy(trx_Stpnam(path),"TSS_DB.mdb");
	m_pathMDB_tmp.SetString(path);
	if(!csPathName.IsEmpty()) m_pathDBF=GetFullPath(csPathName,".dbf");
	else {
		::GetPrivateProfileString("input", "dbfpath", "", path, _MAX_PATH, m_pathINI);
		m_pathDBF.SetString(path);
	}

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CUpdateTSSDlg::~CUpdateTSSDlg()
{
}

static bool initDateStr(LPCSTR p)
{
	LPCSTR pu,pn=p+strlen(p);
	//yyyy_mm_dd
	for(LPCSTR p0=p;(pu=strchr(p0, '-')) && pu-p>=4 && pn-pu>=6 && pu[3]=='-';p0++) {
		int i=-4;
		for(; i<6; i++) if(pu[i]!='-' && !isdigit(pu[i])) continue;
		i=atoi(pu-4);
		if((i=(pu[4]-'0')*10+(pu[5]-'0'))<1 || i>31 ||
		 (i=(pu[1]-'0')*10+(pu[2]-'0'))<1 || i>12 || (i=atoi(pu-4))<2000 || i>2100)
			continue;
		memcpy(pDateStr,pu-4,10);
		return true;
	}
	return false;
}

void CUpdateTSSDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_SAVE_STATS, m_bSaveStats);
	DDX_Text(pDX, IDC_PATHNAME_SHP, m_pathDBF);

	if(pDX->m_bSaveAndValidate) {
		pDX->m_idLastControl=IDC_PATHNAME_SHP;
		pDX->m_bEditLastControl=TRUE;
		if(m_pathDBF.IsEmpty()) {
			AfxMessageBox("A DBF file must be specified.");
			pDX->Fail();
			return;
		}
		m_pathDBF=GetFullPath(m_pathDBF, ".dbf");

		if(!initDateStr(trx_Stpnam(m_pathDBF))) {
			AfxMessageBox("The DBF file name must contain a valid date of the form yyyy-mm-dd.");
			pDX->Fail();
			return;
		}
		if(_access(m_pathDBF, 4)) {
			CMsgBox("File %s in absent or protected.", (LPCSTR)m_pathDBF);
			pDX->Fail();
			return;
		}
		m_pathMDB=m_pathDBF;
		m_pathMDB.Truncate(trx_Stpnam(m_pathMDB)-(LPCSTR)m_pathMDB);
		m_pathMDB+="TSS_DB_";
		m_pathMDB+=pDateStr;
		m_pathMDB+=".mdb";
	}
}

BEGIN_MESSAGE_MAP(CUpdateTSSDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_BROWSE_SHP, &CUpdateTSSDlg::OnBnClickedBrowseDBF)
END_MESSAGE_MAP()

// CUpdateTSSDlg message handlers

BOOL CUpdateTSSDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CUpdateTSSDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CUpdateTSSDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CUpdateTSSDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CUpdateTSSDlg::OnBnClickedBrowseDBF()
{
	BrowseFiles((CEdit *)GetDlgItem(IDC_PATHNAME_SHP),
		m_pathDBF,".dbf",IDS_SHP_FILES,IDS_BROWSE_SHP,0);
}

void CUpdateTSSDlg::CloseDBF()
{
	if(m_dbf.Opened()) {
		m_dbf.Close();
	}
	CDBFile::FreeDefaultCache();
}

BOOL CUpdateTSSDlg::OpenDBF()
{
	if(!dbf_defCache && csh_Alloc((TRX_CSH far *)&dbf_defCache,4096,32)) {
		CMsgBox("Cannot allocate shapefile cache");
		return FALSE;
	}

	if(m_dbf.Open(m_pathDBF,CDBFile::ReadOnly)) {
		CMsgBox("File %s: Open failure - %s.",trx_Stpnam(m_pathDBF),m_dbf.Errstr(dbf_errno));
		CloseDBF(); //only frees cache
		return FALSE;
	}

	//Check for field IDs --
	for(int f=0;f<NUM_DBF_FIELDS;f++) {
		if(!(dbf_fldnum[f]=m_dbf.FldNum(dbf_fldname[f]))) {
			CMsgBox("Required field %s not present in DBF.",dbf_fldname[f]);
			goto _errExit;
		}
		dbf_fldtyp[f]=m_dbf.FldTyp(dbf_fldnum[f]);
	}

	#ifdef _USE_TEMPLATE
	if(!ProcessTmp()) {
		goto _errExit;
	}
	#endif

	return TRUE;

_errExit:
	CloseDBF();  //only frees cache
	return FALSE;

}

void CUpdateTSSDlg::CloseMDB()
{
	if(m_rs.IsOpen()) 
		m_rs.Close();
	if(m_db.IsOpen())
		m_db.Close();
}

BOOL CUpdateTSSDlg::GetRequiredFlds()
{
	CDaoFieldInfo info;

	num_mdb_flds=-2;

	switch(m_pNamTable[4]) {
		case 'a':
			num_mdb_flds=NUM_MDB_FIELDS_A;
			mdb_fldname=mdb_fldnameA;
			mdb_fldnum=mdb_fldnumA;
			mdb_fldsrc=mdb_fldsrcA;
			break;
		case 'd' :
		case 'l' :
			num_mdb_flds=NUM_MDB_FIELDS_D;
			mdb_fldname=mdb_fldnameD;
			mdb_fldnum=mdb_fldnumD;
			mdb_fldsrc=mdb_fldsrcD;
			break;
		case 's':
			num_mdb_flds=NUM_MDB_FIELDS_S;
			mdb_fldname=mdb_fldnameS;
			mdb_fldnum=mdb_fldnumS;
			mdb_fldsrc=NULL;
			break;
	}

	if(m_rs.GetFieldCount()!=num_mdb_flds+1) {
		CMsgBox("Table %s in template MDB does not have the required set of %u fields.",m_pNamTable,num_mdb_flds+1);
		goto _eret;
	}

	for(int n=0;n<num_mdb_flds;n++) {
		try {
			m_rs.GetFieldInfo(mdb_fldname[n], info, AFX_DAO_SECONDARY_INFO);
		}
		catch(...) {
			CMsgBox("Table %s in template MDB is missing field %s.",m_pNamTable,mdb_fldname[n]);
			goto _eret;
		}
		ASSERT(info.m_nOrdinalPosition>=1 && info.m_nOrdinalPosition<=num_mdb_flds);

		mdb_fldnum[n]=info.m_nOrdinalPosition;
	}

	return TRUE;

_eret:
	CloseMDB();
	return FALSE;
}

BOOL CUpdateTSSDlg::OpenMDB_Table()
{
	try {
		if(m_rs.IsOpen()) m_rs.Close();
		m_rs.m_pDatabase=&m_db;
		CString table;
		table.Format("[%s]",m_pNamTable);
		m_rs.Open(dbOpenTable,table);
		//m_rs.SetCurrentIndex("PrimaryKey");
		//m_rs.MoveFirst();
	}
	catch (CDaoException* e)
	{
		// Assume failure is becauase we couldn't find the file
		e->Delete();
		CMsgBox("Failure opening table %s in MDB.",m_pNamTable);
		return FALSE;
	}
	if(!GetRequiredFlds()) {
	   m_rs.Close();
	   return FALSE;
	}
	return TRUE;
}

BOOL CUpdateTSSDlg::OpenMDB()
{
	CString msg;

	if(_access(m_pathMDB_tmp,6)) { //read/write access
		msg="Template database, TSS_DB.mdb, missing or not accessible.";
		goto _rtn;
	}

	if(!::CopyFile(m_pathMDB_tmp,m_pathMDB,FALSE)) {
		msg.Format("File %s could not be created.",m_pathMDB);
		goto _rtn;
	}

	BeginWaitCursor();
	try
	{
		m_db.Open(m_pathMDB,TRUE,FALSE,""); //(path,exclusive,readonly,type));
	}
	catch (CDaoException* e)
	{
		// Assume failure is becauase we couldn't find the file
		e->Delete();
		msg.Format("Error: Can't open %s.",m_pathMDB);
		CloseMDB();
	}
	EndWaitCursor();

_rtn:

	if(!msg.IsEmpty()) {
		AfxMessageBox(msg);
		return FALSE;
	}

	return TRUE;
}

#ifdef _USE_TEMPLATE
BOOL CUpdateTSSDlg::ProcessTmp()
{
	int iTable=m_cbTemplate.GetCurSel();
	ASSERT(iTable>=0);
	CString csName;
	m_cbTemplate.GetLBText(iTable,csName);
	strcpy(pNamTemplate,(LPCSTR)csName);
	strcpy(pNamTemplate+strlen(pNamTemplate),".tsstmp");

	LPSTR pNam,dstFld;

	CFileCfg fdef(deftypes,DEF_CFG_NUMTYPES);
	if(fdef.OpenFail(szAppPath,CFile::modeRead)) {
		CMsgBox("Unable to open template file: %s.",szAppPath);
		return FALSE;
	}

	bConvertUnits=bCreatedIgnore=false;

	pFlddef=m_dbf.FldDef(1);

	CString emsg("Bad field definition");
	FLD_DAT fdat;

	while(fdef.GetLine()==DEF_CFG_FLD) {
		int n;
		if(fdef.Argc()<2) {
			goto _badExit;
		}

		fdat.isrc=fdat.idst=-1;
		fdat.flg=0;

		pNam=fdef.Argv(1);
		if(*pNam=='[') {
			char *p=strchr(pNam,']');
			if(!p) goto _badExit;
			fdat.name.SetString(pNam+1,p-pNam-1);
			n=fdef.GetArgv(p);
			fdat.isrc=0;
		}
		else {
			n=fdef.GetArgv(pNam);
			if(strlen(pNam=fdef.Argv(0))>10) goto _badExit;
			strupr(pNam);
			for(int i=m_dbf.NumFlds()-1;i>=0;i--) {
				if(!strcmp(pNam,pFlddef[i].F_Nam)) {
					fdat.isrc=i+1;
					break;
				}
			}
			if(fdat.isrc<=0) {
				emsg.Format("Field %s not found in DBF",pNam);
				goto _badExit;
			}
			fdat.name=pNam;
		}

		int nDstFld;
		if(n<=3) {
			if(!fdat.isrc) goto _badExit;
			continue;
		}

		dstFld=fdef.Argv(nDstFld=3);
		if(strlen(dstFld)<2 || memcmp(dstFld,">>",2))
			goto _badExit;
		if(!dstFld[2]) {
			if(n<5) goto _badExit;
			dstFld=fdef.Argv(nDstFld=4);
		}
		else dstFld+=2;

		if(*dstFld=='*') {
			if(fdat.isrc==f_type) {
				fdat.idst=-30000;
				goto _next;
			}
			if(!strcmp(dstFld,"*LengthS") || !strcmp(dstFld,"*LengthE")) {
				if(pFlddef[fdat.isrc-1].F_Typ!='C') {
					emsg="C field types are required for *LengthS and *LengthE fields";
					goto _badExit;
				}
				fdat.idst=(toupper(dstFld[7])=='S')?-10000:-20000;

				if(strstr(fdat.name,"_FT")) {
					bConvertUnits=true;
				}

				goto _next;
			}
			if(stricmp(dstFld,"*tss_maps") && stricmp(dstFld,"*tss_photos") &&
				stricmp(dstFld,"*TexBio")) goto _badExit;
			continue; //ignore for now.
		}

		if(*dstFld=='?') {
			if(fdat.isrc==f_created)
				bCreatedIgnore=true; //shouldn't happen
			fdat.flg=1; //do not replace if src is empty (used only with Entrances fld)
			if(*++dstFld=='?') {
				fdat.flg++; //do not replace if dst is nonempty (not used)
				dstFld++;
			}
		}

		if(*dstFld=='!') {
			fdat.flg|=4; //allow rich text
			dstFld++;
		}

		if(strchr(dstFld,';')) {
			if(vAlt_len==VALT_SIZ) {
				emsg="Too many sequence fld types";
				goto _badExit;
			}
			if(pFlddef[fdat.isrc-1].F_Typ!='C') {
				emsg="Sequence fld type must be \'C\'";
				goto _badExit;
			}

			VEC_INT &v=vAlt[vAlt_len];
			fdat.idst=-vAlt_len-1;
			LPSTR p,p0=dstFld;
			while(p=strchr(p0,';')) {
				*p++=0;
				int i=FindFldMDB(p0);
				if(i<0) {
					dstFld=p0;
					goto _badFind;
				}
				v.push_back(i);
				p0=p;
			}
			if(*p0) {
				int i=FindFldMDB(p0);
				if(i<0) {
					dstFld=p0;
					goto _badFind;
				}
				if(mdb_fld[i].ftyp!=dbText) {
					emsg="Field following last \";\" must be type text";
					goto _badExit;
				}
				v.push_back(i);
			}
		}
		else {
			if((fdat.idst=FindFldMDB(dstFld))<0)
				goto _badFind;

			if(strstr(fdat.name,"_M") && !stricmp(dstFld,"Elev") ||	strstr(fdat.name,"_FT") && FindLenDepthFld(dstFld)>=0)
				bConvertUnits=true;
		}
_next:
		vFldDat.push_back(fdat);
	}

	fdef.Close();
	return TRUE;

_badFind:
	emsg.Format("Field %s not found in MDB",dstFld);

_badExit:
	CMsgBox("File %s, Line %u: %s.",trx_Stpnam(szAppPath),fdef.LineCount(),(LPCSTR)emsg);
	vFldDat.clear();
	return FALSE;
}
#endif

static double ParseLength(CString &s, CString &sTyp)
{
	ASSERT(sTyp.IsEmpty());
	double d=atof(s);
	if(d>0.0) {
		char p[2]; p[1]=0;
		int i;
		if((i=s.FindOneOf(",;"))>=0) s.Truncate(i);
		if((i=s.FindOneOf("THth"))>=0) {
			*p=s[i];
			_strupr(p);
			ASSERT(*p=='T' || *p=='H');
		}
		else *p='U';
		sTyp=p;
	}
	else d=0.0;
	return d;
}

static void SetStatFields(CDaoRecordset &m_rs)
{
	for(int col=0; col<NUM_MDB_FIELDS_S; col++) {
		if(!col) {
			mdb_stats[F_DB_RECORDS_S]=nAll_types;
			m_rs.SetFieldValue(mdb_fldnum[0],pDateStr);
			continue;
		}
		m_rs.SetFieldValue(mdb_fldnumS[col], mdb_stats[col]);
	}
}

void CUpdateTSSDlg::AppendStats(CString &msg)
{
	try
	{
		int iMatch=-1;
		CString csDate;
		m_db.Open(m_pathMDB_tmp,TRUE,FALSE,""); //(path,exclusive,readonly,type));
		m_rs.m_pDatabase=&m_db;
		m_rs.Open(dbOpenTable,"[tss_statistics]");
		if(!m_rs.IsBOF()) {
			m_rs.MoveLast();
			GetString(csDate,m_rs,1);
			if((iMatch=csDate.Compare(pDateStr))>1) {
				msg.AppendFormat("\n\nCan't update statistics table in TSS_DB.mdb since the last record, dated %s, is newer than %s.",
					(LPCSTR)csDate,pDateStr);
				goto _ret;
			}
		}
		else csDate="None";
		msg.AppendFormat("\n\nConfirm: %s statistics record dated %s %s %s? (Prior record: %s)",
			iMatch?"Copy":"Update",pDateStr, iMatch?"in":"to",trx_Stpnam(m_pathMDB_tmp),(LPCSTR)csDate);
		if(IDYES==CMsgBox(MB_YESNO, msg)) {
			msg.Empty();
			if(iMatch) m_rs.AddNew();
			else m_rs.Edit();
			SetStatFields(m_rs);
			m_rs.Update();
			CMsgBox("Statistics record number %u successfully %s.",m_rs.GetRecordCount(),iMatch?"appended":"updated");
		}
		CloseMDB();
		return;
	}
	catch (CDaoException* e)
	{
		AfxMessageBox(e->m_pErrorInfo->m_strDescription);
		e->Delete();
		if(!msg.IsEmpty()) msg+="\n\n";
		msg.AppendFormat("No update possible: Can't open or update table tss_statistics in %s.",trx_Stpnam(m_pathMDB_tmp));
	}

_ret:
	if(!msg.IsEmpty()) AfxMessageBox(msg);
	CloseMDB();
}

int CUpdateTSSDlg::ScanDBF(int *mdb_fldnum,int *mdb_fldsrc,int num_mdb_fields)
{
    char cTyp=m_pNamTable[4];

	UINT nRecs=m_dbf.NumRecs();
	CString csVal,errmsg;

	UINT nAppended=0;
	UINT rec=0;
	IT_VEC_DREC itdr;
	VEC_DREC *pDR;
	CString csLenTyp;
	double dLength,dDepth;
	bool bHasQuad,bLocated;

	#if 0
	UINT maxflen=0,maxfrec=0;
	#endif

	while(1) {

		if(cTyp=='a') {
			if(!rec) {
				ASSERT(!vCounty.size());
				numCounties=0;
				trx_Bininit(bufCounty,&numCounties,seq_fcn);
			}
			if(++rec>nRecs) break;
			#if 0
			if(rec==10097) {
				int jj=0;
			}
			#endif
			if(m_dbf.Go(rec)) goto _errF;
			if(m_dbf.IsDeleted()) continue;
			m_dbf.GetTrimmedFldStr(csVal,dbf_fldnum[f_type]);
			if(!csVal.Compare("Cave")) mdb_stats[F_CAVES_S]++;
			else if(!csVal.Compare("Sink")||!csVal.Compare("Cavity")) mdb_stats[F_SINKS_CAVITIES_S]++;
			else if(!csVal.Compare("Shelter")) mdb_stats[F_SHELTERS_S]++;
			else if(!csVal.Compare("Spring")) mdb_stats[F_SPRINGS_S]++;
			else if(csVal.Find('?')>0) mdb_stats[F_UNDEFINED_S]++;
			else continue;

			if(*(char *)m_dbf.FldPtr(dbf_fldnum[f_has_spring])=='Y')
				mdb_stats[F_SPRINGS_S]++;

			m_dbf.GetTrimmedFldStr(csVal,dbf_fldnum[f_latitude]);
			double dLat=atof(csVal);
			m_dbf.GetTrimmedFldStr(csVal,dbf_fldnum[f_longitude]);
			double dLon=atof(csVal);
			bHasQuad=bLocated=true;
			if(dLat>=32.2 && dLon<=-103.3) {
				bLocated=false;
			}
		}
		else if(cTyp!='s') {
			if(!rec) {
				//sort vDepth or vLength vectors --
				pDR=(cTyp=='d')?&vDepth:&vLength;
				std::sort(pDR->begin(),pDR->end(),comp_drec);
				itdr=pDR->begin();
				if(itdr==pDR->end()) break;
			}
			else {
				if(++itdr==pDR->end()) break;
			}
			rec=itdr->rec;
			if(m_dbf.Go(rec)) goto _errF;
		}

		try {
			m_rs.AddNew();

			if(cTyp=='s') {
				SetStatFields(m_rs);
				m_rs.Update();
				nAppended++;
				break;
			}

			dDepth=dLength=0.0;
			csLenTyp.Empty();

			for(int col=0; col<num_mdb_fields; col++) {
				int enSrc=mdb_fldsrc[col];
				if(enSrc<0) {
					if(enSrc==-1) {
						if(!csLenTyp.IsEmpty()) {
							m_rs.SetFieldValue(mdb_fldnum[col],(LPCSTR)csLenTyp);
						}
					}
					else {
						ASSERT(enSrc==-2);
						if(bLocated) mdb_stats[F_LOCATED_S]++;
						else if(bHasQuad) mdb_stats[F_TOPO_ONLY_S]++;
						else mdb_stats[F_UNLOCATED_S]++;
						m_rs.SetFieldValue(mdb_fldnum[col],COleVariant((short)bLocated,VT_BOOL));
					}
					continue;
				}
				
				m_dbf.GetTrimmedFldStr(csVal,dbf_fldnum[enSrc]);
				if(csVal.IsEmpty()) {
					//check if required such as f_tssid, f_name, f_typ --
					if(enSrc==f_tssid || enSrc==f_name || enSrc==f_type) {
						errmsg.Format("DBF record %u: Fields TSSID, NAME, and TYPE must all be non-empty.",rec);
						goto _ret;
					}

					if(dbf_fldtyp[enSrc]=='M') {
						ASSERT(cTyp=='a' && (enSrc==f_maps || enSrc==f_photos));
						m_rs.SetFieldValue(mdb_fldnum[col],COleVariant((short)false,VT_BOOL));
						continue;
					}

					//(f_county and f_quadrangle can be unknown or blank)
					if(cTyp=='a' && enSrc==f_quadrangle && !bLocated) {
						csVal="Unknown";
					}
					else {
						continue;
					}
				}
				switch (dbf_fldtyp[enSrc]) {
					case 'C':
						if(enSrc==f_length) {
							dLength=ParseLength(csVal,csLenTyp);
							if(dLength>0.0) {
								//m_rs.SetFieldValue(mdb_fldnum[col],(cTyp=='a')?dLength:(double)int(dLength+0.5));
								m_rs.SetFieldValue(mdb_fldnum[col],dLength);
							}
							continue;
						}
						if(enSrc==f_depth) {
							dDepth=atof(csVal);
							if(dDepth>0.0) {
								//m_rs.SetFieldValue(mdb_fldnum[col], (cTyp=='a')?dDepth:(double)int(dDepth+0.5));
								m_rs.SetFieldValue(mdb_fldnum[col],dDepth);
							}
							continue;
						}
						if(cTyp=='a' && enSrc==f_quadrangle && !bLocated &&	!csVal.CompareNoCase("Unknown")) {
							bHasQuad=false;
						}

						#if 0
						if(enSrc==f_other_name) {
							UINT ll=csVal.GetLength();
							if(rec==6512 || rec==6748 || rec==2002) {
								ll=0;
							}
							else if(maxflen<ll) {
								maxflen=ll;
								maxfrec=rec;
							}
						}
						#endif

						m_rs.SetFieldValue(mdb_fldnum[col],(LPCSTR)csVal);
						csVal.MakeUpper();
						if(cTyp=='a' && enSrc==f_county) {
							csVal.MakeUpper();
							if(csVal.Compare("UNKNOWN")) {
								pCounty=(LPCSTR)csVal;
								trx_Binsert(vCounty.size(),TRX_DUP_NONE);
								if(!trx_binMatch) {
									vCounty.push_back(csVal);
								}
							}
						}
						continue;

					case 'N':
						ASSERT(cTyp=='a' && (enSrc==f_latitude || enSrc==f_longitude));
						if(bLocated) m_rs.SetFieldValue(mdb_fldnum[col], atof(csVal));
						continue;

					case 'M' :
						ASSERT(cTyp=='a' && (enSrc==f_maps || enSrc==f_photos));
						m_rs.SetFieldValue(mdb_fldnum[col],COleVariant((short)true,VT_BOOL));
						if(enSrc==f_maps) mdb_stats[F_MAPPED_S]++;
						else mdb_stats[F_PHOTOGRAPHED_S]++;
						continue;
				
					case 'L':
					    {
						  ASSERT(cTyp=='a'); //for now
						  short b=(*(LPCSTR)csVal=='Y' || *(LPCSTR)csVal=='T')?1:0;
						  m_rs.SetFieldValue(mdb_fldnum[col],COleVariant(b,VT_BOOL));
						  if(b && cTyp=='a') {
							  switch(enSrc) {
								  case f_rumored : mdb_stats[F_RUMORED_S]++; break;
								  case f_aesthetic : mdb_stats[F_AESTHETIC_S]++; break;
								  case f_archeological : mdb_stats[F_ARCHEOLOGICAL_S]++; break;
								  case f_bad_air : mdb_stats[F_BAD_AIR_S]++; break;
								  case f_bats : mdb_stats[F_BATS_S]++; break;
								  case f_biological : mdb_stats[F_BIOLOGICAL_S]++; break;
								  case f_endangered_sp : mdb_stats[F_ENDANGERED_SP_S]++; break;
								  case f_geological : mdb_stats[F_GEOLOGICAL_S]++; break;
								  case f_historical : mdb_stats[F_HISTORICAL_S]++; break;
								  case f_hydrological : mdb_stats[F_HYDROLOGICAL_S]++; break;
								  case f_paleontological : mdb_stats[F_PALEONTOLOGICAL_S]++; break;
								  default: ASSERT(0);
							  }
						  } //true
					    } //L
				} //dbf_FldTyp
			} //col loop

			if(cTyp=='a') {
				/*
				if(dLength>=300.0) vLength.push_back(TYP_DREC(int(dLength+0.5), int(dDepth+0.5), rec));
				if(dDepth>=30.0) vDepth.push_back(TYP_DREC(int(dDepth+0.5), int(dLength+0.5), rec));
				*/
				if(dLength>=300.0) vLength.push_back(TYP_DREC(float(dLength), float(dDepth), rec));
				if(dDepth>=30.0) vDepth.push_back(TYP_DREC(float(dDepth), float(dLength), rec));
			}
			m_rs.Update();
			nAppended++;
		}
		catch (CDaoException* e)
		{
			e->ReportError();
			e->Delete();
			goto _appErr;
		}

	} //rec loop

	if(cTyp=='a') {
		#if 0
			UINT jj=maxflen;
			jj=maxfrec;
		#endif
		ASSERT(numCounties==vCounty.size());
		mdb_stats[F_COUNTIES_S]=numCounties;
		vCounty.clear(); //not needed
	}
	return nAppended;

_appErr:
	errmsg.Format("Table %s append failure: %u records actually appended",
			m_pNamTable,nAppended);
	goto _ret;

_errF:
	errmsg.Format("Scan terminated: Error accessing DBF record %u",rec);

_ret:
	if(!errmsg.IsEmpty()) CMsgBox("Aborted -- %s.",(LPCSTR)errmsg);
	return -1;
}

void CUpdateTSSDlg::OnBnClickedOk()
{
	CString msg;
	if(!UpdateData()) return;

	if(!OpenDBF()) {
		return;
	}

	if(!OpenMDB()) {
		CloseDBF();
		EndDialog(IDCANCEL);
		return;
	}

	#ifdef _USE_TEMPLATE
	if(!ProcessTmp()) {
		return;
	}
	#endif
	
	nAll_types=nDeep=nLong=0;
	
	BeginWaitCursor();

	m_pNamTable="tss_all_types";
	if(!OpenMDB_Table() || 0>(nAll_types=ScanDBF(mdb_fldnumA,mdb_fldsrcA,NUM_MDB_FIELDS_A)))
		goto _exitErr;

	m_pNamTable="tss_deep_caves";
	if(!OpenMDB_Table() || 0>(nDeep=ScanDBF(mdb_fldnumD,mdb_fldsrcD,NUM_MDB_FIELDS_D)))
		goto _exitErr;

	m_pNamTable="tss_long_caves";
	if(!OpenMDB_Table() || 0>(nLong=ScanDBF(mdb_fldnumD,mdb_fldsrcD,NUM_MDB_FIELDS_D)))
		goto _exitErr;

	m_pNamTable="tss_statistics";
	if(!OpenMDB_Table() || 1!=ScanDBF(mdb_fldnumS,NULL,NUM_MDB_FIELDS_S))
		goto _exitErr;

	EndWaitCursor();
	CloseMDB();
	CloseDBF();

	::WritePrivateProfileString("input","dbfpath",m_pathDBF,m_pathINI);

	msg.Format("File %s successfully written.\n\nRecord counts: %u (all_types), %u (deep caves), %u (long caves).",
		trx_Stpnam(m_pathMDB),nAll_types,nDeep,nLong);

	if(m_bSaveStats) {
		AppendStats(msg);
	}
	else CMsgBox(msg);
	EndDialog(IDOK);

_exitErr:
	EndWaitCursor();
	CloseMDB();
	CloseDBF();
	EndDialog(IDCANCEL);
}
