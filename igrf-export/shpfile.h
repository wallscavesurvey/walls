#ifndef __SHPFILE_H
#define __SHPFILE_H

#ifndef __DBF_FILE_H
#include <dbf_file.h>
#endif


#define SHP_ERR_WRITE -1
#define SHP_ERR_CREATE -2


#ifdef _USE_LLOYD
enum {F_SITE_ID=0,
	  F_EASTING=3,
	  F_NORTHING=6,
	  F_ELEV=7,
	  F_UTM_ZONE=8,
	  F_NAME=9,
	  F_DEPTH=10,
	  F_LENGTH=11,
	  F_LOCATION=13,
	  F_STATE=14,
	  F_TOPO=15,
	  F_DESC_ACCURACY=16,
	  F_DESC_ENT=17,
	  F_DESC_GEN=18,
	  F_DESC_PITCHES=19,
	  F_DESC_STATUS=20,
	  F_DESC_LEADS=21,
	  F_DESC_GEAR=22,
	  F_DESC_COMMENTS=23,
	  F_DESC_EXP_DATE=24,
	  F_DESC_EXP_BY=25,
	  F_DESC_REFERENCES=26
};

#define SHP_LEN_SITE_ID 10
#define SHP_LEN_NAME 60
#define SHP_LEN_ALT_NAMES 60
#define SHP_LEN_MUNICIPIO 35
#define SHP_LEN_STATE 35
#define SHP_LEN_ELEV 5
#define SHP_LEN_TOPO 6
#define SHP_LEN_LENGTH 6
#define SHP_LEN_DEPTH 4
#define SHP_LEN_LOCATION 32
#define SHP_LEN_COMMENTS 10

#define SHP_LEN_MEMO 10

struct SHP_DATA {
	double fEasting;
	double fNorthing;
	long nLength;
	long nDepth;
	long nElev;
	char Name[SHP_LEN_NAME+1];
	char State[SHP_LEN_STATE+1];
	char Topo[SHP_LEN_TOPO+1];
	char Location[SHP_LEN_LOCATION+1];
	char Alt_Names[SHP_LEN_ALT_NAMES+1];
	char Comments[SHP_LEN_COMMENTS+1];
};
#endif

#ifdef _USE_GUA
enum {F_ID=0,
	  F_NAME=1,
	  F_ALT_NAMES=2,
	  F_TYPE=3,
	  F_EXP_STATUS=4,
	  F_UTM_ZONE=5,
	  F_EASTING=6,
	  F_NORTHING=7,
	  F_DATUM=8,
	  F_ACCURACY=9,
	  F_ELEV=10,
	  F_QUAD=11,
	  F_LOCATION=12,
	  F_DEPARTMENT=13,
	  F_DESC=14,
	  F_MAP_YEAR=15,
	  F_MAP_STATUS=16,
	  F_LENGTH=17,
	  F_DEPTH=18,
	  F_MAP_LINKS=19,
	  F_DISCOVERY=20,
	  F_MAPPING=21,
	  F_PROJECT=22,
	  F_OWNER=23,
	  F_GEAR=24,
	  F_HISTORY=25,
	  F_YEAR_FOUND=26,
	  F_REFERENCES=27
};

#define SHP_LEN_ID 7
#define SHP_LEN_NAME 40
#define SHP_LEN_ALT_NAMES 40
#define SHP_LEN_TYPE 12
#define SHP_LEN_EXP_STATUS 14
#define SHP_LEN_ZONE 3
#define SHP_LEN_ACCURACY 6
#define SHP_LEN_ELEV 6
#define SHP_LEN_QUAD 25
#define SHP_LEN_LOCATION 30
#define SHP_LEN_DEPARTMENT 15
#define SHP_LEN_DESC 120
#define SHP_LEN_MAP_YEAR 4
#define SHP_LEN_MAP_STATUS 4
#define SHP_LEN_LENGTH 8
#define SHP_LEN_DEPTH 8
#define SHP_LEN_MAP_LINKS 30
#define SHP_LEN_DISCOVERY 30
#define SHP_LEN_MAPPING 30
#define SHP_LEN_PROJECT 30
#define SHP_LEN_OWNER 30
#define SHP_LEN_GEAR 30
#define SHP_LEN_HISTORY 10
#define SHP_LEN_YEAR_FOUND 4
#define SHP_LEN_REFERENCES 80


#define SHP_LEN_MEMO 10

struct SHP_DATA {
	double fLat;
	double fLon;
	char ID[SHP_LEN_ID+1];
	char Name[SHP_LEN_NAME+1];
	char Alt_Names[SHP_LEN_ALT_NAMES+1];
	char Type[SHP_LEN_TYPE+1];
	char Exp_Status[SHP_LEN_EXP_STATUS+1];
	char Accuracy[SHP_LEN_ACCURACY+1];
	char Elev[SHP_LEN_ELEV+1];
	char Quad[SHP_LEN_QUAD+1];
	char Location[SHP_LEN_LOCATION+1];
	char Department[SHP_LEN_DEPARTMENT+1];
	char Desc[SHP_LEN_DESC+1];
	char Map_Year[SHP_LEN_MAP_YEAR+1];
	char Map_Status[SHP_LEN_MAP_STATUS+1];
	char Length[SHP_LEN_LENGTH+1];
	char Depth[SHP_LEN_DEPTH+1];
	char Map_Links[SHP_LEN_MAP_LINKS+1];
	char Discovery[SHP_LEN_DISCOVERY+1];
	char Mapping[SHP_LEN_MAPPING+1];
	char Project[SHP_LEN_PROJECT+1];
	char Owner[SHP_LEN_OWNER+1];
	char Gear[SHP_LEN_GEAR+1];
	char History[SHP_LEN_HISTORY+1];
	char Year_Found[SHP_LEN_YEAR_FOUND+1];
	char References[SHP_LEN_REFERENCES+1];
};
#endif

#ifdef _USE_RUSSELL
enum {F_TSSID=0,
	  F_EAST=2,
	  F_NORTH=3,
	  F_DATUM=4,
	  F_NAME=5,
	  F_TYPE=6,
	  F_LENGTH=7,
	  F_DEPTH=8,
	  F_VOLUME=9,
	  F_ELEV=10,
	  F_QUAD=11,
	  F_AREA=12,
	  F_ENTRANCE=13,
	  F_OWNER=14,
	  F_SYNOPSIS=15
};

#define SHP_LEN_NAME 40
#define SHP_LEN_ALT 60
#define SHP_LEN_TYPE 12
#define SHP_LEN_TSSID 7
#define SHP_LEN_LENGTH 5
#define SHP_LEN_DEPTH 3
#define SHP_LEN_ELEV 4
#define SHP_LEN_VOLUME 6
#define SHP_LEN_AREA 8
#define SHP_LEN_ENTRANCE 10
#define SHP_LEN_OWNER 8
#define SHP_LEN_SYNOPSIS 80
#define SHP_LEN_QUAD 25
#define SHP_LEN_MEMO 10

struct SHP_DATA {
	double fEasting;
	double fNorthing;
	long nElev;
	long nLength;
	long nDepth;
	long nVolume;
	char Name[SHP_LEN_NAME+1];
	char Alt_Names[SHP_LEN_ALT+1];
	char Type[SHP_LEN_TYPE+1];
	char TssID[SHP_LEN_TSSID+1];
	char Area[SHP_LEN_AREA+1];
	char Entrance[SHP_LEN_ENTRANCE+1];
	char Owner[SHP_LEN_OWNER+1];
	char Synopsis[SHP_LEN_SYNOPSIS+1];
	char Report[SHP_LEN_MEMO+1];
	char Maps[SHP_LEN_MEMO+1];
	char Photos[SHP_LEN_MEMO+1];
	char TexBio[SHP_LEN_MEMO+1];
	char Quad[SHP_LEN_QUAD+1];
};
#endif

#ifdef _USE_GOA
enum {
	  F_KDE_NO=0,
	  F_FAUNAL_SRV=1,	
	  F_TXSSID=2,
	  F_NAME=3,
	  F_TYPE=4,
	  F_AQUIFER=5,
	  F_MEMBER=6,
	  F_CATCH_AREA=7,
	  F_STATE83_X=8,
	  F_STATE83_Y=9,
	  F_FEAT_SRV=10,
	  F_CATCH_SRV=11,
	  F_KDB_STATUS=12,
	  F_ORIG_62=13,
	  F_LAT=14,
	  F_LON=15
};

#define SHP_LEN_KDE_NO 4
#define SHP_LEN_FAUNAL_SRV 1	
#define SHP_LEN_TXSSID 4
#define SHP_LEN_NAME 40
#define SHP_LEN_TYPE 16
#define SHP_LEN_AQUIFER 11
#define SHP_LEN_MEMBER 17
#define SHP_LEN_CATCH_AREA 12
#define SHP_LEN_STATE83_X 10
#define SHP_LEN_STATE83_Y 10
#define SHP_LEN_FEAT_SRV 27
#define SHP_LEN_CATCH_SRV 27
#define SHP_LEN_KDB_STATUS 12
#define SHP_LEN_ORIG_62 1

struct SHP_DATA {
	double fEasting;
	double fNorthing;
	long kde_no;
	char faunal_srv[SHP_LEN_FAUNAL_SRV+1];	
	char txssid[SHP_LEN_TXSSID+1];
	char name[SHP_LEN_NAME+1];
	char type[SHP_LEN_TYPE+1];
	char aquifer[SHP_LEN_AQUIFER+1];
	char member[SHP_LEN_MEMBER+1];
	double catch_area;
	double state83_x;
	double state83_y;
	char feat_srv[SHP_LEN_FEAT_SRV+1];
	char catch_srv[SHP_LEN_CATCH_SRV+1];
	char kdb_status[SHP_LEN_KDB_STATUS+1];
	char orig_62[SHP_LEN_ORIG_62+1];
};
#endif

#ifdef _USE_WHE
enum {
	  F_NAME=0,
	  F_TYPE=1,	
	  F_EASTING=2,
	  F_NORTHING=3,
	  F_EPE=4,
	  F_EXC_RECOM=5,
	  F_EURYCEA=6,
	  F_GEO_EVAL=7,
	  F_COMMENT=8
};

#define SHP_LEN_NAME 40
#define SHP_LEN_TYPE 12	
#define SHP_LEN_EPE 3
#define SHP_LEN_EXC_RECOM 34
#define SHP_LEN_EURYCEA 1
#define SHP_LEN_GEO_EVAL 1
#define SHP_LEN_COMMENT 50

struct SHP_DATA {
	double fEasting;
	double fNorthing;
	long epe;
	char name[SHP_LEN_NAME+1];
	char type[SHP_LEN_TYPE+1];
	char exc_recom[SHP_LEN_EXC_RECOM+1];
	char eurycea[SHP_LEN_EURYCEA+1];
	char geo_eval[SHP_LEN_GEO_EVAL+1];
	char comment[SHP_LEN_COMMENT+1];
};
#endif

#ifdef _USE_BIO
#define NUM_SPECIES 6

enum {
	  F_NAME=0,
	  F_LAT=1,	
	  F_LON=2,
	  F_SPECIES=9
};

#define SHP_LEN_NAME 40
#define SHP_LEN_DATE 10	

struct SHP_DATA {
	double fEasting;
	double fNorthing;
	char name[SHP_LEN_NAME+1];
	char date[SHP_LEN_DATE+1];
};
extern char shp_pathName[NUM_SPECIES][_MAX_PATH];
extern char *shp_pBaseExt[NUM_SPECIES];

char *GetShpPathName(int i,char *ext);
int CreateFiles(CString *lpPathname);
int CloseFiles();
void DeleteFiles();
int WriteShpRec(int i,SHP_DATA *pData);
UINT dbt_AppendRec(LPCSTR pData);

#else
extern char shp_pathName[_MAX_PATH];
extern char *shp_pBaseExt;

char *GetShpPathName(char *ext);
int CreateFiles(LPCSTR lpPathname);
int CloseFiles();
void DeleteFiles();
int WriteShpRec(SHP_DATA *pData);
UINT dbt_AppendRec(LPCSTR pData);

#endif

#endif
