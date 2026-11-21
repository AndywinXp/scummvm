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

#include "topgun/topgun_v0.h"

namespace Topgun {

SceneRecord *TopgunEngine_v0::findOrCreateSceneEntry(const Common::String &name) {
	warning("findOrCreateSceneEntry UNIMPLEMENTED");
	return nullptr;
}

bool TopgunEngine_v0::loadSceneContainerAndRunEntryScript(const Common::String &name, bool withProgressCallback) {
	warning("loadSceneContainerAndRunEntryScript UNIMPLEMENTED");
	return false;
}

void TopgunEngine_v0::switchToNextScene(const Common::String &name) {
	warning("switchToNextScene UNIMPLEMENTED");
}

bool TopgunEngine_v0::loadInitialScene(const Common::String &name, bool runMemoryCheck) {
	warning("loadInitialScene UNIMPLEMENTED");
	return false;
}

void TopgunEngine_v0::cleanupCurrentSceneAndContainer() {
	warning("cleanupCurrentSceneAndContainer UNIMPLEMENTED");
}

} // End of namespace Topgun
