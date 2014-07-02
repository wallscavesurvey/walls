// FileDropTarget.cpp : implementation file
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
//#include "resource.h"
#include "logfont.h"
#include "WallsMap.h"
#include "WallsMapDoc.h"
#include "FileDropTarget.h"
#include "MainFrm.h"
#include "LayerSetSheet.h"
#include "ImageViewDlg.h"

#ifndef IMAGE_NO_DRAGDROP

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFileDropTarget

UINT CFileDropTarget::m_nOutside=0;
bool CFileDropTarget::m_bLinking=false;

V_FILELIST CFileDropTarget::m_vFiles;

CFileDropTarget::CFileDropTarget() : m_pWndRegistered(NULL)
    , m_piDropHelper(NULL)
	, m_bUseDnDHelper(false)
	, m_bAddingLayers(false)
#ifdef _DEBUG
	, m_bTesting(false)
#endif
{
    if ( SUCCEEDED( CoCreateInstance ( CLSID_DragDropHelper, NULL, 
                                       CLSCTX_INPROC_SERVER,
                                       IID_IDropTargetHelper, 
                                       (void**) &m_piDropHelper ) ))
    {
		m_bUseDnDHelper = true;
    }

}

CFileDropTarget::~CFileDropTarget()
{
    if ( NULL != m_piDropHelper )
        m_piDropHelper->Release();
}

BOOL CFileDropTarget::Register(CWnd *pWnd,DROPEFFECT dwEffect /*=DROPEFFECT_NONE*/)

{
	if(!COleDropTarget::Register(pWnd)) pWnd=NULL;
	m_pWndRegistered=pWnd;
	m_pMainFrame=(CMainFrame *)AfxGetMainWnd();
	m_dwEffect=pWnd?dwEffect:DROPEFFECT_NONE;

	if(m_dwEffect) {
		if ( SUCCEEDED( CoCreateInstance ( CLSID_DragDropHelper, NULL, 
                                       CLSCTX_INPROC_SERVER,
                                       IID_IDropTargetHelper, 
                                       (void**) &m_piDropHelper ) ))
		{
			m_bUseDnDHelper = true;
		}
	}
	return pWnd!=NULL;
}

void CFileDropTarget::Revoke()
{
	if(m_pWndRegistered) {
		m_pWndRegistered=NULL;
		COleDropTarget::Revoke();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CFileDropTarget message handlers

DROPEFFECT CFileDropTarget::OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, 
                                        DWORD dwKeyState, CPoint point)
{
    ASSERT(m_pWndRegistered==pWnd);

	if(!m_dwEffect) {
		m_nOutside++;
		m_bLinking=false;
		return DROPEFFECT_NONE;
	}
#ifdef _DEBUG
	if(m_bTesting) {
		int jj=0;
	}
#endif

	if (pDataObject->GetGlobalData(CF_HDROP)) {
		if(m_dwEffect==DROPEFFECT_COPY) {
			if(m_bLinking) return DROPEFFECT_NONE;
		}
		else m_bLinking=true;
		if (m_bUseDnDHelper) {
			IDataObject* piDataObj = pDataObject->GetIDataObject(FALSE);
			m_piDropHelper->DragEnter(pWnd->GetSafeHwnd(), piDataObj, 
                                    &point, m_dwEffect);
        }
		return m_dwEffect;
	}
 
    return DROPEFFECT_NONE;
}


DROPEFFECT CFileDropTarget::OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, 
                                       DWORD dwKeyState, CPoint point)
{
    ASSERT(m_pWndRegistered==pWnd);

	if(!m_dwEffect || m_bLinking && m_dwEffect==DROPEFFECT_COPY ||
		!m_bLinking && m_dwEffect==DROPEFFECT_LINK)
		return DROPEFFECT_NONE;

	if (m_bUseDnDHelper ) {
		m_piDropHelper->DragOver( &point, m_dwEffect );
    }
	return m_dwEffect;
}

void CFileDropTarget::OnDragLeave(CWnd* pWnd)
{
	if(!m_dwEffect) {
		ASSERT(m_nOutside);
		if(m_nOutside) m_nOutside--;
	}
	else {
		if (m_bUseDnDHelper) {
			m_piDropHelper->DragLeave();
        }
		if(m_dwEffect==DROPEFFECT_LINK)
			m_bLinking=false;
	}
}

BOOL CFileDropTarget::OnDrop(CWnd* pWnd, COleDataObject* pDataObject,
                             DROPEFFECT dropEffect, CPoint point)
{
    ASSERT(pWnd==m_pWndRegistered);

	ASSERT(m_dwEffect==dropEffect);
	ASSERT(!m_nOutside);
	m_nOutside=0;

	BOOL bRet = ReadHdropData(pDataObject);

    if (m_bUseDnDHelper )
        {
        // The DnD helper needs an IDataObject interface, so get one from
        // the COleDataObject.  Note that the FALSE param means that
        // GetIDataObject will not AddRef() the returned interface, so 
        // we do not Release() it.

        IDataObject* piDataObj = pDataObject->GetIDataObject(FALSE); 

        m_piDropHelper->Drop (piDataObj, &point, dropEffect);
    }
    
	if(bRet) {
		if(dropEffect==DROPEFFECT_COPY) {
			if(m_bAddingLayers) {
				((CPageLayers *)pWnd)->OnDrop();
			}
			else
				m_pMainFrame->OnDrop();
		}
		else
			((CImageViewDlg *)pWnd)->OnDrop();

		V_FILELIST().swap(m_vFiles);
	}
	else FileTypeErrorMsg();

	return TRUE;
}

BOOL CFileDropTarget::ReadHdropData(COleDataObject* pDataObject)
{
	USES_CONVERSION;		// For T2COLE() below

    HGLOBAL hg = pDataObject->GetGlobalData(CF_HDROP);
    
    if(!hg) return FALSE;

    HDROP hdrop=(HDROP)GlobalLock(hg);

    if (!hdrop ) {
        GlobalUnlock (hg);
        return FALSE;
    }

	ASSERT(m_vFiles.size()==0);

    IShellLink* pIShellLink=NULL;
	IPersistFile* pIPersistFile=NULL;
	TCHAR szNextFile[MAX_PATH];
	TCHAR szExt[8];

	szExt[0]='*'; szExt[7]=0;

    // Get the # of files being dropped.
    UINT uNumFiles = DragQueryFile( hdrop, -1, NULL, 0 );


    for (UINT uFile = 0; uFile < uNumFiles; uFile++ ) {
        // Get the next filename from the HDROP info.

		if(DragQueryFile(hdrop,uFile,szNextFile, MAX_PATH ) > 0 ) {

			bool bExpanded=false;
_retry:
			strncpy(szExt+1,trx_Stpext(szNextFile),7);
			_strlwr(szExt+1);

			if(!bExpanded && !strcmp(szExt+2,"lnk")) {
				if(!pIShellLink) {
					::CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
						IID_IShellLink, (LPVOID*) &pIShellLink);
					if(!pIShellLink) continue;

					if(!SUCCEEDED(pIShellLink->QueryInterface(IID_IPersistFile,(LPVOID*)&pIPersistFile))) {
						pIShellLink->Release();
						pIShellLink=NULL;
						continue;
					}
				}
				ASSERT(pIPersistFile);
				if(SUCCEEDED(pIPersistFile->Load(T2COLE(szNextFile),STGM_READ))) {
					WIN32_FIND_DATA wfd;
					pIShellLink->GetPath(szNextFile,MAX_PATH,&wfd,SLGP_UNCPRIORITY);
					bExpanded=true;
					goto _retry;

				}
				continue;
			}

			if(!strstr(CWallsMapDoc::doctypes[0].ext,szExt)) continue;
			
			if(m_bAddingLayers) {
				if(!strcmp(szExt+2,"ntl"))
					continue;
			}
			else if(m_dwEffect!=DROPEFFECT_COPY && !CImageViewDlg::IsValidExt(szExt+1)) {
				continue;
			}

			try {
				m_vFiles.push_back(CString(szNextFile));
			}
			catch(...) {
				V_FILELIST().swap(m_vFiles);
				break;
			}
			/*
			hFind = FindFirstFile(szNextFile,&rFind);

			if (INVALID_HANDLE_VALUE != hFind) {
				StrFormatByteSize (rFind.nFileSizeLow,szFileLen,64);
				FindClose ( hFind );
			}
			*/
       }
    }   // end for

	if(pIShellLink) {
		pIPersistFile->Release();
		pIShellLink->Release();
	}
    GlobalUnlock ( hg );

    return m_vFiles.size()>0;
}

void CFileDropTarget::FileTypeErrorMsg()
{
	AfxMessageBox("None of the dropped or pasted files are of a supported type.");
}

#endif // IMAGEL_NO_DRAGDROP