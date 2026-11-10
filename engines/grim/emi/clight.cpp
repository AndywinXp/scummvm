/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <math.h>

#include "common/scummsys.h"

#include "engines/grim/emi/clight.h"

namespace Grim {

static const float kMinLength = 1e-20f;
static const float kNoLight = -1.0f;

CLight::CLight(Color lightColor, float lightIntensity, float lightFocusDistance, float lightSpreadDistance) {
	type = kTypeOmni;
	color = lightColor;
	spreadDistance = lightSpreadDistance;
	intensity = lightIntensity;
	umbraAngle = 0.0f;
	penumbraAngle = 0.0f;
	focusDistance = lightFocusDistance;
}

void CLight::setColor(int newR, int newG, int newB) {
	color.r = newR;
	color.g = newG;
	color.b = newB;
}

void CLight::addLighting(Color *accumulator, const float *lightTransform, const float *position, const float *normal) {
	double factor = evaluate(lightTransform, position, normal);
	float factorStored = (float)factor;

	if (factor >= 0.0) {
		float addedR = (float)((double)(uint)color.r * factorStored);

		accumulator->r += (int)addedR;

		float addedG = (float)((double)(uint)color.g * factorStored);

		accumulator->g += (int)addedG;

		float addedB = (float)((double)(uint)color.b * factorStored);

		accumulator->b += (int)addedB;
	}
}

float CLight::evaluate(const float *lightTransform, const float *position, const float *normal) {
	// The original could divide by zero in this function, let's avoid that...
	switch (type) {
	case kTypeOmni: {
		float dirX = lightTransform[9] - position[0];
		float dirY = lightTransform[10] - position[1];
		float dirZ = lightTransform[11] - position[2];
		double distance = sqrt((double)dirZ * dirZ + (double)dirY * dirY + (double)dirX * dirX);

		if (distance >= (double)kMinLength) {
			double inverse = 1.0 / distance;

			dirX = (float)(dirX * inverse);
			dirY = (float)(dirY * inverse);
			dirZ = (float)(dirZ * inverse);
		} else {
			distance = 0.0;
		}

		float distanceStored = (float)distance;

		if (distance > spreadDistance)
			return kNoLight;

		double dotProduct = (double)dirY * normal[1] + (double)dirZ * normal[2] + (double)dirX * normal[0];

		if (dotProduct < 0.0)
			return kNoLight;

		if (distanceStored > (double)focusDistance && spreadDistance != focusDistance)
			dotProduct = dotProduct * (((double)spreadDistance - distanceStored) / ((double)spreadDistance - focusDistance));

		return (float)(dotProduct * intensity);
	}
	case kTypeSpot: {
		float axisX = -lightTransform[6];
		float axisY = -lightTransform[7];
		float axisZ = -lightTransform[8];
		double dirX = (double)lightTransform[9] - position[0];
		float dirYStored = (float)((double)lightTransform[10] - position[1]);
		double dirZ = (double)lightTransform[11] - position[2];
		float distance = (float)sqrt((double)dirYStored * dirYStored + dirZ * dirZ + dirX * dirX);

		if (distance > (double)spreadDistance)
			return kNoLight;

		if (distance == 0.0f)
			return kNoLight;

		double inverse = 1.0 / distance;
		float unitX = (float)(inverse * dirX);
		float unitY = (float)(dirYStored * inverse);
		double unitZ = dirZ * inverse;
		double cosine = unitZ * normal[2] + (double)unitY * normal[1] + (double)unitX * normal[0];
		float cosineStored = (float)cosine;

		if (cosine < 0.0)
			return kNoLight;

		float angleCosine = (float)(unitZ * axisZ + (double)unitY * axisY + (double)axisX * unitX);
		float angle = (float)acos(angleCosine);

		if (angle > penumbraAngle)
			return kNoLight;

		double result;

		if (angle <= (double)umbraAngle)
			result = cosineStored;
		else if (penumbraAngle != umbraAngle)
			result = ((double)penumbraAngle - angle) / ((double)penumbraAngle - umbraAngle) * cosineStored;
		else
			result = cosineStored;

		if (distance > (double)focusDistance && spreadDistance != focusDistance)
			result = result * (((double)spreadDistance - distance) / ((double)spreadDistance - focusDistance));

		return (float)(result * intensity);
	}
	case kTypeDirect: {
		double result = (double)-lightTransform[8] * normal[2] + (double)-lightTransform[7] * normal[1] + (double)-lightTransform[6] * normal[0];

		if (result >= 0.0)
			return (float)(result * intensity);

		return kNoLight;
	}
	case kTypeAmbient:
		return intensity;
	default:
		return kNoLight;
	}
}

bool CLight::isOutOfRange(const float *lightTransform, const BBox *box) {
	if (type != kTypeOmni && type != kTypeSpot)
		return false;

	double squaredDistance = 0.0;
	float posX = lightTransform[9];
	float posY = lightTransform[10];
	float posZ = lightTransform[11];

	if (posX >= box->min.x) {
		if (posX > box->max.x) {
			double delta = (double)posX - box->max.x;

			squaredDistance = delta * delta;
		}
	} else {
		double delta = (double)posX - box->min.x;

		squaredDistance = delta * delta;
	}

	if (posY >= box->min.y) {
		if (posY > box->max.y) {
			double delta = (double)posY - box->max.y;

			squaredDistance = squaredDistance + delta * delta;
		}
	} else {
		double delta = (double)posY - box->min.y;

		squaredDistance = squaredDistance + delta * delta;
	}

	if (posZ >= box->min.z) {
		if (posZ > box->max.z) {
			double delta = (double)posZ - box->max.z;

			squaredDistance = squaredDistance + delta * delta;
		}
	} else {
		double delta = (double)posZ - box->min.z;

		squaredDistance = squaredDistance + delta * delta;
	}

	return squaredDistance > (double)spreadDistance * spreadDistance;
}

} // End of namespace Grim
