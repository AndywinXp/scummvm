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

#include "engines/grim/emi/math/cplane.h"

namespace Grim {

static const float kMinLength = 1e-20f;
static const float kPlaneTolerance = 1e-4f;

void CPlane::setFromNormalAndPoint(const CVector &normal, const CVector &point) {
	n.x = normal.x;
	n.y = normal.y;
	n.z = normal.z;

	double length = sqrt((double)n.x * n.x + (double)n.y * n.y + (double)normal.z * normal.z);

	if (length >= (double)kMinLength) {
		double inverse = 1.0 / length;

		n.x = (float)(inverse * n.x);
		n.y = (float)(inverse * n.y);
		n.z = (float)(inverse * n.z);
	}

	d = (float)-((double)point.z * n.z + (double)point.y * n.y + (double)point.x * n.x);
}

void CPlane::projectPoint(CVector *point) {
	double distance = (double)point->y * n.y + (double)point->x * n.x + (double)n.z * point->z + d;
	float offsetX = (float)(distance * n.x);
	float offsetY = (float)(distance * n.y);
	double offsetZ = distance * n.z;

	point->x = (float)((double)point->x - offsetX);
	point->y = (float)((double)point->y - offsetY);
	point->z = (float)((double)point->z - offsetZ);
}

int CPlane::classifyPoint(const CVector &point) {
	double distance = (double)point.z * n.z + (double)point.y * n.y + (double)point.x * n.x + d;

	if (distance <= (double)kPlaneTolerance)
		return distance < -(double)kPlaneTolerance ? kPointBehind : kPointOnPlane;

	return kPointInFront;
}

void CPlane::transformToLocal(const CTransform *transform) {
	double negD = -(double)d;
	float centerX = (float)(negD * n.x);
	float centerY = (float)(negD * n.y);
	float localX = (float)((double)centerX - transform->pos.x);
	float localY = (float)((double)centerY - transform->pos.y);
	double localZ = negD * n.z - transform->pos.z;
	float rotatedX = (float)((double)localX * transform->rot.m[0] + localZ * transform->rot.m[2] + (double)localY * transform->rot.m[1]);
	float rotatedY = (float)((double)localX * transform->rot.m[3] + localZ * transform->rot.m[5] + (double)localY * transform->rot.m[4]);
	double rotatedZ = (double)localX * transform->rot.m[6] + localZ * transform->rot.m[8] + (double)localY * transform->rot.m[7];
	double normalX = n.x;
	double normalY = n.y;
	double normalZ = n.z;

	n.x = (float)(normalX * transform->rot.m[0] + normalZ * transform->rot.m[2] + normalY * transform->rot.m[1]);
	n.y = (float)(normalX * transform->rot.m[3] + normalZ * transform->rot.m[5] + normalY * transform->rot.m[4]);
	n.z = (float)(normalX * transform->rot.m[6] + normalZ * transform->rot.m[8] + normalY * transform->rot.m[7]);
	d = (float)-(rotatedZ * n.z + (double)rotatedX * n.x + (double)rotatedY * n.y);
}

} // End of namespace Grim
