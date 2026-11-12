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

#include "engines/grim/emi/input/keyeventmapper.h"

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

class EmiEngine;

class SputControl_v2 {
public:
	enum {
		kNumControls = 137
	};

	explicit SputControl_v2(EmiEngine *engine);
	~SputControl_v2();

	int startup();
	void shutdown();
	void clearAll();
	void enableControl(uint index);
	void disableControl(uint index);
	bool open();
	bool close();
	void update();
	void clearKeyStates();
	static bool isModifierKey(int vk);

	ControlEntry _controlTable[kNumControls];

private:
	static const ControlEntry kPcControlTable[kNumControls];
	static const ControlEntry kPs2ControlTable[kNumControls];
	static const KeyBinding kDefaultBinding;

	EmiEngine *_engine;
	KeyEventMapper *_keyEventMapper;
	bool _started;
	bool _opened;
};

} // End of namespace Grim

#endif
