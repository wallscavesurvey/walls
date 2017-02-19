/* CSH_CACHE/CSH_FILE Test Program*/

#undef _DUMPFAR

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <a__csh.h>

#define _fardump(s)

static char csh_ext[4]="DAT";

/*==================================================================*/
/*CSH_FILE open/close - Normally file type-specific versions will be used.*/

static apfcn_i csf_Open(CSF_NO far *pcsf,ptr pathname,BOOL bCreate,DWORD flags,UINT bufsiz)
{
  int handle,e;
  CSF_NO csf;

  *pcsf=0;
  if(!(pathname=dos_FullPath(pathname,csh_ext))) return DOS_ErrName;
  if((e=(bCreate?dos_CreateFile:dos_OpenFile)(&handle,pathname,flags))!=0) return e;
  if((csf=(CSF_NO)dos_AllocNear(sizeof(CSH_FILE)))==0) {
    dos_CloseFile(handle);
    return DOS_ErrMem;
  }
  csf->Handle=handle;
  csf->Recsiz=bufsiz;
  *pcsf=csf;
  return(0);
}

static apfcn_i csf_Close(CSF_NO csf)
{
  int e;

  if(!csf) return DOS_ErrArg;
  e=_csf_DetachCache(csf);
  if(dos_CloseFile(csf->Handle) && !e) e=DOS_ErrClose;
  dos_FreeNear(csf);
  return e;
}

static apfcn_i csf_CloseDel(CSF_NO csf)
{
  int e=0;

  if(!csf) return DOS_ErrArg;
  _csf_Purge(csf);
  _csf_DetachCache(csf);
  if(dos_CloseDelete(csf->Handle)) e=DOS_ErrClose;
  dos_FreeNear(csf);
  return e;
}

//===========================================================
int main(int argc,char **argv)
{
   UINT rec;
   int i;
   CSH_HDRP hp;
   CSH_NO mb;
   CSF_NO fp;
   BYTE *pt;
   clock_t tm;
   int *ip;
   char *filnam="CSH";
   UINT bufsiz=1024,bufcnt=128;
   UINT filsiz=512,seed=1;
   long cnt,count=50000L;
   BOOL writing=0,updating=0;

   DWORD flags=0;

   if(argc<2) {
     printf("CSHTEST -c<bufcnt> -b<bufsiz> -f<filsiz> -n<filnam>\n"
            "        -s<seed> -write -update <count>\n");
     exit(0);
   }

   while(--argc) {
    argv++;
    if(**argv=='-') {
      (*argv)++;
      switch(**argv|' ') {
        case 'c' : ip=&bufcnt; break;
		case 'f' : ip=&filsiz; break;
        case 'b' : ip=&bufsiz; break;
        case 's' : ip=&seed; break;
        case 'n' : if(*(*argv+1)) filnam=*argv+1; continue;
		case 'w' : writing=1; continue;
		case 'u' : updating=1; continue;
		default  : continue;
      }
      *ip=atoi(*argv+1);
    }
    else count=atol(*argv);
   }

   if((i=csh_Alloc((TRX_CSH *)&mb,bufsiz,bufcnt))!=0) {
     printf("Cache error: %s.\n",csh_Errstr(i));
     exit(1);
   }

   _fardump("After cache create");

   printf("\n=== BUFFERS: %lu  Blk Size: %lu  File size: %u blks.\n",
    mb->Bufcnt,mb->Bufsiz,filsiz);

   i=csf_Open(&fp,filnam,writing,flags,bufsiz);
   if(!i && (i=_csf_AttachCache(fp,mb))!=0) csf_Close(fp);
   if(i) {
	  printf("File %s - %s.\n",filnam,csh_Errstr(i));
	  csh_Free(mb);
	  exit(1);
   }

   _fardump("After csf_Open");

   if(writing) {
     printf("\nWriting..");
     tm=clock();
     for(rec=0;rec<filsiz;rec++) {
		 if(!(hp=_csf_GetRecord(fp,rec,CSH_New|CSH_Updated)) || !(pt=hp->Buffer)) {
		     printf("\rWrite failure: Rec=%u - %s.\n",rec,_csf_LastErrstr(fp));
			 csf_CloseDel(fp);
			 csh_Free(mb);
			 exit(1);
		 }
         memset(pt,'A'+(rec%26),bufsiz);
     }
     tm=clock()-tm;
     printf("\r=== WROTE %d records - %.2f seconds.\n",rec,
		 (double)tm/CLOCKS_PER_SEC);
   }

   printf("\n%lu random reads%s.. ",count,updating?" with update":"");

   srand(seed);

   tm=clock();
   for(cnt=count;cnt;cnt--) {
     hp=_csf_GetRecord(fp,rec=(rand()%filsiz),0);
     if(!hp) {
		printf("\rRead failure: Rec=%u - %s.\n",rec,_csf_LastErrstr(fp));
		break;
     }
	 pt=hp->Buffer;
     if(*pt!='A'+(rec%26)) {
       printf("\rCorrupted file: Rec=%u\n",rec);
       break;
     }
     if(updating) {
       *(pt+1)='*';
       _csh_MarkHdr(hp);
     }
   }

   if(updating) _csf_Flush(fp);

   tm=clock()-tm;
   printf("\n=== READ %lu recs - Errno: %d - %.2f seconds - Hits: %lu.\n",
     count-cnt,fp->Errno,
     (double)tm/CLOCKS_PER_SEC,
     mb->HitCount);

   csf_Close(fp);

   csh_Free(mb);

   _fardump("After csh_Free");

   return 0;
}
