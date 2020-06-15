//	SerialWnd.cpp - Implementation of the CSerialWnd class
//
//	Copyright (C) 1999-2003 Ramon de Klein (Ramon.de.Klein@ict.nl)
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


//////////////////////////////////////////////////////////////////////
// Include the standard header files

#include "StdAfx.h"

//////////////////////////////////////////////////////////////////////
// Include module headerfile

#include "SerialWnd.h"


//////////////////////////////////////////////////////////////////////
// Disable warning C4127: conditional expression is constant, which
// is generated when using the _RPTF and _ASSERTE macros.

#pragma warning(disable: 4127)


//////////////////////////////////////////////////////////////////////
// Enable debug memory manager

#ifdef _DEBUG

#ifdef THIS_FILE
#undef THIS_FILE
#endif

static const char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW

#endif


//////////////////////////////////////////////////////////////////////
// Code

// Register the standard CSerialWnd COM message
const UINT CSerialWnd::mg_nDefaultComMsg = ::RegisterWindowMessage(_T("CSerialWnd_DefaultComMsg"));


CSerialWnd::CSerialWnd()
	: m_hwndDest(0)
	, m_nComMsg(WM_NULL)
	, m_lParam(0)
	, m_bReceiveAll(0)
{
	ghMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);             // unnamed mutex
}

CSerialWnd::~CSerialWnd()
{
	// Check if the thread handle is still there. If so, then we
	// didn't close the serial port. We cannot depend on the
	// CSerial destructor, because if it calls Close then it
	// won't call our overridden Close.
	if (m_hThread)
	{
		// Display a warning
		_RPTF0(_CRT_WARN, "CSerialWnd::~CSerialWnd - Serial port not closed\n");

		// Close implicitly
		Close();
	}
	CloseHandle(ghMutex);
}

LONG CSerialWnd::Open(LPCTSTR lpszDevice, EBaudrate eBaudrate, HWND hwndDest, UINT nComMsg, LPARAM lParam, DWORD dwInQueue, DWORD dwOutQueue)
{
	// if eBaudrate==EBaudUnknown, no port initialization will occur after CSerialEx::Open() is called

	long lLastError = CSerialEx::Open(lpszDevice, dwInQueue, dwOutQueue);

	if (lLastError != ERROR_SUCCESS)
		return lLastError;

	if (eBaudrate != EBaudUnknown) {
		if (Setup(eBaudrate, EData8, EParNone, EStop1)) {
			Close();
			return lLastError;
		}
	}

	// Save the window handle, notification message and user message
	m_hwndDest = hwndDest;
	m_nComMsg = nComMsg ? nComMsg : mg_nDefaultComMsg;
	m_lParam = lParam;

	// Start the listener thread
	lLastError = StartListener();
	if (lLastError != ERROR_SUCCESS)
	{
		// Close the serial port
		Close();

		// Return the error-code
		m_lLastError = lLastError;
		return m_lLastError;
	}

	// Return the error
	m_lLastError = ERROR_SUCCESS;
	return m_lLastError;
}

LONG CSerialWnd::Close(void)
{
	// Reset all members
	m_hwndDest = 0;
	m_nComMsg = WM_NULL;

	// Call the base class
	return CSerialEx::Close();
}

void CSerialWnd::OnEvent(EEvent eEvent, EError eError)
{
	static GPS_POSITION gp; //protected by mutex

	m_lParam = 0;

	if (eEvent & EEventRecv)
	{
		DWORD dwProcessedCount = m_NMEAParser.m_dwProcessedCount;
		DWORD dwGGACount = m_NMEAParser.m_dwGGACount;

		// Create a clean buffer
		DWORD dwRead;
		BYTE szData[256];
		const int nBuflen = sizeof(szData) - 1;

		// Obtain the data from the serial port
		do
		{
			Read(szData, nBuflen, &dwRead);
			m_NMEAParser.ParseBuffer(szData, dwRead);
		} while (dwRead == nBuflen);

		if (dwProcessedCount != m_NMEAParser.m_dwProcessedCount &&
			(m_bReceiveAll || dwGGACount != m_NMEAParser.m_dwGGACount))
		{
			// lock mutex for gp access --
			DWORD dwWaitResult = ::WaitForSingleObject(ghMutex, 1000); //1 sec vs INFINITE

			switch (dwWaitResult)
			{
			case WAIT_OBJECT_0:
				// The thread got ownership of the mutex
				gp = m_NMEAParser.m_Pos;
				ReleaseMutex(ghMutex);
				m_lParam = (LPARAM)&gp;
				break;

			case WAIT_TIMEOUT:
			case WAIT_FAILED:
				ASSERT(0);
				return;

			case WAIT_ABANDONED:
				// The thread got ownership of an abandoned mutex
				ASSERT(0);
				ReleaseMutex(ghMutex); //necessary??
				return;

			default:
				ASSERT(0);
				return;
			}
		}
		else return;
	}
	else if (!eError) return;

	// Post message to the client window
	::PostMessage(m_hwndDest, m_nComMsg, MAKEWPARAM(eEvent, eError), m_lParam);
}
