// UpdateTSSDlg.cpp : implementation file
//

#include "stdafx.h"
#include "utility.h"
#include "UpdateTSS.h"
#include "UpdateTSSDlg.h"
#include "PromptPath.h"
#include "filecfg.h"
#include <trx_str.h>
#include <trxfile.h>

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
#ifdef _USE_EXP
	"DEPTH_EXP",
	"LENGTH_EXP",
#endif
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
#ifdef _USE_EXP
	f_depth_exp,
	f_length_exp,
#endif
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

//MDB tss_longdeep fields --
enum {
	F_LONG_DEEP_ID,
	F_LD_TSSID,
	F_LD_CAVE_NAME,
	F_LD_CAVE_COUNTY,
	F_LD_CAVE_LENGTH,
	F_LD_LENGTH_TYPE,
	F_LD_CAVE_DEPTH
};

static LPCSTR mdb_fldnameLD[]={
	"longdeep_id",
	"ld_tssid",
	"ld_cave_name",
	"ld_cave_county",
	"ld_cave_length",
	"ld_cave_length_type",
	"ld_cave_depth"
};

#define NUM_MDB_FIELDS_LD (sizeof(mdb_fldnameLD)/sizeof(LPCSTR))
static int mdb_fldnumLD[NUM_MDB_FIELDS_LD];

//MDB tss_county_totals fields --
enum {
	F_CT_COUNTY_TOTALS_ID,
	F_CT_COUNTY_NAME,
	F_CT_CAVE_COUNT,
	F_CT_SINK_FEATURE_COUNT,
	F_CT_SHELTER_COUNT,
	F_CT_SPRING_COUNT,
	F_CT_UNDEFINED_COUNT,
	F_CT_COUNTY_TOTAL
};

static LPCSTR mdb_fldnameCT[]={
	"county_totals_id",
	"county_name",
	"cave_count",
	"sink_feature_count",
	"shelter_count",
	"spring_count",
	"undefined_count",
	"county_total"
};

#define NUM_MDB_FIELDS_CT (sizeof(mdb_fldnameCT)/sizeof(LPCSTR))
static int mdb_fldnumCT[NUM_MDB_FIELDS_CT];

//tss_statistics fields --
enum {
	F_ST_STATISTICS_ID,
	F_ST_DATABASE_DATE,
	F_ST_DATABASE_RECORDS,
	F_ST_CAVE_COUNT,
	F_ST_SINK_FEATURE_COUNT,
	F_ST_SHELTER_COUNT,
	F_ST_SPRING_COUNT,
	F_ST_UNDEFINED_COUNT,
	F_ST_RUMORED_COUNT,
	F_ST_AESTHETIC_COUNT,
	F_ST_ARCHEOLOGICAL_COUNT,
	F_ST_BAD_AIR_COUNT,
	F_ST_BAT_CAVE_COUNT,
	F_ST_BIOLOGICAL_COUNT,
	F_ST_ENDANGERED_COUNT,
	F_ST_GEOLOGICAL_COUNT,
	F_ST_HISTORICAL_COUNT,
	F_ST_HYDROLOGICAL_COUNT,
	F_ST_PALEONTOLOGICAL_COUNT,
	F_ST_COUNTY_COUNT,
	F_ST_LOCATION_COUNT,
	F_ST_TOPOGRAPHIC_ONLY,
	F_ST_LOST_CAVE_COUNT,
	F_ST_MAPPED_COUNT,
	F_ST_PHOTOGRAPHED_COUNT
};

static LPCSTR mdb_fldnameS[]={
	"statistics_id",
	"database_date",
	"database_records",
	"cave_count",
	"sink_feature_count",
	"shelter_count",
	"spring_count",
	"undefined_count",
	"rumored_count",
	"aesthetic_count",
	"archeological_count",
	"bad_air_count",
	"bat_cave_count",
	"biological_count",
	"endangered_count",
	"geological_count",
	"historical_count",
	"hydrological_count",
	"paleontological_count",
	"county_count",
	"location_count",
	"topographic_only_count",
	"lost_cave_count",
	"mapped_count",
	"photographed_count"
};

#define NUM_MDB_FIELDS_S (sizeof(mdb_fldnameS)/sizeof(LPCSTR))
static int mdb_fldnumS[NUM_MDB_FIELDS_S];
static long mdb_stats[NUM_MDB_FIELDS_S];

static UINT num_dbfRecs;
static int num_mdb_flds;
static LPCSTR *mdb_fldname;
static int *mdb_fldnum;
static char pDateStr[]="yyyy_mm_dd";

static int nLongDeep=0,nDeep=0,nLong=0,nDeleted=0;

struct CNTY_DATA {
	void Clear() {nCaves=nSinks=nShelters=nSprings=nUndefined=0;}
	WORD nCaves;
	WORD nSinks;
	WORD nShelters;
	WORD nSprings;
	WORD nUndefined;
};

static CTRXFile trx_cnty;
static char keybuf[256];

#ifdef _USE_TEMPLATE
#define MAX_TEMPLATES	30
static char szAppPath[MAX_PATH];
static LPSTR pNamTemplate;
static int nTemplates=0,nMemoTrunc=0;
static LPCSTR spaces=":                                                           ";
#define LEFT_MGN 25


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
			CMsgBox("File %s is absent or protected.", (LPCSTR)m_pathDBF);
			pDX->Fail();
			return;
		}
		m_pathMDB=m_pathDBF;
		m_pathMDB.Truncate(trx_Stpnam(m_pathMDB)-(LPCSTR)m_pathMDB);
		m_pathMDB+="TSS_DB_";
		m_pathMDB+=pDateStr;
		m_pathMDB+=".mdb";
		m_pNamMDB=trx_Stpnam(m_pathMDB);
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
	trx_cnty.CloseDel(/*bDelCache=TRUE*/);
}



BOOL CUpdateTSSDlg::OpenDBF()
{
	if(!dbf_defCache && csh_Alloc((TRX_CSH far *)&dbf_defCache,4096,32)) {
		CMsgBox("Cannot allocate shapefile cache");
		return FALSE;
	}

	if(m_dbf.Open(m_pathDBF,CDBFile::ReadOnly)) {
		CMsgBox("File %s: Open failure - %s.",trx_Stpnam(m_pathDBF),m_dbf.Errstr());
		CloseDBF(); //only frees cache
		return FALSE;
	}
	num_dbfRecs=m_dbf.NumRecs();

	CString sTemp;
	if(!GetTempFilePathWithExtension(sTemp,"trx") || trx_cnty.Create(sTemp,sizeof(CNTY_DATA)) ||
		   trx_cnty.AllocCache(32/*,makedef=TRUE*/)) {
		CMsgBox("Failed to created temporary index: ",CTRXFile::Errstr());
		goto _errExit;
	}
	trx_cnty.SetUnique();
	trx_cnty.SetExact();

	//Check for field IDs --
	for(int f=0;f<NUM_DBF_FIELDS;f++) {
		if(!(dbf_fldnum[f]=m_dbf.FldNum(dbf_fldname[f]))) {
			CMsgBox("Expected field %s not present in DBF.",dbf_fldname[f]);
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
	CloseDBF(); //only frees cache
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
		case 'l':
			num_mdb_flds=NUM_MDB_FIELDS_LD;
			mdb_fldname=mdb_fldnameLD;
			mdb_fldnum=mdb_fldnumLD;
			break;
		case 'c':
			num_mdb_flds=NUM_MDB_FIELDS_CT;
			mdb_fldname=mdb_fldnameCT;
			mdb_fldnum=mdb_fldnumCT;
			break;
		case 's':
			num_mdb_flds=NUM_MDB_FIELDS_S;
			mdb_fldname=mdb_fldnameS;
			mdb_fldnum=mdb_fldnumS;
			break;
		default: ASSERT(0);
	}

	if(m_rs.GetFieldCount()!=num_mdb_flds) {
		CMsgBox("Table %s in template MDB does not have the required set of %u fields.",m_pNamTable,num_mdb_flds);
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
		ASSERT(info.m_nOrdinalPosition>=0 && info.m_nOrdinalPosition<num_mdb_flds);

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
		// Assume failure is becauase we couldn't find the table
		e->Delete();
		CMsgBox("Failure opening table %s in %s.",m_pNamTable,m_pNamMDB);
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

static bool SetStatFields(CDaoRecordset &m_rs)
{
	mdb_stats[F_ST_COUNTY_COUNT]=trx_cnty.NumRecs();
	mdb_stats[F_ST_DATABASE_RECORDS]=num_dbfRecs;
	m_rs.SetFieldValue(mdb_fldnum[F_ST_DATABASE_DATE],pDateStr);
	int col=1;
	try {
	    for(; col<NUM_MDB_FIELDS_S; col++) {
			if(col!=F_ST_DATABASE_DATE)
				m_rs.SetFieldValue(mdb_fldnum[col], mdb_stats[col]);
		}
		m_rs.Update();
	}
	catch(CDaoException* e) {
		CString s;
		if(col<NUM_MDB_FIELDS_S) s.Format(" field %s of",mdb_fldname[col]);
		CMsgBox("Error updating%s table tss_statistics: %s",(LPCSTR)s,(LPCSTR)e->m_pErrorInfo->m_strDescription);
		e->Delete();
		return false;
	}
	return true;
}

bool CUpdateTSSDlg::AppendStats(BOOL bLocal,CString &msg)
{
	try
	{
		if(bLocal) {
			m_pNamMDB=trx_Stpnam(m_pathMDB_tmp);
			m_db.Open(m_pathMDB_tmp,TRUE,FALSE,""); //(path,exclusive,readonly,type));
			m_rs.m_pDatabase=&m_db;
		}

		if(!OpenMDB_Table()) {
			return false;
		}

		int iMatch=-1;
		CString csDate;
		if(!m_rs.IsBOF()) {
			m_rs.MoveLast();
			GetString(csDate,m_rs,F_ST_DATABASE_DATE);
			if((iMatch=csDate.Compare(pDateStr))>0) {
				msg.AppendFormat("\n\nCan't update table %s in %s since the last record that was appended, dated %s, is newer than %s.",
					m_pNamTable,m_pNamMDB,(LPCSTR)csDate,pDateStr);
				goto _ret;
			}
		}
		else csDate="None";
		bool bSet=true;
		if(bLocal) {
			CString s;
			if(iMatch) s.Format(" (Prior record: %s)",(LPCSTR)csDate);
			msg.AppendFormat("\n\nConfirm: %s record dated %s %s template's tss_statistics table?%s",
				iMatch?"Copy":"Update",pDateStr, iMatch?"to":"in",(LPCSTR)s);
	
			bSet=(IDYES==CMsgBox(MB_YESNO, msg));
			msg.Empty();
		}
		if(bSet) {
		    if(iMatch) m_rs.AddNew();
			else m_rs.Edit();
			if(!SetStatFields(m_rs)) return false; //calls m_rs.Update() if successful
			//if(bLocal) CMsgBox("Record dated %s successfully %s.", pDateStr, iMatch?"appended":"updated");
		}
		CloseMDB();
		return true;
	}
	catch (CDaoException* e)
	{
		if(!msg.IsEmpty()) msg+="\n\n";
		msg.AppendFormat("Can't open or update table tss_statistics in %s: %s.",
			m_pNamMDB, (LPCSTR)e->m_pErrorInfo->m_strDescription);
		e->Delete();
	}

_ret:
	if(!msg.IsEmpty()) AfxMessageBox(msg);
	return false;
}

static LPCSTR FixCountyName()
{
	_strlwr(trx_Stxpc(keybuf));
	bool bsp=true;
	for(LPSTR p=keybuf; *p; p++) {
		if(*p==' ') bsp=true;
		else {
			if(bsp) *p=_toupper(*p);
			bsp=false;
		}
	}
	return keybuf;
}

bool CUpdateTSSDlg::ScanCounties()
{
	CNTY_DATA cData;
	CString errmsg("Error scanning county index.");
	int e=trx_cnty.First();
	try {
		while(!e) {
			if(trx_cnty.Get(&cData,sizeof(cData),keybuf))
				goto _eret;
			long total=0;
			m_rs.AddNew();
			m_rs.SetFieldValue(F_CT_COUNTY_NAME,FixCountyName());
			m_rs.SetFieldValue(F_CT_CAVE_COUNT,(long)cData.nCaves); total+=cData.nCaves;
			m_rs.SetFieldValue(F_CT_SINK_FEATURE_COUNT,(long)cData.nSinks); total+=cData.nSinks;
			m_rs.SetFieldValue(F_CT_SHELTER_COUNT,(long)cData.nShelters); total+=cData.nShelters;
			m_rs.SetFieldValue(F_CT_SPRING_COUNT,(long)cData.nSprings); total+=cData.nSprings;
			m_rs.SetFieldValue(F_CT_UNDEFINED_COUNT,(long)cData.nUndefined); total+=cData.nUndefined;
			m_rs.SetFieldValue(F_CT_COUNTY_TOTAL,total);
			m_rs.Update();
			e=trx_cnty.Next();
		}
		if(e!=CTRXFile::ErrEOF) {
			goto _eret;
		}
	}
	catch(CDaoException* e) {
		errmsg.Format("Error appending to %s: %s.",m_pNamTable,(LPCSTR)e->m_pErrorInfo->m_strDescription);
		e->Delete();
		goto _eret;
	}
	return true;

_eret:
	AfxMessageBox(errmsg);
	return false;
}

static bool IndexCounty(CString &csName,CNTY_DATA &cData)
{
	csName.MakeUpper();
	if(csName.Compare("UNKNOWN")) {
		trx_Stccp(keybuf, csName);
		if(!trx_cnty.Find(keybuf)) {
			CNTY_DATA data;
			if(trx_cnty.GetRec(&data,sizeof(cData)))
				return false;
			data.nCaves+=cData.nCaves;
			data.nSinks+=cData.nSinks;
			data.nShelters+=cData.nShelters;
			data.nSprings+=cData.nSprings;
			data.nUndefined+=cData.nUndefined;
			if(trx_cnty.PutRec(&data))
				return false;
		}
		else if(trx_errno==CTRXFile::ErrMatch || trx_errno==CTRXFile::ErrEOF) {
			if(trx_cnty.InsertKey(&cData,keybuf))
				return false;
		}
		else return false;
	}
	return true;
}

#ifdef _USE_EXP
struct DLX {
	DLX(const CString &_name, double _d, double _dx) : name(_name), d(_d), dx(_dx) {}
	CString name;
	double d;
	double dx;
};

typedef std::vector<DLX> VEC_DLX;
typedef VEC_DLX::iterator IT_DLX;
static VEC_DLX v_deep,v_long,v_longx,v_deepx;

static FILE *fp;
static bool compare_dlx(DLX &d0,DLX &d1)
{
	return d0.dx>d1.dx;
}

void ListCaves(VEC_DLX &v,LPCSTR msg)
{
	char spc[80];
	memset(spc, '-',62);
	spc[62]=0;
	fprintf(fp, "\n%s\n%s\n",msg,spc);
	std::sort(v.begin(),v.end(),compare_dlx);
	for(IT_DLX it=v.begin(); it!=v.end(); it++) {
		int len=36-it->name.GetLength();
		if(len>0) memset(spc, ' ',len);	else len=0;
		spc[len]=0;
		fprintf(fp,"%s:%s Srv:%5.0f m  Exp:%5.0f m\n",(LPCSTR)it->name,spc,it->d,it->dx);
	}
}
#endif

bool CUpdateTSSDlg::ScanDBF()
{
	CString csVal,errmsg;
	UINT nAppended=0;
	UINT rec;
	CString csLenTyp;
	CNTY_DATA cData;

	double dLength,dDepth;
	bool bIncluded,bLocated;

	for(rec=1;rec<=num_dbfRecs;rec++) {

		if(m_dbf.Go(rec)) goto _errF;
		if(m_dbf.IsDeleted()) continue;
		cData.Clear();

		if(*(char *)m_dbf.FldPtr(dbf_fldnum[f_rumored])=='Y') {
			mdb_stats[F_ST_RUMORED_COUNT]++;
			continue;
		}
		bIncluded=false;
		m_dbf.GetTrimmedFldStr(csVal,dbf_fldnum[f_type]);
		if(!csVal.Compare("Cave")) {
			mdb_stats[F_ST_CAVE_COUNT]++;
			cData.nCaves++;
			m_dbf.GetTrimmedFldStr(csVal,dbf_fldnum[f_length]);
			csLenTyp.Empty();
			dLength=ParseLength(csVal, csLenTyp);
			m_dbf.GetTrimmedFldStr(csVal,dbf_fldnum[f_depth]);
			dDepth=atof(csVal);
			bIncluded=dLength>=300.0 || dDepth>=30.0;
			if(bIncluded) {
				nLongDeep++;
				if(dLength>=300.0) {
					nLong++;
					#ifdef _USE_EXP
					m_dbf.GetTrimmedFldStr(csVal, dbf_fldnum[f_length_exp]);
					double d=atof(csVal);
					if(d>dLength) {
						m_dbf.GetTrimmedFldStr(csVal, dbf_fldnum[f_name]);
						v_long.push_back(DLX(csVal,dLength,d));
					}
					#endif
				}
				if(dDepth>=30.0) {
					nDeep++;
					#ifdef _USE_EXP
					m_dbf.GetTrimmedFldStr(csVal, dbf_fldnum[f_depth_exp]);
					double d=atof(csVal);
					if(d>dDepth) {
						m_dbf.GetTrimmedFldStr(csVal, dbf_fldnum[f_name]);
						v_deep.push_back(DLX(csVal,dDepth,d));
					}
					#endif
				}
			}
			#ifdef _USE_EXP
			else {
				m_dbf.GetTrimmedFldStr(csVal,dbf_fldnum[f_length_exp]);
				double d=atof(csVal);
				if(d>=300.0) {
						m_dbf.GetTrimmedFldStr(csVal, dbf_fldnum[f_name]);
						v_longx.push_back(DLX(csVal,dLength,d));
				}
				m_dbf.GetTrimmedFldStr(csVal,dbf_fldnum[f_depth_exp]);
				d=atof(csVal);
				if(d>=30.0) {
					m_dbf.GetTrimmedFldStr(csVal, dbf_fldnum[f_name]);
					v_deepx.push_back(DLX(csVal,dDepth,d));
				}
			}
			#endif
		}
		else {
			if(!csVal.Compare("Sink")||!csVal.Compare("Cavity")) {
				mdb_stats[F_ST_SINK_FEATURE_COUNT]++;
				cData.nSinks++;
			}
			else if(!csVal.Compare("Shelter")) {
				mdb_stats[F_ST_SHELTER_COUNT]++;
				cData.nShelters++;
			}
			else if(!csVal.Compare("Spring")) {
				mdb_stats[F_ST_SPRING_COUNT]++;
				cData.nSprings++;
			}
			else if(csVal.Find('?')>0) {
				mdb_stats[F_ST_UNDEFINED_COUNT]++;
				cData.nUndefined++;
			}
			else continue;
		}

		if(!cData.nSprings && *(char *)m_dbf.FldPtr(dbf_fldnum[f_has_spring])=='Y') {
			mdb_stats[F_ST_SPRING_COUNT]++;
			cData.nSprings++;
		}

		m_dbf.GetTrimmedFldStr(csVal,dbf_fldnum[f_county]);
		if(!IndexCounty(csVal,cData))
			goto _errTrx;

		m_dbf.GetTrimmedFldStr(csVal,dbf_fldnum[f_latitude]);
		double dLat=atof(csVal);
		m_dbf.GetTrimmedFldStr(csVal,dbf_fldnum[f_longitude]);
		bLocated=(dLat<32.2 && atof(csVal)>-103.3);
		if(bLocated) mdb_stats[F_ST_LOCATION_COUNT]++;
		else {
			m_dbf.GetTrimmedFldStr(csVal,dbf_fldnum[f_quadrangle]);
			if(!csVal.IsEmpty() && csVal.CompareNoCase("Unknown")) mdb_stats[F_ST_TOPOGRAPHIC_ONLY]++;
			else mdb_stats[F_ST_LOST_CAVE_COUNT]++;
		}

		//Now  scan relevant fields in m_dbf to accumulate additional statistics.
		//If this a long or deep cave (bIncluded) also append new record to ld_longdeep table in m_db.
		try {
			if(bIncluded) {
				m_rs.AddNew();
			}

			for(UINT f=0; f<NUM_DBF_FIELDS; f++) {
				UINT fn=dbf_fldnum[f];
				char fTyp=m_dbf.FldTyp(fn);

				if(bIncluded) {
					if(fTyp=='C') {
						//Only four C fields in appended record --
						if(f==f_length) {
							if(dLength>0.0) {
								m_rs.SetFieldValue(mdb_fldnum[F_LD_CAVE_LENGTH],dLength);
								m_rs.SetFieldValue(mdb_fldnum[F_LD_LENGTH_TYPE],(LPCSTR)csLenTyp);
							}
						}
						else if(f==f_depth) {
							if(dDepth>0.0) {
								m_rs.SetFieldValue(mdb_fldnum[F_LD_CAVE_DEPTH],dDepth);
							}
						}
						else if(f==f_name || f==f_county || f==f_tssid) {
							m_dbf.GetTrimmedFldStr(csVal,dbf_fldnum[f]);
							m_rs.SetFieldValue(mdb_fldnum[(f==f_name)?F_LD_CAVE_NAME:((f==f_tssid)?F_LD_TSSID:F_LD_CAVE_COUNTY)], (LPCSTR)csVal);
						}
						continue;
					}
				}
				//Whether appended or not, accumulate stats for two M fields and 11 L fields --
				if(fTyp=='M') {
					ASSERT(f==f_maps || f==f_photos);
					if(*(LPCSTR)m_dbf.FldPtr(fn)!=' ') {
						if(f==f_maps) mdb_stats[F_ST_MAPPED_COUNT]++;
						else if(f==f_photos) mdb_stats[F_ST_PHOTOGRAPHED_COUNT]++;
					}
				}
				else if(fTyp=='L') {
					LPCSTR pL=(LPCSTR)m_dbf.FldPtr(fn);
					if(*pL=='Y' || *pL=='T') {
						switch(f) {
							case f_rumored : mdb_stats[F_ST_RUMORED_COUNT]++; break;
							case f_aesthetic : mdb_stats[F_ST_AESTHETIC_COUNT]++; break;
							case f_archeological : mdb_stats[F_ST_ARCHEOLOGICAL_COUNT]++; break;
							case f_bad_air : mdb_stats[F_ST_BAD_AIR_COUNT]++; break;
							case f_bats : mdb_stats[F_ST_BAT_CAVE_COUNT]++; break;
							case f_biological : mdb_stats[F_ST_BIOLOGICAL_COUNT]++; break;
							case f_endangered_sp : mdb_stats[F_ST_ENDANGERED_COUNT]++; break;
							case f_geological : mdb_stats[F_ST_GEOLOGICAL_COUNT]++; break;
							case f_historical : mdb_stats[F_ST_HISTORICAL_COUNT]++; break;
							case f_hydrological : mdb_stats[F_ST_HYDROLOGICAL_COUNT]++; break;
							case f_paleontological : mdb_stats[F_ST_PALEONTOLOGICAL_COUNT]++; break;
							case f_has_spring : break; //already handled
							default: ASSERT(0);
						}
					}
				} //L
			} //f loop

			if(bIncluded) {
				m_rs.Update();
				nAppended++;
			}
		}
		catch (CDaoException* e)
		{
			errmsg.Format("Error updating %s (%u recs appended): %s", m_pNamTable, nAppended, (LPCSTR)e->m_pErrorInfo->m_strDescription);
			e->Delete();
			goto _ret;
		}

	} //rec loop

	#ifdef _USE_EXP
	if(!(fp=fopen("SRV_EXP.txt", "w"))) return true;

	ListCaves(v_long,"Lengths of Long Caves with Explored Length > Surveyed Length");
	ListCaves(v_longx,"Lengths of Unlisted Caves with Explored Length >= 300 m");

	ListCaves(v_deep,"Depths of Deep Caves with Explored Depth > Surveyed Depth");
	ListCaves(v_deepx,"Depths of Unlisted Caves with Explored Depth >= 30");

	fclose(fp);
	#endif

	return true;

_errTrx:
	errmsg.Format("County indexing failure (dbf record %u): %s",rec,CTRXFile::Errstr());
	goto _ret;

_errF:
	errmsg.Format("Scan terminated: Error accessing DBF record %u",rec);

_ret:
	if(!errmsg.IsEmpty()) CMsgBox("Aborted -- %s.",(LPCSTR)errmsg);
	return false;
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
	
	nLongDeep=nDeep=nLong=0;
	
	BeginWaitCursor();

	m_pNamTable="tss_longdeep";
	if(!OpenMDB_Table() || !ScanDBF())
		goto _exitErr;

	m_pNamTable="tss_county_totals";
	if(!OpenMDB_Table() || !ScanCounties())
		goto _exitErr;

	//Finally append (or update last) one record to MDB and local statisics table --
	m_pNamTable="tss_statistics";
	if(!AppendStats(FALSE,msg) || (m_bSaveStats && !AppendStats(TRUE,msg)))
		goto _exitErr;

	EndWaitCursor();
	trx_cnty.CloseDel();
	CloseMDB();
	CloseDBF();

	::WritePrivateProfileString("input","dbfpath",m_pathDBF,m_pathINI);

	CMsgBox("File %s successfully written.\n\nRecords in table tss_longdeep: %u (%u long caves, %u deep caves)",
		trx_Stpnam(m_pathMDB),nLongDeep,nLong,nDeep);

	EndDialog(IDOK);
	return;

_exitErr:
	EndWaitCursor();
	trx_cnty.CloseDel();
	CloseMDB();
	CloseDBF();
	EndDialog(IDCANCEL);
}
