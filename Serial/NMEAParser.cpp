///////////////////////////////////////////////////////////////////////////////
// NMEAParser.cpp: 
// Description:	Implementation of the CNMEAParser class.
///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1998-2002 VGPS
// All rights reserved.
//
// VGPS licenses this source code for use within your application in
// object form. This source code is not to be distributed in any way without
// prior written permission from VGPS.
//
///////////////////////////////////////////////////////////////////////////////
// turns off MFC's hiding of some common and often safely ignored warning messages
#include "StdAfx.h"
#include "NMEAParser.h"
#include <trx_str.h>

LPCSTR CNMEAParser::m_pCommandName[NUM_PROCESSES + 1] = { "GPGGA","GPGSA","GPGSV","GPRMB","GPRMC","GPZDA","PGRMM",NULL };

#if 0
static LPCSTR pCmdTarget;
static VEC_CMD_NAME *pvec_cmd_seen;

static int FAR PASCAL cmd_compare(int i)
{
	return strcmp(pCmdTarget, ((*pvec_cmd_seen)[i]).name);
}
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CNMEAParser::CNMEAParser()
{
	m_nState = NP_STATE_SOM;
	m_dwCommandCount = m_dwProcessedCount = 0;
	m_pProcess[0] = &CNMEAParser::ProcessGPGGA;
	m_pProcess[1] = &CNMEAParser::ProcessGPGSA;
	m_pProcess[2] = &CNMEAParser::ProcessGPGSV;
	m_pProcess[3] = &CNMEAParser::ProcessGPRMB;
	m_pProcess[4] = &CNMEAParser::ProcessGPRMC;
	m_pProcess[5] = &CNMEAParser::ProcessGPZDA;
	m_pProcess[6] = &CNMEAParser::ProcessPGRMM;
	Reset();
}

CNMEAParser::~CNMEAParser()
{
}

///////////////////////////////////////////////////////////////////////////////
// ParseBuffer:	Parse the supplied buffer for NMEA sentence information. Since
//				the parser is a state machine, partial NMEA sentence data may
//				be supplied where the next time this method is called, the
//				rest of the partial NMEA sentence will complete	the sentence.
//
//				NOTE:
//
// Returned:	TRUE all the time....
///////////////////////////////////////////////////////////////////////////////
BOOL CNMEAParser::ParseBuffer(BYTE *pBuff, DWORD dwLen)
{
	for (DWORD i = 0; i < dwLen; i++)
	{
		ProcessNMEA(pBuff[i]);
	}
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// ProcessNMEA: This method is the main state machine which processes individual
//				bytes from the buffer and parses a NMEA sentence. A typical
//				sentence is constructed as:
//
//					$CMD,DDDD,DDDD,....DD*CS<CR><LF>
//
//				Where:
//						'$'			HEX 24 Start of sentence
//						'CMD'		Address/NMEA command
//						',DDDD'		Zero or more data fields
//						'*CS'		Checksum field
//						<CR><LF>	Hex 0d 0A End of sentence
//
//				When a valid sentence is received, this function sends the
//				NMEA command and data to the ProcessCommand method for
//				individual field parsing.
//
//				NOTE:
//						
///////////////////////////////////////////////////////////////////////////////
void CNMEAParser::ProcessNMEA(BYTE btData)
{
	switch (m_nState)
	{
		///////////////////////////////////////////////////////////////////////
		// Search for start of message '$'
	case NP_STATE_SOM:
		if (btData == '$')
		{
			m_btChecksum = 0;			// reset checksum
			m_wIndex = 0;				// reset index
			m_nState = NP_STATE_CMD;
		}
		break;

		///////////////////////////////////////////////////////////////////////
		// Retrieve command (NMEA Address)
	case NP_STATE_CMD:
		if (btData != ',' && btData != '*')
		{
			m_pCommand[m_wIndex++] = btData;
			m_btChecksum ^= btData;

			// Check for command overflow
			if (m_wIndex >= NP_MAX_CMD_LEN)
			{
				m_nState = NP_STATE_SOM;
			}
		}
		else
		{
			m_pCommand[m_wIndex] = '\0';	// terminate command
			m_btChecksum ^= btData;
			m_wIndex = 0;
			m_nState = NP_STATE_DATA;		// goto get data state
		}
		break;

		///////////////////////////////////////////////////////////////////////
		// Store data and check for end of sentence or checksum flag
	case NP_STATE_DATA:
		if (btData == '*') // checksum flag?
		{
			m_pData[m_wIndex] = '\0';
			m_nState = NP_STATE_CHECKSUM_1;
		}
		else // no checksum flag, store data
		{
			//
			// Check for end of sentence with no checksum
			//
			if (btData == '\r')
			{
				m_pData[m_wIndex] = '\0';
				ProcessCommand(m_pCommand, m_pData);
				m_nState = NP_STATE_SOM;
				return;
			}

			//
			// Store data and calculate checksum
			//
			m_btChecksum ^= btData;
			m_pData[m_wIndex] = btData;
			if (++m_wIndex >= NP_MAX_DATA_LEN) // Check for buffer overflow
			{
				m_nState = NP_STATE_SOM;
			}
		}
		break;

		///////////////////////////////////////////////////////////////////////
	case NP_STATE_CHECKSUM_1:
		if ((btData - '0') <= 9)
		{
			m_btReceivedChecksum = (btData - '0') << 4;
		}
		else
		{
			m_btReceivedChecksum = (btData - 'A' + 10) << 4;
		}

		m_nState = NP_STATE_CHECKSUM_2;

		break;

		///////////////////////////////////////////////////////////////////////
	case NP_STATE_CHECKSUM_2:
		if ((btData - '0') <= 9)
		{
			m_btReceivedChecksum |= (btData - '0');
		}
		else
		{
			m_btReceivedChecksum |= (btData - 'A' + 10);
		}

		if (m_btChecksum == m_btReceivedChecksum)
		{
			ProcessCommand(m_pCommand, m_pData);
		}

		m_nState = NP_STATE_SOM;
		break;

		///////////////////////////////////////////////////////////////////////
	default: m_nState = NP_STATE_SOM;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Process NMEA sentence - Use the NMEA address (*pCommand) and call the
// appropriate sentense data prossor.
///////////////////////////////////////////////////////////////////////////////
BOOL CNMEAParser::ProcessCommand(BYTE *pCommand, BYTE *pData)
{
	m_dwCommandCount++;
	for (LPCSTR *ppCommandName = m_pCommandName; *ppCommandName; ppCommandName++) {
		if (!strcmp((LPCSTR)pCommand, *ppCommandName)) {
			(this->*m_pProcess[ppCommandName - m_pCommandName])(pData);
			m_dwProcessedCount++;
			return TRUE;
		}
	}
#if 0
	if (m_num_cmd_seen < MAX_PROCESSES && strlen((LPCSTR)pCommand) < 6) {
		pvec_cmd_seen = &m_vec_cmd_seen;
		pCmdTarget = (LPCSTR)pCommand;
		trx_Bininit(m_seq_cmd_seen, &m_num_cmd_seen, (TRXFCN_IF)cmd_compare);
		trx_Binsert(m_num_cmd_seen, TRX_DUP_NONE);
		if (!trx_binMatch) {
			m_vec_cmd_seen.push_back(CMD_NAME(pCmdTarget));
			ASSERT(m_num_cmd_seen == m_vec_cmd_seen.size());
		}
	}
#endif
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// Name:		GetField
//
// Description:	This function will get the specified field in a NMEA string.
//
// Entry:		BYTE *pData -		Pointer to NMEA string
//				BYTE *pField -		pointer to returned field
//				int nfieldNum -		Field offset to get
//				int nMaxFieldLen -	Maximum of bytes pFiled can handle
///////////////////////////////////////////////////////////////////////////////
BOOL CNMEAParser::GetField(BYTE *pData, BYTE *pField, int nFieldNum, int nMaxFieldLen)
{
	//
	// Validate params
	//
	if (pData == NULL || pField == NULL || nMaxFieldLen <= 0)
	{
		return FALSE;
	}

	//
	// Go to the beginning of the selected field
	//
	int i = 0;
	int nField = 0;
	while (nField != nFieldNum && pData[i])
	{
		if (pData[i] == ',')
		{
			nField++;
		}

		i++;

		if (pData[i] == NULL)
		{
			pField[0] = '\0';
			return FALSE;
		}
	}

	if (pData[i] == ',' || pData[i] == '*')
	{
		pField[0] = '\0';
		return FALSE;
	}

	//
	// copy field from pData to Field
	//
	int i2 = 0;
	while (pData[i] != ',' && pData[i] != '*' && pData[i])
	{
		pField[i2] = pData[i];
		i2++; i++;

		//
		// check if field is too big to fit on passed parameter. If it is,
		// crop returned field to its max length.
		//
		if (i2 >= nMaxFieldLen)
		{
			i2 = nMaxFieldLen - 1;
			break;
		}
	}
	pField[i2] = '\0';

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Reset: Reset all NMEA data to start-up default values.
///////////////////////////////////////////////////////////////////////////////
void CNMEAParser::Reset()
{
	m_dwCommandCount = m_dwProcessedCount = 0;
	m_Pos = GPS_POSITION();

	{
		TIME_ZONE_INFORMATION tz = { 0 };
		m_LocalTimeOffsetMin = (GetTimeZoneInformation(&tz) == 2) ? -tz.DaylightBias : 0;
		m_LocalTimeOffsetMin -= tz.Bias;
	}

	/* Example: Convert system time to local --
	SYSTEMTIME GmtTime = { 0 };
	GetSystemTime( &GmtTime );
	// Convert UTC time to local --
	SYSTEMTIME LocalTime = { 0 };
	SystemTimeToTzSpecificLocalTime(&tz,&GmtTime,&LocalTime);
	CString csLocalTimeInGmt;
	csLocalTimeInGmt.Format( _T("%ld:%ld:%ld (UTC + %2.1f Hrs"),
								LocalTime.wHour,
								LocalTime.wMinute,
								LocalTime.wSecond,
								float(m_LocalTimeOffsetMin)/60);
	*/


#if 0
	m_vec_cmd_seen.clear();
	m_num_cmd_seen = 0;
#endif
	//
	// GPGGA Data
	//
#if 0
	m_btGGAHour = 0;					//
	m_btGGAMinute = 0;					//
	m_btGGASecond = 0;					//
	m_btGGAGPSQuality = 0;				// 0 = fix not available, 1 = GPS sps mode, 2 = Differential GPS, SPS mode, fix valid, 3 = GPS PPS mode, fix valid
	m_btGGANumOfSatsInUse = 0;			//
	m_dGGAHDOP = 0.0;					//
#endif
	m_dwGGACount = 0;					//

	//
	// GPGSA
	//
#if 0
	m_btGSAMode = 'M';					// M = manual, A = automatic 2D/3D
	m_btGSAFixMode = 1;					// 1 = fix not available, 2 = 2D, 3 = 3D
	for (i = 0; i < NP_MAX_CHAN; i++)
	{
		m_wGSASatsInSolution[i] = 0;	// ID of sats in solution
	}
	m_dGSAPDOP = 0.0;					//
	m_dGSAHDOP = 0.0;					//
	m_dGSAVDOP = 0.0;					//
#endif
	m_dwGSACount = 0;					//

	//
	// GPGSV
	//
#if 0
	m_btGSVTotalNumOfMsg = 0;			//
	m_wGSVTotalNumSatsInView = 0;		//
	for (i = 0; i < NP_MAX_CHAN; i++)
	{
		m_GSVSatInfo[i].m_wAzimuth = 0;
		m_GSVSatInfo[i].m_wElevation = 0;
		m_GSVSatInfo[i].m_wPRN = 0;
		m_GSVSatInfo[i].m_wSignalQuality = 0;
		m_GSVSatInfo[i].m_bUsedInSolution = FALSE;
	}
#endif
	m_dwGSVCount = 0;

	//
	// GPRMB
	//
#if 0
	m_btRMBDataStatus = 'V';			// A = data valid, V = navigation receiver warning
	m_dRMBCrosstrackError = 0.0;		// nautical miles
	m_btRMBDirectionToSteer = '?';		// L/R
	m_lpszRMBOriginWaypoint[0] = '\0';	// Origin Waypoint ID
	m_lpszRMBDestWaypoint[0] = '\0';	// Destination waypoint ID
	m_dRMBDestLatitude = 0.0;			// destination waypoint latitude
	m_dRMBDestLongitude = 0.0;			// destination waypoint longitude
	m_dRMBRangeToDest = 0.0;			// Range to destination nautical mi
	m_dRMBBearingToDest = 0.0;			// Bearing to destination, degrees true
	m_dRMBDestClosingVelocity = 0.0;	// Destination closing velocity, knots
	m_btRMBArrivalStatus = 'V';			// A = arrival circle entered, V = not entered
#endif
	m_dwRMBCount = 0;					//

	//
	// GPRMC
	//
#if 0
	m_btRMCHour = 0;					//
	m_btRMCMinute = 0;					//
	m_btRMCSecond = 0;					//
	m_btRMCDataValid = 'V';				// A = Data valid, V = navigation rx warning
	m_dRMCLatitude = 0.0;				// current latitude
	m_dRMCLongitude = 0.0;				// current longitude
	m_dRMCGroundSpeed = 0.0;			// speed over ground, knots
	m_dRMCCourse = 0.0;					// course over ground, degrees true
	m_dRMCMagVar = 0.0;					// magnetic variation, degrees East(+)/West(-)
	m_btRMCDay = 1;						//
	m_btRMCMonth = 1;					//
	m_wRMCYear = 2000;					//
#endif
	m_dwRMCCount = 0;					//

	//
	// GPZDA
	//
#if 0
	m_btZDAHour = 0;					//
	m_btZDAMinute = 0;					//
	m_btZDASecond = 0;					//
	m_btZDADay = 1;						// 1 - 31
	m_btZDAMonth = 1;					// 1 - 12
	m_wZDAYear = 2000;					//
	m_btZDALocalZoneHour = 0;			// 0 to +/- 13
	m_btZDALocalZoneMinute = 0;			// 0 - 59
#endif
	m_dwZDACount = 0;					//

	//
	//PGRMM
	//
#if 0
	m_strDatum[0] = 0;
#endif
}

#if 0
///////////////////////////////////////////////////////////////////////////////
// Check to see if supplied satellite ID is used in the GPS solution.
// Retruned:	BOOL -	TRUE if satellate ID is used in solution
//						FALSE if not used in solution.
///////////////////////////////////////////////////////////////////////////////
BOOL CNMEAParser::IsSatUsedInSolution(WORD wSatID)
{
	if (wSatID == 0) return FALSE;
	for (int i = 0; i < 12; i++)
	{
		if (wSatID == m_wGSASatsInSolution[i])
		{
			return TRUE;
		}
	}

	return FALSE;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CNMEAParser::ProcessGPGGA(BYTE *pData)
{
	BYTE pField[MAXFIELD];
	CHAR pBuff[10];
	BYTE flags = 0;

	memset(&m_Pos.gpf, 0, sizeof(CGPSFix));

	m_Pos.wFlags &= ~(GPS_FPOS | GPS_FTIME | GPS_FALT | GPS_FHDOP | GPS_FQUAL | GPS_FSATU);

	//
	// Time
	//
	if (GetField(pData, pField, 0, MAXFIELD))
	{
		// Hour
		int m, h;
		pBuff[0] = pField[0];
		pBuff[1] = pField[1];
		pBuff[2] = '\0';
		h = atoi(pBuff);

		// minute
		pBuff[0] = pField[2];
		pBuff[1] = pField[3];
		pBuff[2] = '\0';
		m = atoi(pBuff);

		if (m_LocalTimeOffsetMin) {
			h += m_LocalTimeOffsetMin / 60;
			if (h < 0) h += 24;
			else if (h >= 24) h -= 24;
			m += (m_LocalTimeOffsetMin % 60);
			if (m < 0) m += 60;
		}
		m_Pos.gpf.m = (BYTE)m;
		m_Pos.gpf.h = (BYTE)h;

		// Second
		pBuff[0] = pField[4];
		pBuff[1] = pField[5];
		pBuff[2] = '\0';
		m_Pos.gpf.s = (BYTE)atoi(pBuff);
		flags |= GPS_FTIME;
	}

	//
	// Latitude
	//
	double d = 0.0;

	if (GetField(pData, pField, 1, MAXFIELD))
	{
		d = atof((CHAR *)pField + 2) / 60.0;
		pField[2] = '\0';
		d += atof((CHAR *)pField);
		flags |= GPS_FLAT;
	}
	if (GetField(pData, pField, 2, MAXFIELD))
	{
		if (pField[0] == 'S')
		{
			d = -d;
		}
	}

	m_Pos.gpf.y = (LONG)(d * 1000000);

	//
	// Longitude
	//
	d = 0.0;
	if (GetField(pData, pField, 3, MAXFIELD))
	{
		d = atof((CHAR *)pField + 3) / 60.0;
		pField[3] = '\0';
		d += atof((CHAR *)pField);
		flags |= GPS_FLON;
	}
	if (GetField(pData, pField, 4, MAXFIELD))
	{
		if (pField[0] == 'W')
		{
			d = -d;
		}
	}

	m_Pos.gpf.x = (LONG)(d * 1000000);

	//
	// GPS quality
	//
	if (GetField(pData, pField, 5, MAXFIELD))
	{
		m_Pos.bQual = pField[0] - '0';
		flags |= GPS_FQUAL;
	}

	//
	// Satellites in use
	//
	if (GetField(pData, pField, 6, MAXFIELD))
	{
		pBuff[0] = pField[0];
		pBuff[1] = pField[1];
		pBuff[2] = '\0';
		m_Pos.bSatsUsed = atoi(pBuff);
		flags |= GPS_FSATU;
	}

	//
	// HDOP
	//
	if (GetField(pData, pField, 7, MAXFIELD))
	{
		m_Pos.fHdop = (float)atof((CHAR *)pField);
		flags |= GPS_FHDOP;
	}

	//
	// Altitude
	//
	if (GetField(pData, pField, 8, MAXFIELD))
	{
		m_Pos.gpf.elev = (float)atof((CHAR *)pField);
		flags |= GPS_FALT;
	}

	m_dwGGACount++;
	m_Pos.wFlags |= flags;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CNMEAParser::ProcessGPGSA(BYTE *pData)
{
#if 0
	BYTE pField[MAXFIELD];
	CHAR pBuff[10];

	//
	// Mode
	//
	if (GetField(pData, pField, 0, MAXFIELD))
	{
		m_btGSAMode = pField[0];
	}

	//
	// Fix Mode
	//
	if (GetField(pData, pField, 1, MAXFIELD))
	{
		m_btGSAFixMode = pField[0] - '0';
	}

	//
	// Active satellites
	//
	for (int i = 0; i < 12; i++)
	{
		if (GetField(pData, pField, 2 + i, MAXFIELD))
		{
			pBuff[0] = pField[0];
			pBuff[1] = pField[1];
			pBuff[2] = '\0';
			m_wGSASatsInSolution[i] = atoi(pBuff);
		}
		else
		{
			m_wGSASatsInSolution[i] = 0;
		}
	}

	//
	// PDOP
	//
	if (GetField(pData, pField, 14, MAXFIELD))
	{
		m_dGSAPDOP = atof((CHAR *)pField);
	}
	else
	{
		m_dGSAPDOP = 0.0;
	}

	//
	// HDOP
	//
	if (GetField(pData, pField, 15, MAXFIELD))
	{
		m_dGSAHDOP = atof((CHAR *)pField);
	}
	else
	{
		m_dGSAHDOP = 0.0;
	}

	//
	// VDOP
	//
	if (GetField(pData, pField, 16, MAXFIELD))
	{
		m_dGSAVDOP = atof((CHAR *)pField);
	}
	else
	{
		m_dGSAVDOP = 0.0;
	}
#endif
	m_dwGSACount++;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CNMEAParser::ProcessGPGSV(BYTE *pData)
{
	INT nTotalNumOfMsg, nMsgNum;
	BYTE pField[MAXFIELD];
	m_Pos.wFlags &= ~(GPS_FSATV);

	//
	// Total number of messages
	//
	if (GetField(pData, pField, 0, MAXFIELD))
	{
		nTotalNumOfMsg = atoi((CHAR *)pField);

		//
		// Make sure that the nTotalNumOfMsg is valid. This is used to
		// calculate indexes into an array. I've seen corrept NMEA strings
		// with no checksum set this to large values.
		//
		if (nTotalNumOfMsg > 9 || nTotalNumOfMsg < 0) return;
	}
	if (nTotalNumOfMsg < 1 || nTotalNumOfMsg * 4 >= NP_MAX_CHAN)
	{
		return;
	}

	//
	// message number
	//
	if (GetField(pData, pField, 1, MAXFIELD))
	{
		nMsgNum = atoi((CHAR *)pField);

		//
		// Make sure that the message number is valid. This is used to
		// calculate indexes into an array
		//
		if (nMsgNum > 9 || nMsgNum < 0) return;
	}

	//
	// Total satellites in view
	//
	if (GetField(pData, pField, 2, MAXFIELD))
	{
		m_Pos.bSatsInView = atoi((CHAR *)pField);
		m_Pos.wFlags |= GPS_FSATV;
	}

#if 0
	//
	// Satelite data
	//
	for (int i = 0; i < 4; i++)
	{
		// Satellite ID
		if (GetField(pData, pField, 3 + 4 * i, MAXFIELD))
		{
			m_GSVSatInfo[i + (nMsgNum - 1) * 4].m_wPRN = atoi((CHAR *)pField);
		}
		else
		{
			m_GSVSatInfo[i + (nMsgNum - 1) * 4].m_wPRN = 0;
		}

		// Elevarion
		if (GetField(pData, pField, 4 + 4 * i, MAXFIELD))
		{
			m_GSVSatInfo[i + (nMsgNum - 1) * 4].m_wElevation = atoi((CHAR *)pField);
		}
		else
		{
			m_GSVSatInfo[i + (nMsgNum - 1) * 4].m_wElevation = 0;
		}

		// Azimuth
		if (GetField(pData, pField, 5 + 4 * i, MAXFIELD))
		{
			m_GSVSatInfo[i + (nMsgNum - 1) * 4].m_wAzimuth = atoi((CHAR *)pField);
		}
		else
		{
			m_GSVSatInfo[i + (nMsgNum - 1) * 4].m_wAzimuth = 0;
		}

		// SNR
		if (GetField(pData, pField, 6 + 4 * i, MAXFIELD))
		{
			m_GSVSatInfo[i + (nMsgNum - 1) * 4].m_wSignalQuality = atoi((CHAR *)pField);
		}
		else
		{
			m_GSVSatInfo[i + (nMsgNum - 1) * 4].m_wSignalQuality = 0;
		}

		//
		// Update "used in solution" (m_bUsedInSolution) flag. This is base
		// on the GSA message and is an added convenience for post processing
		//
		m_GSVSatInfo[i + (nMsgNum - 1) * 4].m_bUsedInSolution = IsSatUsedInSolution(m_GSVSatInfo[i + (nMsgNum - 1) * 4].m_wPRN);
	}
#endif
	m_dwGSVCount++;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CNMEAParser::ProcessGPRMB(BYTE *pData)
{
#if 0
	BYTE pField[MAXFIELD];

	//
	// Data status
	//
	if (GetField(pData, pField, 0, MAXFIELD))
	{
		m_btRMBDataStatus = pField[0];
	}
	else
	{
		m_btRMBDataStatus = 'V';
	}

	//
	// Cross track error
	//
	if (GetField(pData, pField, 1, MAXFIELD))
	{
		m_dRMBCrosstrackError = atof((CHAR *)pField);
	}
	else
	{
		m_dRMBCrosstrackError = 0.0;
	}

	//
	// Direction to steer
	//
	if (GetField(pData, pField, 2, MAXFIELD))
	{
		m_btRMBDirectionToSteer = pField[0];
	}
	else
	{
		m_btRMBDirectionToSteer = '?';
	}

	//
	// Orgin waypoint ID
	//
	if (GetField(pData, pField, 3, MAXFIELD))
	{
		strcpy(m_lpszRMBOriginWaypoint, (CHAR *)pField);
	}
	else
	{
		m_lpszRMBOriginWaypoint[0] = '\0';
	}

	//
	// Destination waypoint ID
	//
	if (GetField(pData, pField, 4, MAXFIELD))
	{
		strcpy(m_lpszRMBDestWaypoint, (CHAR *)pField);
	}
	else
	{
		m_lpszRMBDestWaypoint[0] = '\0';
	}

	//
	// Destination latitude
	//
	if (GetField(pData, pField, 5, MAXFIELD))
	{
		m_dRMBDestLatitude = atof((CHAR *)pField + 2) / 60.0;
		pField[2] = '\0';
		m_dRMBDestLatitude += atof((CHAR *)pField);

	}
	if (GetField(pData, pField, 6, MAXFIELD))
	{
		if (pField[0] == 'S')
		{
			m_dRMBDestLatitude = -m_dRMBDestLatitude;
		}
	}

	//
	// Destination Longitude
	//
	if (GetField(pData, pField, 7, MAXFIELD))
	{
		m_dRMBDestLongitude = atof((CHAR *)pField + 3) / 60.0;
		pField[3] = '\0';
		m_dRMBDestLongitude += atof((CHAR *)pField);
	}
	if (GetField(pData, pField, 8, MAXFIELD))
	{
		if (pField[0] == 'W')
		{
			m_dRMBDestLongitude = -m_dRMBDestLongitude;
		}
	}

	//
	// Range to destination nautical mi
	//
	if (GetField(pData, pField, 9, MAXFIELD))
	{
		m_dRMBRangeToDest = atof((CHAR *)pField);
	}
	else
	{
		m_dRMBCrosstrackError = 0.0;
	}

	//
	// Bearing to destination degrees true
	//
	if (GetField(pData, pField, 10, MAXFIELD))
	{
		m_dRMBBearingToDest = atof((CHAR *)pField);
	}
	else
	{
		m_dRMBBearingToDest = 0.0;
	}

	//
	// Closing velocity
	//
	if (GetField(pData, pField, 11, MAXFIELD))
	{
		m_dRMBDestClosingVelocity = atof((CHAR *)pField);
	}
	else
	{
		m_dRMBDestClosingVelocity = 0.0;
	}

	//
	// Arrival status
	//
	if (GetField(pData, pField, 12, MAXFIELD))
	{
		m_btRMBArrivalStatus = pField[0];
	}
	else
	{
		m_dRMBDestClosingVelocity = 'V';
	}
#endif
	m_dwRMBCount++;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CNMEAParser::ProcessGPRMC(BYTE *pData)
{
#if 0
	CHAR pBuff[10];
	BYTE pField[MAXFIELD];

	m_Pos.wFlags &= ~(GPS_FTIME | GPS_FDATE);

	//
	// Time
	//
	if (GetField(pData, pField, 0, MAXFIELD))
	{
		// Hour
		pBuff[0] = pField[0];
		pBuff[1] = pField[1];
		pBuff[2] = '\0';
		m_Pos.bHour = atoi(pBuff);

		// minute
		pBuff[0] = pField[2];
		pBuff[1] = pField[3];
		pBuff[2] = '\0';
		m_Pos.bMin = atoi(pBuff);

		// Second
		pBuff[0] = pField[4];
		pBuff[1] = pField[5];
		pBuff[2] = '\0';
		m_Pos.bSec = atoi(pBuff);
		m_Pos.wFlags |= (GPS_FTIME);
	}

	//
	// Data valid
	//
	if (GetField(pData, pField, 1, MAXFIELD))
	{
		m_btRMCDataValid = pField[0];
	}
	else
	{
		m_btRMCDataValid = 'V';
	}

	//
	// latitude
	//
	if (GetField(pData, pField, 2, MAXFIELD))
	{
		m_dRMCLatitude = atof((CHAR *)pField + 2) / 60.0;
		pField[2] = '\0';
		m_dRMCLatitude += atof((CHAR *)pField);

	}
	if (GetField(pData, pField, 3, MAXFIELD))
	{
		if (pField[0] == 'S')
		{
			m_dRMCLatitude = -m_dRMCLatitude;
		}
	}

	//
	// Longitude
	//
	if (GetField(pData, pField, 4, MAXFIELD))
	{
		m_dRMCLongitude = atof((CHAR *)pField + 3) / 60.0;
		pField[3] = '\0';
		m_dRMCLongitude += atof((CHAR *)pField);
	}
	if (GetField(pData, pField, 5, MAXFIELD))
	{
		if (pField[0] == 'W')
		{
			m_dRMCLongitude = -m_dRMCLongitude;
		}
	}

	//
	// Ground speed
	//
	if (GetField(pData, pField, 6, MAXFIELD))
	{
		m_dRMCGroundSpeed = atof((CHAR *)pField);
	}
	else
	{
		m_dRMCGroundSpeed = 0.0;
	}

	//
	// course over ground, degrees true
	//
	if (GetField(pData, pField, 7, MAXFIELD))
	{
		m_dRMCCourse = atof((CHAR *)pField);
	}
	else
	{
		m_dRMCCourse = 0.0;
	}
	//
	// Date
	//
	if (GetField(pData, pField, 8, MAXFIELD))
	{
		// Day
		pBuff[0] = pField[0];
		pBuff[1] = pField[1];
		pBuff[2] = '\0';
		m_Pos.bDay = atoi(pBuff);

		// Month
		pBuff[0] = pField[2];
		pBuff[1] = pField[3];
		pBuff[2] = '\0';
		m_Pos.bMonth = atoi(pBuff);

		// Year (Only two digits. I wonder why?)
		pBuff[0] = pField[4];
		pBuff[1] = pField[5];
		pBuff[2] = '\0';
		m_Pos.bYear = atoi(pBuff);

		m_Pos.wFlags |= GPS_FDATE;
	}

	//
	// course over ground, degrees true
	//
	if (GetField(pData, pField, 9, MAXFIELD))
	{
		m_dRMCMagVar = atof((CHAR *)pField);
	}
	else
	{
		m_dRMCMagVar = 0.0;
	}
	if (GetField(pData, pField, 10, MAXFIELD))
	{
		if (pField[0] == 'W')
		{
			m_dRMCMagVar = -m_dRMCMagVar;
		}
	}
#endif
	m_dwRMCCount++;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CNMEAParser::ProcessGPZDA(BYTE *pData)
{
#if 0
	CHAR pBuff[10];
	BYTE pField[MAXFIELD];

	//
	// Time
	//
	if (GetField(pData, pField, 0, MAXFIELD))
	{
		// Hour
		pBuff[0] = pField[0];
		pBuff[1] = pField[1];
		pBuff[2] = '\0';
		m_btZDAHour = atoi(pBuff);

		// minute
		pBuff[0] = pField[2];
		pBuff[1] = pField[3];
		pBuff[2] = '\0';
		m_btZDAMinute = atoi(pBuff);

		// Second
		pBuff[0] = pField[4];
		pBuff[1] = pField[5];
		pBuff[2] = '\0';
		m_btZDASecond = atoi(pBuff);
	}

	//
	// Day
	//
	if (GetField(pData, pField, 1, MAXFIELD))
	{
		m_btZDADay = atoi((CHAR *)pField);
	}
	else
	{
		m_btZDADay = 1;
	}

	//
	// Month
	//
	if (GetField(pData, pField, 2, MAXFIELD))
	{
		m_btZDAMonth = atoi((CHAR *)pField);
	}
	else
	{
		m_btZDAMonth = 1;
	}

	//
	// Year
	//
	if (GetField(pData, pField, 3, MAXFIELD))
	{
		m_wZDAYear = atoi((CHAR *)pField);
	}
	else
	{
		m_wZDAYear = 1;
	}

	//
	// Local zone hour
	//
	if (GetField(pData, pField, 4, MAXFIELD))
	{
		m_btZDALocalZoneHour = atoi((CHAR *)pField);
	}
	else
	{
		m_btZDALocalZoneHour = 0;
	}

	//
	// Local zone hour
	//
	if (GetField(pData, pField, 5, MAXFIELD))
	{
		m_btZDALocalZoneMinute = atoi((CHAR *)pField);
	}
	else
	{
		m_btZDALocalZoneMinute = 0;
	}
#endif
	m_dwZDACount++;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CNMEAParser::ProcessPGRMM(BYTE *pData)
{
#if 0
	GetField(pData, (LPBYTE)m_strDatum, 0, MAXFIELD);
#endif
}
