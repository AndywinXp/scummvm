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

#ifndef GRIM_EMI_SPUTCONTROL_V2_H
#define GRIM_EMI_SPUTCONTROL_V2_H

#include "common/scummsys.h"

#include "engines/grim/emi/keyeventmapper.h"

namespace Grim {

struct ControlEntry {
	enum {
		kEnabled = 0x1,
		kAxis = 0x2,
		kKey = 0x4,
		kJoystickButton = 0x8
	};

	uint flags;
	uint code;
	byte state;
	float axisValue;
	const char *name;
};

class SputControl_v2 {
public:
	enum {
		kNumControls = 137
	};

	static int startup();
	static void shutdown();
	static void clearAll();
	static void enableControl(uint index);
	static void disableControl(uint index);
	static bool open();
	static bool close();
	static void update();
	static void clearKeyStates();
	static bool isModifierKey(int vk);

	static ControlEntry controlTable[kNumControls];
	static const KeyBinding defaultBinding;

private:
	static KeyEventMapper *_keyEventMapper;
	static bool _started;
	static bool _opened;
};

} // End of namespace Grim

#endif
