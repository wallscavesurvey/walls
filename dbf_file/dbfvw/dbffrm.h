// dbffrm.h : interface of the CDbfFrame class
//
// We override CMDIChildWnd to customize the MDI child's title bar.
// By default the title bar shows the document name.  But we want
// it to instead show the text defined as the first string in
// the document template STRINGTABLE resource.

/////////////////////////////////////////////////////////////////////////////

class CDbfFrame : public CMDIChildWnd
{
        DECLARE_DYNCREATE(CDbfFrame)
protected:
	BOOL PreCreateWindow(CREATESTRUCT& cs);
};

