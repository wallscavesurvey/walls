// dbf-filter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <dbfile.h>
#include <trx_str.h>

int _tmain(int argc, _TCHAR* argv[])
{
	if(argc!=2) {
		printf("USAGE: dbf_filter <dbf_path>[.dbf]\n");
		return 0;
	}
	CDBFile db;
	CString s,s1;
	int nupd=0,nrecs=0,i0,i1;
	int e=db.Open(argv[1]);
	char buf[30]="F ";

	if(e) {
		printf("Can't open file %s.", argv[1]);
		return 1;
	}

	int f_name=db.FldNum("NAME");
	/*
	int f_lenExp=db.FldNum("LENGTH_EXP");
	int f_lenSrv=db.FldNum("LENGTH_SRV");
	int f_depExp=db.FldNum("DEPTH_EXP");
	int f_depSrv=db.FldNum("DEPTH_SRV");
	ASSERT(f_lenExp*f_depExp*f_lenSrv*f_lenSrv!=0);
	*/
	e=db.First();
	while(!e) {
		nrecs++;
		if(db.GetTrimmedFldStr(s,f_name)) goto _err;
		i1=s.ReverseFind('(');
		if(i1>=0) {
			s.Truncate(i1);
			s.Trim();
			db.PutFldStr(s, f_name);
			nupd++;
		}
		/*
		if(db.GetTrimmedFldStr(s,f_lenExp)) goto _err;
		i1=s.ReverseFind(',');
		if(i1>=0) s.Truncate(i1);
		db.PutFldStr(s,f_lenSrv);
		db.PutFldStr("",f_lenExp);
		if(db.GetTrimmedFldStr(s,f_depExp)) goto _err;
		db.PutFldStr(s,f_depSrv);
		db.PutFldStr("",f_depExp);
		nupd++;
		*/
		e=db.Next();
	}
	if(nrecs!=db.NumRecs()) goto _err;
	if(db.Close()) goto _err;
	printf("Completed - %u of %u records updated",nupd,nrecs);
	return 0;

_err:
	db.Close();
	printf("Error encountered at record %u: %s.",nrecs,db.Errstr());
	return -1;
}

