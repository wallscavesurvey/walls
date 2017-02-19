// UpdZipDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "ShpLayer.h"
#include "UpdZipDlg.h"

enum {
  ZIP_ERR_ADDFILE=1,
  ZIP_ERR_CREATE,
  ZIP_ERR_CLOSE,
  ZIP_ERR_UNKNOWN
};

#define SIZ_MSGBUF 512

// CUpdZipDlg dialog

IMPLEMENT_DYNAMIC(CUpdZipDlg, CDialog)

CUpdZipDlg::~CUpdZipDlg()
{
}

void CUpdZipDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_ADDSHP, m_bAddShp);
}

BEGIN_MESSAGE_MAP(CUpdZipDlg, CDialog)
END_MESSAGE_MAP()


// CUpdZipDlg message handlers

void CUpdZipDlg::SetS3Test(CString &s0,bool bInit)
{
	s0.Format("%sThe updated reference %s %u new records with %s keys.",
		m_pShp->m_pKeyGenerator?"":"CAUTION: ",
		bInit?"will contain":"contains",
		m_nNew,
		m_pShp->m_pKeyGenerator?"assigned":"empty");
	SetText(IDC_ST_STATUS3,s0);
}

BOOL CUpdZipDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CString s;
	FixPath(".zip");
	s.Format("Creating %s",trx_Stpnam(m_pathName));
	SetWindowText(s);

	LPCSTR p;
	UINT nFiles,nLinks;
	__int64 szFiles;
	CShpLayer::GetCompareStats(nLinks,m_nNew,nFiles,szFiles);

	CString s0("The ZIP will contain ");
	if(m_iFlags&1) {
		if(m_iFlags&4) s0+="an updated version of the reference";
		else s0+="one update shapefile";
	}
	if(m_iFlags&2) {
		p=(m_iFlags&1)?" and ":"";
		s.Format("%s%u linked files.",p,nFiles);
		s0+=s;
		SetText(IDC_ST_STATUS,s0);
		char szBuf[20];
		s0.Format("Uncompressed size of linked files: %s KB",CommaSep(szBuf,(szFiles+1023)/1024));
	}
	else {
		s0+='.';
		SetText(IDC_ST_STATUS,s0);
		if(nLinks) {
			s0.Format("You've chosen not to include the %u new linked files that are accessible.",nLinks);
		}
		else s0="There are no memo fields with new file links.";
	}
	SetText(IDC_ST_STATUS2,s0);

	if(m_nNew && (m_iFlags&4)) SetS3Test(s0,true);
	return TRUE;  // return TRUE unless you set the focus to a control
}

int CUpdZipDlg::CopyShpComponent(HZIP hz,CString &path,LPCSTR pExt)
{
	char srcpath[MAX_PATH];
	strcpy(trx_Stpext(strcpy(srcpath,m_pShp->PathName())),pExt);
	FixPath(pExt);
	if(!::CopyFile(srcpath,m_pathName,FALSE)) {
		CMsgBox("CAUTION: A %s file could not be created for the exported shapefile.",pExt);
		return -1;
	}
	VERIFY(SetFileToCurrentTime(m_pathName));
	ReplacePathName(path,trx_Stpnam(m_pathName));
	return ZipAdd(hz,path,m_pathName);
}

int CUpdZipDlg::AddShpComponent(HZIP hz,CString &path,LPCSTR pExt)
{
		FixPath(pExt);
		ReplacePathName(path,trx_Stpnam(m_pathName));
		return ZipAdd(hz,path,m_pathName);
}

void CUpdZipDlg::OnCancel()
{
	if(m_bCompleted) {
		if(m_iFlags&1)	UpdateData(); //initialize m_bAddShp
		EndDialog(IDCANCEL); //don't open log but add shapefile if m_bAddShp==TRUE
		return;
	}
	//choosing not to proceed with ZIP generation --
	ASSERT(!m_bAddShp);
	m_iFlags=0;
	Show(IDOK,1); SetText(IDOK,"Open Log");
	Show(IDCANCEL,1); SetText(IDCANCEL,"Exit");
	SetText(IDC_ST_COMPLETE,"Would you like to open the generated log?");
	m_bCompleted++;
}

void CUpdZipDlg::OnOK()
{
	CString s0,s1,path;
	HZIP hz;
	WORD rootPathLen;
	char keybuf[MAX_PATH+4];

	if(m_bCompleted) {
		if(m_iFlags) UpdateData(); //initialize m_bAddShp
		ASSERT(m_iFlags || !m_bAddShp);
		EndDialog(IDOK);
		return;
	}
	m_bCompleted++;
	SetText(IDC_ST_COMPLETE,"Please wait...");
	Show(IDCANCEL,0);
	Show(IDOK,0);

	bool bWait=true;
	BeginWaitCursor();

	hz=CreateZip(FixPath(".zip"),0);
	if(!hz) {
		goto _zipfailmsg;
	}

	UINT nLinked=0,nFiles=0,nRecs=0;

	if(m_iFlags&1) {
		m_pShp->InitRootPath(path,rootPathLen);
		path=m_pShp->PathName()+rootPathLen;
		//path with name portion replaced will be the zip's path of created components --

		if(!CShpLayer::DeleteComponents(m_pathName)) goto _fail;

		//Write DBF/DBT components --
		if((nRecs=m_pShp->ExportUpdateDbf(keybuf,m_pathName,m_iFlags))<0) {
			goto _fail;
		}

		//Write SHP/SHX components --
		if((m_pShp->ExportUpdateShp(keybuf,m_pathName,m_iFlags))!=nRecs) {
			goto _fail;
		}

		if(AddShpComponent(hz,path,".txt"))
			goto _zipfailmsg;
		if(AddShpComponent(hz,path,SHP_EXT_SHP))
			goto _zipfailmsg;
		if(AddShpComponent(hz,path,SHP_EXT_SHX))
			goto _zipfailmsg;
		if(AddShpComponent(hz,path,SHP_EXT_DBF))
			goto _zipfailmsg;
		if(m_pShp->HasMemos()) {
			if(AddShpComponent(hz,path,SHP_EXT_DBT))
				goto _zipfailmsg;
			nFiles++;
		}
		CopyShpComponent(hz,path,SHP_EXT_TMPSHP);
		CopyShpComponent(hz,path,SHP_EXT_PRJ);
		nFiles+=6;
	}

	if(m_iFlags&2) {
		CShpLayer *pShpR;

		for(int i=c_Trxlinks.First();!i;i=c_Trxlinks.Next()) {
			if(c_Trxlinks.Get(&pShpR,keybuf) || !pShpR) {
				continue;
			}
			pShpR->InitRootPath(path,rootPathLen); //pRootPath==(LPCSTR)path
			//rootPathLen is leading portion (with trail '\') to prepend to file link in keybuf
			//to access file itelf --
			path+=trx_Stxpc(keybuf);
			if(!_access(path,4)) {
				s0.Format("Adding linked files: %s",trx_Stpnam(keybuf));
				SetText(IDC_ST_COMPLETE,s0);
				if(ZR_OK!=ZipAdd(hz,(LPCSTR)path+rootPathLen,path)) {
					//Disk full?
					goto _zipfailmsg;
				}
				nLinked++;
			}
			else {
				//Already tested this
				ASSERT(0);
			}
		}
	}

	ASSERT(hz);
	if(ZR_OK!=CloseZipZ(hz)) { 
		hz=0;
		goto _zipfailmsg;
	}

	EndWaitCursor();

	SetWindowText("Export Completed");
	s0.Format("Archive %s created successfully.",trx_Stpnam(FixPath(".zip")));
	SetText(IDC_ST_STATUS,s0);

	s1.Empty(); path.Empty();
	if(m_iFlags&1) {
		path.Format("1 shapefile (%u recs)",nRecs);
	}
	if(m_iFlags&2) {
		s1.Format("%u linked files",nLinked);
	}
	s0.Format("Contents: %s%s%s",(LPCSTR)path,((m_iFlags&3)==3)?" and ":"",(LPCSTR)s1);
 	//get zip file size in s0 --
    struct __stat64 status;
    if(!_stat64(FixPath(".zip"),&status)) {
	    char sizebuf[20];
		s1.Format(", Size: %s KB",CommaSep(sizebuf,(status.st_size+1023)/1024));
		s0+=s1;
	}
	SetText(IDC_ST_STATUS2,s0);

	if(m_nNew && (m_iFlags&4)) SetS3Test(s0, false);

	if(m_iFlags&1) {
		Show(IDC_ADDSHP,1);
		((CButton *)GetDlgItem(IDC_ADDSHP))->SetCheck(m_bAddShp=1);
	}
	Show(IDOK,1); SetText(IDOK,"Open Log");
	Show(IDCANCEL,1); SetText(IDCANCEL,"Exit");
	SetText(IDC_ST_COMPLETE,"Would you like to open the log?");
	MessageBeep(MB_OK);
	SetFocus();
	return;

_zipfailmsg:
	EndWaitCursor(); bWait=false;
	LPSTR pMsg=(LPSTR)malloc(SIZ_MSGBUF);
	FormatZipMessageZ(lasterrorZ,pMsg,SIZ_MSGBUF);
	CMsgBox("Failure creating archive: %s",pMsg);
	free(pMsg);

_fail:
	if(bWait) EndWaitCursor();
	if(hz) {
		CloseZipZ(hz);
		FixPath(".zip");
		_unlink(m_pathName);
	}
	if(m_iFlags&1)
		CShpLayer::DeleteComponents(m_pathName);

	Show(IDOK,1); SetText(IDOK,"Open Log");
	Show(IDCANCEL,1); SetText(IDCANCEL,"Exit");
	SetText(IDC_ST_STATUS3,"");
	SetText(IDC_ST_COMPLETE,"ZIP creation aborted. Would you like to open the log?");
	m_iFlags=0;
}
