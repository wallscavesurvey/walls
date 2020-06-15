//<file>
//
// Name:        wrltraverse.C
//
// Purpose:     sample traversal of a VRMLScene, prints scene information
//
// Created:     24 Apr 95   taken from QvTraverse
//
// Changed:      9 Aug 95   Michael Pichler
//
//
//</file>


#if defined(PMAX) || defined (HPUX9)
enum Part { goofyPart };        // cfront confused about QvCone::Part and QvCylinder::Part
enum Binding { goofyBinding };  // cfront confused about QvMaterialBinding/QvNormalBinding::Binding
#endif


#include "qvlib/QvElement.h"
#include "qvlib/QvNodes.h"
#include "qvlib/QvExtensions.h"
#include "qvlib/QvUnknownNode.h"  /* mpichler */
#include "qvlib/QvState.h"

//////////////////////////////////////////////////////////////////////////////
//
// Traversal code for all nodes. The default method (in QvNode) does
// nothing. Because traverse() is defined in the header for ALL node
// classes, each one has an implementation here.
//
//////////////////////////////////////////////////////////////////////////////

// For debugging
static int indent = 0;
static void
announce(const char *className)
{
	for (int i = 0; i < indent; i++)
		printf("\t");
	printf("Traversing a %s\n", className);
}
#define ANNOUNCE(className) announce(QV__QUOTE(className))

#define DEFAULT_TRAVERSE(className)					      \
void									      \
className::traverse(QvState *)						      \
{									      \
    ANNOUNCE(className);						      \
}


// Extensions
DEFAULT_TRAVERSE(QvLabel)
DEFAULT_TRAVERSE(QvLightModel)


//////////////////////////////////////////////////////////////////////////////
//
// Groups.
//
//////////////////////////////////////////////////////////////////////////////

void
QvGroup::traverse(QvState *state)
{
	ANNOUNCE(QvGroup);
	indent++;
	for (int i = 0; i < getNumChildren(); i++)
		getChild(i)->traverse(state);
	indent--;
}

void
QvLOD::traverse(QvState *state)
{
	ANNOUNCE(QvLOD);
	indent++;

	// ??? In a real implementation, this would choose a child based
	// ??? on the distance to the eye point.
	if (getNumChildren() > 0)
		getChild(0)->traverse(state);

	indent--;
}

void
QvSeparator::traverse(QvState *state)
{
	ANNOUNCE(QvSeparator);
	state->push();
	indent++;
	for (int i = 0; i < getNumChildren(); i++)
		getChild(i)->traverse(state);
	indent--;
	state->pop();
}

void
QvSwitch::traverse(QvState *state)
{
	ANNOUNCE(QvSwitch);
	indent++;

	int which = (int)whichChild.value;

	if (which == QV_SWITCH_NONE)
		;

	else if (which == QV_SWITCH_ALL)
		for (int i = 0; i < getNumChildren(); i++)
			getChild(i)->traverse(state);

	else
		if (which < getNumChildren())
			getChild(which)->traverse(state);

	indent--;
}

void
QvTransformSeparator::traverse(QvState *state)
{
	ANNOUNCE(QvTransformSeparator);

	// We need to "push" just the transformation stack. We'll
	// accomplish this by just pushing a no-op transformation onto
	// that stack. When we "pop", we'll restore that stack to its
	// previous state.

	QvElement *markerElt = new QvElement;
	markerElt->data = this;
	markerElt->type = QvElement::NoOpTransform;
	state->addElement(QvState::TransformationIndex, markerElt);

	indent++;
	for (int i = 0; i < getNumChildren(); i++)
		getChild(i)->traverse(state);
	indent--;

	// Now do the "pop"
	while (state->getTopElement(QvState::TransformationIndex) != markerElt)
		state->popElement(QvState::TransformationIndex);
}

//////////////////////////////////////////////////////////////////////////////
//
// Properties.
//
//////////////////////////////////////////////////////////////////////////////

#define DO_PROPERTY(className, stackIndex)				      \
void									      \
className::traverse(QvState *state)					      \
{									      \
    ANNOUNCE(className);						      \
    QvElement *elt = new QvElement;					      \
    elt->data = this;							      \
    state->addElement(QvState::stackIndex, elt);			      \
}

#define DO_TYPED_PROPERTY(className, stackIndex, eltType)		      \
void									      \
className::traverse(QvState *state)					      \
{									      \
    ANNOUNCE(className);						      \
    QvElement *elt = new QvElement;					      \
    elt->data = this;							      \
    elt->type = QvElement::eltType;					      \
    state->addElement(QvState::stackIndex, elt);			      \
}

DO_PROPERTY(QvCoordinate3, Coordinate3Index)
DO_PROPERTY(QvFontStyle, FontStyleIndex)
DO_PROPERTY(QvMaterial, MaterialIndex)
DO_PROPERTY(QvMaterialBinding, MaterialBindingIndex)
DO_PROPERTY(QvNormal, NormalIndex)
DO_PROPERTY(QvNormalBinding, NormalBindingIndex)
DO_PROPERTY(QvShapeHints, ShapeHintsIndex)
DO_PROPERTY(QvTextureCoordinate2, TextureCoordinate2Index)
DO_PROPERTY(QvTexture2, Texture2Index)
DO_PROPERTY(QvTexture2Transform, Texture2TransformationIndex)

DO_TYPED_PROPERTY(QvDirectionalLight, LightIndex, DirectionalLight)
DO_TYPED_PROPERTY(QvPointLight, LightIndex, PointLight)
DO_TYPED_PROPERTY(QvSpotLight, LightIndex, SpotLight)

DO_TYPED_PROPERTY(QvOrthographicCamera, CameraIndex, OrthographicCamera)
DO_TYPED_PROPERTY(QvPerspectiveCamera, CameraIndex, PerspectiveCamera)

DO_TYPED_PROPERTY(QvTransform, TransformationIndex, Transform)
DO_TYPED_PROPERTY(QvRotation, TransformationIndex, Rotation)
DO_TYPED_PROPERTY(QvMatrixTransform, TransformationIndex, MatrixTransform)
DO_TYPED_PROPERTY(QvTranslation, TransformationIndex, Translation)
DO_TYPED_PROPERTY(QvScale, TransformationIndex, Scale)

//////////////////////////////////////////////////////////////////////////////
//
// Shapes.
//
//////////////////////////////////////////////////////////////////////////////

static void
printProperties(QvState *state)
{
	printf("--------------------------------------------------------------\n");
	state->print();
	printf("--------------------------------------------------------------\n");
}

#define DO_SHAPE(className)						      \
void									      \
className::traverse(QvState *state)					      \
{									      \
    ANNOUNCE(className);						      \
    printProperties(state);						      \
}

DO_SHAPE(QvAsciiText)
DO_SHAPE(QvCone)
DO_SHAPE(QvCube)
DO_SHAPE(QvCylinder)
DO_SHAPE(QvIndexedFaceSet)
DO_SHAPE(QvIndexedLineSet)
DO_SHAPE(QvPointSet)
DO_SHAPE(QvSphere)

//////////////////////////////////////////////////////////////////////////////
//
// WWW-specific nodes.
//
//////////////////////////////////////////////////////////////////////////////

// ???
DEFAULT_TRAVERSE(QvWWWAnchor)
DEFAULT_TRAVERSE(QvWWWInline)

//////////////////////////////////////////////////////////////////////////////
//
// Default traversal methods. These nodes have no effects during traversal.
//
//////////////////////////////////////////////////////////////////////////////

DEFAULT_TRAVERSE(QvInfo)
DEFAULT_TRAVERSE(QvUnknownNode)

//////////////////////////////////////////////////////////////////////////////

#undef ANNOUNCE
#undef DEFAULT_TRAVERSE
#undef DO_PROPERTY
#undef DO_SHAPE
#undef DO_TYPED_PROPERTY
