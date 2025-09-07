/*CMP2SRV.C v1.0 -- Data file Conversion from CMAP v16 to WALLS v1.1b2
  D.McKenzie, July 1996*/

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include <sys\stat.h>

/*NOTE: TRX_STR.H defines a few basic types I'm in the habit of using
  (e.g., UINT) and two string functions (trx_Stpnam and trx_Stpext) for
  parsing pathnames. It would be trivial to remove dependence on any
  of this.
*/ 
#include <trx_str.h>

#define LINMAX 256
#define INCMAX 16
#define PATHMAX 80
#define PI 3.141592653589793
#define U_DEGREES (PI/180.0)

static FILE *fp,*fpto,*fptp;
static FILE *inc_fp[INCMAX];
static char *toname,*frname,*pTitle;
static char inc_name[INCMAX][PATHMAX];
static int  inc_line[INCMAX];
static int  lnlen,inc_idx,dirnest;
static char ln[LINMAX];
static char punits[LINMAX],lnbuf[LINMAX],prjbuf[LINMAX],titlebuf[LINMAX];
static char topath[PATHMAX],frpath[PATHMAX],prjname[16],srvpfx[16];
static UINT linecount,filecount,veccount,nTitle;
static BOOL bTransit,bSegs,bProject;

/*Data fields --*/
enum e_fld {F_FROM,F_TO,F_DIST,F_TYPE,F_AZ,F_T,F_VERT,F_HTFROM,F_HTTO,F_TAPE};
static int fld[10][2]={
   {1,6},{8,6},{15,7},{22,1},{23,10},{34,1},{36,10},{47,5},{53,5},{58,2}
};

enum {TAPE_IT,TAPE_SS,TAPE_IS,TAPE_ST,TAPE_HZ,NUM_TTYPE};
static NPSTR ttype[NUM_TTYPE]={"IT","SS","IS","ST","HZ"};
static double iab,tab;
static char hc[20];
static char fldbuf[20];

static NPSTR prjtypes[]={
   "BOOK",
   "ENDBOOK",
   "NAME",
   "STATUS",
   "SURVEY"
};

enum {
  PRJ_BOOK,
  PRJ_ENDBOOK,
  PRJ_NAME,
  PRJ_STATUS,
  PRJ_SURVEY
};

static VOID write_prj(int id,char *tail)
{
  int len=sprintf(prjbuf,".%s\n",prjtypes[id]);
  
  if(tail && *tail) sprintf(prjbuf+len-1,"\t%s\n",tail);
  fputs(prjbuf,fptp);
}

static VOID up_dir(void)
{
  if(dirnest>0) {
    write_prj(PRJ_ENDBOOK,0);
    dirnest--;
  }
}

static VOID new_dir(char *title,char *name)
{
  write_prj(PRJ_BOOK,title);
  write_prj(PRJ_NAME,name);
  write_prj(PRJ_STATUS,dirnest?"24":"19");
  dirnest++;
}

static VOID new_project(char *name)
{
  *_NCHR(trx_Stpext(name))=0; /*Trim extension*/
  sprintf(lnbuf,"CMAP Project: %s",name);
  dirnest=0;
  new_dir(lnbuf,name);
}

static void trimtb(char *p)
{
	int len=strlen(p);
	while(len && p[len-1]==' ') len--;
	p[len]=0;
}

static int getline()
{
  if(NULL==fgets(ln,LINMAX,fp)) return -1;
  inc_line[inc_idx-1]++;
  lnlen=strlen(ln);
  if(lnlen && ln[lnlen-1]=='\n') ln[--lnlen]=0;
  trimtb(ln);
  return 0;
}

static void line_abort(char *msg)
{
    inc_idx--;
	printf("\nAborted - File: %s, Line: %d - %s.\n",
	  inc_name[inc_idx],inc_line[inc_idx],msg);
	exit(1);
}

static void outln(void)
{
	fprintf(fpto,"\n");
}

static void outtab(void)
{
	fprintf(fpto,"\t");
}

static void outstr(char *p)
{
	fprintf(fpto,"%s",p);
}

static void open_inc(char *path)
{
     if(inc_idx+1>=INCMAX) line_abort("Include nesting limit reached");
     /*Save name portion for err messages --*/
     strncpy(inc_name[inc_idx],_NCHR(trx_Stpnam(path)),PATHMAX-1);
     inc_line[inc_idx]=0;
     if((fp=inc_fp[inc_idx]=fopen(path,"r"))==NULL) {
        if(inc_idx) {
          sprintf(lnbuf,"Cannot open include file: %s",path);
          line_abort(lnbuf);
        }
        else {
          printf("\nAborted - Cannot open %s\n",path);
          exit(1);
        }
     }
     filecount++;
     inc_idx++;
}

static void close_inc(void)
{
	fclose(fp);
	inc_idx--;
	fp=inc_idx?inc_fp[inc_idx-1]:NULL;
}

static char *skipspc(char *p)
{
	/*Skip to first argument if any --*/
	while(*p && *p==' ') p++;
	return p;
}

static char *getarg(char *p)
{
   char *pbuf=lnbuf;
   while(*p && *p!=' ') *pbuf++=*p++;
   *pbuf=0;
   return p;
}

static char *getfld(int i)
{ 
	int len=fld[i][1];
	int idx=fld[i][0];
	
	if(idx>=lnlen) *fldbuf=0;
	else {
	  if(idx+len>lnlen) len=lnlen-idx;
	  memcpy(fldbuf,ln+idx,len);
	  while(len && fldbuf[len-1]==' ') len--;
	  fldbuf[len]=0;
	}
	return fldbuf;
}

static void data_abort()
{
    line_abort("Bad measurement");
} 

static apfcn_v trimzeros(char *p)
{
	char *p0;
	
	if(p0=strchr(p,'.')) {
	  p0+=strlen(p0);
	  while(*(p0-1)=='0') p0--;
	  if(*(p0-1)=='.') {
	    p0--;
	    if(p0-p==1 && (*p=='-' || *p=='+')) p0--;
	    if(p0==p) *p0++='0';
	  }
	  *p0=0;
	}
}

static void append_note(char *p,char *ptyp)
{
  char *pdest=punits+strlen(punits);
  sprintf(pdest," (%s=%s)",ptyp,p);
} 

static char *get_angle(char *p,BOOL bSign)
{
	int isign=1;
	double val;
	char *pi,*p0;
	char qsfx,qpfx=0,usfx=0;
	
	while(*p==' ') p++;
	if(!*p) return strcpy(lnbuf,"0");
	
	_strupr(p0=p);
	
	if(*p=='N' || *p=='S') {
	  if(bSign) data_abort();
	  append_note(p0,"A");
	  qpfx=*p++;
	  pi=p+strlen(p)-1;
	  if(pi<=p || *pi!='E' && *pi!='W') data_abort();
	  qsfx=*pi;
	  *pi=0;
	  trimtb(p);
	  while(*p==' ') p++;
	}
	 
	if(*p=='+'|| *p=='-') {
	  if(qpfx) data_abort();
	  if(*p++=='-') isign=-1;
	  while(*p==' ') p++;
	  if(!*p && isign<0 && !bSign) {
	    /*Let's allow an isolated "-" for an azimuth*/
	    p0[1]=0;
	    return p0;
	  }
	}
	
	if(pi=strchr(p,'D')) {
	  if(qpfx) data_abort();
	  *pi=' ';
	  trimtb(p);
	  while(*p==' ') p++;
	}
    
    /*Allow other suffixes --*/
	pi=p+strcspn(p,"%PMGF");
	if(*pi) {
	  if(qpfx || *(pi+1)) data_abort();
	  if(*pi=='F') line_abort("French mils not supported");
	  append_note(p0,bSign?"V":"A");
	  usfx=*pi;
	  *pi=0;
	  trimtb(p);
	}
	
    /*Get degree portion --*/
	if(*p!='.' && !isdigit((BYTE)*p)) data_abort();
	val=atof(p);
	
	if(pi=strchr(p,' ')) {
	  if(qpfx || usfx) data_abort();
	  append_note(p0,bSign?"V":"A");
	  p=++pi;
	  while(*p && *p==' ') p++;
	  /*Get minutes portion --*/
	  if(*p!='.' && !isdigit((BYTE)*p)) data_abort();
	  val+=atof(p)/60.0;
	  if(pi=strchr(p,' ')) {
		  p=++pi;
		  while(*p && *p==' ') p++;
		  /*Get seconds portion --*/
		  if(*p!='.' && !isdigit((BYTE)*p)) data_abort();
		  val+=atof(p)/3600.0;
	  }
	}
	
	if(usfx) {
	  switch(usfx) {
	    case 'P' :
	    case '%' : if(val>100.0) data_abort();
	     		   val*=0.45;
	     		   break;
	    case 'M' : if(val>=6400.0) data_abort();
	               val*=360.0/6400.0;
	               break;
	    case 'G' : if(val>=400.0) data_abort();
	    		   val*=360.0/400.0;
	    		   break;
      }
	}
	
	if(qpfx) {
	  if(val>=180.0) data_abort();
	  if(qpfx=='N') {
	    if(qsfx=='W') val=-val;
	  }
	  else {
	    if(qsfx=='W') val+=180.0;
	    else val=180.0-val;
	  }
	}
	else {
	  val=fmod(val,360.0);
	  if(bSign) {
	    if(bTransit) {
	      if(isign<0 || val>180.0) data_abort();
	      val=90.0-val;
	      append_note("Tr","V");
	    }
	    else {
	      if(val>90.0) data_abort();
	      val*=isign;
	    }
	  }
	  else if(isign<0) val=360.0-val;
	}
	
	sprintf(lnbuf,"%.2f",val);
	trimzeros(lnbuf);
	return lnbuf;
}

static char *get_inchstr(double val,int isign)
{
	double frac=fmod(val,1.0);
	char *p=lnbuf;
	
	if(isign<0) *p++='-';
	if(val>=1.0) p+=sprintf(p,"%lu",(DWORD)val);
	*p++='i';
	sprintf(p,"%.2f",frac*12.0);
	trimzeros(p);
	return lnbuf;
}

static void adjust_dist(double *dist)
{
    int ityp,idir;
    double h,finc;
    
	if(!*getfld(F_TAPE)) return;
	_strupr(fldbuf);
	
	for(ityp=0;ityp<NUM_TTYPE;ityp++) if(!strcmp(ttype[ityp],fldbuf)) break;
	if(ityp==NUM_TTYPE) line_abort("Unrecognized taping method");
		
	/*We may need to adjust the distance to make it a proper
	INSTRUMENT-TO-TARGET measurement --*/
	
	if(ityp==TAPE_IT) return;
		
	finc=U_DEGREES*atof(get_angle(getfld(F_VERT),TRUE));
	
	switch(ityp) {
		case TAPE_SS: idir=1; h=tab-iab;
		              break;
		              /*Haven't seen any of these yet --*/
		case TAPE_IS: idir=1; h=tab;
		              break;
		case TAPE_ST: idir=-1; h=iab;
		              break;
		case TAPE_HZ: if((finc=cos(finc))<=0.0) return;
					  finc=*dist/finc;
					  goto _fix;
	}
	
	finc=h*sin(finc*idir);
	h=*dist*(*dist)+finc*finc-h*h;
	finc+=sqrt(h);

_fix:	
	if(fabs(*dist-finc)>=.005) {
	  append_note(getfld(F_DIST),ttype[ityp]);
      *dist=finc;
    }
}

static char *get_dist(char *p,BOOL bSign)
{
	char *pi;
	BOOL bMetric=FALSE;
	BOOL bInches=FALSE;
	int isign=1;
	double val;
	
	while(*p==' ') p++;
	
	if(*p=='+') p++;
	else if(*p=='-') {
	  if(!bSign) data_abort(); 
	  isign=-1;
	  p++;
	}
	while(*p==' ') p++;
	
	if(*p!='.' && !isdigit((BYTE)*p)) data_abort();
	
	_strupr(p);
	
	if(strchr(p,'Y')) line_abort("Yards not supported"); /*Give me a break!*/
	
	if(pi=strchr(p,'M')) {
	  *pi=' ';
	  bMetric=TRUE;
	}

	if(pi=strchr(p,'F')) {
	  if(bMetric) data_abort();
	  *pi=' ';
	}
	
	if(pi=strchr(p,'I')) {
	  if(bMetric) data_abort();
	  *pi=' ';
	  bInches=TRUE;
	}
	
	trimtb(p);
	val=atof(p);
	
	if(pi=strchr(p,' ')) {
	  while(*pi && *pi==' ') pi++;
	  if(bMetric) {
	    if(!isdigit((BYTE)*pi)) data_abort();
	    *--pi='.';
	    val+=atof(pi);
	  }
	  else {
	    if(*pi!='.' && !isdigit((BYTE)*pi)) data_abort();
	    val+=atof(pi)/12.0;
	    bInches=TRUE;
	  }
	}
	else if(bInches) val/=12.0;
	
	if(!bSign) adjust_dist(&val);
	
	if(bInches) get_inchstr(val,isign);
	else {
	  sprintf(lnbuf,"%.2f",isign*val);
	  trimzeros(lnbuf);
	  if(bMetric) strcat(lnbuf,"M");
	}
	return lnbuf;
}
	
static BOOL process_command(void)
{
	char *p;
	int e;
	
	/*Retrieve keyword --*/
	p=getarg(ln);
	_strupr(lnbuf);
	
	if(!strcmp(lnbuf,"DEFINE")) {
	  p=getarg(skipspc(p));
	  fprintf(fpto,"#FIX\t%s",lnbuf);
	  /*Assume EAST-NORTH-UP wrt true north?*/
	  for(e=0;e<3;e++) {
	    /*Apparently spaces are used as separators, not column positions --*/
	    p=getarg(skipspc(p));
	    fprintf(fpto,"\t%s",get_dist(lnbuf,TRUE));
	  }
	  outln();
	  veccount++;
	  return TRUE;
	}
	
	if(!strcmp(lnbuf,"DECLIN")) {
	  /*Organ data suggests no embedded spaces allowed in angle --*/
	  getarg(skipspc(p));
	  fprintf(fpto,"#UNITS Decl=%s\n",get_angle(lnbuf,TRUE));
	  return TRUE;
	}
	
	if(!strcmp(lnbuf,"NOTE")) {
	  p=getarg(skipspc(p));
	  fprintf(fpto,"#NOTE\t%s\t%s\n",_strupr(lnbuf),skipspc(p));
	  return TRUE;
	}
	
	/*All other commands are output as comments --*/
	fprintf(fpto,";%s\n",ln);
	
	if(!strcmp(lnbuf,"INCLUDE")) {
	  getarg(skipspc(p));
	  open_inc(_strupr(lnbuf));
	  if(bSegs) fprintf(fpto,"#SEG %s\n",inc_name[inc_idx-1]);
	  return TRUE;
	}
	
	if(!strcmp(lnbuf,"STOP")) return FALSE;
	
	if(bProject && linecount==nTitle) {
	  for(p=ln+1;*p && *p!=' ';p++);
	  while(*p==' ') p++;
	  if(*p) strcpy(pTitle=titlebuf,p);
	}

	return TRUE;
} 

static char *get_name(int i)
{
	/*Returns pointer to compressed name --*/
	char *pSrc=getfld(i); /*places uncompressed name in fldbuf*/
	char *pDst=lnbuf;
	
	while(*pSrc) {
	  if(*pSrc!=' ') *pDst++=*pSrc;
	  pSrc++;
	}
	*pDst=0;
	if(!*lnbuf) line_abort("Empty name field");
	return lnbuf;
}

static void get_hc(void)
{
	iab=tab=0.0;
	getfld(F_HTFROM);
	if(*fldbuf) iab=atof(get_dist(fldbuf,TRUE));
	getfld(F_HTTO);
	if(*fldbuf) tab=atof(get_dist(fldbuf,TRUE));
	if(iab-tab!=0.0) {
	  sprintf(hc,"%.2f",iab-tab);
	  trimzeros(hc);
	}
}

static char *get_fptime(void)
{
  static struct _stat st;
  struct tm *ptm;
  lnbuf[0]=0;
  if(!_fstat(_fileno(fp),&st)) {
    ptm=localtime(&st.st_ctime);
    sprintf(lnbuf,"%02d:%02d %02d/%02d/%02d",
      ptm->tm_hour,ptm->tm_min,
      ptm->tm_mon+1,ptm->tm_mday,ptm->tm_year);
  }
  return lnbuf;
}

static void convert_file(void)
{
	inc_idx=0;
	filecount=veccount=0;
	
	open_inc(frpath);
	
	if((fpto=fopen(topath,"w"))==NULL) {
		printf("Cannot create file %s.\n",topath);
		exit(1);
	}
	
	printf("%-13s--> %-13s",frname,toname);
	linecount=0;
	
	fprintf(fpto,";%s - CMAP File Dated %s\n",inc_name[0],get_fptime());
	fprintf(fpto,"#UNITS Feet Case=Upper\n");
    
    if(bProject) pTitle=frname;
     
	/*Loop to read CMAP formatted lines --*/
	while(TRUE) {
		if(getline()<0) {
		  close_inc();
		  if(!fp) break;
		  if(bSegs) fprintf(fpto,"#SEG ..\t");
		  fprintf(fpto,";END INCLUDE: %s\n",inc_name[inc_idx]);
		  continue;
		}
		linecount++;
		
		*hc=0;          /*Examined by get_dist()*/
	    bTransit=FALSE; /*Examined by get_angle()*/
		
		if(*ln!=' ') {
			if(!process_command()) break;
			continue;
		}
		else {
		  /*Vector data --*/
		  
		  *punits=0; /*Use for units comments*/
		  
		  /*Get height correction in advance --*/
		  get_hc();
		  
		  outstr(get_name(F_FROM));
		  outtab();
		  outstr(get_name(F_TO));
		  outtab();
		  outstr(get_dist(getfld(F_DIST),FALSE));
		  outtab();
		  outstr(get_angle(getfld(F_AZ),FALSE));
		  
		  /*Check if transit elevation is supplied --*/
		  if(*getfld(F_T) && toupper(*fldbuf)=='T') bTransit=TRUE;
		  
		  outtab();
		  outstr(get_angle(getfld(F_VERT),TRUE));
		  if(*hc) {
		    outtab();
		    outstr(hc);
		  }
		  if(*getfld(F_TYPE)) fprintf(fpto,"\t#S %c",*fldbuf);
		  if(*punits) fprintf(fpto,"\t;!%s",punits);
		  outln();
		  veccount++;
		}
	}
	fclose(fpto);
	
	/*Add file to project script --*/
	if(bProject) {
		write_prj(PRJ_SURVEY,pTitle);
		write_prj(PRJ_NAME,toname);
		write_prj(PRJ_STATUS,"24");
	}
	
	printf("Includes:%3u  Vectors:%5u\n",
	  filecount-1,veccount);
}

void main(int argc,char **argv)
{
	int e;
	UINT vectotal,filetotal;
	char *p;
    struct _find_t ft;
	
	*topath=*frpath=0;
	nTitle=0;
	bSegs=TRUE;
	    
	for(e=1;e<argc;e++) {
		p=_strupr(argv[e]);
		if(*p=='-') {
		  if(*++p=='X') bSegs=FALSE;
		  else if(*p++=='T') nTitle=atoi(p);
		  continue;
		}
		if(!*frpath) strncpy(frpath,p,PATHMAX-5);
		else if(!*topath) strncpy(topath,p,PATHMAX-5);
	}
  
	if(!*frpath) {
		printf("\nCMP2SRV v1.0 - Convert CMAP v16 Data Files to WALLS Format\n\n"
		       "USAGE: CMP2SRV <filespec> [<prjpath>] [-tN] [-x]\n\n"
		       "NOTES: Each CMAP file matching <filespec> will produce just one\n"
		       "WALLS data file (extension SRV) which will also incorporate any\n"
		       "\"included\" files. If <filespec> contains wildcards, <prjpath> defines\n"
		       "the pathname of the generated project script (extension PRJ). Name\n"
		       "prefixes for the SRV data files are then formed from the first four\n"
		       "characters of the name in <prjpath>. However, if <filespec> is without\n"
		       "wildcards, no project script is created, and the SRV file base name is\n"
		       "obtained from either <prjpath> or <filespec>.\n\n"
		       "Use \"-tN\" to extract survey titles from Nth line of each data file.\n"
		       "Use \"-x\" to suppress the defining of segments for included files.\n"
		       "Comments prefixed with \";!\" will flag data that had to be converted\n"
		       "to different units or adjusted to reflect instrument-target taping.\n"
		);
		exit(0);
	}
	
	toname=_NCHR(trx_Stpnam(topath));  /*_NCHR() converts ptr from FAR to NEAR*/
	frname=_NCHR(trx_Stpnam(frpath));
	/*These ptrs now point to name portion of paths, or to trailing 0*/
	
	bProject= strchr(frname,'*') || strchr(frname,'?');
	
	if(bProject) {
		if(!*toname) {
			printf("Use of wildcards requires specification of a project pathname.\n"
			       "For instructions, type the command without arguments.\n");
			exit(1);
		}
		
		if(_dos_findfirst(frpath,_A_NORMAL,&ft)) {
			printf("No files matching %s\n",frpath);
			exit(1);
		}
		
		/*Set up prefix for generated SRV filenames --*/
		strcpy(srvpfx,toname);
		srvpfx[4]=0;
		
		/*Form PRJ script name, overriding any file extension --*/
		strcpy(_NCHR(trx_Stpext(toname)),".PRJ");
		
		if((fptp=fopen(topath,"w"))==NULL) {
			printf("Cannot create project script %s.\n",topath);
			exit(1);
		}
		/*Create root directory in PRJ file --*/
		strcpy(prjname,toname);
		new_project(toname);
    }
    else {
        /*We are creating a single SRV file*/
		if(!*toname) {
			if(!*topath) {
				strcpy(topath,frpath);
				toname=_NCHR(trx_Stpnam(topath));
			}
			strcpy(toname,frname);
		}
		strcpy(_NCHR(trx_Stpext(toname)),".SRV");
    }
	  
	filetotal=vectotal=0;
	
	while(TRUE) {
		filetotal++;
		
		if(bProject) {
		    strcpy(frname,ft.name); /*Fix source pathname, frpath*/
			/*Generate unique filename and fix destination pathname, topath*/
			sprintf(toname,"%s%04u.SRV",srvpfx,filetotal);
		}
		
		/*Read and convert one CMAP file and any nested includes --*/
		convert_file();
		
		vectotal+=veccount;
		
		if(!bProject || _dos_findnext(&ft)) break;
	}
	
	if(bProject) {
	    /*Finish writing project script --*/
		while(dirnest) up_dir();
		fclose(fptp);
		printf("\nProject %s created. Files: %u Vectors: %u\n",
		  prjname,filetotal,vectotal);       
	}
	else printf("Conversion complete.\n");
	exit(0);
}
