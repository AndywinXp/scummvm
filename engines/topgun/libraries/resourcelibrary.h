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

#ifndef TOPGUN_LIBRARIES_RESOURCELIBRARY_H
#define TOPGUN_LIBRARIES_RESOURCELIBRARY_H

#include "common/scummsys.h"
#include "common/str.h"

namespace Topgun {

class TopgunEngine;

class ResourceLibrary {
public:
	ResourceLibrary(TopgunEngine *engine);
	~ResourceLibrary();

	uint16 resCloseFile(uint16 file);
	uint16 resCountResources(uint16 file);
	uint16 resCountVariables(uint16 file);
	uint16 resCreateWaveEvent(uint16 file, uint16 index);
	uint16 resEnumResources(uint16 file);
	uint32 resGetSize(uint16 file, uint16 index);
	uint16 resGetType(uint16 file, uint16 index);
	byte *resLoadNameTable(uint16 file, uint32 *outSize);
	uint16 resLoadPalette(uint16 file, byte *buffer);
	byte *resLoadResource(uint16 file, uint16 index, uint32 *outSize);
	uint16 resOpenFile(const Common::String &path);
	uint32 resRead(uint16 file, byte *buffer, uint32 count);
	uint32 resSeek(uint16 file, int32 offset, int16 origin);
	uint32 resSeekResource(uint16 file, uint16 index);
	bool resSetCallback(uint16 file, bool callbackActive, uint32 interval);
	void waveDone(uint16 waveInfo, uint16 flag);
	int32 waveRead(uint16 waveInfo, byte *buffer, int32 count);
	int32 waveSeek(uint16 waveInfo, int32 offset, int16 origin);

private:
	TopgunEngine *_engine;
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_RESOURCELIBRARY_H
