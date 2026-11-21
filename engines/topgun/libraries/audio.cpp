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

#include "common/textconsole.h"

#include "topgun/libraries/audio.h"
#include "topgun/topgun.h"

namespace Topgun {

AudioLibrary::AudioLibrary(TopgunEngine *engine) : _engine(engine) {
}

AudioLibrary::~AudioLibrary() {
}

uint16 AudioLibrary::audioInitLibrary(uint16 version) {
	if ((version >> 8) != 1 || (version & 0xFF) != 0)
		return 0;

	_initCount++;

	if (_owner != 0)
		_engine->requestClose();

	return _initCount;
}

void AudioLibrary::audioExitLibrary() {
	warning("audioExitLibrary UNIMPLEMENTED");
}

void AudioLibrary::audioService() {
	warning("audioService UNIMPLEMENTED");
}

uint32 AudioLibrary::audioCommand(uint16 command, uint16 handle, uint32 value, void *block) {
	warning("audioCommand UNIMPLEMENTED");
	return 0;
}

uint16 AudioLibrary::audioActivate(uint16 active) {
	warning("audioActivate UNIMPLEMENTED");
	return 0;
}

uint32 AudioLibrary::audioResampleWave(const WaveFormat *destinationFormat, const WaveFormat *sourceFormat, uint32 sourceBytes, byte *destinationBuffer, const byte *sourceBuffer) {
	warning("audioResampleWave UNIMPLEMENTED");
	return 0;
}

byte *AudioLibrary::waveLoadFile(const Common::String &fileName, WaveFormat *outFormat, uint32 *outSize) {
	warning("waveLoadFile UNIMPLEMENTED");
	return nullptr;
}

bool AudioLibrary::waveSaveFile(const Common::String &fileName, const WaveFormat *format, const byte *data, uint32 size) {
	warning("waveSaveFile UNIMPLEMENTED");
	return false;
}

uint16 AudioLibrary::wavePlayFile(const Common::String &fileName, uint16 flags) {
	warning("wavePlayFile UNIMPLEMENTED");
	return 0;
}

uint16 AudioLibrary::audioSetOwner(uint16 owner) {
	warning("audioSetOwner UNIMPLEMENTED");
	return 0;
}

} // End of namespace Topgun
