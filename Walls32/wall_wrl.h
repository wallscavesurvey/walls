//WALLWRL.DLL functions --

#ifndef __WALL_WRL_H
#define __WALL_WRL_H 

enum e_wrl_flags {WRL_F_COLORS=1,WRL_F_GRID=2,WRL_F_ELEVGRID=4,WRL_F_LRUD=8,WRL_F_LRUDPT=16};

enum e_wrl {WRL_INDEX,WRL_POINT,WRL_COLOR,WRL_BACKCOLOR,WRL_RANGE,WRL_FLAGS,WRL_DIMGRID,WRL_LRUDPT,WRL_LRUDCOLOR,WRL_LRUDPTCOLOR};

typedef int (FAR PASCAL *LPFN_EXPORT_CB)(int typ,LPVOID pData);

int WrlExport(LPCSTR pathname,LPCSTR title,LPFN_EXPORT_CB wall_GetData); 
LPCSTR WrlErrMsg(int code);

struct FLTPT {
	double xyz[3];
};

struct WALL_TYP_LRUDPT {
	//will make two passes -- one for coords, one for 
	FLTPT *pwpt; //On input, points to FLTPT structure
	UINT ispt; //coord index of station
	UINT iwpt; //coord index of station
};

#define WALL_INDEX(addr) wall_GetData(WRL_INDEX,addr)
#define WALL_POINT(addr) wall_GetData(WRL_POINT,addr)
#define WALL_COLOR(addr) wall_GetData(WRL_COLOR,addr)
#define WALL_BACKCOLOR(addr) wall_GetData(WRL_BACKCOLOR,addr)
#define WALL_RANGE(addr) wall_GetData(WRL_RANGE,addr)
#define WALL_FLAGS(addr) wall_GetData(WRL_FLAGS,addr)
#define WALL_DIMGRID(addr) wall_GetData(WRL_DIMGRID,addr)
#define WALL_LRUDPT(addr) wall_GetData(WRL_LRUDPT,addr)
#define WALL_LRUDPTCOLOR(addr) wall_GetData(WRL_LRUDPTCOLOR,addr)
#define WALL_LRUDCOLOR(addr) wall_GetData(WRL_LRUDCOLOR,addr)

#endif
