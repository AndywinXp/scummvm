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

#ifndef GRIM_LEGACY_SAVEGAMES_CONTROLS_H
#define GRIM_LEGACY_SAVEGAMES_CONTROLS_H

#include "common/str.h"

namespace Grim {

struct ControlSymbol;

// The restored Lua state holds the key numbers of the legacy engine (system.controls, the key globals of
// _controls.lua, the keys of control_map and of the axis tables); this gives them the numbers of the
// symbol list. The system table is the ref of the running engine. Returns a summary for the log.
Common::String legacyRemapControls(int systemRef, const ControlSymbol *aSymbols, int numSymbols);

} // End of namespace Grim

#endif
