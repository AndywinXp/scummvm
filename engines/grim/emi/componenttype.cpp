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

#include "engines/grim/emi/componenttype.h"

namespace Grim {

ComponentType::ComponentType(const char *name) {
	getRegistry()[LString(name)] = this;
}

ComponentType::~ComponentType() {
}

bool ComponentType::valueToString(float value, LString &result) {
	return false;
}

float ComponentType::stringToValue(const char *string) {
	return 0.0f;
}

ComponentType::TypeMap &ComponentType::getRegistry() {
	static TypeMap registry;

	return registry;
}

} // End of namespace Grim
