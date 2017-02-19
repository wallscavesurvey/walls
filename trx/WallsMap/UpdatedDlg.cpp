// UpdatedDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MainFrm.h"
#include "ShpLayer.h"
#include "UpdatedDlg.h"

// CUpdatedDlg dialog

IMPLEMENT_DYNAMIC(CUpdatedDlg, CDialog)

CUpdatedDlg::CUpdatedDlg(SHP_DBFILE *pdbfile,BOOL bUpdating,CWnd* pParent /*=NULL*/)
	: CDialog(CUpdatedDlg::IDD, pParent)
	, m_pdbfile(pdbfile)
	, m_bUpdating(bUpdating)
	, m_EditorName(SHP_DBFILE::csEditorName)
	, m_bEditorPreserved(SHP_DBFILE::bEditorPreserved)
	, m_bIgnoreEditTS(bIgnoreEditTS?TRUE:FALSE)
{
}

CUpdatedDlg::~CUpdatedDlg()
{
}

void CUpdatedDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_PRESERVE_NAME, m_bEditorPreserved);
	DDX_Control(pDX, IDC_EDIT_SOURCE,m_ceEditorName);
	DDX_Text(pDX, IDC_EDIT_SOURCE, m_EditorName);
	DDV_MaxChars(pDX, m_EditorName, 30);
	DDX_Check(pDX, IDC_SUPPRESS_TS, m_bIgnoreEditTS);

	if(pDX->m_bSaveAndValidate) {
		ASSERT(bTS_OPT || !bIgnoreEditTS && !m_bIgnoreEditTS);
		m_EditorName.Trim();
		int len=m_EditorName.GetLength();

		if(len<6 || !strchr(m_EditorName,' ')) {
			//name empty or questionable --
			if(!len) {
				CString msg("A non-blank name is required when editing or extending shapefiles with timestamp fields. ");

				if(!m_bUpdating) {
					msg+="If you leave the name blank you'll be prompted for a name when attempting "
						"to modify such a shapefile. Is that your intention?";
				}
				else {
					msg+="You must either enter a name or cancel the dialog and refrain from editing.";
				}
				int id=CMsgBox(m_bUpdating?IDOK:MB_YESNO,"Empty Name Field: %s",(LPCSTR)msg);
				if(m_bUpdating) id=0;
				else {
					if(id==IDYES) {
						m_bEditorPreserved=FALSE; //Clear registy of any saved name
					}
					else id=0;
				}
				if(!id) {
					GetDlgItem(IDC_EDIT_SOURCE)->SetWindowText(m_EditorName=SHP_DBFILE::csEditorName);
					pDX->m_idLastControl=IDC_EDIT_SOURCE;
					pDX->m_bEditLastControl=TRUE;
					pDX->Fail();
				}
			}
			else {
				if(IDYES!=CMsgBox(MB_YESNO,"Caution: \"%s\" doesn't appear to be a complete name. Note that once "
					"a record is added there's no direct way to modify the CREATED_ timestamp field.\n\n"
					"Are you sure you want to use \"%s\"?",(LPCSTR)m_EditorName,(LPCSTR)m_EditorName)) {
						pDX->m_idLastControl=IDC_EDIT_SOURCE;
						pDX->m_bEditLastControl=TRUE;
						pDX->Fail();
				}
			}
		}

		if(/*!m_bUpdating && */ (SHP_DBFILE::bEditorPreserved!=(m_bEditorPreserved==TRUE) ||
			m_EditorName.Compare(SHP_DBFILE::csEditorName))) {
				//might as well fix registry at this point
				theApp.WriteProfileString(szPreferences,m_bEditorPreserved?szEditorName:NULL,m_bEditorPreserved?m_EditorName:NULL);
				SHP_DBFILE::bEditorPreserved=(m_bEditorPreserved==TRUE);
		}
		SHP_DBFILE::csEditorName=m_EditorName;
		SHP_DBFILE::bEditorPrompted=!m_EditorName.IsEmpty();
		bIgnoreEditTS=(m_bIgnoreEditTS==TRUE);
	}
}

BEGIN_MESSAGE_MAP(CUpdatedDlg, CDialog)
	ON_BN_CLICKED(IDCANCEL, &CUpdatedDlg::OnBnClickedCancel)
	ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()


// CUpdatedDlg message handlers

BOOL CUpdatedDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if(m_bUpdating) {
		CString s;
		s.Format("%s this shapefile updates a date-time field tagged with a name for identifying the source. Once you enter a name, "
			"no more prompting will occur during this program session. The name is also a Preferences "
			"menu setting that's optionally preserved across program sessions.",
			(m_bUpdating==2)?"Adding records to":"Modifying records in");

		GetDlgItem(IDC_ST_DESC)->SetWindowText(s);

		s.Format("Shapefile: %s",m_pdbfile->db.FileName());
		SetWindowText(s);

		//GetDlgItem(IDC_PRESERVE_NAME)->ShowWindow(SW_HIDE);
		
	}

	//generate UTC timestamp --
	char ts_buf[64];
	struct tm tm_utc;
	__time64_t ltime;

	_time64(&ltime);

	if(ltime==-1 || _gmtime64_s(&tm_utc, &ltime)) *ts_buf=0;
	else GetTimestamp(ts_buf,tm_utc);
	GetDlgItem(IDC_UTC_TIME)->SetWindowText(ts_buf);

	if(*ts_buf) {
		memset(&tm_utc,0,sizeof(tm_utc));
		if(_localtime64_s(&tm_utc, &ltime)) *ts_buf=0;
		else GetTimestamp(ts_buf,tm_utc);
	}
	GetDlgItem(IDC_LOCAL_TIME)->SetWindowText(ts_buf);

	if(bTS_OPT) GetDlgItem(IDC_SUPPRESS_TS)->ShowWindow(SW_SHOW);

	//GetDlgItem(IDC_EDIT_SOURCE)->SetFocus();

	return TRUE;  // return TRUE unless you set the focus to a control
}


void CUpdatedDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	CDialog::OnCancel();
}

LRESULT CUpdatedDlg::OnCommandHelp(WPARAM,LPARAM)
{
	AfxGetApp()->WinHelp(1019);
	return TRUE;
}
