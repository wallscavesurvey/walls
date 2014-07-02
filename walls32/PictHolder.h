// PictureHolderEx.h: interface for the CPictureHolderEx class.
//
///////////////////////////////////////////////////////////////

#pragma once

#include <afxctl.h>

class CPictureHolderEx : public CPictureHolder {
public:
    // Constructors (including copy and assign!!)
    CPictureHolderEx();
    //CPictureHolderEx(const CPictureHolderEx& holder);
    //CPictureHolderEx& operator=(const CPictureHolderEx& from);

    // Handles to picture objects
    //OLE_HANDLE GetHandle();

    // Do we have a picture? what type?
    inline bool HasPicture()
    { return NULL != m_pPict; }

    inline bool IsBitmap()
    { return GetType() == PICTYPE_BITMAP; }

    inline bool IsMetafile()
    { return GetType() == PICTYPE_METAFILE; }

    inline bool IsEnhMetafile()
    { return GetType() == PICTYPE_ENHMETAFILE; }
/*
    // Allow use of a CPictureHolderEx what an HBITMAP etc
    // could be used
    inline operator HBITMAP()
    { return IsBitmap() ? (HBITMAP)GetHandle() :  NULL; }
    inline operator HMETAFILE() { return IsMetafile() ?
        (HMETAFILE)GetHandle() : NULL; }
    inline operator HENHMETAFILE() { return IsEnhMetafile() ?
        (HENHMETAFILE)GetHandle() : NULL; }
*/
	bool OleLoadPictureFromResource(UINT nID, LPCTSTR lpszType, HINSTANCE hInstResource=NULL);
	//bool GetDimensions(CSize& size);
    bool OleLoadPictureFromGlobal(HGLOBAL hGlobal, int maxLen = 0);
    bool OleLoadPictureFromStream(LPSTREAM lpStream, int maxLen = 0);
};
