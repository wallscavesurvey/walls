// wall-wrl.cpp : CSegView::ExportWRL() implementation

#include "stdafx.h"
#include "wall_wrl.h"

#ifdef _DEBUG
#define NET_SIZNAME 8
typedef struct { /* 14 * 4 = 56*/ 
				 char   flag;     /* Flag '.' if this is a revisit to an established point*/
				 BYTE   date[3];
				 WORD  end_pfx;  /* Name prefix (same as in vectyp)*/
                 WORD  seg_id;   /* Vector segment index if vec_id!=0*/
                 long  end_id;   /* Name id (same as in vectyp) */
                 long  str_id;   /* NTS rec # of string (0 if none)*/
                 DWORD vec_id;   /* MoveTo if 0, else NTV rec = (1+vec_id)/2, TO station if vec_id even*/
                 long	end_pt;	  /* index in virtual array of network points*/ 
                 double  xyz[3];   /* Coordinates: East, North, Up*/
				 char	name[NET_SIZNAME];
               } plttyp;
typedef plttyp near *pltptr;
extern plttyp *pPLT;
#endif

#define SIZ_FMT 10 
#define SIZ_RGB 20
#define SIZ_FLT 60
#define MAX_COL 20

static FILE *fp;
static char pathname[_MAX_PATH];
static char argv_buf[_MAX_PATH];
static char fmt_buf[SIZ_FMT];
static char flt_buf[SIZ_FLT];
static char flt3_buf[3*SIZ_FLT];
static double xyz[6];

#define ADD_FLT(p,f,d) (p+strlen(strcpy(p,fltstr(f,d))))

static NPSTR errstr[]={
  "User abort",
  "Cannot open",
  "Error writing",
  "Cannot create",
};

enum {
  WRL_ERR_ABORTED=1,
  WRL_ERR_SEFOPEN,
  WRL_ERR_WRITE,
  WRL_ERR_CREATE,
};

LPCSTR WrlErrMsg(int code)
{
  if(code<=WRL_ERR_CREATE) {
    if(code==WRL_ERR_ABORTED) strcpy(argv_buf,errstr[code-1]);
    else wsprintf(argv_buf,"%s file %s",(LPSTR)errstr[code-1],(LPSTR)pathname);
  }
  return argv_buf;
}

static char *fltstr(double r,int prec)
{
	char *buffer,*buff;
	static int dec,sign;  // must use static, don't assume SS==DS
	int i;
	 
	buff=flt_buf;
	 
	buffer = _fcvt(r,prec,&dec,&sign);
	// buffer = ecvt((double)r,prec,&dec,&sign);
	 
	// copy the negative sign if less than zero
	if (sign) *buff++ = '-';
	 
	// copy the non-fractional part before the decimal place
	if(dec<=0) *buff++='0';
	else for(i=0; i<dec; i++) *buff++ = *buffer++;
	 
	*buff++ = '.';  // copy the decimal point into the string
	 
	// copy the fractional part after the decimal place
	if(*buffer) strcpy(buff,buffer);
	else {
	  *buff++='0';
	  *buff=0;
	}
	return flt_buf;
}

static void CDECL WriteArgv(const char *format,...)
{
  LPSTR marker = (LPSTR)&format + _INTSIZEOF(format);
  if(wvsprintf(argv_buf,format,marker)>0) fputs(argv_buf,fp);
}


static int PASCAL ExportID(int i,int icol)
{
	char *p;
    if(!icol) p="\t\t%d";
    else {
      if(icol>=MAX_COL) {
        p=",\r\n\t\t%d";
        icol=0;
      }
      else p=",%d";
    }
    WriteArgv(p,i);
    return icol+1;
}

static char * PASCAL get_rgb_str(COLORREF rgb)
{
    char *p=flt3_buf;
    
    p=ADD_FLT(p,(double)GetRValue(rgb)/255,3);
    *p++=' ';
    p=ADD_FLT(p,(double)GetGValue(rgb)/255,3);
    *p++=' ';
    ADD_FLT(p,(double)GetBValue(rgb)/255,3);
	return flt3_buf;
}

static void PASCAL ExportVertSide(
	double x0,double y0,double zi,int count)
{
	char *p;
	double z0=0.0;
	for(int i=0;i<count;i++) {
	   p=ADD_FLT(flt3_buf,x0,1);
       *p++=' ';
       p=ADD_FLT(p,y0,1);
       *p++=' ';
       ADD_FLT(p,z0+=zi,1);
	   WriteArgv(",\r\n\t\t%s",(LPSTR)flt3_buf);
	}
}

static void PASCAL ExportSide(
	double x0,double xi,double y0,double yi,int count)
{
	char *p;
	for(int i=0;i<count;i++) {
	   p=ADD_FLT(flt3_buf,x0+=xi,1);
       *p++=' ';
       ADD_FLT(p,y0+=yi,1);
	   WriteArgv(",\r\n\t\t%s 0",(LPSTR)flt3_buf);
	}
}

static void PASCAL ExportLines(
	int i0,int ii,int j0,int ji,int count,BOOL bCont)
{
	for(int i=0;i<=count;i++) {
	   if(bCont) fputs(",-1,\r\n",fp);
	   bCont=TRUE;
	   WriteArgv("\t\t%u,%u",i0,j0);
	   i0+=ii;
	   j0+=ji;
	}
}

static void PASCAL ExportListEnd()
{
   fputs("]\r\n}\r\n",fp);
}

static void PASCAL ExportGrid(UINT iElev,UINT dim)
{ 
    double center[3];
    double size[3];
    double width=0;
    char *p;
    
    for(int i=0;i<3;i++) {
    	size[i]=xyz[i+3]-xyz[i];
    	if(i<2 && size[i]>width) width=size[i];
    	center[i]=(xyz[i+3]+xyz[i])/2;
    }
    if(size[2]<0.3*width) size[2]=0.3*width;
    else if(width<0.3*size[2]) width=0.3*size[2];
    center[2]-=size[2]/2.0;
	
	p=ADD_FLT(flt3_buf,center[0]-width/2.0,1);
	*p++=' ';
	p=ADD_FLT(p,center[1]-width/2.0,1);
	*p++=' ';
	ADD_FLT(p,center[2],1);
	
	WriteArgv("# Grid --\r\nTranslation {\r\n"
			  "\ttranslation %s\r\n"
			  "}\r\n",
			  (LPSTR)flt3_buf);
	
    fputs("Material {\r\n\temissiveColor 0.5 0.5 0.5\r\n\tdiffuseColor 0.5 0.5 0.5\r\n}\r\n",fp);
    fputs("Coordinate3 {\r\n\tpoint [\r\n\t\t0 0 0",fp);
    
    double inc=width/dim;
    ExportSide(0.0,inc,0.0,0.0,dim);
    ExportSide(width,0.0,0.0,inc,dim);
    ExportSide(width,-inc,width,0.0,dim);
    ExportSide(0.0,0.0,width,-inc,dim);
    
    if(iElev) {
      iElev=(UINT)(DWORD)((dim*size[2])/width+0.5);
      if(iElev) {
        ExportVertSide(0.0,0.0,inc,iElev);
        ExportVertSide(width,0.0,inc,iElev);
        ExportVertSide(0,width,inc,iElev);
      }
    }
    
    ExportListEnd();
    fputs("IndexedLineSet {\r\n\tcoordIndex [\r\n",fp);
    ExportLines(0,1,3*dim,-1,dim,FALSE);
    ExportLines(4*dim,-1,dim,1,dim,TRUE);
    if(iElev) {
      //Draw 3 vertical axes --
      ExportLines(4*dim,0,4*dim+iElev,0,1,TRUE);
      ExportLines(dim,0,4*dim+2*iElev,0,1,TRUE);
      ExportLines(3*dim,0,4*dim+3*iElev,0,1,TRUE);
      //Draw lines between axes --
      ExportLines(4*dim+1,1,4*dim+1+iElev,1,iElev-1,TRUE);
      ExportLines(4*dim+1,1,4*dim+1+2*iElev,1,iElev-1,TRUE);
    }
    ExportListEnd();
}    

int WrlExport(LPCSTR lpPathname,LPCSTR title,LPFN_EXPORT_CB wall_GetData)
{
	char *p;
	COLORREF c;
	int index;
	UINT flags,dim;
	bool bInit;
	
    strcpy(pathname,lpPathname);
    if((fp=fopen(pathname,"wb"))==NULL) return WRL_ERR_CREATE;
	
	WriteArgv("#VRML V1.0 ascii\r\n# %s\r\n",title);
		
	if(WALL_BACKCOLOR(&c)) {
	  WriteArgv("DEF BackgroundColor Info {\r\n"
	  			"\tstring \"%s\"\r\n"
	  			"}\r\n",(LPSTR)get_rgb_str(c));
	}
	
	strcpy(fmt_buf," \r\n\t\t%s");
	bInit=false;
	while(WALL_POINT(xyz)) {
		if(!bInit) {
			bInit=true;
			fputs("# Vectors --\r\nCoordinate3 {\r\n\tpoint [",fp);
		}
		p=ADD_FLT(flt3_buf,xyz[0],2);
		*p++=' ';
		p=ADD_FLT(p,xyz[1],2);
		*p++=' ';
		ADD_FLT(p,xyz[2],2);
		WriteArgv(fmt_buf,(LPSTR)flt3_buf);
        *fmt_buf=',';
    }
		
	if(bInit) {
		//write out vectors --
		fputs("]\r\n}\r\n",fp);
		while(WALL_COLOR(&c)) {
			get_rgb_str(c);
			WriteArgv("Material {\r\n"
					  "\temissiveColor %s\r\n\tdiffuseColor %s\r\n"
					  "}\r\n"
					  "IndexedLineSet {\r\n"
					  "\tcoordIndex [\r\n",flt3_buf,flt3_buf);
			
			int icol=0;
			while(WALL_INDEX(&index)) icol=ExportID(index,icol);
			fputs("]\r\n}\r\n",fp);
		}
	}
	
	WALL_FLAGS(&flags);

#ifdef _DEBUG
		long end_id=-1;
		char nam[10];
		nam[8]=0;
#endif

	if(flags&WRL_F_LRUD) {
		//Write new coordinate node for stations with LRUDs --
		FLTPT wpt;
		WALL_TYP_LRUDPT lp;
		lp.pwpt=&wpt;
		lp.ispt=1;
		bInit=false;
#ifdef _DEBUG
		index=end_id=-1;
#endif
		WALL_LRUDPT(NULL);
		while(WALL_LRUDPT(&lp)) {
			if(!bInit) {
				strcpy(fmt_buf," \r\n\t\t%s");
				fputs("# Rays --\r\nCoordinate3 {\r\n\tpoint [",fp);
				bInit=true;
			}
			p=ADD_FLT(flt3_buf,wpt.xyz[0],2);
			*p++=' ';
			p=ADD_FLT(p,wpt.xyz[1],2);
			*p++=' ';
			ADD_FLT(p,wpt.xyz[2],2);
			WriteArgv(fmt_buf,(LPSTR)flt3_buf);
#ifdef _DEBUG
			index++;
			if(end_id!=pPLT->end_id) {
				end_id=pPLT->end_id;
				memcpy(nam,pPLT->name,8);
				WriteArgv(", # %s %u",nam,index);
				*fmt_buf=' ';
			}
			else
#endif
			*fmt_buf=',';
		}

		if(bInit) {
#ifdef _DEBUG
			index=-1;
			bool bNew=false;
#endif
			WALL_LRUDCOLOR(&c);
			get_rgb_str(c);
			fputs("]\r\n}\r\n",fp);
			WriteArgv("Material {\r\n"
					  "\temissiveColor %s\r\n\tdiffuseColor %s\r\n"
					  "}\r\n"
					  "IndexedLineSet {\r\n"
					  "\tcoordIndex [\r\n",flt3_buf,flt3_buf);
			
			WALL_LRUDPT(NULL);
			lp.pwpt=NULL;
			while(WALL_LRUDPT(&lp)) {
				if(!bInit) {
#ifdef _DEBUG
					if(!bNew)
#endif
					fputs(",-1,\r\n",fp);
				}
				WriteArgv("\t\t%u,%u",lp.ispt,lp.iwpt);
				bInit=false;
#ifdef _DEBUG
				if(bNew=(index!=lp.ispt)) index=lp.ispt;
				if(bNew) {
					memcpy(nam,pPLT->name,8);
					WriteArgv(",-1, # %s\r\n",nam);
				}
#endif
			}
			fputs("]\r\n}\r\n",fp);
		}
	}

	if(flags&WRL_F_LRUDPT) {
		//Write new coordinate node for stations with LRUDs --
#ifdef _DEBUG
		end_id=-1;
#endif
		FLTPT wpt;
		WALL_TYP_LRUDPT lp;
		lp.pwpt=&wpt;
		lp.ispt=0;
		bInit=false;
		WALL_LRUDPT(NULL);
		while(WALL_LRUDPT(&lp)) {
			if(!bInit) {
				strcpy(fmt_buf," \r\n\t\t%s");
				fputs("# Wall points --\r\nCoordinate3 {\r\n\tpoint [",fp);
				bInit=true;
			}
			p=ADD_FLT(flt3_buf,wpt.xyz[0],2);
			*p++=' ';
			p=ADD_FLT(p,wpt.xyz[1],2);
			*p++=' ';
			ADD_FLT(p,wpt.xyz[2],2);
			WriteArgv(fmt_buf,(LPSTR)flt3_buf);
#ifdef _DEBUG
			if(end_id!=pPLT->end_id) {
				end_id=pPLT->end_id;
				memcpy(nam,pPLT->name,8);
				WriteArgv(", # %s",nam);
				*fmt_buf=' ';
			}
			else
#endif
			*fmt_buf=',';
		}

		if(bInit) {
			WALL_LRUDPTCOLOR(&c);
			get_rgb_str(c);
			fputs("]\r\n}\r\n",fp);
			WriteArgv("Material {\r\n"
					  "\temissiveColor %s\r\n\tdiffuseColor %s\r\n"
					  "}\r\n"
					  "PointSet { }\r\n",flt3_buf,flt3_buf);
		}
	}
	
	if(flags&WRL_F_GRID) {
	    WALL_DIMGRID(&dim);
		WALL_RANGE(xyz);
		ExportGrid((flags&WRL_F_ELEVGRID)!=0,dim);
	}
		
	fclose(fp);
 	
 	return 0;
}
