#include "QvSpotLight.h"
#include "QvBasic.h"

QV_NODE_SOURCE(QvSpotLight);

QvSpotLight::QvSpotLight()
{
	QV_NODE_CONSTRUCTOR(QvSpotLight);
	isBuiltIn = TRUE;

	QV_NODE_ADD_FIELD(on);
	QV_NODE_ADD_FIELD(intensity);
	QV_NODE_ADD_FIELD(color);
	QV_NODE_ADD_FIELD(location);
	QV_NODE_ADD_FIELD(direction);
	QV_NODE_ADD_FIELD(dropOffRate);
	QV_NODE_ADD_FIELD(cutOffAngle);

	on.value = TRUE;
	intensity.value = 1.0f;
	color.value[0] = color.value[1] = color.value[2] = 1.0f;
	location.value[0] = 0.0f;
	location.value[1] = 0.0f;
	location.value[2] = 1.0f;
	direction.value[0] = 0.0f;
	direction.value[1] = 0.0f;
	direction.value[2] = -1.0f;
	dropOffRate.value = 0.0f;
	cutOffAngle.value = (float)(M_PI / 4.0);
}

QvSpotLight::~QvSpotLight()
{
}
