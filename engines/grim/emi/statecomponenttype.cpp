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

#include "engines/grim/emi/statecomponenttype.h"

namespace Grim {

StateComponentType::StateComponentType(const char *name) : ComponentType(name) {
}

bool StateComponentType::valueToString(float value, LString &result) {
	for (uint i = 0; i < valueNames.size(); i++) {
		if (valueNames[i].value == value) {
			result = valueNames[i].name;

			return true;
		}
	}

	result = "UNKN";

	return false;
}

float StateComponentType::stringToValue(const char *string) {
	for (uint i = 0; i < valueNames.size(); i++) {
		if (valueNames[i].name == string)
			return valueNames[i].value;
	}

	return 0.0f;
}

void StateComponentType::addValue(float value, const char *valueName) {
	uint position = 0;

	while (position < valueNames.size() && valueNames[position].value < value)
		position++;

	if (position < valueNames.size() && !(value < valueNames[position].value)) {
		valueNames[position].name = valueName;

		return;
	}

	ValueName entry;

	entry.value = value;
	entry.name = valueName;
	valueNames.insert_at(position, entry);
}

} // End of namespace Grim
