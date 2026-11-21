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

#ifndef TOPGUN_LIBRARIES_AUDIO_WAVEOUT_H
#define TOPGUN_LIBRARIES_AUDIO_WAVEOUT_H

#include "audio/audiostream.h"
#include "common/scummsys.h"

namespace Topgun {

struct WaveRingEntry {
	byte *data;
	uint32 length;
	uint32 flags;
	bool busy;

	WaveRingEntry() : data(nullptr), length(0), flags(0), busy(false) {}
};

class WaveOutStream : public Audio::AudioStream {
public:
	enum {
		kRingSize = 4,
		kHeaderDone = 0x01,
		kHeaderBeginLoop = 0x04,
		kHeaderEndLoop = 0x08,
		kHeaderInQueue = 0x10
	};

	WaveOutStream(WaveRingEntry *ring, uint32 rate, bool stereo, bool eightBit);

	int readBuffer(int16 *buffer, const int numSamples) override;
	bool isStereo() const override { return _stereo; }
	int getRate() const override { return _rate; }
	bool endOfData() const override;
	bool endOfStream() const override { return false; }

	void resetPlayback();

private:
	WaveRingEntry *_ring;
	uint32 _rate;
	bool _stereo;
	bool _eightBit;
	uint32 _playIndex = 0;
	uint32 _playPos = 0;
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_AUDIO_WAVEOUT_H
