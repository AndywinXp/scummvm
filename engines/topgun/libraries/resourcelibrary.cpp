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

#include "topgun/libraries/resourcelibrary.h"
#include "topgun/topgun.h"

namespace Topgun {

ResourceLibrary::ResourceLibrary(TopgunEngine *engine) : _engine(engine) {
}

ResourceLibrary::~ResourceLibrary() {
}

uint16 ResourceLibrary::resCloseFile(uint16 file) {
	warning("resCloseFile UNIMPLEMENTED");
	return 0;
}

uint16 ResourceLibrary::resCountResources(uint16 file) {
	warning("resCountResources UNIMPLEMENTED");
	return 0;
}

uint16 ResourceLibrary::resCountVariables(uint16 file) {
	warning("resCountVariables UNIMPLEMENTED");
	return 0;
}

uint16 ResourceLibrary::resCreateWaveEvent(uint16 file, uint16 index) {
	warning("resCreateWaveEvent UNIMPLEMENTED");
	return 0;
}

uint16 ResourceLibrary::resEnumResources(uint16 file) {
	warning("resEnumResources UNIMPLEMENTED");
	return 0;
}

uint32 ResourceLibrary::resGetSize(uint16 file, uint16 index) {
	warning("resGetSize UNIMPLEMENTED");
	return 0;
}

uint16 ResourceLibrary::resGetType(uint16 file, uint16 index) {
	warning("resGetType UNIMPLEMENTED");
	return 0;
}

byte *ResourceLibrary::resLoadNameTable(uint16 file, uint32 *outSize) {
	warning("resLoadNameTable UNIMPLEMENTED");
	return nullptr;
}

uint16 ResourceLibrary::resLoadPalette(uint16 file, byte *buffer) {
	warning("resLoadPalette UNIMPLEMENTED");
	return 0;
}

byte *ResourceLibrary::resLoadResource(uint16 file, uint16 index, uint32 *outSize) {
	warning("resLoadResource UNIMPLEMENTED");
	return nullptr;
}

uint16 ResourceLibrary::resOpenFile(const Common::String &path) {
	warning("resOpenFile UNIMPLEMENTED");
	return 0;
}

uint32 ResourceLibrary::resRead(uint16 file, byte *buffer, uint32 count) {
	warning("resRead UNIMPLEMENTED");
	return 0;
}

uint32 ResourceLibrary::resSeek(uint16 file, int32 offset, int16 origin) {
	warning("resSeek UNIMPLEMENTED");
	return 0;
}

uint32 ResourceLibrary::resSeekResource(uint16 file, uint16 index) {
	warning("resSeekResource UNIMPLEMENTED");
	return 0;
}

bool ResourceLibrary::resSetCallback(uint16 file, bool callbackActive, uint32 interval) {
	warning("resSetCallback UNIMPLEMENTED");
	return false;
}

void ResourceLibrary::waveDone(uint16 waveInfo, uint16 flag) {
	warning("waveDone UNIMPLEMENTED");
}

int32 ResourceLibrary::waveRead(uint16 waveInfo, byte *buffer, int32 count) {
	warning("waveRead UNIMPLEMENTED");
	return 0;
}

int32 ResourceLibrary::waveSeek(uint16 waveInfo, int32 offset, int16 origin) {
	warning("waveSeek UNIMPLEMENTED");
	return 0;
}

} // End of namespace Topgun
