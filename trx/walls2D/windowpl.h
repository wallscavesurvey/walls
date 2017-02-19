////////////////////////////////////////////////////////////////
// CWindowPlacement 1996 Microsoft Systems Journal.
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.

////////////////
// CWindowPlacement reads and writes WINDOWPLACEMENT 
// from/to application profile and CArchive.
//
struct CWindowPlacement : public tagWINDOWPLACEMENT {
   CWindowPlacement();
   ~CWindowPlacement();
   
   // Read/write to app profile
   void GetProfileWP();
   void WriteProfileWP();

   // Save/restore window pos (from app profile)
   void Save(CWnd* pWnd);
   BOOL Restore(CWnd* pWnd);
};
