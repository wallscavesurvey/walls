#ifndef __METAFILE_H_
#define __METAFILE_H_

#pragma pack(1)
//
//placeable metafile data definitions 
//
typedef struct tagOLDRECT
{
    short   left;
    short   top;
    short   right;
    short   bottom;
} OLDRECT;

typedef struct 
{
	DWORD		key;
	WORD		hmf;
	OLDRECT		bbox;
	WORD		inch;
	DWORD		reserved;
	WORD		checksum;
} ALDUSMFHEADER, * LPALDUSMFHEADER;
#pragma pack()

#define OFFSET_TO_META		sizeof (ALDUSMFHEADER)
#define METAHEADER_SIZE		sizeof (METAHEADER)
#define ALDUS_KEY			0x9AC6CDD7

class CMetafile: public CObject
{
// Operations
public:
	CMetafile();
	~CMetafile();
	BOOL				Load(LPCTSTR pszFullFileName);
	BOOL				Draw(CDC* pDC, RECT* pRect);
    LPENHMETAHEADER		GetEMFHeader();
    inline LPTSTR		GetEMFDescString();
    inline HENHMETAFILE	GetEMFHandle();
	
protected:
    BOOL				GetEMFCoolStuff();
    BOOL				LoadPalette();

	BOOL				LoadWMF(LPCTSTR pszFullFileName);
	BOOL				LoadEMF(LPCTSTR pszFullFileName);
	BOOL				LoadAldusMF(LPCTSTR pszFullFileName);
	BOOL				LoadWindowsMF(LPCTSTR pszFullFileName);

	BOOL				DrawAldusMF(CDC* pDC,RECT* pRect);
	BOOL				DrawWindowsMF(CDC* pDC,RECT* pRect);
	BOOL				DrawEMF(CDC* pDC, RECT *pRect);

	void				Init();
	void				ReleaseMemory();

	static int CALLBACK	EnumMFCProc(HDC hdc, HANDLETABLE FAR *lpht, METARECORD FAR *lpmr, int cObj, LPARAM lParam);

// Attributes
protected:

	HMETAFILE			m_hMF;
	HENHMETAFILE		m_hEMF;

	LPENHMETAHEADER		m_pEMFHdr;
	LPTSTR				m_pDescStr;
	LPPALETTEENTRY		m_pPal;
	UINT				m_palNumEntries;
	LPLOGPALETTE		m_pLogPal;
	HPALETTE			m_hPal;


	LPALDUSMFHEADER		m_pAldusMFHdr;
	HANDLE				m_hMem;
};

inline LPENHMETAHEADER CMetafile::GetEMFHeader() 
	{return ((m_pEMFHdr) ? m_pEMFHdr : NULL);};
inline LPTSTR CMetafile::GetEMFDescString()
	{return ((m_pDescStr) ? m_pDescStr : NULL);};
inline HENHMETAFILE CMetafile::GetEMFHandle()
	{return ((m_hEMF) ? m_hEMF : NULL);};

#endif	// __METAFILE_H_
