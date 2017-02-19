//NTFILE.CPP -- WALLS Database Routines
//
#include "stdafx.h"
#include "ntfile.h"

DBF_FLDDEF CNTVFile::m_FldDef[NTV_NUMFLDS] = //15*8 = 120 bytes
{
	{"_T_DATE"	 ,'C',3,0,0},
	{"_W_FR_PFX" ,'C',2,0,0},
	{"_W_TO_PFX" ,'C',2,0,0},
	{"FR_NAM"    ,'C',8,0,0},
	{"TO_NAM"    ,'C',8,0,0},
	{"_D_EAST"   ,'C',8,0,0},
	{"_D_NORTH"  ,'C',8,0,0},
	{"_D_UP"     ,'C',8,0,0},
	{"_D_VARH"   ,'C',8,0,0},
	{"_D_VARV"   ,'C',8,0,0},
	{"_L_FR_ID"  ,'C',4,0,0},
	{"_L_TO_ID"  ,'C',4,0,0},
	{"_L_STR_ID" ,'C',4,0,0},
	{"_L_STR_NXT",'C',4,0,0},
	{"_D_A_EAST" ,'C',8,0,0},
	{"_D_A_NORTH",'C',8,0,0},
	{"_D_A_UP"   ,'C',8,0,0},
	{"_U_LINENO" ,'C',4,0,0},
	{"_W_SEG_ID" ,'C',2,0,0},
	{"_W_FILEID" ,'C',2,0,0},
	{"FILE_NAM"  ,'C',8,0,0}
};

int CNTVFile::alloc_cache()
{
   if(!Cache()) {
     ASSERT(!DefaultCache());
     return AllocCache(NTV_BUFRECS*SizRec(),NTV_NUMBUFS,TRUE);
   }
   return 0;
}

int CNTVFile::Open(const char* pszFileName)
{
    int e=CDBFile::Open(pszFileName,ReadWrite);
    if(!e && (e=alloc_cache())!=0) CDBFile::Close();
    return e;
}

int CNTVFile::Create(const char* pszFileName)
{
    int e=CDBFile::Create(pszFileName,NTV_NUMFLDS,m_FldDef,ReadWrite);
    if(!e && (e=alloc_cache())!=0) CDBFile::CloseDel();
    return e;
}
