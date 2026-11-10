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

#include "engines/grim/emi/animcomponenttype.h"

namespace Grim {

AnimComponentType AnimComponentType::global;

AnimComponentType::AnimComponentType() : StateComponentType("anim") {
	addValue(0.0f, "STOP");
	addValue(1.0f, "PLAY");
	addValue(2.0f, "PAUSE");
	addValue(3.0f, "LOOP");
	addValue(4.0f, "NLOOP");
	addValue(5.0f, "FI1");
	addValue(6.0f, "FI3/4");
	addValue(7.0f, "FI1/2");
	addValue(8.0f, "FI1/4");
	addValue(9.0f, "FI1/8");
	addValue(10.0f, "FO1");
	addValue(11.0f, "FO3/4");
	addValue(12.0f, "FO1/2");
	addValue(13.0f, "FO1/4");
	addValue(14.0f, "FO1/8");
}

} // End of namespace Grim
