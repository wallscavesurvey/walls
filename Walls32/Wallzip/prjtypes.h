#if !defined(PRJFILE_H)
#define PRJFILE_H

static LPSTR prjtypes[]={
   "BOOK",
   "ENDBOOK",
   "NAME",
   "OPTIONS",
   "PATH",
   "REF",
   "STATUS",
   "SURVEY"
};

enum {PRJ_CFG_BOOK=1,
	  PRJ_CFG_ENDBOOK,
	  PRJ_CFG_NAME,
      PRJ_CFG_OPTIONS,
	  PRJ_CFG_PATH,
	  PRJ_CFG_REF,
	  PRJ_CFG_STATUS,
      PRJ_CFG_SURVEY
};

#define PRJ_CFG_NUMTYPES (sizeof(prjtypes)/sizeof(LPSTR))
#endif