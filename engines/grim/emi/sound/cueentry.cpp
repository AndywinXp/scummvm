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

#include "engines/grim/emi/sound/cueentry.h"
#include "engines/grim/emi/core/lmemoryservices.h"

namespace Grim {

CueEntry::CueEntry(const char *description, const int *id, const int *sync, CueEntry *next, bool isEpisode) {
	_stream.reset();

	this->_id = *id;
	this->_description = description;
	this->_sync = *sync;
	this->_isEpisode = isEpisode;
	_lastPositionMs = 0;
	_looped = false;
	_ended = false;
	_fadeRate = isEpisode ? 1.0f : 0.5f;
	this->_next = next;
	_fadeVolume = 0.0f;
	_state = 0;
}

void *CueEntry::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void CueEntry::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

} // End of namespace Grim
