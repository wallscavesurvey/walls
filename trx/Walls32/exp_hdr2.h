
static char * timestr()
{
  time_t t;
  struct tm *ptm;
  static char timestr[20];

  timestr[0]=0;
  time(&t);
  ptm=localtime(&t);
  sprintf(timestr,"%02d:%02d %04d-%02d-%02d",
      ptm->tm_hour,ptm->tm_min,ptm->tm_year+1900,
      ptm->tm_mon+1,ptm->tm_mday);
  return timestr;
}

static int get_SEF_name(char *buf,int i)
{
	char nam[12];
	char *p;

	memcpy(nam,pVec->nam[i],SRV_SIZNAME);
	p=nam+SRV_SIZNAME;
	while(p>nam && isspace((BYTE)p[-1])) p--;
	*p=0;
	if(vec_pfx[i][0]) {
		strcat(strcpy(buf,vec_pfx[i]),":");
		if(strlen(buf)+strlen(nam)>12) {
			return EXP_ERR_LENGTH;
		}
		strcat(buf,nam);
	}
	else strcpy(buf,nam);
	return 0;
}

static int AddCP(void)
{
	CPTYP *pcp;
	if(numCP>=maxCP) {
		if(!numCP) {
			if(u->bFeet || (pEXP->flags&EXP_FORCEFEET)) {
				cp_units="feet";
				cp_scale=1.0/U_FEET;
			}
			else {
				cp_units="meters";
				cp_scale=1.0;
			}
		}
		pCP=(CPTYP *)realloc(pCP,(maxCP+=MAXCP_INC)*sizeof(CPTYP));
		if(!pCP) {
			numCP=maxCP=0;
			return SRV_ERR_NOMEM;
		}
	}
	pcp=&pCP[numCP++];
	memcpy(pcp->xyz,pVec->xyz,sizeof(pVec->xyz));
	if(get_SEF_name(pcp->nam,1)) {
		return EXP_ERR_LENGTH;
	}
	return 0;
}

static void Out_CT_Header(void)
{
	DWORD dw=*(DWORD *)pVec->date;
	char buf[48];
	strncpy(buf,pEXP->title,41);
	buf[40]=0;
	outSEF("#ctsurvey %s\r\n",buf);
	
	if(u->bFeet || (pEXP->flags&EXP_FORCEFEET)) {
		ct_units="feet";
		ct_scale=1.0/U_FEET;
	}
	else {
		ct_units="meters";
		ct_scale=1.0;
	}

	if(pCC) outSEF_Comments();

	outSEF("#decl %.2f\r\n",ct_decl=u->declination);
	// dw=(y<<9)+(m<<5)+d; //15:4:5
	if(dw&0xFFFFFF)	outSEF("#date %02d/%02d/%d\r\n",(dw>>5)&0xF,dw&0x1F,(dw>>9)&0x7FFF);
	if(*ct_units=='m')
		outSEF("#units %s\r\n#mtunits %s\r\n#atunits %s\r\n",ct_units,ct_units,ct_units);

	if(pEXP->flags&EXP_FIXEDCOLS) {
		outSEF_s("#data from(2,12)to(15,12)dist(28,8)fazi(37,6)finc(44,6)left(51,8)right(60,8)ceil(69,8)floor(78,8)\r\n");
		ct_fmt=" %-13s%-12s%9.2f%7.2f%7.2f";
	}
	else {
		outSEF_s("#data from to dist fazi finc left right ceil floor\r\n");
		ct_fmt=" %s %s %.2f %.2f %.2f";
	}
}

static int Out_CT_Vector(void)
{
	int e;
	char nam1[32],nam2[32];
	if(!(e=get_SEF_name(nam1,0))) e=get_SEF_name(nam2,1);
	if(!e) {
		get_ct_sp(pVec->xyz);
		outSEF(ct_fmt,nam1,nam2,ct_sp.di*ct_scale,ct_sp.az,ct_sp.va);
		if(ct_islrud) {
			if(pEXP->flags&EXP_FIXEDCOLS) {
				for(e=0;e<4;e++) {
					if(ct_lrud[e]==LRUD_BLANK) outSEF_s("        *");
					else outSEF("%9.2f",ct_scale*ct_lrud[e]);
				}
			}
			else {
				for(e=0;e<4;e++) {
					if(ct_lrud[e]==LRUD_BLANK) outSEF_s(" *");
					else outSEF(" %.2f",ct_scale*ct_lrud[e]);
				}
			}
			e=0;
		}
		outSEF_ln();
		ctvectors++;
	}
	return e;
}

static int Out_CP_Vectors(void)
{
	int e;
	CPTYP *p;
	char *cp_fmt;
	char buf[48];
	
	if(ctvectors) outSEF("#cpoint %s - Fixed Points\r\n",pEXP->name);
	else {
		strncpy(buf,pEXP->title,41);
		buf[40]=0;
		outSEF("#cpoint %s\r\n",buf);
	}

	if(pCC) outSEF_Comments();

	if(*cp_units=='m')
		outSEF("#units %s\r\n#elevunits %s\r\n",cp_units,cp_units);

	if(pEXP->flags&EXP_FIXEDCOLS) {
		outSEF_s("#data station(2,12)east(15,16)north(32,16)elev(49,16)\r\n");
		cp_fmt=" %-12s%17.2f%17.2f%17.2f\r\n";
	}
	else {
		outSEF_s("#data station east north elev\r\n");
		cp_fmt=" %s %.2f %.2f %.2f\r\n";
	}

	for(e=0;e<numCP;e++) {
		p=&pCP[e];
		outSEF(cp_fmt,p->nam,cp_scale*p->xyz[0],cp_scale*p->xyz[1],cp_scale*p->xyz[2]);
	}
	outSEF_s("#endcpoint\r\n");
	return 0;
}


DLLExportC int PASCAL Export(EXPTYP *pexp)
{
	int e=0;

	pEXP=pexp;
	pexp->lineno=0;

	switch(pEXP->fcn) {
		case EXP_OPEN:
			if(pEXP->flags!=EXP_VERSION) e=SRV_ERR_VERSION;
			else {
				pEXP->numvectors=0;
				pEXP->fTotalLength=0.0;
				srv_CB=(LPSTR_CB)pEXP->title;
				//Initialize SEF file --
				if(!(fpout=fopen(strcpy(outpath,pEXP->path),"wb"))) e=EXP_ERR_INITSEF;
				else {
					exp_InitDefaults(); //returns 0
					outSEF(";SEF Created by Walls v2.0 - %s\r\n",timestr());
				}
			}
			break;

		case EXP_SETOPTIONS:
			if(!pEXP->title) exp_InitDefaults();
			else if((e=exp_SetOptions(pEXP->title))) {
				exp_Close();
			}
			break;

		case EXP_SRVOPEN:
			//exp_Open() should be called before exp_SetOptions()!
			e=exp_Open();
			break;

		case EXP_SURVEY:
			ct_islrud=FALSE;

			while(!(e=srv_Next())) {
				if(pVec->flag&NET_VEC_FIXED) {
					if(e=AddCP()) break;
				}
				else {
					if(!ctvectors) Out_CT_Header();
					//Output SEF vector --
					if(e=Out_CT_Vector()) break;

					pEXP->fTotalLength+=pVec->len_ttl;
				}
				pEXP->numvectors++;
				ct_islrud=FALSE;
			}
			if(e==CFG_ERR_EOF) e=0;

			if(!e) {
				if(ctvectors) outSEF_s("#endctsurvey\r\n");
				if(numCP) e=Out_CP_Vectors();
			}

			exp_Close();
			break;

		case EXP_CLOSE:
			//Close SEF file -- delete if pEXP->charno!=0 --
			if(fpout) {
				if(fclose(fpout)) e=EXP_ERR_CLOSE;
				if(pEXP->charno) _unlink(outpath);
				fpout=NULL;
			}
			break;

		case EXP_OPENDIR:
			outSEF("#dir %s\r\n",pEXP->title);
			break;
		case EXP_CLOSEDIR:
			outSEF_s("#up\r\n");
			break;
	}

	if(e>0) {
		pEXP->title=errmsg[e-1];
		pEXP->lineno=pVec->lineno;
		pEXP->charno=pVec->charno;
	}
	return e;
}
