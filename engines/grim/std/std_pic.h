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

#ifndef GRIM_STD_STD_PIC_H
#define GRIM_STD_STD_PIC_H

#include "common/scummsys.h"

namespace Common {
class SeekableReadStream;
}

namespace Grim {

class StdDisplay;
class StdFile;
class SysStruct;
struct StdBitmapStruct;

class StdPic {
public:
	StdPic(StdDisplay *stdDisplay, StdFile *stdFile, SysStruct *hs);

	StdBitmapStruct *load(const char *path, int videoMemory, int noZ);
	int loadEntry(const char *path, StdBitmapStruct *bitmap, int videoMemory, int noZ);

private:
	void readChannels(Common::SeekableReadStream *file, int packet, const int *channelPacket, byte *pixel);

	StdDisplay *_stdDisplay;
	StdFile *_stdFile;
	SysStruct *_hs;
};

} // End of namespace Grim

#endif
