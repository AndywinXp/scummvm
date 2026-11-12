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

#ifndef GRIM_EMI_KEYEVENTMAPPER_H
#define GRIM_EMI_KEYEVENTMAPPER_H

#include "common/array.h"
#include "common/scummsys.h"

#include "engines/grim/emi/eventhandler.h"

namespace Grim {

struct KeyBinding {
	enum {
		kOnKeyUp = 0x1,
		kOnKeyDown = 0x2,
		kIgnoreRepeat = 0x8
	};

	int reserved0;
	uint flags;
	int reserved8;
};

class EmiEngine;

class KeyEventMapper : public EventHandler {
public:
	explicit KeyEventMapper(EmiEngine *engine);
	~KeyEventMapper() override;

	bool handleEvent(const Event *event) override;
	void bindKey(int vk, const KeyBinding *binding);

private:
	struct BindingEntry {
		int vk;
		const KeyBinding *binding;
	};

	Common::Array<BindingEntry> _bindings;
};

} // End of namespace Grim

#endif
