#include <stdlib.h>
#include "stdafx.h"
#include "metafile.h"

CMetafile::CMetafile()
{
	Init();
}

void CMetafile::Init()
{
	m_hMF = NULL;
	m_hEMF = NULL;

	m_pEMFHdr = NULL;
	m_pDescStr = NULL;
	m_pPal = NULL;
	m_palNumEntries = 0;
	m_pLogPal = NULL;
	m_hPal = NULL;


	m_pAldusMFHdr = NULL;
	m_hMem = NULL;
}

CMetafile::~CMetafile()
{
	ReleaseMemory();
}

void CMetafile::ReleaseMemory()
{
	if (m_hMF)
		::DeleteMetaFile(m_hMF);
	if (m_hEMF)
		::DeleteEnhMetaFile(m_hEMF);

	if (m_pEMFHdr)
		free(m_pEMFHdr);
	if (m_pDescStr)
		free(m_pDescStr);
	if (m_pPal)
		free(m_pPal);
	if (m_pLogPal)
		free(m_pLogPal);
	if (m_hPal)
		::DeleteObject(m_hPal);
	
	if (m_pAldusMFHdr)
		delete m_pAldusMFHdr;
	if (m_hMem)
		::GlobalFree(m_hMem);

	Init();
}

BOOL CMetafile::Load(LPCTSTR pszFullFileName)
{
	TCHAR		pszDrive[_MAX_DRIVE];
	TCHAR		pszDir[_MAX_DIR];
	TCHAR		pszFileName[_MAX_FNAME];
	TCHAR		pszExt[_MAX_EXT];
	CString		sExt;
	BOOL		bReturn;
	CFileStatus	status;

	ReleaseMemory();

	if (CFile::GetStatus(pszFullFileName, status) == FALSE)
	{
		// The file does not exist. Return
		return FALSE;
	}
	
	_splitpath(pszFullFileName,pszDrive,pszDir,pszFileName,pszExt);
	sExt = pszExt;
	sExt.MakeUpper();
	if (sExt == ".WMF")
		bReturn = LoadWMF(pszFullFileName);
	if (sExt == ".EMF")
		bReturn = LoadEMF(pszFullFileName);

	//if (sExt == ".CLP")
	//	bReturn = LoadCLP(pszFullFileName);

	return FALSE;
}

BOOL CMetafile::LoadWMF(LPCTSTR pszFullFileName)
{
	DWORD	dwlsAldus;
	CFile 	File(pszFullFileName, CFile::modeRead | CFile::shareDenyWrite);

	if (File.Read((LPSTR)&dwlsAldus, sizeof(dwlsAldus)) <  sizeof(dwlsAldus))
	{
		return FALSE;
	}

	File.Close();

	if (dwlsAldus == ALDUS_KEY)
	{
		return LoadAldusMF(pszFullFileName);
	}

	return LoadWindowsMF(pszFullFileName);
}

BOOL CMetafile::LoadEMF(LPCTSTR pszFullFileName)
{
	UINT	uiSignature;
	CFile	File(pszFullFileName, CFile::modeRead | CFile::shareDenyWrite);

	File.Read(&uiSignature, sizeof(UINT));
	File.Close();

	//if this is an EMF then obtain a handle to it
	if (uiSignature == EMR_HEADER)
	{
		m_hEMF = ::GetEnhMetaFile(pszFullFileName);
		GetEMFCoolStuff();
		LoadPalette();
	}
	else
	{
		m_hEMF = NULL;
	}

	//return success
	return m_hEMF ? TRUE : FALSE;
}

BOOL CMetafile::LoadAldusMF(LPCTSTR pszFullFileName)
{
	// Llegeix fitxers conforme al format Aldus	
    METAHEADER	mfHeader;
  	LPSTR		lpMem;
  	CFile		File(pszFullFileName, CFile::modeRead | CFile::shareDenyWrite);

	File.Seek(0, CFile::begin);
	m_pAldusMFHdr = new ALDUSMFHEADER;
	if (File.Read((LPSTR)m_pAldusMFHdr, OFFSET_TO_META) < OFFSET_TO_META)
		goto LoadFailure;
	    
	File.Seek(OFFSET_TO_META, CFile::begin);
  	if (File.Read((LPSTR)&mfHeader, METAHEADER_SIZE) < METAHEADER_SIZE)
		goto LoadFailure;
	
	if ( mfHeader.mtType != 1 && mfHeader.mtType != 2 ) 
		goto LoadFailure;
  
	if (!(m_hMem = ::GlobalAlloc(GHND, (mfHeader.mtSize * 2L))))
		return FALSE;
  	
	if (!(lpMem =(LPSTR)::GlobalLock(m_hMem)))
	{
		::GlobalFree(m_hMem);
		goto LoadFailure;
	}

	File.Seek(OFFSET_TO_META, CFile::begin);

  	// Llegim el contingut del fixer (MetaFile bits)
  	if (File.ReadHuge(lpMem, (DWORD)(mfHeader.mtSize * 2)) == -1)
	{
		::GlobalUnlock(m_hMem);
		::GlobalFree(m_hMem);
    	goto LoadFailure;
  	}

	if (!(m_hMF = SetMetaFileBitsEx(mfHeader.mtSize * 2L,(unsigned char *)lpMem)))
	{
		::GlobalUnlock(m_hMem);
		::GlobalFree(m_hMem);	
    	goto LoadFailure;
	}
    
	::GlobalUnlock(m_hMem);
    File.Close();
    return TRUE;

LoadFailure:
	delete m_pAldusMFHdr;
	m_pAldusMFHdr = NULL;

	return FALSE;
}

BOOL CMetafile::LoadWindowsMF(LPCTSTR pszFullFileName)
{
	METAHEADER	mfHeader;
	LPSTR		lpMem;
	CFile		File(pszFullFileName,CFile::modeRead | CFile::shareDenyWrite);

	if (File.Read((LPSTR)&mfHeader, METAHEADER_SIZE) < METAHEADER_SIZE)
		return FALSE;
	
	if ( mfHeader.mtType != 1 && mfHeader.mtType != 2 ) 
		return FALSE;
  
	if (!(m_hMem = ::GlobalAlloc(GHND, (mfHeader.mtSize * 2L))))
		return FALSE;
  	
	if (!(lpMem =(LPSTR)::GlobalLock(m_hMem)))
	{
		::GlobalFree(m_hMem);
		return FALSE;
	}

	File.Seek(0, CFile::begin);

  	// Llegim el contingut del fixer (MetaFile bits)
  	if (File.ReadHuge(lpMem, (DWORD)(mfHeader.mtSize * 2)) == -1)
	{
		::GlobalUnlock(m_hMem);
		::GlobalFree(m_hMem);
    	return FALSE;
  	}

	if (!(m_hMF = SetMetaFileBitsEx(mfHeader.mtSize * 2L,(unsigned char *)lpMem)))
	{
		::GlobalUnlock(m_hMem);
		::GlobalFree(m_hMem);	
    	return FALSE;
	}
    
	::GlobalUnlock(m_hMem);
    File.Close();
    return TRUE;
}

BOOL CMetafile::Draw(CDC* pDC, RECT* pRect)
{
	ASSERT (pDC != NULL);
	
	if (NULL != m_pAldusMFHdr)
		return DrawAldusMF(pDC, pRect);

	if (m_hMF)
		return DrawWindowsMF(pDC, pRect);

	if (m_hEMF)
		return DrawEMF(pDC, pRect);

	return FALSE;
}

                              
BOOL CMetafile::DrawAldusMF(CDC* pDC, RECT* pRect)
{
	RECT	rc;
	
	rc.left = m_pAldusMFHdr->bbox.left;
	rc.top = m_pAldusMFHdr->bbox.top;
	rc.bottom = m_pAldusMFHdr->bbox.bottom;
	rc.right = m_pAldusMFHdr->bbox.right;
	
	pDC->SaveDC();

	pDC->LPtoDP(pRect);

	  /* set the windows origin to correspond to the bounding box origin
     contained in the placeable header */ 
    pDC->LPtoDP(&rc);
	pDC->SetMapMode(MM_ANISOTROPIC);
  	pDC->SetWindowOrg( 0,0);
  	// pDC->SetWindowOrg( m_pAldusMFHdr->bbox.left, m_pAldusMFHdr->bbox.top);

	/* set the window extents based on the abs value of the bbox coords */
  	pDC->SetWindowExt(abs(rc.left) + abs(rc.right), abs(rc.top) + abs(rc.bottom));

  	/* set the viewport origin and extents */
  	pDC->SetViewportOrg(pRect->left,pRect->top);
  	pDC->SetViewportExt(pRect->right, pRect->bottom);

	::EnumMetaFile(pDC->GetSafeHdc(), m_hMF, EnumMFCProc, (LPARAM)(LPCTSTR)pDC);
	// pDC->PlayMetaFile(m_hMF);
		
	pDC->RestoreDC(-1);
	return TRUE;
}

BOOL CMetafile::DrawWindowsMF(CDC* pDC, RECT* pRect)
{
	pDC->SaveDC();

	pDC->SetMapMode(MM_ANISOTROPIC);

	pDC->SetViewportExt(pRect->right, pRect->bottom);
	pDC->SetViewportOrg(pRect->left, pRect->top);

	::EnumMetaFile(pDC->GetSafeHdc(), m_hMF, EnumMFCProc, (LPARAM)(LPCTSTR)pDC);
	//pDC->PlayMetaFile(m_hMF);

	pDC->RestoreDC(-1);

	return TRUE;		
}

BOOL CMetafile::DrawEMF(CDC* pDC, RECT *pRect)
{                      
	ASSERT(m_hEMF);

	BOOL		bReturn = FALSE;
	CRect		cRect;
	CPalette*	pOldPalette = NULL;

	if (m_hEMF)
	{
		if (m_hPal) 
		{
			pOldPalette = pDC->SelectPalette(CPalette::FromHandle(m_hPal), FALSE);
			if (NULL != pOldPalette)
				pDC->RealizePalette();
		}

		bReturn = pDC->PlayMetaFile(m_hEMF, pRect);

		if (pOldPalette)
			pDC->SelectPalette(pOldPalette, FALSE);
	}
	return (bReturn);
}              

BOOL CMetafile::GetEMFCoolStuff()
{
	if (m_hEMF)
	{
		//
		//obtain the sizes of the emf header, description string and palette
		//
		UINT	uiHdrSize		= ::GetEnhMetaFileHeader(m_hEMF, 0, NULL);
		UINT	uiDescStrSize	= ::GetEnhMetaFileDescription(m_hEMF, 0, NULL);
		UINT	uiPalEntries	= ::GetEnhMetaFilePaletteEntries(m_hEMF, 0, NULL);
		//
		//if these are lengths > 0 then allocate memory to store them
		//
		if (uiHdrSize)
			m_pEMFHdr = (LPENHMETAHEADER)malloc(uiHdrSize);
		if (uiDescStrSize)
			m_pDescStr = (LPTSTR)malloc(uiDescStrSize);
		if (uiPalEntries)
			m_pPal = (LPPALETTEENTRY)malloc(uiPalEntries * sizeof(PALETTEENTRY));
		//
		//so far the emf seems to be valid so continue
		//
		if (uiHdrSize)
		{
			//
			//get the actual emf header and description string
			//
			if (!::GetEnhMetaFileHeader(m_hEMF, uiHdrSize, m_pEMFHdr))
				return (FALSE);
			else
			{
				//
				//get the description string if it exists
				//
				if (uiDescStrSize)
					::GetEnhMetaFileDescription(m_hEMF, uiDescStrSize, m_pDescStr);
				//
				//get the palette if it exists
				//
				if (uiPalEntries)
				{
					::GetEnhMetaFilePaletteEntries(m_hEMF, uiPalEntries, m_pPal);
					m_palNumEntries = uiPalEntries;
				}
			}
		}
	}
	return (TRUE);
}

//////////////////////////////////////////////////////////////////////////
//LoadPalette()
BOOL CMetafile::LoadPalette()
{
	if (m_pPal)
	{
		m_pLogPal = (LPLOGPALETTE) malloc(sizeof(LOGPALETTE) +(sizeof (PALETTEENTRY) * m_palNumEntries));
		m_pLogPal->palVersion = 0x300;
		m_pLogPal->palNumEntries = m_palNumEntries;

		//copy palette entries into palentry array
		for (int i = 0; i < m_pLogPal->palNumEntries; i++)
			m_pLogPal->palPalEntry[i] = *m_pPal++;

		//position the ptr back to the beginning should we call this
		//code again
		m_pPal -= m_pLogPal->palNumEntries;

		//create the palette
		if ((m_hPal = ::CreatePalette((LPLOGPALETTE)m_pLogPal)))
			return TRUE;
	} 
	return FALSE;
}

int CALLBACK CMetafile::EnumMFCProc(HDC hdc, HANDLETABLE FAR *lpht, METARECORD FAR *lpmr, int cObj, LPARAM lParam)
{
	CDC* pDC = (CDC*)lParam;

	switch (lpmr->rdFunction)
	{
		case META_SETWINDOWEXT:
			pDC->SetWindowExt(lpmr->rdParm[1], lpmr->rdParm[0]);
			break;

		case META_SETMAPMODE:
			pDC->SetMapMode(lpmr->rdParm[0]);
			break;

		case META_SETVIEWPORTEXT:
			pDC->SetViewportExt(lpmr->rdParm[1], lpmr->rdParm[0]);
			break;

		case META_SETVIEWPORTORG:
			pDC->SetViewportOrg(lpmr->rdParm[1], lpmr->rdParm[0]);
			break;

		default:

			PlayMetaFileRecord(hdc, lpht, lpmr, cObj);
	}

	return 1;
}