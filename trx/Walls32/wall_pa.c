#include "wall-srv.h"

/*NOTE: VC++ 5.0 SP1 generates bad floating point code with
 the default optimization (Ot) turned on for this routine.
 Using Os (small size) is a temporary workaround.*/

#undef _TESTING

#ifdef _TESTING
#pragma optimize("t",on)
int PASCAL parse_angle_arg(int typ,int i)
{
  char *p,*pbak;
  double bakval,delta;
  
  p="0";

  fTemp=1.0;
  if(pbak=strchr(p,'/')) {
      bakval=fTemp;
  }

  if(typ==TYP_V) {
    if(pbak) delta=0.0;
  }
  else {
	if(!pbak) return 0;
	delta=0.0;
  }

  if(pbak) {
    if(fabs(bakval-fTemp)>delta) return 0;
  }
  
  return 0;
}

#else

#pragma optimize("s",on)
int PASCAL parse_angle_arg(int typ,int i)
{
  char *p,*pbak;
  double bakval,delta;
  
  //Calls get_fTempAngle() while handling possible backsight --
  
  if(i>=cfg_argc || *(p=cfg_argv[i])==SRV_CHAR_VAR ||
    *p==SRV_CHAR_LRUD) return SRV_ERR_NUMARGS;
   
  if(pbak=strchr(p,'/')) {
      *pbak++=0;
	  if(!*pbak) return SRV_ERR_SLASH;
    
  	  if((i=get_fTempAngle(pbak,typ==TYP_A))<0) {
  	    if(i!=-SRV_ERR_BLANK || !*p) return -i;
  	    pbak=0;
  	  }
  	  else if(typ==TYP_V) {
        if(!i) fTemp *= u->unitsVB;
        //Apply correction if shot not vertical --
        if(fabs(fabs(fTemp)-PI/2)>ERR_FLT) fTemp+=u->incVB;
        if(fabs(fTemp)>(PI/2.0+ERR_FLT)) return SRV_ERR_VERTICAL;
        if(u->revVB) bakval=fTemp;
        else bakval=-fTemp;
      }
      else {
        if(!i) fTemp *= u->unitsAB;
        if(fTemp>(PI+PI+ERR_FLT) || fTemp<-ERR_FLT) return SRV_ERR_AZIMUTH;
        //Apply correction --
        if((fTemp+=u->incAB)>PI+PI) fTemp-=PI+PI;
        //Reverse shot if required --
        if(!u->revAB && (fTemp+=PI)>PI+PI) fTemp-=PI+PI;
        bakval=fTemp;
      }
  }
  
  if(!*p) {
    if(!pbak) fTemp=0.0;
    else fTemp=bakval;
    return 0;
  }
     
  if((i=get_fTempAngle(p,typ==TYP_A))<0) {
    if(i==-SRV_ERR_BLANK) {
		if(pbak) {
		  fTemp=bakval;
		  return 0;
		}
	}
    return -i;
  }
  
  if(typ==TYP_V) {
    if(!i) fTemp *= u->unitsV;
    if(fabs(fabs(fTemp)-PI/2)>ERR_FLT) fTemp+=u->incV;
    if(fabs(fTemp)>(PI/2+ERR_FLT)) return SRV_ERR_VERTICAL;
    if(pbak) delta=u->errVB;
  }
  else {
    if(!i) fTemp *= u->unitsA;
    if(fTemp>(PI+PI+ERR_FLT) || fTemp<-ERR_FLT) return SRV_ERR_AZIMUTH;
    if((fTemp+=u->incA)>PI+PI) fTemp-=PI+PI;
	if(!pbak) return 0;
    delta=u->errAB;
    if(bakval-fTemp>PI) fTemp+=PI+PI;
    else if(fTemp-bakval>PI) bakval+=PI+PI;
  }
  
  if(pbak) {
    if(fabs(bakval-fTemp)>delta) {
	    delta=fabs(bakval-fTemp)*(180.0/PI);
	    ErrBackTyp=typ;
		if(!log_error("%s FS/BS diff: %6.1f deg, %s to %s",
			(typ==TYP_V)?"Vt":"Az",delta,cfg_argv[0],cfg_argv[1])) return SRV_ERR_BACKDELTA;
    }
    fTemp=(fTemp+bakval)/2.0;
  }
  
  return 0;
}
#endif
