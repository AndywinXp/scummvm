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

#include "common/system.h"
#include "common/textconsole.h"

#include "topgun/topgun_v0.h"

namespace Topgun {

void TopgunEngine_v0::serviceScheduledMsgTimerQueue() {
	uint16 index = 0;

	if (_numTimers == 0)
		return;

	do {
		uint32 now = g_system->getMillis();

		if (_timers[index].expiry <= now) {
			uint16 script = _timers[index].script;
			removeScheduledTimer(index);

			if (_numTimers > index)
				index--;

			runScriptResource(script);
		}

		index++;
	} while (index < _numTimers);
}

void TopgunEngine_v0::removeScheduledTimer(uint16 index) {
	if (_numTimers == 0 || index >= _numTimers) {
		warning("TopgunEngine_v0::removeScheduledTimer(): timer index %d is out of range", index);
		return;
	}

	_numTimers--;

	uint16 count = _numTimers - index;
	if (count != 0)
		memmove(&_timers[index], &_timers[index + 1], count * sizeof(ScheduledTimerRecord));
}

bool TopgunEngine_v0::runScriptBytecodeLoop(uint16 scriptId) {
	warning("runScriptBytecodeLoop UNIMPLEMENTED");
	return false;
}

void TopgunEngine_v0::spriteMessageService(const byte *record) {
	warning("spriteMessageService UNIMPLEMENTED");
}

bool TopgunEngine_v0::runScriptResource(uint16 scriptId) {
	warning("runScriptResource UNIMPLEMENTED");
	return false;
}

} // End of namespace Topgun
