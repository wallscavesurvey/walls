#include <windows.h>
#include "wnetlib.h"
#include <trx_str.h>
#include <dos_io.h>

static DBF_FLDDEF nts_fld[] = { /* 132 bytes */
	{"_B_FLAG"   ,'C',1,0},
	{"_W_SYS_ID" ,'C',2,0},
	{"_L_STR_ID" ,'C',4,0},
	{"_L_FR_ID"  ,'C',4,0},
	{"_L_TO_ID"  ,'C',4,0},
	{"_D_EAST"   ,'C',8,0},
	{"_D_NORTH"  ,'C',8,0},
	{"_D_UP"     ,'C',8,0},
	{"_D_VARH"   ,'C',8,0},
	{"_D_VARV"   ,'C',8,0},
	{"_D_E_EAST" ,'C',8,0},
	{"_D_E_NORTH",'C',8,0},
	{"_D_E_UP"   ,'C',8,0},
	{"_D_E_VARH" ,'C',8,0},
	{"_D_E_VARV" ,'C',8,0},
	{"_D_S_SSH"  ,'C',8,0},
	{"_D_S_SSV"  ,'C',8,0},
	{"_L_VEC_CNT",'C',4,0},
	{"_W_S_NIH"  ,'C',2,0},
	{"_W_S_NIV"  ,'C',2,0},
	{"_L_S_NC"   ,'C',4,0},
	{"_L_VEC_ID" ,'C',4,0},
	{"_L_LNK_NXT" ,'C',4,0}
};

#define NTS_NUMFLDS (sizeof(nts_fld)/sizeof(DBF_FLDDEF))

static DBF_FLDDEF ntp_fld[] = { /* 7*8 = 56*/
	{"_T_DATE"	 ,'C',3,0},
	{"_W_END_PFX",'C',2,0},
	{"_W_SEG_ID",'C',2,0},
	{"_L_END_ID",'C',4,0}, /*recno of 1st appearance of point*/
	{"_L_STR_ID",'C',4,0},
	{"_U_VEC_ID",'C',4,0}, /*(+/-)ntv_rec or 0 if moveto*/
	{"_L_END_PT",'C',4,0},
	{"_D_EAST"  ,'C',8,0},
	{"_D_NORTH" ,'C',8,0},
	{"_D_UP"    ,'C',8,0},
	{"NAME"     ,'C',8,0}
};

#define NTP_NUMFLDS (sizeof(ntp_fld)/sizeof(DBF_FLDDEF))

static DBF_FLDDEF ntn_fld[] = { /* 12*8 = 96 bytes */
	{"_B_FLAG"	 ,'C',1,0},
	{"_W_NUM_SYS",'C',2,0},
	{"_U_VECTORS",'C',4,0},
	{"_L_VEC_ID" ,'C',4,0},
	{"_L_STR_ID1",'C',4,0},
	{"_L_STR_ID2",'C',4,0},
	{"_U_PLT_ID1",'C',4,0},
	{"_U_PLT_ID2",'C',4,0},
	{"_L_ENDPTS" ,'C',4,0},
	{"_D_LENGTH" ,'C',8,0},
	{"_D_E_MIN"  ,'C',8,0},
	{"_D_N_MIN"  ,'C',8,0},
	{"_D_U_MIN"  ,'C',8,0},
	{"_D_E_MAX"  ,'C',8,0},
	{"_D_N_MAX"  ,'C',8,0},
	{"_D_U_MAX"  ,'C',8,0},
	{"NAME"      ,'C',8,0}
};

#define NTN_NUMFLDS (sizeof(ntn_fld)/sizeof(DBF_FLDDEF))

static apfcn_i _net_AllocPath(char FAR *path)
{
	int i = trx_Stpext(path) - path;
	if ((net_pPathName = (nptr)dos_AllocNear(i + 5)) == 0) return 1;
	memcpy(net_pPathName, path, i);
	net_pPathExt = net_pPathName + i;
	*net_pPathExt++ = '.';
	return 0;
}

apfcn_i create_dbnet(void)
{
	FIX_EXT(NET_NET_EXT);
	FCHKP(dbf_Create(&net_dbnet, net_pPathName, DBF_ReadWrite, NTN_NUMFLDS, ntn_fld));
	netopen = TRUE;
	net_dbnetp = (netptr)_NPTR(dbf_RecPtr(net_dbnet));
	return 0;
}

apfcn_i open_dbnet(void)
{
	FIX_EXT(NET_NET_EXT);
	FCHKP(dbf_Open(&net_dbnet, net_pPathName, DBF_ReadWrite));
	netopen = TRUE;
	net_dbnetp = (netptr)_NPTR(dbf_RecPtr(net_dbnet));
	return 0;
}

apfcn_i create_dbstr(void)
{
	FIX_EXT(NET_STR_EXT);
	FCHKS(dbf_Create(&net_dbstr, net_pPathName, DBF_ReadWrite, NTS_NUMFLDS, nts_fld));
	stropen = TRUE;
	*(WORD *)dbf_HdrRes2(net_dbstr) = NET_NTS_VERSION;
	net_dbstrp = (strptr)_NPTR(dbf_RecPtr(net_dbstr));
	return 0;
}

apfcn_i open_dbstr(void)
{
	FIX_EXT(NET_STR_EXT);
	FCHKS(dbf_Open(&net_dbstr, net_pPathName, DBF_ReadWrite | DBF_ForceOpen));
	stropen = TRUE;
	net_dbstrp = (strptr)_NPTR(dbf_RecPtr(net_dbstr));
	return 0;
}

apfcn_i create_dbplt(void)
{
	FIX_EXT(NET_PLT_EXT);
	FCHKP(dbf_Create(&net_dbplt, net_pPathName, DBF_ReadWrite, NTP_NUMFLDS, ntp_fld));
	pltopen = TRUE;
	net_dbpltp = (pltptr)_NPTR(dbf_RecPtr(net_dbplt));
	return 0;
}

apfcn_i open_dbplt(void)
{
	FIX_EXT(NET_PLT_EXT);
	FCHKP(dbf_Open(&net_dbplt, net_pPathName, DBF_ReadWrite));
	pltopen = TRUE;
	net_dbpltp = (pltptr)_NPTR(dbf_RecPtr(net_dbplt));
	return 0;
}

apfcn_i open_dbvec(char *ntvpath)
{
	if (_net_AllocPath(ntvpath)) CHK_ABORT(NET_ErrNearHeap);
	FIX_EXT(NET_VEC_EXT);
	FCHKV(dbf_Open(&net_dbvec, net_pPathName, DBF_ReadWrite));
	vecopen = TRUE;
	FCHKV(dbf_AllocCache(net_dbvec, NTV_BUFRECS*dbf_SizRec(net_dbvec), NTV_NUMBUFS, TRUE));
	net_dbvecp = (vecptr)_NPTR(dbf_RecPtr(net_dbvec));
	return 0;
}

/*====================================================================*/
/* Miscellaneous functions used by wnetupd.c and wallneth.c                                          */
/*====================================================================*/

apfcn_v zero_resources(void)
{
	net_string = NULL;
	net_vendp = net_valist = NULL;
	net_node = net_degree = NULL;
	sys = NULL;
	net_pPathName = 0;
	net_errno = 0;
	vecopen = stropen = pltopen = netopen = FALSE;
}

apfcn_i allochuge(HPVOID *p, DWORD len)
{
	return ((*p = dos_AllocHuge(len)) == NULL) ? NET_ErrFarHeap : 0;
}

apfcn_v freehuge(HPVOID *p)
{
	dos_FreeHuge(*p);
	*p = 0;
}

apfcn_v free_resources(void)
{
	/*close any open files or databases --*/
	if (stropen) {
		stropen = FALSE;
		dbf_Close(net_dbstr);
	}
	if (vecopen) {
		vecopen = FALSE;
		dbf_Close(net_dbvec);
	}
	if (pltopen) {
		pltopen = FALSE;
		dbf_Close(net_dbplt);
	}
	if (netopen) {
		netopen = FALSE;
		dbf_Close(net_dbnet);
	}
	if (sys_numlinkfrag) {
		sys_numlinkfrag = 0;
		linkFrag_clear();
	}

	if (net_string) freefar(&net_string);
	if (sys) freefar(&sys);
	if (net_valist) freehuge(&net_valist);
	if (net_degree) freefar(&net_degree);
	if (net_node) freefar(&net_node);
	if (net_vendp) freehuge(&net_vendp);
	if (net_pPathName) {
		dos_FreeNear(net_pPathName);
		net_pPathName = 0;
	}
}

apfcn_i net_chkfcn(int e, int typ, int line)
{
	if (e) {
		if (typ == NET_ErrCallBack) {
			typ = e;
			e = NET_ErrCallBack;
		}
		net_errno = e;
		net_errtyp = typ;
		net_errline = line;
		free_resources();
	}
	return e;
}

/*==================================================================*/
/* Routines to update NTV database with corrected vectors           */
/*==================================================================*/

apfcn_i next_id(int i)
{
	return net_dbvecp->id[i == net_dbvecp->id[0] ? 1 : 0];
}
