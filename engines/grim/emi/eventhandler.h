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

#ifndef GRIM_EMI_EVENTHANDLER_H
#define GRIM_EMI_EVENTHANDLER_H

#include "common/list.h"
#include "common/scummsys.h"

namespace Grim {

struct Event {
	enum {
		kKeyDown = 1,
		kKeyUp = 2
	};

	enum {
		kModifierRepeat = 0x1,
		kModifierShift = 0x10,
		kModifierAlt = 0x20,
		kModifierControl = 0x40,
		kModifierCapsLock = 0x80,
		kModifierWin = 0x100,
		kModifierApps = 0x200
	};

	void *operator new(size_t size);
	void operator delete(void *block);

	int type;
	uint timestamp;
	int vk;
	uint modifiers;
};

class EventHandler {
public:
	EventHandler(int handlerPriority);
	virtual ~EventHandler();

	void *operator new(size_t size);
	void operator delete(void *block);

	virtual bool handleEvent(const Event *event) = 0;

	uint typeMask;
	int priority;

	static Common::List<EventHandler *> handlers;
};

} // End of namespace Grim

#endif
