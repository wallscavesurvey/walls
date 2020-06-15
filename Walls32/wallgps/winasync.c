#include <windows.h>
#include <winbase.h>
#include "gps.h"

static DCB dcb;
static HANDLE hCom;
static HANDLE inH;
static DWORD BaudRate;
static DWORD ByteSize;
static DWORD Parity;
static DWORD StopBits;

void AsyncSet(int wBaudRate, int wByteSize, int wStopBits, int wParity)
{
	BOOL fSuccess;
	DWORD dwError;

	dcb.BaudRate = wBaudRate;
	dcb.ByteSize = (BYTE)wByteSize;
	dcb.StopBits = (BYTE)wStopBits;
	dcb.Parity = (BYTE)wParity;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	fSuccess = SetCommState(hCom, &dcb);
	if (!fSuccess) {
		/* Handle the error. */
		dwError = GetLastError();
		sprintf(gps_errmsg, "Comm error - code: %d", dwError);
		byebye(-1);
	}
} // AsyncSet(int wBaudRate, int wByteSize, int wStopBits, int wParity)

int AsyncInit(int port)
{
	DWORD dwError;
	char sPort[16];

	sprintf(sPort, "\\\\.\\COM%u", port + 1);

	hCom = CreateFile(sPort,
		GENERIC_READ | GENERIC_WRITE,
		0,    			// comm devices must be opened w/exclusive-access
		NULL, 			// no security attrs
		OPEN_EXISTING,	// comm devices must use OPEN_EXISTING
		0,    			// not overlapped I/O
		NULL  			// hTemplate must be NULL for comm devices
	);

	if (hCom == INVALID_HANDLE_VALUE) goto _err;
	/*
	 * Omit the call to SetupComm to use the default queue sizes.
	 * Get the current configuration.
	 */

	if (!GetCommState(hCom, &dcb)) {
		CloseHandle(hCom);
		goto _err;
	}

	// Fill in the 'saved' DCB

	BaudRate = dcb.BaudRate;
	ByteSize = dcb.ByteSize;
	Parity = dcb.Parity;
	StopBits = dcb.StopBits;
	//
	//  At this point the COM port is opened with default parameters.
	//  The routine AsyncSet should be called to finish configuring the port
	//
	return 0;

_err:
	dwError = GetLastError();
	sprintf(gps_errmsg, "COM%u error - Code: %d", port + 1, dwError);
	byebye(-1);
	return 0;
} // AsyncInit()

void AsyncStop(void)
{
	DWORD dwError;

	dcb.BaudRate = BaudRate;
	dcb.ByteSize = (BYTE)ByteSize;
	dcb.Parity = (BYTE)Parity;
	dcb.StopBits = (BYTE)StopBits;

	if (!SetCommState(hCom, &dcb)) {
		/* Handle the error. */
		dwError = GetLastError();
		sprintf(gps_errmsg, "Comm error - Code: %d", dwError);
		byebye(-1);
	}
	CloseHandle(hCom);
}

void AsyncOut(unsigned char * msg, int count)
{
	// must add error checking before release
	int bytes;

	if (!WriteFile(hCom, msg, count, &bytes, NULL)) byebye(GPS_ERR_PORTOUT);
}

int AsyncIn(void)
{
	DWORD bytes;
	unsigned char c;

	ReadFile(hCom, &c, 1, &bytes, NULL);
	if (bytes) return (int)c;
	else return c + 1000;
}

int AsyncInStat(void)
{
	COMSTAT b;
	unsigned long along;

	ClearCommError(hCom, &along, &b);
	if (b.cbInQue > 0) return 1;
	return 0;
}

int AsyncOutStat(void) { return 0; }
