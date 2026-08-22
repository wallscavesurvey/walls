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
	void writeStartElement(const CStringW &name);
	template<typename T>
	void writeStartElement(const CStringW &name);
	void writeElementString(const CStringW &name, const CStringW &value);
	void writeAttribute(const CStringW &name, const CStringW &value);
	void writeString(const CStringW &string);
	void writeEndElement();
private:
	void writeEscapedValue(const CStringW &value);
	void closeStartTagIfNecessary(bool multiline = true);
	std::ostream &m_out;
	//CStringList m_elementStack;
	CList<CStringW, CStringW &> m_elementStack;
	CString m_indent;
	bool m_startTagIsOpen;
	bool m_multilineElement;
};

