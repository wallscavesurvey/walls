// ZipDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "WallsMapDoc.h"
#include "NtiLayer.h"
#include "zip.h"
#include "ZipDlg.h"

// CZipDlg dialog

IMPLEMENT_DYNAMIC(CZipDlg, CDialog)

static bool bIncHidden=false,bIncLinked=false,bShpOnly=true,bPtsOnly=true;

#define SIZ_MSGBUF 512
//=======================================================

enum {
  ZIP_ERR_ADDFILE=1,
  ZIP_ERR_CREATE,
  ZIP_ERR_CLOSE,
  ZIP_ERR_UNKNOWN
};

static HZIP hz;
static LPCSTR pRootPath;
static int rootPathLen;
static int numErrDir,numErrFind,numErrAccess,numAdded;

static void FixPathName(CString &pathName,LPCSTR ext)
{
	int i=pathName.ReverseFind('.');
	if(i>=0) pathName.Truncate(i);
	pathName+=ext;
}

CZipDlg::CZipDlg(CWallsMapDoc *pDoc,CWnd* pParent /*=NULL*/)
	: CDialog(CZipDlg::IDD, pParent)
	, m_pDoc(pDoc)
	, m_pathName(pDoc->GetPathName())
	, m_bIncHidden(bIncHidden)
	, m_bIncLinked(bIncLinked)
	, m_bShpOnly(bShpOnly)
	, m_bPtsOnly(bPtsOnly)
{
	FixPathName(m_pathName,".zip");
	m_errmsg=(LPSTR)malloc(SIZ_MSGBUF);
}

CZipDlg::~CZipDlg()
{
	free(m_errmsg);
}

BEGIN_MESSAGE_MAP(CZipDlg, CDialog)
	ON_BN_CLICKED(IDBROWSE, &CZipDlg::OnBnClickedBrowse)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

HBRUSH CZipDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    if(pWnd->GetDlgCtrlID()==IDC_ST_FILEMSG) {
		pDC->SetTextColor(RGB(0x80,0,0));
    }
	return hbr;
}

int CZipDlg::AddFileToZip(LPCSTR pFilePath,int iTyp)
{
	LPSTR p=(LPSTR)pFilePath;

	if(iTyp>1) {
		//zip file name not same as pFilePath
		p=m_errmsg;
		strcpy(p,pFilePath);
		strcpy(trx_Stpnam(p),trx_Stpnam(m_pathName));
		strcpy(trx_Stpext(p),".ntl");
	}

	ASSERT((int)strlen(p)>rootPathLen && !_memicmp(p,pRootPath,rootPathLen));

	SetText(IDC_ST_FILEMSG,p+=rootPathLen);

	if(ZR_OK!=ZipAdd(hz,p,pFilePath)) {
		if(!iTyp && lasterrorZ==ZR_READ) {
			lasterrorZ=0;
			numErrAccess++;
			return 0;
		}
		FormatZipMessageZ(lasterrorZ,m_errmsg+sprintf(m_errmsg,"%s: ",p),SIZ_MSGBUF);
		return 1;
	}
	numAdded++;
	return 0;
}

int CZipDlg::AddShpToZip(CShpLayer *pShp)
{
    int iRet=1;

	CShpLayer *pEditor=pShp->m_pdbfile->pShpEditor;
	if(pEditor && !pEditor->SetEditable()) {
		//shouldn't happen at this stage!
		*m_errmsg=0;
		return 1;
	}

	LPSTR pExt=trx_Stpext(strcpy(m_errmsg,pShp->PathName()));

	if(AddFileToZip(m_errmsg,1)) {
		ASSERT(lasterrorZ!=ZR_NOFILE);
		goto _ret;
	}

	for(int i=1;i<CShpLayer::EXT_QPJ;i++) { //skip unsupported extensions
		if(i==CShpLayer::EXT_DBT && !pShp->HasMemos() || i==CShpLayer::EXT_TMPSHP && !pShp->m_pdbfile->bHaveTmpshp)
		   continue;
		strcpy(pExt,CShpLayer::shp_ext[i]);
		if(!_access(m_errmsg,0)) {
			if(AddFileToZip(m_errmsg,1)) {
				goto _ret;
			}
		}
		else if(i!=CShpLayer::EXT_DBE && i!=CShpLayer::EXT_PRJ) {
			sprintf(m_errmsg,"Component %s for %s missing",pExt+1,pShp->Title());
			goto _ret;
		}
	}
	iRet=0;

_ret:
	if(pEditor) {
		VERIFY(pEditor->SetEditable());
	}
	return iRet;
}

void CZipDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PATHNAME, m_cePathName);
	DDX_Text(pDX, IDC_PATHNAME, m_pathName);
	DDV_MaxChars(pDX, m_pathName, 260);
	DDX_Check(pDX, IDC_INC_HIDDEN, m_bIncHidden);
	DDX_Check(pDX, IDC_INC_LINKED, m_bIncLinked);
	DDX_Check(pDX, IDC_INC_SHAPEFILES, m_bShpOnly);
	DDX_Check(pDX, IDC_INC_POINTS, m_bPtsOnly);

	if(pDX->m_bSaveAndValidate) {
		CString csErrFind;

		bIncHidden=(m_bIncHidden==TRUE);
		bIncLinked=(m_bIncLinked==TRUE);
		bShpOnly=(m_bShpOnly==TRUE);
		bPtsOnly=(m_bPtsOnly==TRUE);

		//Set up pathNTL and compute rootPathLen for root path checking --
		//*** fix this to use added shapefile's rootpath len?? check this for out-of-range layers
		CString pathNTL(m_pDoc->GetPathName());
		int i=pathNTL.ReverseFind('\\');
		ASSERT(i>0);
		if(i<=0) return; //shouldn't happen!
		pathNTL.Truncate(i);
		i=pathNTL.ReverseFind('\\');
		if(i>=0) pathNTL.Truncate(i+1);
		else pathNTL+='\\';
		rootPathLen=pathNTL.GetLength();
		pathNTL=m_pDoc->GetPathName();
		pathNTL+='$';
		pRootPath=(LPCSTR)pathNTL;

		CLayerSet &lset=m_pDoc->LayerSet();

		i=0; //count files to add

		for(PML pml=lset.InitTreePML();pml;pml=lset.NextTreePML()) {
			if(bIncHidden || pml->IsVisible()) {
				bool bIsShp=pml->LayerType()==TYP_SHP;
				if(!bIsShp && bShpOnly || bIsShp && bPtsOnly && ((CShpLayer *)pml)->ShpType()!=CShpLayer::SHP_POINT)
					continue;
				LPCSTR p=pml->PathName();
				if((int)strlen(p)<rootPathLen || _memicmp(p,pRootPath,rootPathLen)) {
					pathNTL.Truncate(rootPathLen);
					CMsgBox("%s\n\nTo be included, this layer must reside within %s...\nYou'll need to "
						"either move its file(s) or %s before retrying this operation.",p,(LPCSTR)pathNTL,
						pml->IsVisible()?"make it hidden":"uncheck \"Include hidden layers\"");
					p=NULL;
				}
				else if(bIsShp) {
					//Make sure editing can be toggled off --
					CShpLayer *pEditor=((CShpLayer *)pml)->m_pdbfile->pShpEditor;
					if(pEditor && (pEditor->HasOpenRecord() || pEditor->HasEditedMemo())) {
						CMsgBox("%s\n\nThis function requires that included shapefiles have no records open for editing.",p);
						p=NULL;
					}
				}
				if(!p) {
					pDX->Fail();
					return;
				}
				i++;
			}
		}

		if(!i) {
			CMsgBox("The option settings are not acceptable since all layers would be excluded given the project's current state.");
			pDX->Fail();
			return;
		}

		if(strcmp(trx_Stpext(m_pathName),".zip")) {
			FixPathName(m_pathName,".zip");
			SetText(IDC_PATHNAME,m_pathName);
		}
		if(CheckDirectory(m_pathName)) {
			pDX->m_idLastControl=IDC_PATHNAME;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
			return;
		}
		m_pDoc->OnFileSave();

		{
			CFileCfg *pFile = new CFileCfg(CWallsMapDoc::ntltypes,NTL_CFG_NUMTYPES);
			CFileException ex;
			if(!pFile->Open(pRootPath,CFile::modeCreate|CFile::modeReadWrite|CFile::shareExclusive,&ex)) {
				CMsgBox("%s\nError creating temporary file.",pRootPath);
				delete pFile;
				pFile=NULL;
			}
			else if(!m_pDoc->WriteNTL(pFile,pRootPath,m_bIncHidden,m_bShpOnly+2*m_bPtsOnly)) {
				pFile->Abort(); // will not throw an exception
				delete pFile;
				pFile=NULL;
			}
			if(!pFile) {
				pDX->Fail();
				return;
			}
			delete pFile;
		}

		int e=ZIP_ERR_CREATE;

		hz=CreateZip(m_pathName,0);

		if(!hz) {
			FormatZipMessageZ(lasterrorZ,m_errmsg,SIZ_MSGBUF);
			goto _failmsg;
		}

		SetText(IDC_ST_ADDING,"Adding:");

		numErrDir=numErrFind=numErrAccess=numAdded=0;

		BeginWaitCursor();

		//first, add ntl --
		if(!(e=AddFileToZip(pRootPath,2))) {
			bool bHasMemos=false;
			for(PML pml=lset.InitTreePML();pml;pml=lset.NextTreePML()) {
				if(!bIncHidden && !pml->IsVisible())
					continue;
				if(pml->LayerType()==TYP_SHP) {
					if(bPtsOnly && ((CShpLayer *)pml)->ShpType()!=CShpLayer::SHP_POINT)
						continue;
					if(!bHasMemos && m_bIncLinked && ((CShpLayer *)pml)->HasMemos()) bHasMemos=true;
					e=AddShpToZip((CShpLayer *)pml);
				}
				else {
					if(bShpOnly) continue;
					if(pml->LayerType()==TYP_NTI && ((CNtiLayer *)pml)->HasNTERecord()) {
						strcpy(m_errmsg,pml->PathName());
						if(!(e=AddFileToZip(m_errmsg,1))) {
							strcpy(trx_Stpext(m_errmsg),".nte");
							e=AddFileToZip(m_errmsg,1);
						}
					}
					else e=AddFileToZip(pml->PathName(),1);
				}
			}
			if(!e && bHasMemos) {
				SetText(IDC_ST_ADDING,"Scanning:");

				ASSERT(!seq_vlinks.size());
				seq_vlinks.clear();

				{
					seq_data.Init((TRXFCN_IF)seq_fcn,(LPVOID *)&seq_pstrval);

					for(rit_Layer it=lset.BeginLayerRit();e<1 && it!=lset.EndLayerRit();it++) {
						if(!bIncHidden && !(*it)->IsVisible() || (*it)->LayerType()!=TYP_SHP ||
							!((CShpLayer *)*it)->HasMemos())
							continue;
						//scan memos and append to link array --
						CShpLayer *pShp=(CShpLayer *)*it;
						SetText(IDC_ST_FILEMSG,pShp->Title());
						e=pShp->GetMemoLinks(seq_vlinks,seq_data,pRootPath,rootPathLen);
						if(e<0) {
							break;
						}
						numErrDir+=e;
					}
					seq_data.Free();
				}
				if(e>=0) {
					e=0;
					if(seq_vlinks.size()) {
						strcpy(m_errmsg,pRootPath);
						SetText(IDC_ST_ADDING,"Adding:");
						for(it_cstr it=seq_vlinks.begin();it!=seq_vlinks.end();it++) {
							SetText(IDC_ST_FILEMSG,*it);
							strcpy(m_errmsg+rootPathLen,*it);
							if(_access(m_errmsg, 0)) {
								numErrFind++;
								if(csErrFind.IsEmpty()) csErrFind=m_errmsg;
							}
							else {
								if(e=AddFileToZip(m_errmsg,0)) {
									break;
								}
							}
						}
					}
				}
				if(seq_vlinks.size()) {
					VEC_CSTR().swap(seq_vlinks);
				}
			}
		}

		EndWaitCursor();

		if(e<0) {
			e=ZIP_ERR_ADDFILE;
			strcpy(m_errmsg,"Link scan failure - out of memory");
		}

		if(CloseZipZ(hz) && !e) {
			FormatZipMessageZ(lasterrorZ,m_errmsg,SIZ_MSGBUF);
			e=ZIP_ERR_CLOSE;
		}

		if(!e) {
			CString msg;
			msg.Format("Archive %s with %u files was successfully created.",
				trx_Stpnam(m_pathName),numAdded);
			if(numErrFind || numErrDir) {
				msg+="\n\nCAUTION: ";
				CString s;
				if(numErrFind) {
					s.Format("%u linked files were not found.\nExample: %s.\n",numErrFind,(LPCSTR)csErrFind);
					msg+=s;
				}
				if(numErrAccess) {
					s.Format("%u linked files were pressent but could not be read. ",numErrAccess);
					msg+=s;
				}
				if(numErrDir) {
					s.Format("%u linked files were skipped due to being outside the branch represented by "
						"the parent of the folder containing the NTL file.",numErrDir);
					msg+=s;
				}

			}
			AfxMessageBox(msg);
			goto _finish;
		}

_failmsg:
		if(e!=ZIP_ERR_CREATE)
			_unlink(m_pathName);
		CMsgBox("Failure creating archive %s\n\n%s",trx_Stpnam(m_pathName),m_errmsg);

_finish:
		_unlink(pRootPath);
	}
}

// CZipDlg message handlers

BOOL CZipDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString title;

	title.Format("Archiving %s (%u of %u layers visible)",(LPCSTR)m_pDoc->GetTitle(),
		m_pDoc->NumLayersVisible(),m_pDoc->NumMapLayers());

	SetWindowText(title);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CZipDlg::OnBnClickedBrowse()
{
	CString strFilter;
	if(!AddFilter(strFilter,IDS_ZIP_FILES)) return;
	if(!DoPromptPathName(m_pathName,
		OFN_HIDEREADONLY,
		1,strFilter,FALSE,IDS_ZIP_SAVEAS,".zip")) return;
	m_cePathName.SetWindowTextA(m_pathName);
}
