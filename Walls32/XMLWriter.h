#pragma once

#include "stdafx.h"
#include "afxtempl.h"
#include "atlstr.h"
//#include "afxcoll.h"
#include <ostream>

class XMLWriter
{
public:
	XMLWriter(std::ostream &out);
	void writeStartDocument();
	void writeStartElement(CStringW name);
	template<typename T>
	void writeStartElement(CStringW name);
	void writeEscapedValue(CStringW value);
	void writeAttribute(CStringW name, CStringW value);
	void writeElementString(CStringW name, CStringW value);
	void writeEndElement();
private:
	void closeStartTagIfNecessary();
	std::ostream &m_out;
	//CStringList m_elementStack;
	CList<CStringW, CStringW &> m_elementStack;
	CString m_indent;
	bool m_startTagIsOpen;
};

