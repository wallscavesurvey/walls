#include "stdafx.h"
#include "walls.h"
#include "KMLExporter.h"
#include <atlconv.h>
#include <functional>

const int CP_ISO_8859_1 = 28591;

KMLExporter::KMLExporter(CExportShp * pExpShp, LPCSTR pathname, UINT flags)
	:  m_flags(flags), m_pathname(pathname), m_out(), m_xml(m_out), m_ex(pExpShp)
{
}

void KMLExporter::writeKML()
{
	m_out.exceptions(std::ios::badbit | std::ios::failbit);
	m_out.open(m_pathname);
	m_xml.writeStartDocument();
	m_xml.writeStartElement(L"kml");
	m_xml.writeAttribute(L"xmlns", L"http://www.opengis.net/kml/2.2");
	m_xml.writeStartElement(L"Document");

	if (m_flags & SHP_STATIONS) {
		writeStations();
	}
	if (m_flags & SHP_VECTORS) {
		writeVectors();
	}
	if (m_flags & SHP_WALLS) {
		writeWalls();
	}
	writeSchema();

	m_xml.writeEndElement(); // Document
	m_xml.writeEndElement(); // kml
}

void KMLExporter::writeStations()
{
	m_xml.writeStartElement(L"Folder");
	m_xml.writeElementString(L"name", L"Stations");

	SHP_TYP_STATION pS;

	// we need to call with NULL first to initialize (yuck)
	m_ex->GetStation(NULL, true);
	while (m_ex->GetStation(&pS, true)) {
		writeStation(pS);
	}

	m_xml.writeEndElement(); // Folder
}

void KMLExporter::writeStation(const SHP_TYP_STATION &pS)
{
	USES_CONVERSION;

	CStringW name;
	CStringW latitude, longitude, altitude;
	latitude.Format(L"%.*f", 8, pS.xyz[1]);
	longitude.Format(L"%.*f", 8, pS.xyz[0]);
	altitude.Format(L"%.*f", 3, pS.xyz[2]);

	m_xml.writeStartElement(L"Placemark");
	name = CA2W(pS.label, CP_ISO_8859_1);
	m_xml.writeElementString(L"name", name);

	m_xml.writeStartElement(L"ExtendedData");
	m_xml.writeStartElement(L"SchemaData");
	m_xml.writeAttribute(L"schemaUrl", L"#Waypoint");

	writeSimpleData(L"latitude", latitude);
	writeSimpleData(L"longitude", longitude);
	writeSimpleData(L"altitude", altitude);

	m_xml.writeEndElement(); // SchemaData
	m_xml.writeEndElement(); // ExtendedData

	m_xml.writeStartElement(L"Point");
	CStringW coords;
	coords.Format(L"%s,%s,%s", longitude, latitude, altitude);
	m_xml.writeElementString(L"coordinates", coords);
	m_xml.writeEndElement(); // Point

	m_xml.writeEndElement(); // Placemark
}

void KMLExporter::writeVectors()
{
	m_xml.writeStartElement(L"Folder");
	m_xml.writeElementString(L"name", L"Track");

	SHP_TYP_VECTOR pV;
	pV.titles = pV.attributes = NULL;

	// we need to call with NULL first to initialize (yuck)
	m_ex->GetVector(NULL);
	while (m_ex->GetVector(&pV)) {
		writeVector(pV);
	}

	m_xml.writeEndElement(); // Folder
}

void KMLExporter::writeVector(const SHP_TYP_VECTOR &pV)
{
	USES_CONVERSION;

	CStringW format;
	CStringW from(CA2W(pV.station[0].label, CP_ISO_8859_1));
	CStringW to(CA2W(pV.station[1].label, CP_ISO_8859_1));
	CStringW latitude, longitude, altitude;
	latitude.Format(L"%.*f", 8, pV.station[0].xyz[1]);
	longitude.Format(L"%.*f", 8, pV.station[0].xyz[0]);
	altitude.Format(L"%.*f", 3, pV.station[0].xyz[2]);

	m_xml.writeStartElement(L"Placemark");

	format.Format(L"%s to %s", from, to);
	m_xml.writeElementString(L"name", format);

	m_xml.writeStartElement(L"Style");
	m_xml.writeStartElement(L"LineStyle");
	// kml uses aabbggrr for whatever reason,

	format = CA2W(pV.linetype, CP_ISO_8859_1);
	format.MakeLower();
	format.Format(
		L"ff%C%C%C%C%C%C",
		format.GetAt(4),
		format.GetAt(5),
		format.GetAt(2),
		format.GetAt(3),
		format.GetAt(0),
		format.GetAt(1)
	);
	m_xml.writeElementString(L"color", format);
	m_xml.writeEndElement(); // LineStyle
	m_xml.writeStartElement(L"PolyStyle");
	m_xml.writeElementString(L"fill", L"0");
	m_xml.writeEndElement(); // PolyStyle
	m_xml.writeEndElement(); // Style

	m_xml.writeStartElement(L"ExtendedData");
	m_xml.writeStartElement(L"SchemaData");
	m_xml.writeAttribute(L"schemaUrl", L"#Track");
	writeSimpleData(L"fromStation", from);
	writeSimpleData(L"toStation", to);
	writeSimpleData(L"latitude", latitude);
	writeSimpleData(L"latitude", latitude);
	writeSimpleData(L"longitude", longitude);
	writeSimpleData(L"altitude", altitude);
	format = CA2W(pV.date, CP_ISO_8859_1);
	format.Format(L"%s-%s-%s", format.Left(4), format.Mid(4, 2), format.Right(2));
	writeSimpleData(L"date", format);
	format = CA2W(pV.filename, CP_ISO_8859_1);
	format.Append(L".SRV");
	writeSimpleData(L"filename", format);
	m_xml.writeEndElement(); // SchemaData
	m_xml.writeEndElement(); // ExtendedData

	m_xml.writeStartElement(L"MultiGeometry");
	m_xml.writeStartElement(L"LineString");
	m_xml.writeStartElement(L"coordinates");
	format.Format(L"%s,%s,%s", longitude, latitude, altitude);
	m_xml.writeString(format);
	latitude.Format(L"%.*f", 8, pV.station[1].xyz[1]);
	longitude.Format(L"%.*f", 8, pV.station[1].xyz[0]);
	altitude.Format(L"%.*f", 3, pV.station[1].xyz[2]);
	format.Format(L" %s,%s,%s", longitude, latitude, altitude);
	m_xml.writeString(format);
	m_xml.writeEndElement(); // coordinates
	m_xml.writeEndElement(); // LineString
	m_xml.writeEndElement(); // MultiGeometry

	m_xml.writeEndElement(); // Placemark
}

void KMLExporter::writeWalls()
{
	m_xml.writeStartElement(L"Style");
	m_xml.writeAttribute(L"id", L"wallsStyle");
	m_xml.writeStartElement(L"LineStyle");
	m_xml.writeElementString(L"color", L"ff0000ff");
	m_xml.writeEndElement(); // LineStyle
	m_xml.writeStartElement(L"PolyStyle");
	m_xml.writeElementString(L"fill", L"1");
	m_xml.writeElementString(L"color", L"ffff0080");
	m_xml.writeEndElement(); // PolyStyle
	m_xml.writeEndElement(); // Style

	m_xml.writeStartElement(L"Folder");
	m_xml.writeElementString(L"name", L"Walls");

	if (m_ex->HasSVGFloors()) {
		writeSVGFloors();
	}
	else {
		writeLRUDs();
	}

	m_xml.writeEndElement(); // Folder
}

int WriteLrudPolygons(std::function<int(SHP_TYP_LRUDPOLY *)>  pOutPoly);

void KMLExporter::writeSVGFloors()
{
	// we need to call with NULL first to initialize (yuck)
	m_ex->GetPolygon(NULL);
	SHP_TYP_POLYGON sP;

	int e;
	while (!(e = m_ex->GetPolygon(&sP)))
	{
		CStringW format;
		m_xml.writeStartElement(L"Placemark");
		//format.Format(L"Section%d", index);
		//m_xml.writeElementString(L"name", format);
		m_xml.writeElementString(L"styleUrl", L"#wallsStyle");
		
		m_xml.writeStartElement(L"MultiGeometry");

		CStringW latitude, longitude;
		double point[2];

		int pointStartIndex = 0;
		for (int i = 0; i < sP.nPolygons; i++) {
			m_xml.writeStartElement(L"Polygon");
			m_xml.writeStartElement(L"outerBoundaryIs");
			m_xml.writeStartElement(L"LinearRing");

			m_xml.writeStartElement(L"coordinates");

			int numPoints = sP.pPolygons[i].flgcnt & NTW_FLG_MASK;
			for (int p = 0; p < numPoints; p++) {
				int pointIndex = pointStartIndex + p;
				point[0] = sP.pPoints[pointStartIndex + p].x * sP.scale + sP.datumE;
				point[1] = sP.pPoints[pointStartIndex + p].y * sP.scale + sP.datumN;

				// convert UTM to latitude/longitude
				CMainFrame::ConvertUTM2LL(point);

				latitude.Format(L"%.*f", 8, point[1]);
				longitude.Format(L"%.*f", 8, point[0]);
				format.Format(L"%s,%s ", longitude, latitude);
				m_xml.writeString(format);
			}
			pointStartIndex += numPoints;

			m_xml.writeEndElement(); // coordinates

			m_xml.writeEndElement(); // LinearRing
			m_xml.writeEndElement(); // outerBoundaryIs
			m_xml.writeEndElement(); // Polygon
		}

		m_xml.writeEndElement(); // MultiGeometry

		m_xml.writeEndElement(); // Placemark
	}

	if (e != SHP_ERR_FINISHED) {
		throw std::runtime_error("error processing SVG mask");
	}
}

void KMLExporter::writeLRUDs()
{
	int index = 0;
	auto callback = [&](SHP_TYP_LRUDPOLY *pLP) {
		this->writeLRUDPolygon(pLP, index++);
		return 0;
	};
	WriteLrudPolygons(callback);
}

void KMLExporter::writeLRUDPolygon(SHP_TYP_LRUDPOLY *pLP, int index)
{
	CStringW format;
	m_xml.writeStartElement(L"Placemark");
	format.Format(L"Section%d", index);
	m_xml.writeElementString(L"name", format);
	m_xml.writeElementString(L"styleUrl", L"#wallsStyle");

	CStringW latitude, longitude, altitude;
	latitude.Format(L"%.*f", 8, pLP->fpt[0].y);
	longitude.Format(L"%.*f", 8, pLP->fpt[0].x);
	altitude.Format(L"%.*f", 3, pLP->elev);

	m_xml.writeStartElement(L"ExtendedData");
	m_xml.writeStartElement(L"SchemaData");
	m_xml.writeAttribute(L"schemaUrl", L"#Track");
	writeSimpleData(L"latitude", latitude);
	writeSimpleData(L"longitude", longitude);
	writeSimpleData(L"altitude", altitude);

	m_xml.writeEndElement(); // SchemaData
	m_xml.writeEndElement(); // ExtendedData

	m_xml.writeStartElement(L"MultiGeometry");
	m_xml.writeStartElement(L"Polygon");
	m_xml.writeStartElement(L"outerBoundaryIs");
	m_xml.writeStartElement(L"LinearRing");

	m_xml.writeStartElement(L"coordinates");
	for (int i = 0; i < pLP->nPts; i++) {
		latitude.Format(L"%.*f", 8, pLP->fpt[i].y);
		longitude.Format(L"%.*f", 8, pLP->fpt[i].x);
		format.Format(L"%s,%s,%s ", longitude, latitude, altitude);
		m_xml.writeString(format);
	}
	m_xml.writeEndElement(); // coordinates

	m_xml.writeEndElement(); // LinearRing
	m_xml.writeEndElement(); // outerBoundaryIs
	m_xml.writeEndElement(); // Polygon
	m_xml.writeEndElement(); // MultiGeometry

	m_xml.writeEndElement(); // Placemark
}

void KMLExporter::writeSimpleData(const CStringW &name, const CStringW &value)
{
	m_xml.writeStartElement(L"SimpleData");
	m_xml.writeAttribute(L"name", name);
	m_xml.writeString(value);
	m_xml.writeEndElement();
}

void KMLExporter::writeSimpleField(const CStringW &name, const CStringW &type)
{
	m_xml.writeStartElement(L"SimpleField");
	m_xml.writeAttribute(L"name", name);
	m_xml.writeAttribute(L"type", type);
	m_xml.writeEndElement();
}

void KMLExporter::writeSchema()
{
	if (m_flags & SHP_STATIONS) {
		m_xml.writeStartElement(L"Schema");
		m_xml.writeAttribute(L"name", L"Waypoint");
		m_xml.writeAttribute(L"id", L"Waypoint");
		writeSimpleField(L"latitude", L"float");
		writeSimpleField(L"longitude", L"float");
		writeSimpleField(L"altitude", L"float");
		m_xml.writeEndElement();
	}

	if (m_flags & SHP_VECTORS) {
		m_xml.writeStartElement(L"Schema");
		m_xml.writeAttribute(L"name", L"Track");
		m_xml.writeAttribute(L"id", L"Track");
		writeSimpleField(L"fromStation", L"string");
		writeSimpleField(L"toStation", L"string");
		writeSimpleField(L"latitude", L"float");
		writeSimpleField(L"longitude", L"float");
		writeSimpleField(L"altitude", L"float");
		writeSimpleField(L"date", L"string");
		writeSimpleField(L"filename", L"string");
		m_xml.writeEndElement();
	}
}
