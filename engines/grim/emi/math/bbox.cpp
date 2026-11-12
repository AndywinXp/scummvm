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

#include "common/scummsys.h"

#include "engines/grim/emi/math/bbox.h"

namespace Grim {

enum {
	kFaceMaxY = 0x01,
	kFaceMinY = 0x02,
	kFaceMinX = 0x04,
	kFaceMaxX = 0x08,
	kFaceMinZ = 0x10,
	kFaceMaxZ = 0x20
};

BBox::BBox() {
	max.z = 0.0f;
	max.y = 0.0f;
	max.x = 0.0f;
	min.z = 0.0f;
	min.y = 0.0f;
	min.x = 0.0f;
}

void BBox::init(const float *point) {
	min.x = point[0];
	min.y = point[1];
	min.z = point[2];
	max.x = point[0];
	max.y = point[1];
	max.z = point[2];
}

void BBox::expand(const float *point) {
	if (point[0] >= min.x) {
		if (point[0] > max.x)
			max.x = point[0];
	} else {
		min.x = point[0];
	}

	if (point[1] >= min.y) {
		if (point[1] > max.y)
			max.y = point[1];
	} else {
		min.y = point[1];
	}

	if (point[2] >= min.z) {
		if (point[2] > max.z)
			max.z = point[2];
	} else {
		min.z = point[2];
	}
}

bool BBox::segmentCrossesFaces(const float *start, const float *end, int faceMask) const {
	float deltaX = end[0] - start[0];
	float deltaY = end[1] - start[1];
	float deltaZ = end[2] - start[2];

	// The original could divide by zero below, let's avoid that...
	if ((faceMask & (kFaceMinX | kFaceMaxX)) != 0 && deltaX != 0.0f) {
		float inverse = (float)(1.0 / deltaX);

		if ((faceMask & kFaceMinX) != 0) {
			double factor = ((double)min.x - start[0]) * inverse;
			float crossY = (float)(factor * deltaY + start[1]);
			float crossZ = (float)(factor * deltaZ + start[2]);

			if (crossY >= min.y && crossY <= max.y && crossZ >= min.z && crossZ <= max.z)
				return true;
		}

		if ((faceMask & kFaceMaxX) != 0) {
			double factor = ((double)max.x - start[0]) * inverse;
			float crossY = (float)(factor * deltaY + start[1]);
			float crossZ = (float)(factor * deltaZ + start[2]);

			if (crossY >= min.y && crossY <= max.y && crossZ >= min.z && crossZ <= max.z)
				return true;
		}
	}

	if ((faceMask & (kFaceMaxY | kFaceMinY)) != 0 && deltaY != 0.0f) {
		float inverse = (float)(1.0 / deltaY);

		if ((faceMask & kFaceMinY) != 0) {
			double factor = ((double)min.y - start[1]) * inverse;
			float crossX = (float)(factor * deltaX + start[0]);
			float crossZ = (float)(factor * deltaZ + start[2]);

			if (crossX >= min.x && crossX <= max.x && crossZ >= min.z && crossZ <= max.z)
				return true;
		}

		if ((faceMask & kFaceMaxY) != 0) {
			double factor = ((double)max.y - start[1]) * inverse;
			float crossX = (float)(factor * deltaX + start[0]);
			float crossZ = (float)(factor * deltaZ + start[2]);

			if (crossX >= min.x && crossX <= max.x && crossZ >= min.z && crossZ <= max.z)
				return true;
		}
	}

	if ((faceMask & (kFaceMinZ | kFaceMaxZ)) == 0 || deltaZ == 0.0f)
		return false;

	float inverse = (float)(1.0 / deltaZ);

	if ((faceMask & kFaceMinZ) != 0) {
		double factor = ((double)min.z - start[2]) * inverse;
		float crossX = (float)(factor * deltaX + start[0]);
		float crossY = (float)(factor * deltaY + start[1]);

		if (crossX >= min.x && crossX <= max.x && crossY >= min.y && crossY <= max.y)
			return true;
	}

	if ((faceMask & kFaceMaxZ) != 0) {
		double factor = ((double)max.z - start[2]) * inverse;
		float crossX = (float)(factor * deltaX + start[0]);
		float crossY = (float)(factor * deltaY + start[1]);

		if (crossX >= min.x && crossX <= max.x && crossY >= min.y && crossY <= max.y)
			return true;
	}

	return false;
}

} // End of namespace Grim
