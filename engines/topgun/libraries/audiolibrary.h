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

#ifndef TOPGUN_LIBRARIES_AUDIOLIBRARY_H
#define TOPGUN_LIBRARIES_AUDIOLIBRARY_H

#include "common/scummsys.h"
#include "common/str.h"

namespace Topgun {

class TopgunEngine;

struct WaveFormat {
	uint16 tag;
	uint16 channels;
	uint32 sampleRate;
	uint32 averageBytesPerSecond;
	uint16 blockAlign;
	uint16 bitsPerSample;

	WaveFormat() : tag(0), channels(0), sampleRate(0), averageBytesPerSecond(0), blockAlign(0), bitsPerSample(0) {}
};

class AudioLibrary {
public:
	AudioLibrary(TopgunEngine *engine);
	~AudioLibrary();

	uint16 audioInitLibrary(uint16 version);
	void audioExitLibrary();
	void audioService();
	uint32 audioCommand(uint16 command, uint16 handle, uint32 value, void *block);
	uint16 audioActivate(uint16 active);
	uint32 audioResampleWave(const WaveFormat *destinationFormat, const WaveFormat *sourceFormat,
	                         uint32 sourceBytes, byte *destinationBuffer, const byte *sourceBuffer);
	byte *waveLoadFile(const Common::String &fileName, WaveFormat *outFormat, uint32 *outSize);
	bool waveSaveFile(const Common::String &fileName, const WaveFormat *format,
	                  const byte *data, uint32 size);
	uint16 wavePlayFile(const Common::String &fileName, uint16 flags);
	uint16 audioSetOwner(uint16 owner);

private:
	TopgunEngine *_engine;
	uint16 _initCount = 0;
	uint16 _owner = 0;
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_AUDIOLIBRARY_H
