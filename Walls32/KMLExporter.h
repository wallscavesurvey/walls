#pragma once

#include "stdafx.h"
#include "wall_shp.h"
#include "XMLWriter.h"
#include <fstream>

class KMLExporter
{
public:
	KMLExporter(CExportShp *pExpShp, LPCSTR pathname, UINT flags);
	void writeKML();
	void writeStations();
	void writeStation(const SHP_TYP_STATION &pS);
	void writeVectors();
	void writeVector(const SHP_TYP_VECTOR &pV);
	void writeWalls();
	void writeSVGFloors();
	void writeLRUDs();
	void writeLRUDPolygon(SHP_TYP_LRUDPOLY *pLP, int index);
	void writeSimpleData(const CStringW &name, const CStringW &value);
	void writeSimpleField(const CStringW &name, const CStringW &type);
	void writeSchema();
private:
	UINT m_flags;
	LPCSTR m_pathname;
	std::ofstream m_out;
	XMLWriter m_xml;
	CExportShp *m_ex;
};

