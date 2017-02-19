#include "QvTransform.h"

QV_NODE_SOURCE(QvTransform);

QvTransform::QvTransform()
{
    QV_NODE_CONSTRUCTOR(QvTransform);
    isBuiltIn = TRUE;

    QV_NODE_ADD_FIELD(translation);
    QV_NODE_ADD_FIELD(rotation);
    QV_NODE_ADD_FIELD(scaleFactor);
    QV_NODE_ADD_FIELD(scaleOrientation);
    QV_NODE_ADD_FIELD(center);

    translation.value[0] = translation.value[1] = translation.value[2] = 0.0f;
    rotation.axis[0] = 0.0f;
    rotation.axis[1] = 0.0f;
    rotation.axis[2] = 1.0f;
    rotation.angle = 0.0f;
    scaleFactor.value[0] = scaleFactor.value[1] = scaleFactor.value[2] = 1.0f;
    scaleOrientation.axis[0] = 0.0f;
    scaleOrientation.axis[1] = 0.0f;
    center.value[0] = center.value[1] = center.value[2] = 0.0f;
}

QvTransform::~QvTransform()
{
}
