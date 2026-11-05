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

StdPcx::StdPcx(StdDisplay *stdDisplay, StdFile *stdFile, StdBitmap *stdBitmap, SysStruct *pHS) : _stdDisplay(stdDisplay), _stdFile(stdFile), _stdBitmap(stdBitmap), _pHS(pHS) {
}

StdBitmapStruct *StdPcx::load(const char *path, int bVideoMemory, int bNoZ) {
	Common::SeekableReadStream *pFile = _pHS->openFile(path);

	if (!pFile)
		return nullptr;

	StdBitmapStruct *pBitmap = new StdBitmapStruct();
	byte headerBytes[128];

	memset(headerBytes, 0, sizeof(headerBytes));
	pFile->read(headerBytes, 128);

	Common::MemoryReadStream headerStream(headerBytes, sizeof(headerBytes));
	StdPcxHeader header;

	readHeader(&headerStream, &header);

	if (header.manufacturer != 10)
		return loadFailed(pFile, pBitmap);

	pBitmap->encoding = 0;
	pBitmap->flags = 2;
	pBitmap->numFrames = 1;
	pBitmap->unk_104 = 0;
	pBitmap->format.colorMode = 0;
	pBitmap->format.bpp = header.bitsPerPixel;
	pBitmap->apFrames = new StdVBuffer *[1]();

	StdVBufferTexFmt format;

	memset(&format, 0, sizeof(format));
	format.format.colorMode = 0;
	format.format.bpp = header.bitsPerPixel;

	int bytesPerPixel = header.bitsPerPixel >> 3;

	if (bytesPerPixel == 0) {
		warning("StdPcx::load(): unsupported bits per pixel");
		return loadFailed(pFile, pBitmap);
	}

	format.width = header.bytesPerLine / bytesPerPixel;
	format.height = header.yMax + 1;

	if (format.width < 0 || format.height < 0 || (int64)format.width * (int64)format.height * (int64)bytesPerPixel > 0x7FFFFFFF) {
		warning("StdPcx::load(): invalid image size");
		return loadFailed(pFile, pBitmap);
	}

	pBitmap->apFrames[0] = _stdDisplay->vBufferNew(&format, bVideoMemory, bNoZ, nullptr);

	if (!pBitmap->apFrames[0])
		return loadFailed(pFile, pBitmap);

	pBitmap->apFrames[0]->tex.width = header.xMax + 1;
	_stdDisplay->vBufferLock(pBitmap->apFrames[0]);

	byte *pPixels = pBitmap->apFrames[0]->pPixels;
	int sizeInBytes = pBitmap->apFrames[0]->tex.sizeInBytes;

	if (sizeInBytes) {
		int pos = 0;

		do {
			byte value = pFile->readByte();

			if ((value & 0xC0) == 0xC0) {
				int count = value & 0x3F;
				byte fill = pFile->readByte();

				if (count) {
					if (count > sizeInBytes - pos) {
						warning("StdPcx::load(): run goes past the end of the image");
						count = sizeInBytes - pos;
					}

					memset(pPixels + pos, fill, count);
					pos += count;
				}
			} else {
				pPixels[pos] = value;
				pos++;
			}
		} while (pos < sizeInBytes);
	}

	_stdDisplay->vBufferUnlock(pBitmap->apFrames[0]);
	pBitmap->pPalette = new byte[768];
	pFile->readByte();
	pFile->read(pBitmap->pPalette, 768);
	delete pFile;

	return pBitmap;
}

int StdPcx::write(const char *path, StdBitmapStruct *pBitmap) {
	StdVBuffer *pFrame = pBitmap->apFrames[0];
	StdPcxHeader header;

	header.manufacturer = 10;
	header.version = 5;
	header.encoding = 1;
	header.bitsPerPixel = 8;
	header.xMin = 0;
	header.yMin = 0;
	header.xMax = (int16)(pFrame->tex.width - 1);
	header.yMax = (int16)(pFrame->tex.height - 1);
	header.hDpi = (int16)pFrame->tex.width;
	header.vDpi = (int16)pFrame->tex.height;
	memset(header.colorMap, 0, sizeof(header.colorMap));
	header.reserved = 0;
	header.nPlanes = 1;
	header.bytesPerLine = (int16)pFrame->tex.width;
	header.paletteInfo = 0;
	memset(header.filler, 0, sizeof(header.filler));

	if (!pBitmap->pPalette) {
		warning("StdPcx::write(): bitmap without a palette");
		return 0;
	}

	Common::DumpFile file;

	if (!file.open(path))
		return 0;

	writeHeader(&file, &header);

	const byte *pPixels = pFrame->pPixels;
	int width = pFrame->tex.width;

	for (int y = 0; y < pFrame->tex.height; y++) {
		const byte *pRow = pPixels + width * y;
		int x = 0;

		while (x < width) {
			byte value = pRow[x];
			int run = 1;

			while (run < 63 && x + run < width && pRow[x + run] == value)
				run++;

			if (run > 1 || value > 0xBF)
				file.writeByte((byte)(run | 0xC0));

			file.writeByte(value);
			x += run;
		}
	}

	file.writeByte(12);
	file.write(pBitmap->pPalette, 768);
	file.close();

	return 1;
}

StdBitmapStruct *StdPcx::loadFailed(Common::SeekableReadStream *pFile, StdBitmapStruct *pBitmap) {
	delete pFile;
	_stdBitmap->unloadBitmap(pBitmap);

	return nullptr;
}

void StdPcx::readHeader(Common::ReadStream *pStream, StdPcxHeader *pHeader) {
	pHeader->manufacturer = pStream->readByte();
	pHeader->version = pStream->readByte();
	pHeader->encoding = pStream->readByte();
	pHeader->bitsPerPixel = pStream->readByte();
	pHeader->xMin = pStream->readSint16LE();
	pHeader->yMin = pStream->readSint16LE();
	pHeader->xMax = pStream->readSint16LE();
	pHeader->yMax = pStream->readSint16LE();
	pHeader->hDpi = pStream->readSint16LE();
	pHeader->vDpi = pStream->readSint16LE();
	pStream->read(pHeader->colorMap, sizeof(pHeader->colorMap));
	pHeader->reserved = pStream->readByte();
	pHeader->nPlanes = pStream->readByte();
	pHeader->bytesPerLine = pStream->readSint16LE();
	pHeader->paletteInfo = pStream->readSint16LE();
	pStream->read(pHeader->filler, sizeof(pHeader->filler));
}

void StdPcx::writeHeader(Common::WriteStream *pStream, const StdPcxHeader *pHeader) {
	pStream->writeByte(pHeader->manufacturer);
	pStream->writeByte(pHeader->version);
	pStream->writeByte(pHeader->encoding);
	pStream->writeByte(pHeader->bitsPerPixel);
	pStream->writeSint16LE(pHeader->xMin);
	pStream->writeSint16LE(pHeader->yMin);
	pStream->writeSint16LE(pHeader->xMax);
	pStream->writeSint16LE(pHeader->yMax);
	pStream->writeSint16LE(pHeader->hDpi);
	pStream->writeSint16LE(pHeader->vDpi);
	pStream->write(pHeader->colorMap, sizeof(pHeader->colorMap));
	pStream->writeByte(pHeader->reserved);
	pStream->writeByte(pHeader->nPlanes);
	pStream->writeSint16LE(pHeader->bytesPerLine);
	pStream->writeSint16LE(pHeader->paletteInfo);
	pStream->write(pHeader->filler, sizeof(pHeader->filler));
}

} // End of namespace Grim
