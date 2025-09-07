// CExportKmlDialog.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "plotview.h"
#include "compile.h"
#include "segview.h"
#include "CExportKmlDialog.h"
#include "wall_shp.h"

// CExportKmlDialog dialog

CExportKmlDialog::CExportKmlDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_EXPORT_KML_DIALOG, pParent)
{
	CPrjDoc *pDoc = pSV->GetDocument();
	m_pathname = pDoc->KmlFilePath();
	m_uTypes = pDoc->m_uShapeTypes;
	m_bFlags = (m_uTypes&SHP_FLAGS) != 0;
	m_bNotes = (m_uTypes&SHP_NOTES) != 0;
	m_bStations = (m_uTypes&SHP_STATIONS) != 0;
	m_bVectors = (m_uTypes&SHP_VECTORS) != 0;
	m_bWalls = pSV->HasFloors() && (m_uTypes&SHP_WALLS) != 0;
}

CExportKmlDialog::~CExportKmlDialog()
{
}

void CExportKmlDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_KML_PATH, m_pathname);
	DDV_MaxChars(pDX, m_pathname, 250);
	DDX_Check(pDX, IDC_SHP_FLAGS, m_bFlags);
	DDX_Check(pDX, IDC_SHP_NOTES, m_bNotes);
	DDX_Check(pDX, IDC_SHP_STATIONS, m_bStations);
	DDX_Check(pDX, IDC_SHP_VECTORS, m_bVectors);
	DDX_Check(pDX, IDC_SHP_WALLS, m_bWalls);

	UINT flags = m_uTypes = (m_bFlags*SHP_FLAGS) + (m_bNotes*SHP_NOTES) +
		(m_bStations*SHP_STATIONS) + (m_bVectors*SHP_VECTORS) + (m_bWalls*SHP_WALLS);

	if (!(flags&(SHP_STATIONS + SHP_VECTORS + SHP_FLAGS + SHP_NOTES + SHP_WALLS))) {
		AfxMessageBox(IDS_SHP_NOTYPES);
		return;
	}

	if (pDX->m_bSaveAndValidate) {
		CPrjDoc *pDoc = pSV->GetDocument();

		pDoc->SetKmlFilePath(m_pathname);
		pDoc->m_uShapeTypes = m_uTypes;

		int i = pSV->ExportKML(this, m_pathname, flags);

		if (i) {
			EndDialog(IDCANCEL);
			pDX->Fail();
			return;
		}
	}
}


BEGIN_MESSAGE_MAP(CExportKmlDialog, CDialog)
	//{{AFX_MSG_MAP(CExportKmlDialog)
	ON_BN_CLICKED(IDBROWSE, OnBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CExportKmlDialog::Browse(UINT id)
{
	BrowseFiles((CEdit *)GetDlgItem(id), m_pathname, ".kml", IDS_KML_FILES, IDS_KML_BROWSE);
}

void CExportKmlDialog::OnBrowse()
{
	Browse(IDC_KML_PATH);
}


// CExportKmlDialog message handlers
