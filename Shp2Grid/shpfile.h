#ifndef __SHPFILE_H
#define __SHPFILE_H

#ifndef __DBF_FILE_H
#include <dbf_file.h>
#endif
#ifndef _FLTPOINT_H
#include <FltPoint.h>
#endif

#define SHP_ERR_WRITE -1
#define SHP_ERR_CREATE -2

struct TRXREC {
	TRXREC() : nCaves(0), nShelters(0), nCavities(0), nSprings(0) {}
	UINT nCaves;
	UINT nShelters;
	UINT nCavities;
	UINT nSprings;
};

struct DWORD2 {
	DWORD2(CFltPoint &fpt) : x(((DWORD)fpt.x/1000)*1000+500), y(((DWORD)fpt.y/1000)*1000+500) {}
	DWORD x;
	DWORD y;
};

typedef std::vector<DWORD2> VEC_DWORD2;

int CreateFiles(LPCSTR lpPathname);
int CloseFiles();
void DeleteFiles();
int WriteShpRec(TRXREC &trxrec,DWORD2 &xy);
void WritePrjFile();
#endif
