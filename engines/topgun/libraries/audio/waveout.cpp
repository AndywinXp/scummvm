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

#include "common/endian.h"
#include "common/util.h"

#include "topgun/libraries/audio/waveout.h"

namespace Topgun {

WaveOutStream::WaveOutStream(WaveRingEntry *ring, uint32 rate, bool stereo, bool eightBit) :
	_ring(ring), _rate(rate), _stereo(stereo), _eightBit(eightBit) {
}

int WaveOutStream::readBuffer(int16 *buffer, const int numSamples) {
	int done = 0;

	while (done < numSamples) {
		WaveRingEntry &entry = _ring[_playIndex];

		if (!(entry.flags & kHeaderInQueue) || (entry.flags & kHeaderDone))
			break;

		uint32 totalValues = entry.length / (_eightBit ? 1 : 2);
		uint32 count = MIN<uint32>(numSamples - done, totalValues - _playPos);

		for (uint32 i = 0; i < count; i++) {
			if (_eightBit)
				buffer[done + i] = (int16)(((int)entry.data[_playPos + i] - 128) << 8);
			else
				buffer[done + i] = READ_LE_INT16(entry.data + (_playPos + i) * 2);
		}

		done += count;
		_playPos += count;

		if (_playPos >= totalValues) {
			entry.flags = (entry.flags & ~kHeaderInQueue) | kHeaderDone;
			_playIndex = (_playIndex + 1) % kRingSize;
			_playPos = 0;
		}
	}

	return done;
}

bool WaveOutStream::endOfData() const {
	const WaveRingEntry &entry = _ring[_playIndex];

	return !(entry.flags & kHeaderInQueue) || (entry.flags & kHeaderDone);
}

void WaveOutStream::resetPlayback() {
	_playIndex = 0;
	_playPos = 0;
}

} // End of namespace Topgun
