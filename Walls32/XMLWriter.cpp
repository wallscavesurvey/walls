#include "stdafx.h"
#include <atlstr.h>
#include <atlconv.h>
#include "XMLWriter.h"

XMLWriter::XMLWriter(std::ostream &out)
	: m_out(out),
	m_elementStack(),
	m_startTagIsOpen(false),
	m_multilineElement(false),
	m_indent("")
{
}

void XMLWriter::writeStartDocument()
{
	USES_CONVERSION;
	m_out << CW2A(L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>", CP_UTF8) << std::endl;
}

void XMLWriter::closeStartTagIfNecessary(bool multiline)
{
	if (!m_startTagIsOpen) return;
	m_multilineElement = multiline;
	m_out << ">";
	if (multiline) m_out << std::endl;
	m_startTagIsOpen = false;
}

void XMLWriter::writeStartElement(const CStringW &name)
{
	USES_CONVERSION;
	closeStartTagIfNecessary();
	CW2A convName(name, CP_UTF8);
	m_out << m_indent << "<" << convName;
	m_elementStack.AddTail(CStringW(name));
	m_indent += "  ";
	m_startTagIsOpen = true;
}

bool isReservedChar(wchar_t c)
{
	switch (c) {
	case 0:
	case '&':
	case '<':
	case '>':
	case '"':
	case '\'':
		return true;
	}
	return false;
}

void XMLWriter::writeEscapedValue(const CStringW &value)
{
	USES_CONVERSION;
	int start = 0;
	int length = value.GetLength();
	for (int i = 0; i < length; i++) {
		wchar_t c = value.GetAt(i);
		if (!isReservedChar(c)) continue;
		if (i > start) {
			m_out << CW2A(value.Mid(start, i - start), CP_UTF8);
			start = i + 1;
		}
		switch (c) {
		case 0:
			// TODO: error
			break;
		case '<':
			m_out << "&lt;";
			break;
		case '>':
			m_out << "&gt;";
			break;
		case '&':
			m_out << "&amp;";
			break;
		case '"':
			m_out << "&quot;";
			break;
		case '\'':
			m_out << "&apos;";
			break;
		}
	}
	if (start < length) {
		m_out << CW2A(value.Mid(start), CP_UTF8);
	}
}

void XMLWriter::writeAttribute(const CStringW &name, const CStringW &value)
{
	USES_CONVERSION;
	if (!m_startTagIsOpen) {
		// TODO: set error
		return;
	}
	CW2A convName(name, CP_UTF8);
	m_out << " " << convName << "=\"";
	writeEscapedValue(value);
	m_out << '"';
}

void XMLWriter::writeElementString(const CStringW &name, const CStringW &value)
{
	USES_CONVERSION;
	closeStartTagIfNecessary();
	CW2A convName(name, CP_UTF8);
	m_out << m_indent << "<" << convName << ">";
	writeEscapedValue(value);
	m_out << "</" << convName << ">" << std::endl;
}

void XMLWriter::writeString(const CStringW &string)
{
	USES_CONVERSION;
	closeStartTagIfNecessary(false);
	writeEscapedValue(string);
}

void XMLWriter::writeEndElement()
{
	USES_CONVERSION;
	if (m_elementStack.IsEmpty()) {
		// TODO: set error
		return;
	}
	closeStartTagIfNecessary();
	m_indent.Truncate(m_indent.GetLength() - 2);
	CStringW name = m_elementStack.RemoveTail();
	CW2A convName(name, CP_UTF8);
	if (m_multilineElement) m_out << m_indent;
	m_out << "</" << convName << ">" << std::endl;
	m_multilineElement = true;
}
