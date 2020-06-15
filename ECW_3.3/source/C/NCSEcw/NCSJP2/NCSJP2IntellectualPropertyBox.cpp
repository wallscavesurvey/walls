/********************************************************
** Copyright 2002 Earth Resource Mapping Ltd.
** This document contains proprietary source code of
** Earth Resource Mapping Ltd, and can only be used under
** one of the three licenses as described in the
** license.txt file supplied with this distribution.
** See separate license.txt file for license details
** and conditions.
**
** This software is covered by US patent #6,442,298,
** #6,102,897 and #6,633,688.  Rights to use these patents
** is included in the license agreements.
**
** FILE:     $Archive: /NCS/Source/C/NCSEcw/NCSJP2/NCSJP2IntellectualPropertyBox.cpp $
** CREATED:  04/12/2002 3:27:34 PM
** AUTHOR:   Simon Cope
** PURPOSE:  NCSJP2IntellectualPropertyBox class implementation
** EDITS:    [xx] ddMmmyy NAME COMMENTS
 *******************************************************/

#include "NCSJP2File.h"

 //////////////////////////////////////////////////////////////////////
 // Construction/Destruction
 //////////////////////////////////////////////////////////////////////

UINT32 CNCSJP2File::CNCSJP2IntellectualPropertyBox::sm_nTBox = 'jp2i';

// Constructor
CNCSJP2File::CNCSJP2IntellectualPropertyBox::CNCSJP2IntellectualPropertyBox()
{
	// Initialise members
	m_nTBox = sm_nTBox;
}

// Destructor
CNCSJP2File::CNCSJP2IntellectualPropertyBox::~CNCSJP2IntellectualPropertyBox()
{
}

// Parse in the box from the stream.
CNCSError CNCSJP2File::CNCSJP2IntellectualPropertyBox::Parse(class CNCSJP2File &JP2File, CNCSJPCIOStream &Stream)
{
	CNCSError Error;

	NCSJP2_CHECKIO_BEGIN(Error, Stream);
	NCSJP2_CHECKIO(Seek(m_nLDBox, CNCSJPCIOStream::CURRENT));
	m_bValid = true;
	NCSJP2_CHECKIO_END();
	return(Error);
}
