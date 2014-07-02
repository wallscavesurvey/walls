// WallsGDL.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <ntifile.h>
//#include <zlib.h>

static CNTIFile nti;

//All users of nti_file.lib must define this function to establish
//suported compression methods --
CSF_XFRFCN nti_XfrFcn(int typ)
{
	switch(typ) {
		case NTI_FCNLOCO : return (CSF_XFRFCN)nti_loco_e;

		case -1 : //FreeDecode(): Uninitialize all encode/decode fcns --
			{
				nti_loco_e(NULL,0,0,NULL); //encode uninit
				nti_loco_d(NULL,0,0,NULL); //decode uninit - also frees buffers
			}
	}
	return NULL;
}

static void exitMsg(int err,char *fmt,...)
{
	CNTIFile::FreeDecode();
	nti.Close();
	CNTIFile::FreeDefaultCache();

	if(*fmt) {
		va_list marker;
		va_start(marker,fmt);
		vprintf(fmt,marker);
		va_end(marker);
	}
	if(err>0) printf("%s.\n",nti.Errstr(err));
    exit(err);
}

static void exitNTI(LPSTR msg)
{
	exitMsg(nti.Errno(),"NTI Error: %s - ",msg);
}

static void Usage()
{
	printf("Usage: locotimer [-t<cycles>] pathName[.nti]\n");
}

void main(int argc, char * argv[])
{
	char pathName[_MAX_PATH+1];
	*pathName=0;

	int nLoops=1;

	for(int i=1;i<argc;i++) {
		char *p=argv[i];
		if(*p=='-' || *p=='/') {
			switch(*++p) {
				case '?' : *pathName=0; break;
				//case 'z' : zOpt=(p[1]=='1')?NTI_FCNLOCO:NTI_FCNZLIB; break;
				case 't' : {
							 nLoops=atoi(p+1);
							 break;
						   }
				default: return exitMsg(-1,"Invalid option: %s",p);
			}
		}
		else strncpy(pathName,p,_MAX_PATH-1);
	}
	if(!*pathName) {
		Usage();
		exit(0);
	}

	if(nti.Open(pathName,0) || (!CNTIFile::DefaultCache() && nti.AllocCache(1))) {
		exitNTI("File open");
	}

	printf("File: %s -- (%u x %u) %u first-level records.\n"
		"Image size -- Decoded: %u  Encoded: %u (%.2f%%)\n",trx_Stpnam(pathName),
		nti.Width(),nti.Height(),nti.NumRecs(0),nti.SizeDecoded(),nti.SizeEncoded(),nti.LvlPercent(0));

	LPBYTE pRec;
	nti_tm_inf=0; //We'll time decoding

	printf("\nTiming...\n");

	for(UINT rec=nti.NumRecs(0);rec-->0;) {
		pRec=nti.GetRecord(rec);
		assert(pRec);
	}

	printf("Processing complete - Total Decode time: %.2f, Specific times:\n",
		GetPerformanceSecs(nti_tm_inf));

	for(int i=0;i<nti_tmSz;i++) printf("%d: %.3f\n",i,GetPerformanceSecs(nti_tm[i]));

	exitMsg(0,"");
}

