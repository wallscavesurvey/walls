///////////////////////////////////////////////////////////////////////////////
// NMEAParser.h: 
// Desctiption:	interface for the CNMEAParser class.
//
// Notes:
//		NMEA Messages parsed:
//			GPGGA, GPGSA, GPGSV, GPRMB, GPRMC, GPZDA
///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1998-2002 VGPS
// All rights reserved.
//
// VGPS licenses this source code for use within your application in
// object form. This source code is not to be distributed in any way without
// prior written permission from VGPS.
//
// Visual Source Safe: $Revision: 6 $
///////////////////////////////////////////////////////////////////////////////
#ifndef _NMEAPARSER_H_
#define _NMEAPARSER_H_
//////////////////////////////////////////////////////////////////////

#ifndef _VECTOR_
#include <vector>
#endif

#ifndef _FLTPOINT_H
#include <FltPoint.h>
#endif

struct CGPSPoint : public CPoint
{
	CGPSPoint() : CPoint() {}
	CGPSPoint(const CFltPoint &fp)
	{
		x=(LONG)(fp.x*1000000);
		y=(LONG)(fp.y*1000000);
	}
	operator CFltPoint() {return CFltPoint(x/1000000.0,y/1000000.0);}
};

typedef std::vector<CGPSPoint> VEC_GPSPOINT;
typedef VEC_GPSPOINT::iterator it_gpspt;

struct CGPSFix : public CGPSPoint
{
	CGPSFix() : elev(0),h(0),m(0),s(0),flag(0) {}
	float elev;
	BYTE h,m,s,flag;
};

typedef std::vector<CGPSFix> VEC_GPSFIX;
typedef VEC_GPSFIX::iterator it_gpsfix;

//===========================================================

#define MAXFIELD	25		// maximum field length

enum NP_STATE {
	NP_STATE_SOM =				0,		// Search for start of message
	NP_STATE_CMD,						// Get command
	NP_STATE_DATA,						// Get data
	NP_STATE_CHECKSUM_1,				// Get first checksum character
	NP_STATE_CHECKSUM_2,				// get second checksum character
};

#define NP_MAX_CMD_LEN			8		// maximum command length (NMEA address)
#define NP_MAX_DATA_LEN			256		// maximum data length
#define NP_MAX_CHAN				36		// maximum number of channels
#define NP_WAYPOINT_ID_LEN		32		// waypoint max string len

#if 0
//////////////////////////////////////////////////////////////////////
class CNPSatInfo
{
public:
	WORD	m_wPRN;						//
	WORD	m_wSignalQuality;			//
	BOOL	m_bUsedInSolution;			//
	WORD	m_wAzimuth;					//
	WORD	m_wElevation;				//
};
#endif
//////////////////////////////////////////////////////////////////////

enum {GPS_FLAT=1,GPS_FLON=2,GPS_FPOS=3,GPS_FALT=4,GPS_FTIME=8,GPS_FDATE=16,GPS_FHDOP=32,GPS_FQUAL=64,GPS_FSATV=128,GPS_FSATU=256};
 
struct GPS_POSITION { //size=36 bytes
	GPS_POSITION() :
		bSatsUsed(0),bSatsInView(0),fHdop(0),bQual(0),
		bYear(0),bMonth(0),bDay(0),wFlags(0)
	{}

	CGPSFix gpf;   //=16
	float fHdop;  //+4=20
	WORD wFlags;
	BYTE bQual;
	BYTE bSatsUsed;
	BYTE bSatsInView;
	BYTE bYear;
	BYTE bMonth;
	BYTE bDay;    //+8=28
};

struct CMD_NAME
{
	CMD_NAME(LPCSTR p) {strcpy(name,p);}
	char name[6];
};

typedef std::vector<CMD_NAME> VEC_CMD_NAME;


//////////////////////////////////////////////////////////////////////
class CNMEAParser  
{
private:
	NP_STATE m_nState;					// Current state protocol parser is in
	BYTE m_btChecksum;					// Calculated NMEA sentence checksum
	BYTE m_btReceivedChecksum;			// Received NMEA sentence checksum (if exists)
	WORD m_wIndex;						// Index used for command and data
	BYTE m_pCommand[NP_MAX_CMD_LEN];	// NMEA command
	BYTE m_pData[NP_MAX_DATA_LEN];		// NMEA data
	LONG m_LocalTimeOffsetMin;          // minutes to add to UTC time to get local time

public:
	DWORD m_dwCommandCount;				// number of NMEA commands received (processed or not processed)
	DWORD m_dwProcessedCount;			// number of NMEA commands processed

	GPS_POSITION m_Pos;

	//
	// GPGGA Data
	//
#if 0
	BYTE m_btGGAHour;					//
	BYTE m_btGGAMinute;					//
	BYTE m_btGGASecond;					//
	BYTE m_btGGAGPSQuality;				// 0 = fix not available, 1 = GPS sps mode, 2 = Differential GPS, SPS mode, fix valid, 3 = GPS PPS mode, fix valid
	BYTE m_btGGANumOfSatsInUse;			//
	double m_dGGAHDOP;					//
	int m_nGGAOldVSpeedSeconds;			//
#endif
	DWORD m_dwGGACount;					//
	//
	// GPGSA
	//
#if 0
	BYTE m_btGSAMode;					// M = manual, A = automatic 2D/3D
	BYTE m_btGSAFixMode;				// 1 = fix not available, 2 = 2D, 3 = 3D
	WORD m_wGSASatsInSolution[NP_MAX_CHAN]; // ID of sats in solution
	double m_dGSAPDOP;					//
	double m_dGSAHDOP;					//
	double m_dGSAVDOP;					//
#endif
	DWORD m_dwGSACount;					//

	//
	// GPGSV
	//
#if 0
	BYTE m_btGSVTotalNumOfMsg;			//
	WORD m_wGSVTotalNumSatsInView;		//
	CNPSatInfo m_GSVSatInfo[NP_MAX_CHAN];	//
#endif
	DWORD m_dwGSVCount;					//

	//
	// GPRMB
	//
#if 0
	BYTE m_btRMBDataStatus;				// A = data valid, V = navigation receiver warning
	double m_dRMBCrosstrackError;		// nautical miles
	BYTE m_btRMBDirectionToSteer;		// L/R
	CHAR m_lpszRMBOriginWaypoint[NP_WAYPOINT_ID_LEN]; // Origin Waypoint ID
	CHAR m_lpszRMBDestWaypoint[NP_WAYPOINT_ID_LEN]; // Destination waypoint ID
	double m_dRMBDestLatitude;			// destination waypoint latitude
	double m_dRMBDestLongitude;			// destination waypoint longitude
	double m_dRMBRangeToDest;			// Range to destination nautical mi
	double m_dRMBBearingToDest;			// Bearing to destination, degrees true
	double m_dRMBDestClosingVelocity;	// Destination closing velocity, knots
	BYTE m_btRMBArrivalStatus;			// A = arrival circle entered, V = not entered
#endif
	DWORD m_dwRMBCount;					//
	//
	// GPRMC
	//
#if 0
	BYTE m_btRMCHour;					//
	BYTE m_btRMCMinute;					//
	BYTE m_btRMCSecond;					//
	BYTE m_btRMCDataValid;				// A = Data valid, V = navigation rx warning
	double m_dRMCLatitude;				// current latitude
	double m_dRMCLongitude;				// current longitude
	double m_dRMCGroundSpeed;			// speed over ground, knots
	double m_dRMCCourse;				// course over ground, degrees true
	double m_dRMCMagVar;				// magnetic variation, degrees East(+)/West(-)
	BYTE m_btRMCDay;					//
	BYTE m_btRMCMonth;					//
	WORD m_wRMCYear;					//
#endif
	DWORD m_dwRMCCount;					//

	// GPZDA
	//
#if 0
	BYTE m_btZDAHour;					//
	BYTE m_btZDAMinute;					//
	BYTE m_btZDASecond;					//
	BYTE m_btZDADay;					// 1 - 31
	BYTE m_btZDAMonth;					// 1 - 12
	WORD m_wZDAYear;					//
	BYTE m_btZDALocalZoneHour;			// 0 to +/- 13
	BYTE m_btZDALocalZoneMinute;		// 0 - 59
#endif
	DWORD m_dwZDACount;					//

	//
	// PGRMM
	//
#if 0
	char m_strDatum[MAXFIELD];
#endif

public:
	enum {NUM_PROCESSES=7,MAX_PROCESSES=20};

#if 0
	DWORD m_seq_cmd_seen[MAX_PROCESSES];
	DWORD m_num_cmd_seen;
	VEC_CMD_NAME m_vec_cmd_seen;
#endif
	typedef void (CNMEAParser::* PTR_PROCESS)(BYTE *);

	static LPCSTR m_pCommandName[NUM_PROCESSES+1];
	PTR_PROCESS m_pProcess[NUM_PROCESSES];

	void ProcessGPZDA(BYTE *pData); //empty (date/time)
	void ProcessGPRMC(BYTE *pData); //date
	void ProcessGPRMB(BYTE *pData); //empty
	void ProcessGPGSV(BYTE *pData); //sats in view
	void ProcessGPGSA(BYTE *pData); //empty
	void ProcessGPGGA(BYTE *pData); // lat/lon, alt, time, HDOP, qual
	void ProcessPGRMM(BYTE *pData); //empty

	//BOOL IsSatUsedInSolution(WORD wSatID);
	void Reset();
	static BOOL GetField(BYTE *pData, BYTE *pField, int nFieldNum, int nMaxFieldLen);
	BOOL ProcessCommand(BYTE *pCommand, BYTE *pData);
	void ProcessNMEA(BYTE btData);
	BOOL ParseBuffer(BYTE *pBuff, DWORD dwLen);
	CNMEAParser();
	virtual ~CNMEAParser();
};

//////////////////////////////////////////////////////////////////////
#endif // _NMEAPARSER_H_
