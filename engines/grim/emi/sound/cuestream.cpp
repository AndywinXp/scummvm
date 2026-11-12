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

#include "engines/grim/emi/sound/cuestream.h"
#include "engines/grim/emi/sound/musicstream.h"

namespace Grim {

CueStream::CueStream() {
	_musicStream = nullptr;
}

CueStream::~CueStream() {
	if (_musicStream) {
		delete _musicStream;
		_musicStream = nullptr;
	}
}

void CueStream::reset() {
	for (int i = 0; i < 2; i++) {
		_jumps[i].srcMs = -1.0;
		_jumps[i].dstMs = 0.0;
		_jumps[i].field16 = 0;
	}

	_musicStream = nullptr;
	_paused = 0;
	_ratio = 1.0;
	_invRatio = 1.0;
	_pcmBytesPerMs = 0.0;
	_invPcmBytesPerMs = 0.0;
	_startByte = 0;
	_srcByte = -1;
	_dstByte = 0;
	_loopCount = 1;
	_fileName.clear();
	_trim = 1.0f;
	_field43c = 0;
	_start = 0.0;
	_end = -1.0;
	_hasJump = 0;
}

} // End of namespace Grim
