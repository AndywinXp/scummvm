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

#include "common/textconsole.h"

#include "engines/grim/emi/scene/cclipvolume.h"

namespace Grim {

struct BoxEdge {
	int cornerA;
	int cornerB;
	int faceMask;
	float dir[3];
};

static const BoxEdge kBoxEdges[12] = {
	{ 0, 1, 6, { 0.0f, 0.0f, 1.0f } },
	{ 1, 3, 36, { 0.0f, 1.0f, 0.0f } },
	{ 3, 2, 5, { 0.0f, 0.0f, 1.0f } },
	{ 2, 0, 20, { 0.0f, 1.0f, 0.0f } },
	{ 4, 5, 10, { 0.0f, 0.0f, 1.0f } },
	{ 5, 7, 40, { 0.0f, 1.0f, 0.0f } },
	{ 7, 6, 9, { 0.0f, 0.0f, 1.0f } },
	{ 6, 4, 24, { 0.0f, 1.0f, 0.0f } },
	{ 0, 4, 18, { 1.0f, 0.0f, 0.0f } },
	{ 1, 5, 34, { 1.0f, 0.0f, 0.0f } },
	{ 3, 7, 33, { 1.0f, 0.0f, 0.0f } },
	{ 2, 6, 17, { 1.0f, 0.0f, 0.0f } }
};

static const float kBoxMargin = 0.01f;
static const float kOutsideLimit = 1e-4f;

CClipVolume::CClipVolume() {
	for (int i = 0; i < kMaxPlanes; i++) {
		_planes[i]._n._z = 0.0f;
		_planes[i]._n._y = 0.0f;
		_planes[i]._n._x = 0.0f;
		_planes[i]._d = 0.0f;
		_planeType[i] = 0;
	}

	reset(0);
}

CClipVolume::~CClipVolume() {
}

void CClipVolume::reset(int newNumPlanes) {
	_numPlanes = newNumPlanes;
	_numPoints = 0;
	_planeMask = 0;
}

int CClipVolume::testBox(const BBox *box, const float *transform) {
	float corners[8][3];
	int codes[8];

	float minY = (float)((double)box->_min._y - kBoxMargin);
	float minZ = (float)((double)box->_min._z - kBoxMargin);
	float maxX = (float)((double)box->_max._x + kBoxMargin);
	float maxY = (float)((double)box->_max._y + kBoxMargin);
	float maxZ = (float)((double)box->_max._z + kBoxMargin);
	float minX = (float)((double)box->_min._x - kBoxMargin);

	corners[0][0] = minX;
	corners[0][1] = minY;
	corners[0][2] = minZ;
	corners[1][0] = minX;
	corners[1][1] = minY;
	corners[1][2] = maxZ;
	corners[2][0] = minX;
	corners[2][1] = maxY;
	corners[2][2] = minZ;
	corners[3][0] = minX;
	corners[3][1] = maxY;
	corners[3][2] = maxZ;
	corners[4][0] = maxX;
	corners[4][1] = minY;
	corners[4][2] = minZ;
	corners[5][0] = maxX;
	corners[5][1] = minY;
	corners[5][2] = maxZ;
	corners[6][0] = maxX;
	corners[6][1] = maxY;
	corners[6][2] = minZ;
	corners[7][0] = maxX;
	corners[7][1] = maxY;
	corners[7][2] = maxZ;

	for (int i = 0; i < 8; i++) {
		float x = corners[i][0];
		float y = corners[i][1];
		float z = corners[i][2];
		double partial = (double)transform[3] * y + (double)transform[6] * z;
		float newY = (float)((double)(float)((double)transform[1] * x + (double)transform[4] * y + (double)transform[7] * z) + transform[10]);
		float newZ = (float)((double)(float)((double)z * transform[8] + (double)transform[2] * x + (double)transform[5] * y) + transform[11]);

		corners[i][0] = (float)(partial + (double)x * transform[0] + transform[9]);
		corners[i][1] = newY;
		corners[i][2] = newZ;
	}

	bool anyInside = false;
	int andCodes = -1;
	int orCodes = 0;

	for (int i = 0; i < 8; i++) {
		codes[i] = computeOutcode(corners[i]);

		if (!codes[i])
			anyInside = true;

		andCodes &= codes[i];
		orCodes |= codes[i];
	}

	if (andCodes != 0)
		return kOutside;

	if (orCodes == 0)
		return kInside;

	if (anyInside)
		return kPartial;

	int count = _numPoints;

	if (count > kMaxPoints) {
		warning("CClipVolume::testBox(): invalid number of points %d", count);
		count = kMaxPoints;
	}

	if (count <= 0)
		return kOutside;

	float local[kMaxPoints][3];
	int pointCodes[kMaxPoints];
	int pointAnd = -1;

	for (int i = 0; i < count; i++) {
		double deltaX = (double)_points[i]._x - transform[9];
		float deltaY = (float)((double)_points[i]._y - transform[10]);
		float deltaZ = (float)((double)_points[i]._z - transform[11]);

		local[i][0] = (float)(deltaX * transform[0] + (double)deltaY * transform[1] + (double)deltaZ * transform[2]);
		local[i][1] = (float)((double)deltaZ * transform[5] + (double)deltaY * transform[4] + deltaX * transform[3]);
		local[i][2] = (float)((double)deltaZ * transform[8] + deltaX * transform[6] + (double)deltaY * transform[7]);

		int code = 0;

		if (local[i][0] >= box->_min._x) {
			if (local[i][0] > box->_max._x)
				code = 8;
		} else {
			code = 4;
		}

		if (local[i][1] >= box->_min._y) {
			if (local[i][1] > box->_max._y)
				code |= 1;
		} else {
			code |= 2;
		}

		if (local[i][2] >= box->_min._z) {
			if (local[i][2] > box->_max._z)
				code |= 0x20;
		} else {
			code |= 0x10;
		}

		if (code == 0)
			return kPartial;

		pointAnd &= code;
		pointCodes[i] = code;
	}

	if (pointAnd != 0)
		return kOutside;

	int half = count / 2;

	for (int i = 0; i < half; i++) {
		int previous = i == 0 ? half - 1 : i - 1;

		if (box->segmentCrossesFaces(local[previous], local[i], pointCodes[i] ^ pointCodes[previous]))
			return kPartial;

		if (box->segmentCrossesFaces(local[i], local[half + i], pointCodes[half + i] ^ pointCodes[i]))
			return kPartial;

		if (box->segmentCrossesFaces(local[half + previous], local[half + i], pointCodes[half + i] ^ pointCodes[half + previous]))
			return kPartial;
	}

	for (int e = 0; e < 12; e++) {
		const float *from = corners[kBoxEdges[e].cornerA];
		const float *to = corners[kBoxEdges[e].cornerB];
		int differing = codes[kBoxEdges[e].cornerA] ^ codes[kBoxEdges[e].cornerB];
		uint bit = 1;

		for (int k = 0; k < _numPlanes; k++) {
			if ((bit & differing) != 0) {
				const CPlane &plane = _planes[k];
				double fromDistance = (double)plane._n._z * from[2] + (double)plane._n._y * from[1] + (double)plane._n._x * from[0] + plane._d;
				double toDistance = (double)plane._n._z * to[2] + (double)plane._n._x * to[0] + (double)plane._n._y * to[1] + plane._d;
				double range = fromDistance - toDistance;

				// The original could divide by zero here, let's avoid that...
				if (range != 0.0) {
					double t = fromDistance / range;
					float deltaY = to[1] - from[1];
					float deltaZ = to[2] - from[2];
					float scaledX = (float)(((double)to[0] - from[0]) * t);
					float scaledY = (float)(deltaY * t);
					float point[3];

					point[0] = (float)((double)scaledX + from[0]);
					point[1] = (float)((double)scaledY + from[1]);
					point[2] = (float)(t * deltaZ + from[2]);

					if ((computeOutcode(point) & ~bit) == 0)
						return kPartial;
				}
			}

			bit *= 2;
		}
	}

	return kOutside;
}

int CClipVolume::testSphere(const float *sphere) {
	bool inside = true;
	float radius = sphere[3];

	for (int i = 0; i < _numPlanes; i++) {
		if (((1u << i) & (uint)_planeMask) == 0) {
			double distance = (double)_planes[i]._n._z * sphere[2] + (double)_planes[i]._n._x * sphere[0] + (double)sphere[1] * _planes[i]._n._y + _planes[i]._d;

			if (distance < -(double)radius)
				return kOutside;

			if (distance < radius)
				inside = false;
		}
	}

	return inside ? kInside : kPartial;
}

int CClipVolume::testPoints(const float *testedPoints, int count) {
	int used = count;
	int insideCounts[100];

	if (used > 100)
		used = 100;

	if (used > 0)
		memset(insideCounts, 0, used * sizeof(int));

	for (int i = 0; i < _numPlanes; i++) {
		if (((1u << i) & (uint)_planeMask) != 0)
			continue;

		bool allBehind = true;

		if (used <= 0)
			return kOutside;

		for (int j = 0; j < used; j++) {
			const float *point = testedPoints + j * 3;
			double distance = (double)point[2] * _planes[i]._n._z + (double)point[0] * _planes[i]._n._x + (double)_planes[i]._n._y * point[1] + _planes[i]._d;

			if (distance > -(double)kOutsideLimit)
				allBehind = false;

			if (distance < (double)kOutsideLimit)
				insideCounts[j]++;
		}

		if (allBehind)
			return kOutside;
	}

	for (int j = 0; j < used; j++) {
		if (insideCounts[j] > 0)
			return kPartial;
	}

	return kInside;
}

int CClipVolume::computeOutcode(const float *point) {
	int code = 0;
	uint bit = 1;

	for (int i = 0; i < _numPlanes; i++) {
		if (((uint)_planeMask & bit) == 0) {
			if (-(double)kOutsideLimit > (double)_planes[i]._n._z * point[2] + (double)_planes[i]._n._x * point[0] + (double)point[1] * _planes[i]._n._y + _planes[i]._d)
				code |= bit;

			bit *= 2;
		}
	}

	return code;
}

void CClipVolume::computeVertexOutcodes(int count, DrawVertex **vertices) {
	for (int i = 0; i < count; i++) {
		DrawVertex *vertex = vertices[i];
		int code = 0;
		uint bit = 1;

		for (int j = 0; j < _numPlanes; j++) {
			if (((uint)_planeMask & bit) == 0) {
				if (-(double)kOutsideLimit > (double)_planes[j]._n._z * vertex->z + (double)_planes[j]._n._x * vertex->x + (double)vertex->y * _planes[j]._n._y + _planes[j]._d)
					code |= bit;

				bit *= 2;
			}
		}

		vertex->outcode = code;
	}
}

} // End of namespace Grim
