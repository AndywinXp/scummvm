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

StdPic::StdPic(StdDisplay *stdDisplay, StdFile *stdFile, SysStruct *pHS) : _stdDisplay(stdDisplay), _stdFile(stdFile), _pHS(pHS) {
}

StdBitmapStruct *StdPic::load(const char *path, int bVideoMemory, int bNoZ) {
	StdBitmapStruct *pBitmap = new StdBitmapStruct();

	if (loadEntry(path, pBitmap, bVideoMemory, bNoZ))
		return pBitmap;

	return nullptr;
}

int StdPic::loadEntry(const char *path, StdBitmapStruct *pBitmap, int bVideoMemory, int bNoZ) {
	Common::SeekableReadStream *pFile = _pHS->openFile(path);

	if (!pFile)
		return 0;

	byte header[104];

	if (pFile->read(header, 104) != 104)
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

	if (pFile->read(packet, 4) != 4)
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

		if (pFile->read(packet, 4) != 4)
			return 0;
	}

	if ((int64)width * (int64)height * 3 > 0x7FFFFFFF) {
		warning("StdPic::loadEntry(): invalid image size");
		return 0;
	}

	memset(pBitmap, 0, sizeof(StdBitmapStruct));
	strncpy(pBitmap->aName, _stdFile->fromPath(path), 31);
	pBitmap->aName[31] = 0;
	pBitmap->encoding = 0;
	pBitmap->flags = 0;
	pBitmap->pPalette = nullptr;
	pBitmap->numFrames = 1;
	pBitmap->unk_104 = 0;
	pBitmap->format.colorMode = 1;
	pBitmap->format.bpp = 24;
	pBitmap->format.redBits = 8;
	pBitmap->format.greenBits = 8;
	pBitmap->format.blueBits = 8;
	pBitmap->format.redShift = 16;
	pBitmap->format.greenShift = 8;
	pBitmap->format.blueShift = 0;
	pBitmap->apFrames = new StdVBuffer *[1]();

	StdVBufferTexFmt format;

	memset(&format, 0, sizeof(format));
	format.format = pBitmap->format;
	format.width = width;
	format.height = height;
	pBitmap->apFrames[0] = _stdDisplay->vBufferNew(&format, bVideoMemory, bNoZ, nullptr);

	if (!pBitmap->apFrames[0])
		error("Unable to allocate memory.");

	if (!_stdDisplay->vBufferLock(pBitmap->apFrames[0]))
		error("Unable to lock VBuffer memory.");

	StdVBuffer *pFrame = pBitmap->apFrames[0];
	int pitchInBytes = pFrame->tex.pitchInBytes;
	byte *pRow = pFrame->pPixels;

	for (int y = 0; y < height; y++) {
		for (int currentPacket = 0; currentPacket < numPackets; currentPacket++) {
			int x = 0;

			if (packetType[currentPacket] != 0) {
				if (width > 0) {
					do {
						byte control = pFile->readByte();

						if (control >= 0x80) {
							int runLength;

							if (control == 0x80) {
								byte high = pFile->readByte();
								byte low = pFile->readByte();

								runLength = (high << 8) | low;
							} else {
								runLength = control - 127;
							}

							byte value[3] = { 0, 0, 0 };

							readChannels(pFile, currentPacket, channelPacket, value);

							if (runLength != 0) {
								int count = runLength;

								if (count > width - x) {
									warning("StdPic::loadEntry(): run goes past the end of the row");
									count = width - x;
								}

								for (int i = 0; i < count; i++) {
									byte *pPixel = pRow + 3 * (x + i);

									if (channelPacket[0] == currentPacket)
										pPixel[0] = value[0];

									if (channelPacket[1] == currentPacket)
										pPixel[1] = value[1];

									if (channelPacket[2] == currentPacket)
										pPixel[2] = value[2];
								}

								x += runLength;
							}
						} else {
							int count = control + 1;

							for (int i = 0; i < count; i++) {
								if (x < width) {
									readChannels(pFile, currentPacket, channelPacket, pRow + 3 * x);
								} else {
									warning("StdPic::loadEntry(): literal goes past the end of the row");

									byte discard[3] = { 0, 0, 0 };

									readChannels(pFile, currentPacket, channelPacket, discard);
								}

								x++;
							}
						}
					} while (x < width);
				}
			} else {
				for (x = 0; x < width; x++)
					readChannels(pFile, currentPacket, channelPacket, pRow + 3 * x);
			}
		}

		pRow += pitchInBytes;
	}

	_stdDisplay->vBufferUnlock(pBitmap->apFrames[0]);
	delete pFile;

	return 1;
}

void StdPic::readChannels(Common::SeekableReadStream *pFile, int packet, const int *pChannelPacket, byte *pPixel) {
	if (pChannelPacket[0] == packet)
		pPixel[0] = pFile->readByte();

	if (pChannelPacket[1] == packet)
		pPixel[1] = pFile->readByte();

	if (pChannelPacket[2] == packet)
		pPixel[2] = pFile->readByte();

	if (pChannelPacket[3] == packet)
		pFile->readByte();
}

} // End of namespace Grim
