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
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/std/std_bitmap.h"
#include "engines/grim/std/std_file.h"
#include "engines/grim/std/std_pic.h"
#include "engines/grim/std/std_sys.h"

namespace Grim {

StdPic::StdPic(StdDisplay *stdDisplay, StdFile *stdFile, SysStruct *hs) : _stdDisplay(stdDisplay), _stdFile(stdFile), _hs(hs) {
}

StdBitmapStruct *StdPic::load(const char *path, int videoMemory, int noZ) {
	StdBitmapStruct *bitmap = new StdBitmapStruct();

	if (loadEntry(path, bitmap, videoMemory, noZ))
		return bitmap;

	return nullptr;
}

int StdPic::loadEntry(const char *path, StdBitmapStruct *bitmap, int videoMemory, int noZ) {
	Common::SeekableReadStream *file = _hs->openFile(path);

	if (!file)
		return 0;

	byte header[104];

	if (file->read(header, 104) != 104)
		return 0;

	uint magic = READ_BE_UINT32(header);
	uint pict = READ_BE_UINT32(header + 88);
	int width = READ_BE_UINT16(header + 92);
	int height = READ_BE_UINT16(header + 94);

	if (magic != 0x5380F634)
		return 0;

	if (pict != MKTAG('P', 'I', 'C', 'T'))
		return 0;

	int channelPacket[4] = { -1, -1, -1, -1 };
	int packetType[4] = { 0, 0, 0, 0 };
	int numPackets = 0;
	byte packet[4];

	if (file->read(packet, 4) != 4)
		return 0;

	for (;;) {
		if (numPackets >= 4) {
			warning("StdPic::loadEntry(): too many channel packets");
			return 0;
		}

		if (packet[3] & 0x80)
			channelPacket[0] = numPackets;

		if (packet[3] & 0x40)
			channelPacket[1] = numPackets;

		if (packet[3] & 0x20)
			channelPacket[2] = numPackets;

		if (packet[3] & 0x10)
			channelPacket[3] = numPackets;

		packetType[numPackets] = packet[2];

		if (packet[1] != 8)
			return 0;

		numPackets++;

		if (!packet[0])
			break;

		if (file->read(packet, 4) != 4)
			return 0;
	}

	if ((int64)width * (int64)height * 3 > 0x7FFFFFFF) {
		warning("StdPic::loadEntry(): invalid image size");
		return 0;
	}

	memset(bitmap, 0, sizeof(StdBitmapStruct));
	strncpy(bitmap->name, _stdFile->fromPath(path), 31);
	bitmap->name[31] = 0;
	bitmap->encoding = 0;
	bitmap->flags = 0;
	bitmap->palette = nullptr;
	bitmap->numFrames = 1;
	bitmap->curFrame = 0;
	bitmap->format.colorMode = 1;
	bitmap->format.bpp = 24;
	bitmap->format.redBits = 8;
	bitmap->format.greenBits = 8;
	bitmap->format.blueBits = 8;
	bitmap->format.redShift = 16;
	bitmap->format.greenShift = 8;
	bitmap->format.blueShift = 0;
	bitmap->frames = new StdVBuffer *[1]();

	StdVBufferTexFmt format;

	memset(&format, 0, sizeof(format));
	format.format = bitmap->format;
	format.width = width;
	format.height = height;
	bitmap->frames[0] = _stdDisplay->vBufferNew(&format, videoMemory, noZ, nullptr);

	if (!bitmap->frames[0])
		error("Unable to allocate memory.");

	if (!_stdDisplay->vBufferLock(bitmap->frames[0]))
		error("Unable to lock VBuffer memory.");

	StdVBuffer *frame = bitmap->frames[0];
	int pitchInBytes = frame->tex.pitchInBytes;
	byte *row = frame->pixels;

	for (int y = 0; y < height; y++) {
		for (int currentPacket = 0; currentPacket < numPackets; currentPacket++) {
			int x = 0;

			if (packetType[currentPacket] != 0) {
				if (width > 0) {
					do {
						byte control = file->readByte();

						if (control >= 0x80) {
							int runLength;

							if (control == 0x80) {
								byte high = file->readByte();
								byte low = file->readByte();

								runLength = (high << 8) | low;
							} else {
								runLength = control - 127;
							}

							byte value[3] = { 0, 0, 0 };

							readChannels(file, currentPacket, channelPacket, value);

							if (runLength != 0) {
								int count = runLength;

								if (count > width - x) {
									warning("StdPic::loadEntry(): run goes past the end of the row");
									count = width - x;
								}

								for (int i = 0; i < count; i++) {
									byte *pixel = row + 3 * (x + i);

									if (channelPacket[0] == currentPacket)
										pixel[0] = value[0];

									if (channelPacket[1] == currentPacket)
										pixel[1] = value[1];

									if (channelPacket[2] == currentPacket)
										pixel[2] = value[2];
								}

								x += runLength;
							}
						} else {
							int count = control + 1;

							for (int i = 0; i < count; i++) {
								if (x < width) {
									readChannels(file, currentPacket, channelPacket, row + 3 * x);
								} else {
									warning("StdPic::loadEntry(): literal goes past the end of the row");

									byte discard[3] = { 0, 0, 0 };

									readChannels(file, currentPacket, channelPacket, discard);
								}

								x++;
							}
						}
					} while (x < width);
				}
			} else {
				for (x = 0; x < width; x++)
					readChannels(file, currentPacket, channelPacket, row + 3 * x);
			}
		}

		row += pitchInBytes;
	}

	_stdDisplay->vBufferUnlock(bitmap->frames[0]);
	delete file;

	return 1;
}

void StdPic::readChannels(Common::SeekableReadStream *file, int packet, const int *channelPacket, byte *pixel) {
	if (channelPacket[0] == packet)
		pixel[0] = file->readByte();

	if (channelPacket[1] == packet)
		pixel[1] = file->readByte();

	if (channelPacket[2] == packet)
		pixel[2] = file->readByte();

	if (channelPacket[3] == packet)
		file->readByte();
}

} // End of namespace Grim
