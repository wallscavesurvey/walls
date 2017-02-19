#include "QvRotation.h"

QV_NODE_SOURCE(QvRotation);

QvRotation::QvRotation()
{
    QV_NODE_CONSTRUCTOR(QvRotation);
    isBuiltIn = TRUE;

    QV_NODE_ADD_FIELD(rotation);

    rotation.axis[0] = 0.0f;
    rotation.axis[1] = 0.0f;
    rotation.axis[2] = 1.0f;
    rotation.angle = 0.0f;
}

QvRotation::~QvRotation()
{
}
