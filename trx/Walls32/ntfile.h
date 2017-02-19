// NTFILE.H -- WALLS Database Operations --

#ifndef __NTFILE_H
#define __NTFILE_H

#ifndef __DBFILE_H
#include <dbfile.h>
#endif

#ifndef __WALL_SRV_H
#include "wall_srv.h"
#endif

#define NTV_SIZNAME 8
#define NTV_NUMFLDS 21
#define NTV_CHKOFFSET 16
#define NTV_VEROFFSET 12

#define NTV_BUFRECS 20
#define NTV_NUMBUFS 10

typedef struct {
    DWORD checksum;
    DWORD maxid;
} NTVHdr;

#define pNTVHdr ((NTVHdr FAR *)HdrRes16())

/* NTVRec byte offset of start of DBF record --*/
#define NTV_RECOFFSET 0 

class CNTVFile : public CDBFile {

private:
	static DBF_FLDDEF m_FldDef[NTV_NUMFLDS];
	int alloc_cache();
   
public:
	int Open(const char* pszFileName);
	int Create(const char* pszFileName);

	DWORD Checksum() {return pNTVHdr->checksum;}
	//long FAR *FixOffset() {return pNTVHdr->fixoffset;}
	DWORD MaxID() {return pNTVHdr->maxid;}
	
	void SetChecksum(DWORD checksum) {pNTVHdr->checksum=checksum;}
	void SetMaxID(DWORD id) {pNTVHdr->maxid=id;}

	WORD Version() {return *(WORD *)HdrRes2();}
	void SetVersion(WORD ver) {*(WORD *)HdrRes2()=ver;}
};

#endif
