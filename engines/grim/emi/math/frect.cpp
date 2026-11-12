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

#include "engines/grim/emi/math/frect.h"

namespace Grim {

FRect::FRect() {
	_x = 0.0f;
	_y = 0.0f;
	_w = 0.0f;
	_h = 0.0f;
}

float FRect::getX() {
	return _x;
}

float FRect::getY() {
	return _y;
}

float FRect::getWidth() {
	return _w;
}

float FRect::getHeight() {
	return _h;
}

void FRect::setX(float newX) {
	_x = newX;
}

void FRect::setY(float newY) {
	_y = newY;
}

void FRect::setWidth(float newWidth) {
	_w = newWidth;
}

void FRect::setHeight(float newHeight) {
	_h = newHeight;
}

} // End of namespace Grim
