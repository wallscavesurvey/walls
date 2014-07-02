//<copyright>
// 
// Copyright (c) 1995,96
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        wrlsave.C
//
// Purpose:     save a node in a VRML structe graph
//
// Created:     14 Dec 95   Alexander Nussbaumer
//
// Changed:      9 Feb 96   Alexander Nussbaumer & Michael Pichler
//
// $Id: wrlsave.C,v 1.1 1996/02/14 17:02:47 mpichler Exp $
//
//</file>


// this source is a modified version of the code by Steven
// M. Niles, traversing a QvLib parse tree, outputing VRML;
// available at http://www.tenet.net/html/qvregen.html

// changes made to the code include:
//
// - upgraded to final 1.0 spec (anuss): AciiText, LOD etc.
// - added some missing fields (anuss)
// - support for DEF/USE mechanism (anuss)
// - removed traversal state and stack (mpichler)
//
// TODO: write unknown nodes to file (requires type information)

// copyright of the original version follows:

/*
 Copyright 1995 by Tenet Networks, Inc.
 Carlsbad, California, USA. All rights reserved.

 Redistribution and use of this software, with or without modification, is
 permitted provided that the following conditions are met:

 1. Redistributions of this software must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
  EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*
Steven M. Niles
Vice President
Tenet Networks
(619) 723-4079
niles@cts.com
http://www.catalog.com/tenet
*/
#include "qvlib/QvElement.h"
#include "qvlib/QvNodes.h"
#include "qvlib/QvInput.h"
#include "qvlib/QvUnknownNode.h"
#include "qvlib/QvBasic.h"
/* #ifdef EXTENSIONS */
#include "qvlib/QvExtensions.h"
/* #endif */

#include <iostream>
using namespace std;

static int indent = 0;

static ostream* os = &cout;  // output stream (set in saveAll)


// saveAll: save the data structure pointed to by root;
// root must be a node obtained by QvDB::read -
// do not call it with an arbitrary node!

void QvNode::saveAll (const QvNode *root, ostream& ost, float version)
{
  if (!root)
    return;

  os = &ost;

  os->precision (2);
  os->setf(ios::showpoint);  // printf ("%1.1f", version);
  *os << "#VRML V" << version;
  os->precision (0);
  os->unsetf (ios::showpoint);
  if (version == 1.0)
    *os << " ascii\n";
  else
  {
    *os << " utf8\n";
    if (version != 1.1)
      cerr << "warning: assumend utf8 encoding for version > 1.1\n";
  }

  QvDict dict;
  // ass.: root node is of type QvGroup (see QvDB::read)
  QvGroup* theroot = (QvGroup*) root;
  for (int i = 0;  i < theroot->getNumChildren ();  i++)
    theroot->getChild (i)->save (&dict);

  os = &cout;  // default output stream

} // saveAll


// anuss:
// if 'node-getName ()' contains a String, it is referenced by two or more parants
// the first time it is written to the outstream whith DEF and as it is, 
// an entry to dict is made;
// the next times only USE and the name is being written to the outstream

QvBool refnode (QvDict *dict, QvNode *node)
{
  const char* name = (node->getName()).getString();

  if (*name == 0)
    return FALSE;

  u_long keyname = (u_long) name;
  void *node1 = (void *)node;

  if (!dict->find(keyname, node1))
  {  dict->enter(keyname, node1);
     *os << "DEF " << (char *)keyname << " ";
     return FALSE;
  }

  *os << "USE " << (char *)keyname << "\n";
  return TRUE;
}


// announe moved into BEGINNODE macro (anuss)


//anuss:
#define BEGINNODE(className)  \
    int i = indent;  \
    while (i--)  *os << "\t";  \
    if (refnode (dict, this))  \
        return;  \
    *os << QV__QUOTE(className) << " {\n";  \
    indent++;                                                 
  


static void end_brack ()
{
  unsigned i = indent;
  while (i--)
    *os << "\t";
  *os << "}\n";
}

#define ENDNODE  \
    indent--;  \
    end_brack ();


void tab_in (unsigned in)
{
  while (in--)
    *os << "\t";
}



void print_bitmask(QvSFEnum enm)
{
        int out_vals_written = 0, all_val;
        QvName all("ALL");

        enm.findEnumValue(all, all_val);

        if (enm.value == all_val) 
                *os << "\tALL\n";
        else  {
                *os << "\t(";
                for (int i = 0; i < enm.numEnums; i++) {    // anuss: vorher i<enm.n..ms-1;
                        if (enm.enumValues[i] & enm.value) {
                                if (out_vals_written++) *os << " | ";
                                *os << (char *)enm.enumNames[i].getString( );
                                }
                        }
                *os << ")\n";
                }
}


void print_enum(QvSFEnum enm)
{
  if (enm.isDefault( )) 
    *os << "DEFAULT\n";
  else
  {
    for (int i = 0; i < enm.numEnums; i++) 
      if (enm.enumValues[i] == enm.value)
      {
        *os << (char *)enm.enumNames[i].getString( ) << "\n";
        break;
      }
  }
}


#define MFIELD(fieldNm, grouping)  \
        if (!(fieldNm.isDefault( ))) {  \
                tab_in( indent );  \
                *os << QV__QUOTE(fieldNm) << "\t[";  \
                for (i = 0; i < fieldNm.num * grouping; i++) {  \
     /* anuss */            if (i /* && (grouping != 1) */ && (i % grouping == 0)) *os << ",";  \
                        *os << " " << fieldNm.values[i];  \
                }  \
                *os << " ]\n";  \
         }


#define BITMASK(fieldNm)  \
        if (!(fieldNm.isDefault( ))) {  \
                if (fieldNm.numEnums) {  \
                        tab_in( indent );  \
                        *os << QV__QUOTE(fieldNm);  \
                        print_bitmask(fieldNm);  \
                        }  \
                }
        

#define FIXEDARRAYFIELD(fieldNm, grouping)  \
        if (!(fieldNm.isDefault( ))) {  \
                tab_in( indent );  \
                *os << QV__QUOTE(fieldNm) << "\t";  \
                for (i = 0; i < grouping; i++) {  \
                        *os << fieldNm.value[i] << " ";  \
                        }  \
                *os << "\n";  \
                }

#define FIELD(fieldNm)  \
        if (!(fieldNm.isDefault( ))) {  \
                tab_in( indent );  \
                *os << QV__QUOTE(fieldNm) << "\t" << fieldNm.value << "\n";  \
                }        

#define STRINGFIELD(fieldNm)  \
        if (!(fieldNm.isDefault( ))) {  \
                tab_in( indent );  \
                *os << QV__QUOTE(fieldNm) << "\t"  \
                        << "\""  << fieldNm.value.getString( ) << "\"\n";  \
                }

//anuss:                 
#define STRINGMFIELD(fieldNm)  \
        if (!(fieldNm.isDefault( ))) {  \
                tab_in( indent );  \
                *os << QV__QUOTE(fieldNm) << "\t[";  \
                for (i = 0; i < fieldNm.num; i++) {  \
                        if (i) *os << ",";  \
                        *os << " \"" << fieldNm.values[i].getString () << "\"";  \
                }  \
                *os << " ]\n";  \
        }

#define ENUMFIELD(fieldNm)  \
        if (!(fieldNm.isDefault( ))) {  \
                if (fieldNm.numEnums) {  \
                        tab_in( indent);  \
                        *os << QV__QUOTE(fieldNm) << "\t";  \
                        print_enum(fieldNm);  \
                        }  \
                }
        
 
#define ROTATIONFIELD(fieldNm)  \
        if (!(fieldNm.isDefault( ))) {  \
                tab_in( indent );  \
                *os << QV__QUOTE(fieldNm) << "\t";  \
                for (i = 0; i < 3 ; i++)  \
                        *os << fieldNm.axis[i] << " ";  \
                *os << fieldNm.angle << "\n"; \
                } 

#define MATRIXFIELD(fieldNm) \
        if (!(fieldNm.isDefault( ))) {  \
                tab_in( indent );  \
                *os << QV__QUOTE(fieldNm) << "\t";  \
                for (i = 0; i < 4; i++)  \
                        for (int j = 0; j < 4; j++)  \
                                *os << fieldNm.value[i][j] << " ";  \
                *os << "\n";  \
                }

#define BOOLFIELD(fieldNm)  \
        if (!(fieldNm.isDefault( ))) {  \
                tab_in( indent );  \
                *os << QV__QUOTE(fieldNm) << "\t" <<  \
                        (fieldNm.value ? "TRUE" : "FALSE") << "\n";  \
                }

//anuss:    
#define SFIMAGE(fieldNm)  \
    if (!(fieldNm.isDefault( )))  \
    {   tab_in( indent );  \
        *os << QV__QUOTE(fieldNm) << "\t" << fieldNm.size[0] << ' '  \
            << fieldNm.size[1] << ' ' << fieldNm.numComponents << hex;  \
        int byte = 0;  \
        unsigned row = fieldNm.size[1];  \
        while (row--)  \
        {   *os << '\n';  \
            tab_in (indent);  \
            unsigned col = fieldNm.size[0];  \
            while (col--)  \
            {   unsigned long val = 0;  \
                unsigned k = fieldNm.numComponents;  \
                while (k--)  \
                    val = (val << 8) | fieldNm.bytes [byte++];  \
                if (val < 256)  \
                    *os << dec << val;  \
                else  \
                    *os << "0x" << hex << val;  \
                *os << ' ';  \
            }  \
        }  \
        *os << '\n' << dec;  \
    }




        
//////////////////////////////////////////////////////////////////////////////
//
// Groups.
//
//////////////////////////////////////////////////////////////////////////////

void QvGroup::save (QvDict *dict)
{
    BEGINNODE(Group)
    for (i = 0; i < getNumChildren(); i++)
    {
        getChild(i)->save (dict);
    }
    ENDNODE
}


//anuss:
void QvLOD::save (QvDict *dict)
{       
    BEGINNODE(LOD)
    MFIELD(range, 1)
    FIXEDARRAYFIELD(center, 3)
    for (i = 0; i < getNumChildren(); i++)
        getChild(i)->save (dict);
    ENDNODE
}

//anuss:
void QvAsciiText::save (QvDict *dict)
{
    BEGINNODE(AsciiText)
    STRINGMFIELD(string)
    FIELD(spacing)
    ENUMFIELD(justification)
    MFIELD(width, 1)
    ENDNODE
}   

//anuss:
void QvFontStyle::save (QvDict *dict)
{
    BEGINNODE(FontStyle)
    FIELD(size)
    ENUMFIELD(family)
    BITMASK(style)
    ENDNODE
}

//anuss:
void QvLabel::save (QvDict *dict)
{
    BEGINNODE(Label)
    STRINGFIELD(label)
    ENDNODE
}

//anuss:
void QvLightModel::save (QvDict *dict)
{
    BEGINNODE(LightModel)
    ENUMFIELD(model)
    ENDNODE
}


//anuss:
void QvUnknownNode::save (QvDict*)
{
//    BEGINNODE(Unknown)
//    QvFieldData* fd = getFieldData ();
//    for (i=0; i<fd->getNumFields (); i++)
//    {   *os << (fd->getFieldName (i)).getString () << '\n';
//    }
//    ENDNODE

    cerr << "QvUnknownNode::save: Sorry, cannot save unknown node in this version\n"
            "  (ignoring; data will be lost)" << endl;
}



void QvSeparator::save (QvDict *dict)
{
    BEGINNODE(Separator)
    ENUMFIELD(renderCulling)
    for (i = 0; i < getNumChildren(); i++)
    {
       getChild(i)->save (dict);
    }
    ENDNODE
}

void QvSwitch::save (QvDict *dict)
{
    BEGINNODE(Switch)
    FIELD(whichChild)
    int which = whichChild.value;

    if (which == QV_SWITCH_NONE)
        ;
    else if (which == QV_SWITCH_ALL)
        for (i = 0; i < getNumChildren(); i++)
            getChild(i)->save (dict);
    else if (which < getNumChildren())
        getChild(which)->save (dict);

    ENDNODE
}

void QvTransformSeparator::save (QvDict *dict)
{
    BEGINNODE(TransformSeparator)

    for (i = 0; i < getNumChildren(); i++)
        getChild(i)->save (dict);

    ENDNODE
}

//////////////////////////////////////////////////////////////////////////////
//
// Properties.
//
//////////////////////////////////////////////////////////////////////////////


#define DO_PROPERTY(className, propName)  \
void className::save (QvDict *dict)  \
{  \
    BEGINNODE(propName)


#define END_PROPERTY  \
    ENDNODE  \
}


DO_PROPERTY(QvCoordinate3, Coordinate3) 
        MFIELD(point, 3);
END_PROPERTY


DO_PROPERTY(QvMaterial, Material)
        MFIELD(ambientColor, 3);
        MFIELD(diffuseColor, 3);
        MFIELD(specularColor, 3);
        MFIELD(emissiveColor, 3);
        MFIELD(shininess, 1);
        MFIELD(transparency, 1);
END_PROPERTY


DO_PROPERTY(QvMaterialBinding, MaterialBinding)
        ENUMFIELD(value);
END_PROPERTY


DO_PROPERTY(QvNormal, Normal)
        MFIELD(vector, 3);
END_PROPERTY


DO_PROPERTY(QvNormalBinding, NormalBinding)
        ENUMFIELD(value);
END_PROPERTY


DO_PROPERTY(QvShapeHints, ShapeHints)
        ENUMFIELD(vertexOrdering);
        ENUMFIELD(shapeType);
        ENUMFIELD(faceType);
        FIELD(creaseAngle);
END_PROPERTY


DO_PROPERTY(QvTextureCoordinate2, TextureCoordinate2)
        MFIELD(point, 2);
END_PROPERTY


DO_PROPERTY(QvTexture2, Texture2)
        STRINGFIELD(filename);
        SFIMAGE(image); //anuss
        ENUMFIELD(wrapS);
        ENUMFIELD(wrapT);
END_PROPERTY


DO_PROPERTY(QvTexture2Transform, Texture2Transform)
        FIXEDARRAYFIELD(translation, 2);
        FIELD(rotation);
        FIXEDARRAYFIELD(scaleFactor, 2);
        FIXEDARRAYFIELD(center, 2);
END_PROPERTY


DO_PROPERTY(QvDirectionalLight, DirectionalLight)
        BOOLFIELD(on);
        FIELD(intensity);
        FIXEDARRAYFIELD(color, 3);
        FIXEDARRAYFIELD(direction,3);
END_PROPERTY


DO_PROPERTY(QvPointLight, PointLight)
        BOOLFIELD(on);
        FIELD(intensity);
        FIXEDARRAYFIELD(color, 3);
        FIXEDARRAYFIELD(location,3);
END_PROPERTY


DO_PROPERTY(QvSpotLight, SpotLight)
        BOOLFIELD(on);
        FIELD(intensity);
        FIXEDARRAYFIELD(color, 3);
        FIXEDARRAYFIELD(location,3);
        FIXEDARRAYFIELD(direction,3);
        FIELD(dropOffRate);
        FIELD(cutOffAngle);
END_PROPERTY


DO_PROPERTY(QvOrthographicCamera, OrthographicCamera)
        FIXEDARRAYFIELD(position,3);
        ROTATIONFIELD(orientation);
        FIELD(focalDistance);
        FIELD(height);
END_PROPERTY


DO_PROPERTY(QvPerspectiveCamera, PerspectiveCamera)
        FIXEDARRAYFIELD(position,3);
        ROTATIONFIELD(orientation);
        FIELD(focalDistance);
        FIELD(heightAngle);
END_PROPERTY


DO_PROPERTY(QvTransform, Transform)
        FIXEDARRAYFIELD(translation,3);
        ROTATIONFIELD(rotation);
        FIXEDARRAYFIELD(scaleFactor,3);
        ROTATIONFIELD(scaleOrientation);
        FIXEDARRAYFIELD(center,3);
END_PROPERTY


DO_PROPERTY(QvRotation, Rotation)
        ROTATIONFIELD(rotation);
END_PROPERTY


DO_PROPERTY(QvMatrixTransform, MatrixTransform)
        MATRIXFIELD(matrix);
END_PROPERTY


DO_PROPERTY(QvTranslation, Translation)
        FIXEDARRAYFIELD(translation,3);
END_PROPERTY


DO_PROPERTY(QvScale, Scale)
        FIXEDARRAYFIELD(scaleFactor,3);
END_PROPERTY


//////////////////////////////////////////////////////////////////////////////
//
// Shapes.
//
//////////////////////////////////////////////////////////////////////////////


#define DO_SHAPE(classNm, name)  \
void classNm::save (QvDict *dict)  \
{  \
        BEGINNODE(name)


#define END_SHAPE  \
        ENDNODE  \
}       


DO_SHAPE(QvCube, Cube)
        FIELD(width);
        FIELD(height);
        FIELD(depth);   
END_SHAPE


DO_SHAPE(QvCone, Cone)
        BITMASK(parts);
        FIELD(bottomRadius);
        FIELD(height);  
END_SHAPE

        
DO_SHAPE(QvCylinder, Cylinder)
        BITMASK(parts);
        FIELD(radius);  
        FIELD(height);
END_SHAPE


DO_SHAPE(QvIndexedFaceSet, IndexedFaceSet)
        MFIELD(coordIndex, 1);
        MFIELD(materialIndex, 1);
        MFIELD(normalIndex, 1);
        MFIELD(textureCoordIndex, 1);
END_SHAPE


DO_SHAPE(QvIndexedLineSet, IndexedLineSet)
        MFIELD(coordIndex, 1);
        MFIELD(materialIndex, 1);
        MFIELD(normalIndex, 1);
        MFIELD(textureCoordIndex, 1);
END_SHAPE


DO_SHAPE(QvPointSet, PointSet)
        FIELD(startIndex);
        FIELD(numPoints);
END_SHAPE


DO_SHAPE(QvSphere, Sphere)
        FIELD(radius);
END_SHAPE


//
// Not really a shape, but this does what it's supposed to.
//
DO_SHAPE(QvInfo, Info)
        STRINGFIELD(string)
END_SHAPE;


//////////////////////////////////////////////////////////////////////////////
//
// WWW-specific nodes.
//
//////////////////////////////////////////////////////////////////////////////


#define DO_WWW(className, propName)  \
void className::save (QvDict *dict)  \
{  \
        BEGINNODE(propName)


#define END_WWW  \
        if (getNumChildren() > 0)  \
                getChild(0)->save (dict);  \
        ENDNODE  \
}


DO_WWW(QvWWWAnchor, WWWAnchor)
        STRINGFIELD(name)
        STRINGFIELD(description) //anuss
        ENUMFIELD(map)
END_WWW


DO_WWW(QvWWWInline, WWWInline)
        STRINGFIELD(name)
        FIXEDARRAYFIELD(bboxSize, 3)
        FIXEDARRAYFIELD(bboxCenter, 3)
        // children of WWWInline are not saved
        ENDNODE
}


#undef BEGINNODE
#undef ENDNODE
#undef SAVE
#undef DO_PROPERTY
#undef DO_SHAPE
#undef DO_WWW
#undef END_SHAPE
#undef END_PROPERTY
#undef END_WWW
#undef FIELD
#undef MFIELD
#undef BITMASK
#undef MATRIXFIELD
