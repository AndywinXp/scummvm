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

#include "common/endian.h"
#include "common/textconsole.h"

#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2.h"

namespace Topgun {

int32 TopgunEngine_v1_2::roundedDivideBy10000(int32 value) {
	if (value > 0)
		return divideSigned32(value + TG_SCRIPT_V1_2_ANGLE_ROUNDING, TG_SCRIPT_V1_2_ANGLE_RESULT_SCALE);

	return divideSigned32(value - TG_SCRIPT_V1_2_ANGLE_ROUNDING, TG_SCRIPT_V1_2_ANGLE_RESULT_SCALE);
}

Common::Point TopgunEngine_v1_2::computeRayRectIntersection(int16 angle, int16 x, int16 y, int16 left, int16 top, int16 right, int16 bottom) {
	if (angle == 0)
		return Common::Point(x, bottom);

	if (angle == TG_UTILS_ANGLE_QUARTER_TURN)
		return Common::Point(right, y);

	if (angle == TG_UTILS_ANGLE_HALF_TURN)
		return Common::Point(x, top);

	if (angle == TG_UTILS_ANGLE_THREE_QUARTER_TURN)
		return Common::Point(left, y);

	int16 quadrant = utilsLibrary()->getQuadrant(angle);
	int16 edgeX = (quadrant > 1) ? left : right;
	int16 edgeY = (quadrant == 1 || quadrant == 2) ? top : bottom;
	int16 deltaY = (int16)(edgeY - y);
	int16 deltaX = (int16)(edgeX - x);

	if (deltaX == 0 && deltaY == 0)
		return Common::Point(x, y);

	int16 cornerAngle = utilsLibrary()->getAngle(deltaX, deltaY);

	if (cornerAngle == angle)
		return Common::Point(edgeX, edgeY);

	bool verticalEdge = (cornerAngle < angle);
	int16 reducedAngle = (int16)(angle % TG_UTILS_ANGLE_QUARTER_TURN);
	int16 remainder = reducedAngle;

	if (quadrant == 1 || quadrant == 3) {
		verticalEdge = !verticalEdge;

		if (!verticalEdge)
			reducedAngle = (int16)(TG_UTILS_ANGLE_QUARTER_TURN - remainder);

		deltaY = (int16)-deltaY;
		deltaX = (int16)-deltaX;
	} else if (verticalEdge) {
		reducedAngle = (int16)(TG_UTILS_ANGLE_QUARTER_TURN - remainder);
	}

	int32 tangent = utilsLibrary()->tangent(reducedAngle);

	if (verticalEdge) {
		int32 offset = roundedDivideBy10000((int32)((uint32)(int32)deltaX * (uint32)tangent));

		return Common::Point(edgeX, (int16)(offset + y));
	}

	int32 offset = roundedDivideBy10000((int32)((uint32)(int32)deltaY * (uint32)tangent));

	return Common::Point((int16)(offset + x), edgeY);
}

void TopgunEngine_v1_2::o_getAngle(const byte *record) {
	int16 firstX = (int16)operandValue(READ_LE_UINT16(record + 4));
	int16 firstY = (int16)operandValue(READ_LE_UINT16(record + 6));
	int16 secondX = (int16)operandValue(READ_LE_UINT16(record + 8));
	int16 secondY = (int16)operandValue(READ_LE_UINT16(record + 10));

	writeVariable(READ_LE_UINT16(record + 2), (uint16)utilsLibrary()->getAngle((int16)(secondX - firstX), (int16)(secondY - firstY)));
}

void TopgunEngine_v1_2::o_clipLineToRect(const byte *record) {
	int16 left = (int16)operandValue(READ_LE_UINT16(record + 12));
	int16 right = (int16)operandValue(READ_LE_UINT16(record + 16));

	if (right < left) {
		int16 swap = left;
		left = right;
		right = swap;
	}

	int16 top = (int16)operandValue(READ_LE_UINT16(record + 14));
	int16 bottom = (int16)operandValue(READ_LE_UINT16(record + 18));

	if (bottom < top) {
		int16 swap = top;
		top = bottom;
		bottom = swap;
	}

	int16 x = (int16)operandValue(READ_LE_UINT16(record + 6));
	if (x < left)
		x = left;
	else if (x > right)
		x = right;

	int16 y = (int16)operandValue(READ_LE_UINT16(record + 8));
	if (y < top)
		y = top;
	else if (y > bottom)
		y = bottom;

	int16 angle = utilsLibrary()->normalizeAngle((int16)operandValue(READ_LE_UINT16(record + 10)));
	Common::Point end = computeRayRectIntersection(angle, x, y, left, top, right, bottom);

	if (end.x < left)
		end.x = left;
	else if (end.x > right)
		end.x = right;

	if (end.y < top)
		end.y = top;
	else if (end.y > bottom)
		end.y = bottom;

	writeVariable(READ_LE_UINT16(record + 2), (uint16)end.x);
	writeVariable(READ_LE_UINT16(record + 4), (uint16)end.y);
}

void TopgunEngine_v1_2::o_getIntersect(const byte *record) {
	int16 x1 = (int16)operandValue(READ_LE_UINT16(record + 6));
	int16 y1 = (int16)operandValue(READ_LE_UINT16(record + 8));
	int16 x2 = (int16)operandValue(READ_LE_UINT16(record + 10));
	int16 y2 = (int16)operandValue(READ_LE_UINT16(record + 12));
	int16 x3 = (int16)operandValue(READ_LE_UINT16(record + 14));
	int16 y3 = (int16)operandValue(READ_LE_UINT16(record + 16));
	int16 x4 = (int16)operandValue(READ_LE_UINT16(record + 18));
	int16 y4 = (int16)operandValue(READ_LE_UINT16(record + 20));
	Common::Point intersection;

	if (utilsLibrary()->getIntersect(&intersection, x1, y1, x2, y2, x3, y3, x4, y4) != 0) {
		writeVariable(READ_LE_UINT16(record + 2), (uint16)intersection.x);
		writeVariable(READ_LE_UINT16(record + 4), (uint16)intersection.y);
	}
}

void TopgunEngine_v1_2::o_sineCosine(const byte *record) {
	int16 angle = (int16)operandValue(READ_LE_UINT16(record + 4));
	int16 result;

	if (READ_LE_UINT16(record) == kOpV1_2Sine)
		result = utilsLibrary()->sine(angle);
	else
		result = utilsLibrary()->cosine(angle);

	writeVariable(READ_LE_UINT16(record + 2), (uint16)result);
}

} // End of namespace Topgun
