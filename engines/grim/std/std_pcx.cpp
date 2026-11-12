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

#include "common/file.h"
#include "common/memstream.h"
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/std/std_bitmap.h"
#include "engines/grim/std/std_file.h"
#include "engines/grim/std/std_pcx.h"
#include "engines/grim/std/std_sys.h"

namespace Grim {

StdPcx::StdPcx(StdDisplay *stdDisplay, StdFile *stdFile, StdBitmap *stdBitmap, SysStruct *hs) : _stdDisplay(stdDisplay), _stdFile(stdFile), _stdBitmap(stdBitmap), _hs(hs) {
}

StdBitmapStruct *StdPcx::load(const char *path, int videoMemory, int noZ) {
	Common::SeekableReadStream *file = _hs->openFile(path);

	if (!file)
		return nullptr;

	StdBitmapStruct *bitmap = new StdBitmapStruct();
	byte headerBytes[128];

	memset(headerBytes, 0, sizeof(headerBytes));
	file->read(headerBytes, 128);

	Common::MemoryReadStream headerStream(headerBytes, sizeof(headerBytes));
	StdPcxHeader header;

	readHeader(&headerStream, &header);

	if (header.manufacturer != 10)
		return loadFailed(file, bitmap);

	bitmap->encoding = 0;
	bitmap->flags = 2;
	bitmap->numFrames = 1;
	bitmap->curFrame = 0;
	bitmap->format.colorMode = 0;
	bitmap->format.bpp = header.bitsPerPixel;
	bitmap->frames = new StdVBuffer *[1]();

	StdVBufferTexFmt format;

	memset(&format, 0, sizeof(format));
	format.format.colorMode = 0;
	format.format.bpp = header.bitsPerPixel;

	int bytesPerPixel = header.bitsPerPixel >> 3;

	if (bytesPerPixel == 0) {
		warning("StdPcx::load(): unsupported bits per pixel");
		return loadFailed(file, bitmap);
	}

	format.width = header.bytesPerLine / bytesPerPixel;
	format.height = header.yMax + 1;

	if (format.width < 0 || format.height < 0 || (int64)format.width * (int64)format.height * (int64)bytesPerPixel > 0x7FFFFFFF) {
		warning("StdPcx::load(): invalid image size");
		return loadFailed(file, bitmap);
	}

	bitmap->frames[0] = _stdDisplay->vBufferNew(&format, videoMemory, noZ, nullptr);

	if (!bitmap->frames[0])
		return loadFailed(file, bitmap);

	bitmap->frames[0]->tex.width = header.xMax + 1;
	_stdDisplay->vBufferLock(bitmap->frames[0]);

	byte *pixels = bitmap->frames[0]->pixels;
	int sizeInBytes = bitmap->frames[0]->tex.sizeInBytes;

	if (sizeInBytes) {
		int pos = 0;

		do {
			byte value = file->readByte();

			if ((value & 0xC0) == 0xC0) {
				int count = value & 0x3F;
				byte fill = file->readByte();

				if (count) {
					if (count > sizeInBytes - pos) {
						warning("StdPcx::load(): run goes past the end of the image");
						count = sizeInBytes - pos;
					}

					memset(pixels + pos, fill, count);
					pos += count;
				}
			} else {
				pixels[pos] = value;
				pos++;
			}
		} while (pos < sizeInBytes);
	}

	_stdDisplay->vBufferUnlock(bitmap->frames[0]);
	bitmap->palette = new byte[768];
	file->readByte();
	file->read(bitmap->palette, 768);
	delete file;

	return bitmap;
}

int StdPcx::write(const char *path, StdBitmapStruct *bitmap) {
	StdVBuffer *frame = bitmap->frames[0];
	StdPcxHeader header;

	header.manufacturer = 10;
	header.version = 5;
	header.encoding = 1;
	header.bitsPerPixel = 8;
	header.xMin = 0;
	header.yMin = 0;
	header.xMax = (int16)(frame->tex.width - 1);
	header.yMax = (int16)(frame->tex.height - 1);
	header.hDpi = (int16)frame->tex.width;
	header.vDpi = (int16)frame->tex.height;
	memset(header.colorMap, 0, sizeof(header.colorMap));
	header.reserved = 0;
	header.planes = 1;
	header.bytesPerLine = (int16)frame->tex.width;
	header.paletteInfo = 0;
	memset(header.filler, 0, sizeof(header.filler));

	if (!bitmap->palette) {
		warning("StdPcx::write(): bitmap without a palette");
		return 0;
	}

	Common::DumpFile file;

	if (!file.open(path))
		return 0;

	writeHeader(&file, &header);

	const byte *pixels = frame->pixels;
	int width = frame->tex.width;

	for (int y = 0; y < frame->tex.height; y++) {
		const byte *row = pixels + width * y;
		int x = 0;

		while (x < width) {
			byte value = row[x];
			int run = 1;

			while (run < 63 && x + run < width && row[x + run] == value)
				run++;

			if (run > 1 || value > 0xBF)
				file.writeByte((byte)(run | 0xC0));

			file.writeByte(value);
			x += run;
		}
	}

	file.writeByte(12);
	file.write(bitmap->palette, 768);
	file.close();

	return 1;
}

StdBitmapStruct *StdPcx::loadFailed(Common::SeekableReadStream *file, StdBitmapStruct *bitmap) {
	delete file;
	_stdBitmap->unloadBitmap(bitmap);

	return nullptr;
}

void StdPcx::readHeader(Common::ReadStream *stream, StdPcxHeader *header) {
	header->manufacturer = stream->readByte();
	header->version = stream->readByte();
	header->encoding = stream->readByte();
	header->bitsPerPixel = stream->readByte();
	header->xMin = stream->readSint16LE();
	header->yMin = stream->readSint16LE();
	header->xMax = stream->readSint16LE();
	header->yMax = stream->readSint16LE();
	header->hDpi = stream->readSint16LE();
	header->vDpi = stream->readSint16LE();
	stream->read(header->colorMap, sizeof(header->colorMap));
	header->reserved = stream->readByte();
	header->planes = stream->readByte();
	header->bytesPerLine = stream->readSint16LE();
	header->paletteInfo = stream->readSint16LE();
	stream->read(header->filler, sizeof(header->filler));
}

void StdPcx::writeHeader(Common::WriteStream *stream, const StdPcxHeader *header) {
	stream->writeByte(header->manufacturer);
	stream->writeByte(header->version);
	stream->writeByte(header->encoding);
	stream->writeByte(header->bitsPerPixel);
	stream->writeSint16LE(header->xMin);
	stream->writeSint16LE(header->yMin);
	stream->writeSint16LE(header->xMax);
	stream->writeSint16LE(header->yMax);
	stream->writeSint16LE(header->hDpi);
	stream->writeSint16LE(header->vDpi);
	stream->write(header->colorMap, sizeof(header->colorMap));
	stream->writeByte(header->reserved);
	stream->writeByte(header->planes);
	stream->writeSint16LE(header->bytesPerLine);
	stream->writeSint16LE(header->paletteInfo);
	stream->write(header->filler, sizeof(header->filler));
}

} // End of namespace Grim
