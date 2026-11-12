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
	musicStream = nullptr;
}

CueStream::~CueStream() {
	if (musicStream) {
		delete musicStream;
		musicStream = nullptr;
	}
}

void CueStream::reset() {
	for (int i = 0; i < 2; i++) {
		jumps[i].srcMs = -1.0;
		jumps[i].dstMs = 0.0;
		jumps[i].field16 = 0;
	}

	musicStream = nullptr;
	paused = 0;
	ratio = 1.0;
	invRatio = 1.0;
	pcmBytesPerMs = 0.0;
	invPcmBytesPerMs = 0.0;
	startByte = 0;
	srcByte = -1;
	dstByte = 0;
	loopCount = 1;
	fileName.clear();
	trim = 1.0f;
	field43c = 0;
	start = 0.0;
	end = -1.0;
	hasJump = 0;
}

} // End of namespace Grim
