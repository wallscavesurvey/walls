/*CSS2SRV.C v1.0 -- Convert Compass (CSS) Data files to Walls (SRV) Format*/

//#include <dos.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <math.h>
#include <time.h>
#include <sys\stat.h>

#include <trx_str.h>

#define LINLEN 512

#define SIZ_SURVEY _MAX_PATH
#define SIZ_WARNMSG 120
#define SIZ_NAMBUF 16

static FILE *fp,*fpto,*fppr;
static char ln[LINLEN],formatstr[LINLEN];
static char frpath[_MAX_PATH],prpath[_MAX_PATH];
static char *frname,*prname;
static char survey[SIZ_SURVEY];
static char datname[SIZ_SURVEY],prjname[SIZ_SURVEY];
static char D_units[16],A_units[16],V_units[16],S_units[16];
static char item_typ[8];
static int	item_idx[8];
static char datestr[16];
static char lastfrom[SIZ_NAMBUF];
static UINT outlines,linecount,slinecount;
static BOOL bRedundant,bLrudOnly,bWarn;
static BOOL A_bQuad,D_bInches,S_bInches,V_bDepth,V_bMinutes;
static char *pIncA,*pIncV,*pIncD;
static char *pZero="0";

int nameinc=0;

static BOOL bSeg,bSuffix;
static BOOL bProject,bKeepColons;

TRXFCN_S trx_Stpnam(LPCSTR pathname)
{
	LPCSTR p=pathname+strlen(pathname);

	while(--p>=pathname && *p!='\\' && *p!='/' && *p!=':');
	return (LPSTR)p+1;
}

TRXFCN_S trx_Stpext(LPCSTR pathname)
{
	int len=strlen(pathname);
	LPCSTR p=pathname+len;

	while(p>pathname && *--p!='.' && *p!='\\' && *p!='/' && *p!=':');
	if(*p!='.') p=pathname+len;
	return (LPSTR)p;
}

static void _cdecl outf(char *fmt,...)
{
	va_list va;
	va_start(va,fmt);
	vfprintf(fpto,fmt,va);
}

static int getline()
{
  int len;
  if(NULL==fgets(ln,LINLEN,fp)) return CFG_ERR_EOF;
  len=strlen(ln);
  if(len && ln[len-1]=='\n') ln[--len]=0;
  return len;
}

static void err_format(char *msg)
{
  printf("Aborted: %s, line: %d - %s.\n",datname,linecount,msg);
  exit(1);
} 

static void warn_format(char *msg)
{
  static char warnmsg[SIZ_WARNMSG];

  if(!msg) {
	outf(";%s",warnmsg);
	outlines++;
	bWarn=FALSE;
	return;
  }
  sprintf(warnmsg,"*** Warning: %s, line: %d - %s.\n",datname,linecount,msg);
  printf(warnmsg);
  bWarn=TRUE;
} 

static void outtab(void)
{
	outf("\t");
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

static void out_numeric(int i)
{
   char *p;
   
   if(i>=cfg_argc) err_format("Numeric value expected");
   p=cfg_argv[i];
   if(strchr(p,'.')) {
	   i=strlen(p);
	   while(i>1 && p[i-1]=='0') i--;
	   if(p[i-1]=='.') i--;
	   p[i]=0;
   }
   outf(p);
}

static void outln(void)
{
  outf("\n");
  outlines++;
}

static void out_dimensions(void)
{
    int i;
    BOOL nonzeros=FALSE;
	BOOL bnul[8];
    double f;
	
    for(i=4;i<8;i++) {
	  bnul[i]=FALSE;
	  f=atof(cfg_argv[item_idx[i]]);
	  if(f>0.0 && f<999.0) nonzeros=TRUE;
	  else if(f!=0.0) bnul[i]=TRUE;
	}
	
	if(nonzeros) {
	  outf("\t<");
      for(i=4;i<8;i++) {
		if(bnul[i]) outf("--");
		else out_numeric(item_idx[i]);
		if(i!=7) outf(",");
	  }
	  outf(">");
	}
}  

static BOOL get_datestr(UINT m,UINT d,UINT y)
{
	if(!m || m>12 || !d || d>31 || y>3000) return FALSE;
	if(y<50) y+=2000;
	else if(y<100) y+=1900;
	else if(y<1800) return FALSE;
    sprintf(datestr,"%4u-%02u-%02u",y,m,d);
	return TRUE;
}

static BOOL out_survey_fld(char *p,char *typ)
{
	if(strlen(p)<11 || _memicmp(p+7,typ,4)) {
	  if(*typ=='N') return FALSE;
	  sprintf(ln,"\"SURVEY %s\" line expected",typ);
	  err_format(ln);
	}

	if(*typ=='T') return TRUE;

	p+=11;
	if(*p==':') p++;
	while(isspace((BYTE)*p)) p++;

	if(*typ=='N') {
		if(!*p) err_format("\"SURVEY NAME:\" has no argument");
		strncpy(survey,p,SIZ_SURVEY-1);
	}
	else if(*typ=='D') {
		outf("\n;==================================================================\n"
			 ";%s\n",formatstr);
		outlines+=3;
		if(typ=strstr(p,"COMMENT:")) {
			*typ=0;
			typ+=8;
			while(isspace((BYTE)*typ)) typ++;
		}
		if(bSeg) outf("#Segment /");
		else outf(";Survey ");
		outf(survey);
		if(typ && *typ) {
			/*Don't output excessively long name string --*/
			if(strlen(typ)<=40) outf(": %s",typ);
			else {
			   outlines++;
			   outf("\n;%s",typ);
			}
		}
		outln();
		cfg_GetArgv(p,CFG_PARSE_ALL);
		*datestr=0;
		if(cfg_argc<3 || !get_datestr(atoi(cfg_argv[0]),atoi(cfg_argv[1]),atoi(cfg_argv[2])))
			warn_format("Bad date format");
		return TRUE;
	}
	return TRUE;
}

static int get_item_typ(int c,int i)
{
	switch(c) {
		case 'L' : c='D'; item_idx[i]=2; break;
		case 'A' : c='A'; item_idx[i]=3; break;
		case 'D' : c='V'; item_idx[i]=4; break;
		default  : err_format("Bad shot item order");
	}
	return c;
}

static void get_formatstr(int i)
{
	char *pf,*pb=formatstr;
	char order[20];

	A_bQuad=D_bInches=S_bInches=V_bDepth=FALSE;

	if(i>=cfg_argc || strlen(pf=_strupr(cfg_argv[i]))<11)
		err_format("FORMAT argument missing");

	bRedundant=pf[11]=='B';

	if(pf[3]=='W') {
		V_bDepth=TRUE;
		/*reorder 'D' (inclination) measurment to be last*/
		if(pf[10]!='D') {
			if(pf[8]=='D') pf[8]=pf[10];
			else pf[9]=pf[10];
			pf[10]='D';
		}
	}

	for(i=0;i<3;i++) {
		item_typ[i]=get_item_typ(pf[i+8],i);
	}
	item_typ[3]=0;

	for(i=4;i<8;i++) item_typ[i]='S';
	item_idx[4]=4+1;
	item_idx[5]=4+4;
	item_idx[6]=4+2;
	item_idx[7]=4+3;

	if(!strchr(item_typ,'D') ||
	   !strchr(item_typ,'A') ||
	   !strchr(item_typ,'V')) err_format("Bad shot item order");

	if(pf[0]=='R') strcpy(A_units,"Grads");
	else {
	  strcpy(A_units,"Deg");
	  if(pf[0]=='Q') A_bQuad=TRUE;
	}

	if(pf[1]=='M') strcpy(D_units,"Meters");
	else {
	  strcpy(D_units,"Feet");
	  if(pf[1]=='I') D_bInches=TRUE;
	}

	if(pf[2]=='M') strcpy(S_units,"Meters");
	else {
	  strcpy(S_units,"Feet");
	  if(pf[2]=='I') S_bInches=TRUE;
	}

	strcpy(order,item_typ);

	if(V_bDepth) {
		item_typ[2]='D';
		strcpy(order+2," Tape=SS");
	}
	else {
		if(pf[3]=='R') strcpy(V_units,"Grads");
		else if(pf[3]=='G') strcpy(V_units,"Pcnt");
		else {
		  strcpy(V_units,"Deg");
		  if(pf[3]=='M') V_bMinutes=TRUE;
		}
	}

	pb+=sprintf(pb,"Order=%s D=%s A=%s",order,D_units,A_units);
	if(!V_bDepth) pb+=sprintf(pb," V=%s",V_units);
	pb+=sprintf(pb," S=%s",S_units);

	if(bRedundant) {
		pb+=sprintf(pb," AB=%s",A_units);
		pb+=sprintf(pb," VB=%s",V_units);
	}

	*pb=0;
}	


static void out_correct(char *p,char *nam)
{
	char c=(p==pIncD)?'f':'d';

	cfg_argv[0]=p;
	outf(" %s=",nam);
	out_numeric(0);
	outf("%c",c);
	if(bRedundant && c=='d') {
		outf(" %sB=",nam);
		out_numeric(0);
		outf("%c",c);
	}
}

static void out_units(char *p)
{
	int i;
	BOOL bFormat=FALSE;
	int format_idx;

	bRedundant=FALSE;

	outln();
	if(bWarn) warn_format(0);
    cfg_GetArgv(p,CFG_PARSE_ALL);
	outf("#Units decl=");
	out_numeric(i=0);
	outf("d");
	if(cfg_argc>i+2 && !stricmp(cfg_argv[i+1],"FORMAT:")) {
		get_formatstr(i+=2);
		format_idx=i;
	}
	else {
		cfg_argv[0]="DDDDUDRLLADN";
		get_formatstr(0);
		format_idx=0;
	}

	if(cfg_argc>i+1 && !stricmp(cfg_argv[++i],"CORRECTIONS:")) {
		  p=(++i<cfg_argc)?cfg_argv[i]:pZero;
		  if(pIncA || atof(p)) pIncA=p;
		  p=(++i<cfg_argc)?cfg_argv[i]:pZero;
		  if(pIncV || atof(p)) pIncV=p;
		  p=(++i<cfg_argc)?cfg_argv[i]:pZero;
		  if(pIncD || atof(p)) pIncD=p;
		  if(cfg_argc<=i) warn_format("Three corrections expected");
	}
	else {
		  if(pIncA) pIncA=pZero;
		  if(pIncV) pIncV=pZero;
		  if(pIncD) pIncD=pZero;
	}

	outf(" %s",formatstr);
	if(format_idx) outf(" ;Format: %s",cfg_argv[format_idx]);
	outln();

	if(pIncA || pIncV || pIncD) {
		outf("#Units");
		if(pIncA) out_correct(pIncA,"IncA");
		if(pIncV) out_correct(pIncV,"IncV");
		if(pIncD) out_correct(pIncD,"IncD");
		outln();
	}
	if(*datestr) {
	    outf("#Date %s\n",datestr);
		outlines++;
	}
}
 
static char *get_fptime(void)
{
  static struct _stat st;
  struct tm *ptm;
  ln[0]=0;
  if(!_fstat(_fileno(fp),&st)) {
    ptm=localtime(&st.st_ctime);
    sprintf(ln,"%d-%02d-%02d %02d:%02d",
      ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday,
      ptm->tm_hour,ptm->tm_min);
  }
  return ln;
}

static void filter_name(char *nam)
{
	char c;
	char *p;
	int len=strlen(nam);
	BOOL bColonSeen=FALSE;

	for(p=nam;len;p++,len--) {
		c=*p;
		if(c==':') {
			if(!bKeepColons || bColonSeen) *p='|';
			else bColonSeen=TRUE;
		}
		else if(c=='#') *p='~';
		else if(c==';') *p='^';
		else if(c==',') *p='`';
	}
}

static char *get_namstr(int i)
{
	char *p;
	int len;
	static char nambuf[SIZ_NAMBUF];

	if(!i) {
		bLrudOnly=!strcmp(cfg_argv[0],cfg_argv[1]);
		if(bLrudOnly && atof(cfg_argv[2])>0.0) warn_format("Self loop not zero length");
	}
	else if(bLrudOnly) {
		if(*lastfrom) return lastfrom;
		warn_format("No reference vector for LRUD");
		return nambuf;
	}

	filter_name(p=cfg_argv[i]);

	if((len=(strlen(p)-8))>0 && (!bKeepColons || p[len-1]!=':')) {
	   memcpy(nambuf,p,len);
	   p+=len;
	   if(nambuf[len-1]=='|') len--;
	   nambuf[len++]=':';
	}
	else len=0;
	strcpy(nambuf+len,p);

	if(!i && !bLrudOnly) strcpy(lastfrom,nambuf);
	return nambuf;
}

static BOOL out_measurement(char *p,int c,BOOL bBack)
{
	double d,w;
	char buf[40];
	BOOL bInches;
	int units;

	switch(c) {
	case 'S' :
	case 'D' :
		if(*p=='-') warn_format("Negative length quantity");
		if(c=='S') {
			bInches=S_bInches;
			units=*S_units;
		}
		else {
			bInches=D_bInches;
			units=*D_units;
		}
		if(units=='M') {
			sprintf(buf,"%.2f",atof(p)*0.3048);
			cfg_argv[0]=buf;
			out_numeric(0);
			if(bSuffix) outf("m");
			break;
		}
		if(bInches) {
			d=atof(p);
			if(d<0.0) {
				outf("-");
				d=-d;
			}
			d=modf(d,&w);
			outf("%.0fi",w);
			sprintf(buf,"%.1f",d*12);
			cfg_argv[0]=buf;
			out_numeric(0);
			break;
		}
		cfg_argv[0]=p;
		out_numeric(0);
		if(bSuffix || (V_bDepth && *S_units!=*D_units)) outf("%c",tolower(units));
		break;

	case 'A' :
		d=atof(p);
		if(d<=-900.0) return FALSE;
		if(bBack) outf("/");
		if(d<0.0 || d>=360.0) {
			if(d>360.0) warn_format("Azimuth out of range");
			d=fmod(d,360.0);
		    if(d<0.0) d+=360.0;
		}
		if(A_bQuad) {
			if(d<=90.0 || d>=270.0) {
			  outf("N");
			  if(d>=270.0) {
				  d=360.0-d;
			      units='W';
			  }
			  else units='E';
			}
			else {
			  outf("S");
			  if(d>180.0) {
				units='W';
				d-=180.0;
			  }
			  else {
				units='E';
				d=180.0-d;
			  }
			}
			sprintf(buf,"%.2f",d);
			cfg_argv[0]=buf;
			out_numeric(0);
			outf("%c",units);
			break;
		}

		if(*A_units=='G') d*=(10.0/9.0);
		sprintf(buf,"%.2f",d);
		cfg_argv[0]=buf;
		out_numeric(0);
		if(bSuffix) outf("%c",tolower(*A_units));
		break;

	case 'V' :
		d=atof(p);
		if(d<=-900.0) return FALSE;
		if(bBack) outf("/");
		if(d<-90.0 || d>90.0) {
			warn_format("Inclination out of range");
			d=fmod(d,90.0);
		}
		if(*V_units=='G') d*=(10.0/9.0);
		else if(*V_units=='P') d*=(100.0/45.0);
		else if(V_bMinutes) {
			if(d<0.0) {
				outf("-");
				d=-d;
			}
			d=modf(d,&w);
			outf("%.0f:",w);
			sprintf(buf,"%.1f",d*60);
			cfg_argv[0]=buf;
			out_numeric(0);
			break;
		}
		sprintf(buf,"%.2f",d);
		cfg_argv[0]=buf;
		out_numeric(0);
		if(bSuffix) outf("%c",tolower(*V_units));
		break;
	}
	return TRUE;
}

static void out_item(int i)
{
	BOOL bEmpty;
	int c=item_typ[i];
	
	bEmpty=!out_measurement(cfg_argv[item_idx[i]],c,FALSE);
	if(bEmpty) outf("--");

	if(bRedundant && (c=='V' || c=='A')) out_measurement(cfg_argv[c=='A'?9:10],c,TRUE);
	else if(bEmpty) warn_format("Measurement out of range");
}

static void out_flagstr(BOOL bFlags)
{
	outf("\t#S ");
	switch(bFlags) {
		case 1: outf("L"); break;
		case 2: outf("P"); break;
		case 3: outf("P/L");
	}
}	

static BOOL out_comment(int i)
{
	static char buf[LINLEN];
	char *pb;

	if(i) {
		for(pb=buf;i<cfg_argc;i++) pb+=sprintf(pb," %s",cfg_argv[i]);
		if(strlen(buf)<=20) return TRUE;
	}
	else outtab();

	for(pb=buf;isspace((BYTE)*pb) || *pb==';';pb++);
	outf(";%s",pb);
	if(i) outln();
	return FALSE;
}

static void out_labels(void)
{
	int e;

	outf(";FROM\tTO");
	for(e=0;e<3;e++) {
	   switch(item_typ[e]) {
		   case 'D' : if(V_bDepth && e==2) outf("\tDEPTH");
					  else outf("\tDIST");
					  break;
		   case 'A' : outf("\tAZ");
					  break;
		   case 'V' : outf("\tINCL");
					  break;
	   }
	}
	outln();
	outln();
}

static void open_fpto(void)
{
	char *path;

	if(bProject) {
		if(!fppr) {
			if((fppr=fopen(prpath,"w"))==NULL) {
				printf("Error creating project %s.\n",prpath);
				exit(1);
			}
			fprintf(fppr,";WALLS Project File\n"
				".BOOK\t%s Project\n"
				".NAME\tPROJECT\n"
				".STATUS\t19\n",prjname);
		}
		strcpy(prname,trx_Stpnam(frpath));
		fprintf(fppr,".SURVEY\tImport from %s\n"
			         ".NAME\t%s\n"
					 ".STATUS\t24\n",datname,prname);
		path=prpath;
	}
	else path=frpath;

	if((fpto=fopen(path,"w"))==NULL) {
		printf("Error creating %s.\n",path);
		exit(1);
	}
	printf("Writing %s...\n",path);

	outlines=2;

	outf(";Import from %s\n"
		 ";Original Compass file last updated %s\n",datname,get_fptime());
}


static int copy_hdr(void)
{
	int e;
	char *p;
	int max_team_lines=10;

    slinecount=0;
	while(!(e=getline())) linecount++;

    while(e>=0) {
		 slinecount++;
		 linecount++;
		 if(e>LINLEN-2) {
			 if(slinecount>2) warn_format("Overlong line in header - file skipped");
			 return 0;
		 }
		 for(p=ln;isspace((BYTE)*p);p++);

         switch(slinecount) {
         
           case 1:	if(!*p) p="Untitled Survey";
					strcpy(formatstr,p);
					break;

           case 2:  *survey=0;
                    if(!out_survey_fld(p,"NAME")) return 0;
					if(!fpto) open_fpto();
					bWarn=FALSE;
					break;

           case 3: out_survey_fld(p,"DATE");
			       break;
           case 4: out_survey_fld(p,"TEAM"); break;
		   case 5: if(*p) {
			         outf(";Team: %s\n",p);
			         outlines++;
				   }
				   break;

		   case 6: if(_memicmp(p,"DECLINATION:",12)) {
			          if(!max_team_lines--) err_format("Missing \"DECLINATION:\" line");
					  if(*p) {
						outf(";      %s\n",p);
					    outlines++;
					  }
				      slinecount--;
					  e=getline();
					  continue;
				   }
				   out_units(p+12);
				   if(bWarn) warn_format(0);
				   outln();
				   break;

		   case 7:
		   case 9: if(*p) warn_format("Blank line expected");
			       break;

		   case	8: if(_memicmp(p,"FROM",4)) warn_format("Missing label line");
				   out_labels();
				   break;

		   default : if(e) return e;
         }
         e=getline();
	}
	if(slinecount && slinecount<10) {
		err_format("Incomplete survey header");
	}
	return e;
}

int main(int argc,char **argv)
{
  int e,c=0;
  char *p;
  UINT filecount,segcount,segtotal,veccount,vectotal;
  BOOL bFixed,bFlags,bComm,bExclude;
  struct _finddata_t ft;
  long hFile;

  bProject=bKeepColons=bSuffix=FALSE;
  bSeg=TRUE;

  for(e=1;e<argc;e++) {
    p=argv[e];
    if(*p=='-') {
      while(c=*++p) {
		  switch(tolower(c)) {
		    case 'p' : if(e==argc-1) {
						   printf("Project pathname must follow option -p\n");
						   exit(1);
					   }
					   argv[e++]=0;
					   strcpy(prpath,argv[e]);
					   prname=trx_Stpnam(prpath);
					   strcpy(trx_Stpext(prname),".wpj");
					   *trx_Stpext(strcpy(prjname,prname))=0;
					   bProject=TRUE;
					   break;

			case 'c' : bKeepColons=TRUE; continue;
			case 's' : bSeg=FALSE; continue;
			case 'u' : bSuffix=TRUE; continue;
			default  : printf("\nInvalid option: -%c\n",*p);
				       exit(1);
		  }
		  break;
      }
	  argv[e]=NULL;
    }
	else c++;
  }
  
  if(!c) {
    printf("CSS2SRV v2.2 - Convert Files from Compass to Walls 2.0 B8 Format\n"
		   "\n"
           "USAGE: css2srv [-s] [-u] [-c] [-p <wpj_file>] <filespec[.DAT]> [...]\n"
		   "\n"
           "Creates Walls .SRV files from Compass data files matching one or more\n"
		   "file specification arguments (default extension .DAT). For each file read\n"
		   "as input, a new file with the the same base name but with extension .SRV\n"
		   "will be created in the same directory as the first input file, possibly\n"
		   "overwriting a preexisting SRV file.\n"
		   "\nNOTE: Input files with base names longer than 8 characters will produce\n"
		   "SRV files with 8-char names containing the same 5-char prefix.\n"
		   "\nExamples:\n"
		   "\n"
		   "css2srv -u fulford fulsurf\n"
		   "css2srv h:\\wcompass\\*.dat\n" 
		   "css2srv -s -p \\walls\\data\\Lechuguilla d:\\caves\\lech95\\*.dat\n"
		   "\n"
		   "OPTIONS: Use \"-p <wpj_file>\" to also create a single-level project script\n"
		   "(extension .WPJ) that can be immediately opened and manipulated in Walls.\n"
		   "In this case, <wpj_file> will determine the location of all created files.\n"
		   "By default, each named survey within a file defines a separate segment,\n"
		   "allowing selection of individual surveys when compiling statistics and\n"
		   "assigning map attributes. Option -s will turn off this feature. Use -c\n"
		   "if colons in names should be preserved as prefix separators. Finally, use\n"
		   "-u to include, for appearances sake, unit suffixes on all measurements.\n"
		   "\n"
		   "If you need help or have suggestions, please contact davidmck@austin.rr.com.\n"); 
    exit(argc==1);
  }

  filecount=segtotal=vectotal=0;
  fppr=NULL;
  /*cfg_argv() will recognize no comment delimeters, quoted strings, etc.*/
  cfg_commchr=cfg_quotes=0;
  cfg_equals=FALSE;

  printf("\nCSS2SRV v2.0 -- Processing...\n\n");

  while(--argc) {
      if(!(p=*++argv)) continue;

	  p=trx_Stpext(strcpy(frpath,p));
	  if(!*p) strcpy(p,".dat");

	  if((hFile=_findfirst(frpath,&ft))==-1L) continue;

	  frname=trx_Stpnam(frpath);
  
	  while(TRUE) {

		 if(ft.attrib&(_A_HIDDEN|_A_SUBDIR|_A_SYSTEM)) goto _next;

		 _strupr(strcpy(frname,ft.name));
		 if(!strcmp(trx_Stpext(frname),".SRV")) goto _next;

		 /*Get name for title line and messages--*/
		 strcpy(datname,frname);

		 if((fp=fopen(frpath,"r"))==NULL) {
		   printf("Cannot open %s. File skipped.\n",frpath);
		   goto _next;
		 }

		 p=trx_Stpext(frname);
		 *p=0;
		 if(strlen(frname)>8) {
			 if(nameinc>99) {
			   printf("Limit on generated 8-char names reached: %s. File skipped.\n",frpath);
			   goto _next;
			 }
			 p=frname+8;
			 sprintf(p-3,"@%02u",nameinc++);
		 }
         strcpy(p,".SRV");

		 fpto=NULL;
		 linecount=segcount=veccount=0;

		 if(!(e=copy_hdr())) {
			 fclose(fp);
			 goto _next;
		 }

		 filecount++;
		 *lastfrom=0;
		 segcount++;

		 do {
		   if(*ln==0x0C) {
			 if((e=copy_hdr())<=0) break;
			 *lastfrom=0;
			 segcount++;
		   }
		   
		   cfg_GetArgv(ln,CFG_PARSE_ALL);
		   if(!cfg_argc) continue; /*Ignore blank data lines*/
       
		   bFixed=bFlags=bComm=bWarn=bExclude=FALSE;
		   e=bRedundant?11:9;
		   if(cfg_argc<e) {
			   warn_format("Short data line skipped");
			   warn_format(0);
			   continue;
		   }

		   if(e<cfg_argc) {
			 if(*(p=cfg_argv[e])=='#' && *++p=='|') {
			   if(strchr(p,'C')) bFixed=TRUE;
			   if(strchr(p,'L')) bFlags=1;
			   if(strchr(p,'P')) bFlags|=2;
       		   /*Check if shot is excluded. If so make it a comment --*/
			   if(strchr(p,'X')) bExclude=TRUE;
			   /*Show only exclusion flag as comment --*/
			   else e++;
			 }
			 if(e<cfg_argc) bComm=out_comment(e);
		   }
       
		   if(bExclude) outf(";");
		   else veccount++;
		   outf(get_namstr(0));
		   outtab();
		   outf(get_namstr(1));

		   if(!bLrudOnly) {
			   for(e=0;e<3;e++) {
				 outtab();
				 out_item(e);
			   }
			   if(bFixed) outf("\t(0)");
		   }

		   out_dimensions();
       
		   if(bFlags) out_flagstr(bFlags);
       
		   if(bComm) out_comment(0);
		   outln();
		   if(bWarn) warn_format(0);
		}
		while ((e=getline())>=0 && (linecount++,slinecount++));

		fclose(fp);
		if(fpto) fclose(fpto);
   
		segtotal+=segcount;
		vectotal+=veccount;
		if(outlines) printf("   Lines written:%7u  Surveys:%5u  Vectors:%7u\n",
			outlines,segcount,veccount);

	_next:	
		if(_findnext(hFile,&ft)) break;

	  } /*true*/

  _findclose(hFile);

  }

  if(!filecount) printf("\nNo data files were found that match your specification.\n");
  else printf("\nSRV files created: %d  Surveys: %d  Vectors: %d\n",filecount,segtotal,vectotal);
  if(nameinc) printf("\nNOTE: %u SRV file(s) were renamed to have 8-char base names.\n",nameinc);
  if(fppr) {
	  fprintf(fppr,".ENDBOOK\n");
	  fclose(fppr);
	  strcpy(prname,prjname);
	  printf("\nCreated Walls project: %s.wpj.\n",prpath);
  }
  return 0;
}

