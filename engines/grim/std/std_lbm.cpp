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
#include "common/file.h"
#include "common/memstream.h"
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/std/std_bitmap.h"
#include "engines/grim/std/std_file.h"
#include "engines/grim/std/std_lbm.h"
#include "engines/grim/std/std_sys.h"

namespace Grim {

StdLbm::StdLbm(StdDisplay *stdDisplay, StdFile *stdFile, StdBitmap *stdBitmap, SysStruct *pHS) : _stdDisplay(stdDisplay), _stdFile(stdFile), _stdBitmap(stdBitmap), _pHS(pHS),
		_packLastByte(0), _packSize(0), _packRunPtr(nullptr), _packOut(nullptr), _packState(0), _packRepeat(0) {
}

StdBitmapStruct *StdLbm::load(const char *path, int bVideoMemory, int bNoZ) {
	Common::SeekableReadStream *pFile = _pHS->openFile(path);

	if (!pFile)
		return nullptr;

	StdBitmapStruct *pBitmap = new StdBitmapStruct();
	byte formHeader[12];

	memset(formHeader, 0, sizeof(formHeader));
	pFile->read(formHeader, 12);

	if (memcmp(formHeader, "FORM", 4))
		return loadFailed(pFile, pBitmap);

	pFile->seek(0);

	bool bFound = false;
	int bmhdSize = 0;
	int value = pFile->readByte();

	do {
		if (pFile->eos())
			break;

		if (value == 'B') {
			value = pFile->readByte();

			if (value == 'M') {
				value = pFile->readByte();

				if (value == 'H') {
					value = pFile->readByte();

					if (value == 'D') {
						byte chunkHeader[8];

						memset(chunkHeader, 0, sizeof(chunkHeader));
						pFile->seek(pFile->pos() - 4);
						pFile->read(chunkHeader, 8);
						bmhdSize = swapLong(READ_LE_UINT32(chunkHeader + 4));
						bFound = true;
					}
				}
			}
		} else {
			value = pFile->readByte();
		}
	} while (!bFound);

	if (!bFound)
		return nullptr;

	byte bmhdBytes[20];

	memset(bmhdBytes, 0, sizeof(bmhdBytes));
	pFile->read(bmhdBytes, 20);

	Common::MemoryReadStream bmhdStream(bmhdBytes, sizeof(bmhdBytes));
	StdLbmBmhd bmhd;

	readBmhd(&bmhdStream, &bmhd);
	swapBMHD(&bmhd);

	if (bmhdSize % 2)
		pFile->readByte();

	if (bmhdSize % 2)
		pFile->readByte();

	bFound = false;
	int bodySize = 0;

	do {
		if (pFile->eos())
			break;

		if (pFile->readByte() == 'B' && pFile->readByte() == 'O' && pFile->readByte() == 'D' && pFile->readByte() == 'Y') {
			byte chunkHeader[8];

			memset(chunkHeader, 0, sizeof(chunkHeader));
			pFile->seek(pFile->pos() - 4);
			pFile->read(chunkHeader, 8);
			bodySize = swapLong(READ_LE_UINT32(chunkHeader + 4));
			bFound = true;
		}
	} while (!bFound);

	if (!bFound)
		return nullptr;

	strncpy(pBitmap->aName, _stdFile->fromPath(path), 31);
	pBitmap->aName[31] = 0;
	pBitmap->encoding = 0;
	pBitmap->flags = 2;
	pBitmap->numFrames = 1;
	pBitmap->curFrame = 0;
	pBitmap->format.colorMode = 0;
	pBitmap->format.bpp = 8;
	pBitmap->apFrames = new StdVBuffer *[1]();

	StdVBufferTexFmt format;

	memset(&format, 0, sizeof(format));

	if (bmhd.width & 1)
		format.width = bmhd.width + 1;
	else
		format.width = bmhd.width;

	format.height = bmhd.height;
	format.format.colorMode = 0;
	format.format.bpp = 8;

	if ((int64)format.width * (int64)format.height > 0x7FFFFFFF) {
		warning("StdLbm::load(): invalid image size");
		return loadFailed(pFile, pBitmap);
	}

	pBitmap->apFrames[0] = _stdDisplay->vBufferNew(&format, bVideoMemory, bNoZ, nullptr);

	if (!pBitmap->apFrames[0])
		return loadFailed(pFile, pBitmap);

	_stdDisplay->vBufferLock(pBitmap->apFrames[0]);

	byte *pPixels = pBitmap->apFrames[0]->pPixels;
	int sizeInBytes = pBitmap->apFrames[0]->tex.sizeInBytes;

	if (bmhd.compression == 0) {
		if (bodySize > 0) {
			int copySize = bodySize;

			if (copySize > sizeInBytes) {
				warning("StdLbm::load(): body larger than the image");
				copySize = sizeInBytes;
			}

			pFile->read(pPixels, copySize);
		}
	} else if (bmhd.compression == 1) {
		int pos = 0;

		for (int row = 0; row < bmhd.height; row++) {
			int rowCount = 0;

			do {
				signed char control = (signed char)pFile->readByte();

				if (control < 0) {
					int count = 1 - control;
					byte fill = pFile->readByte();
					int fillCount = count;

					if (fillCount > sizeInBytes - pos) {
						warning("StdLbm::load(): run goes past the end of the image");
						fillCount = sizeInBytes - pos;

						if (fillCount < 0)
							fillCount = 0;
					}

					if (fillCount > 0)
						memset(pPixels + pos, fill, fillCount);

					pos += count;
					rowCount += count;
				} else {
					int count = control + 1;

					rowCount += count;

					for (int i = 0; i < count; i++) {
						byte literal = pFile->readByte();

						if (pos < sizeInBytes)
							pPixels[pos] = literal;

						pos++;
					}
				}
			} while (rowCount < bmhd.width);
		}
	}

	_stdDisplay->vBufferUnlock(pBitmap->apFrames[0]);
	pBitmap->pPalette = new byte[768];
	pFile->seek(0);

	bFound = false;

	int paletteSize = 0;

	do {
		if (pFile->eos())
			break;

		if (pFile->readByte() == 'C' && pFile->readByte() == 'M' && pFile->readByte() == 'A' && pFile->readByte() == 'P') {
			byte chunkHeader[8];

			memset(chunkHeader, 0, sizeof(chunkHeader));
			pFile->seek(pFile->pos() - 4);
			pFile->read(chunkHeader, 8);
			paletteSize = swapLong(READ_LE_UINT32(chunkHeader + 4));
			bFound = true;
		}
	} while (!bFound);

	if (!bFound)
		return nullptr;

	if ((uint)paletteSize > 768) {
		warning("StdLbm::load(): palette larger than 768 bytes");
		paletteSize = 768;
	}

	pFile->read(pBitmap->pPalette, paletteSize);
	delete pFile;

	return pBitmap;
}

int StdLbm::write(const char *path, StdBitmapStruct *pBitmap) {
	if (!pBitmap->pPalette) {
		warning("StdLbm::write(): bitmap without a palette");
		return 0;
	}

	Common::DumpFile file;

	if (!file.open(path))
		return 0;

	file.write("FORMxxxxPBM BMHD", 16);
	file.writeUint32LE(swapLong(0x14));

	StdLbmBmhd bmhd;

	memset(&bmhd, 0, sizeof(bmhd));
	swapBMHD(&bmhd);
	writeBmhd(&file, &bmhd);
	file.write("CMAP", 4);
	file.writeUint32LE(swapLong(0x300));
	file.write(pBitmap->pPalette, 768);

	int64 bodyStart = file.pos();

	file.write("BODYxxxx", 8);

	StdVBuffer *pFrame = pBitmap->apFrames[0];

	_stdDisplay->vBufferLock(pFrame);

	const byte *pPixels = pFrame->pPixels;
	int width = pFrame->tex.width;
	byte *pPackBuffer = new byte[2 * width + 2];

	for (int row = 0; row < pFrame->tex.height; row++) {
		packRow(&file, pPixels, width, pPackBuffer);
		pPixels += width;
	}

	delete[] pPackBuffer;
	pPackBuffer = nullptr;
	_stdDisplay->vBufferUnlock(pBitmap->apFrames[0]);

	int64 bodyEnd = file.pos();
	uint bodySize = swapLong((uint)(bodyEnd - bodyStart - 8));

	file.seek(bodyStart + 4);
	file.writeUint32LE(bodySize);
	file.seek(0, SEEK_END);

	uint formSize = swapLong((uint)(file.pos() - 8));

	pFrame = pBitmap->apFrames[0];
	bmhd.width = (uint16)pFrame->tex.width;
	bmhd.height = (uint16)pFrame->tex.height;
	bmhd.xOrigin = 0;
	bmhd.yOrigin = 0;
	bmhd.planes = 8;
	bmhd.masking = 0;
	bmhd.compression = 1;
	bmhd.transparentColor = 0;
	bmhd.xAspect = 6;
	bmhd.yAspect = 5;
	bmhd.pageWidth = (uint16)pFrame->tex.width;
	bmhd.pageHeight = (uint16)pFrame->tex.height;
	file.seek(4);
	file.writeUint32LE(formSize);
	file.seek(20);
	swapBMHD(&bmhd);
	writeBmhd(&file, &bmhd);
	file.seek(0, SEEK_END);
	file.close();

	return 1;
}

uint32 StdLbm::packRow(Common::WriteStream *pStream, const byte *pPixels, int width, byte *pBuffer) {
	_packSize = 0;
	_packOut = pBuffer;
	_packState = 3;

	for (int i = 0; i < width; i++)
		packByte(pPixels[i]);

	if (_packState == 2) {
		*_packOut = 0;
		_packOut++;
		*_packOut = _packLastByte;
		_packSize += 2;
	}

	return pStream->write(pBuffer, _packSize);
}

void StdLbm::packByte(byte value) {
	switch (_packState) {
	case 0:
		if (value == _packLastByte)
			_packRepeat++;
		else
			_packRepeat = 0;

		if (_packRepeat >= 3) {
			*_packRunPtr = (byte)(*_packRunPtr - 2);
			_packOut -= 2;
			_packRunPtr = _packOut;
			_packOut++;
			*_packRunPtr = 0xFE;
			_packState = 1;
			*_packOut = value;
			_packLastByte = value;
			_packOut++;
		} else {
			*_packRunPtr = (byte)(*_packRunPtr + 1);
			*_packOut = value;
			_packOut++;

			if (*_packRunPtr == 127)
				_packState = 3;

			_packLastByte = value;
			_packSize++;
		}

		break;
	case 1:
		if (value != _packLastByte) {
			_packState = 2;
		} else {
			*_packRunPtr = (byte)(*_packRunPtr - 1);

			if (*_packRunPtr == 0x81)
				_packState = 3;
		}

		_packLastByte = value;
		break;
	case 2:
		_packRunPtr = _packOut;
		_packOut++;

		if (value == _packLastByte) {
			*_packRunPtr = 0xFF;
			_packState = 1;
			*_packOut = value;
			_packOut++;
			_packSize += 2;
		} else {
			*_packRunPtr = 1;
			_packState = 0;
			*_packOut = _packLastByte;
			_packOut++;
			*_packOut = value;
			_packOut++;
			_packSize += 3;
			_packRepeat = 0;
		}

		_packLastByte = value;
		break;
	case 3:
		_packState = 2;
		_packLastByte = value;
		break;
	default:
		_packLastByte = value;
		break;
	}
}

uint StdLbm::swapLong(uint value) {
	return SWAP_BYTES_32(value);
}

void StdLbm::swapBMHD(StdLbmBmhd *pBmhd) {
	pBmhd->width = swapShort(pBmhd->width);
	pBmhd->height = swapShort(pBmhd->height);
	pBmhd->xOrigin = (int16)swapShort((uint16)pBmhd->xOrigin);
	pBmhd->yOrigin = (int16)swapShort((uint16)pBmhd->yOrigin);
	pBmhd->transparentColor = swapShort(pBmhd->transparentColor);
	pBmhd->pageWidth = swapShort(pBmhd->pageWidth);
	pBmhd->pageHeight = swapShort(pBmhd->pageHeight);
}

uint16 StdLbm::swapShort(uint16 value) {
	return SWAP_BYTES_16(value);
}

const char *StdLbm::stripPath(const char *path) {
	const char *pSlash = strrchr(path, '\\');

	if (!pSlash)
		return path;

	if (*pSlash == '\\') {
		do {
			pSlash++;
		} while (*pSlash == '\\');
	}

	return pSlash;
}

const char *StdLbm::getExtension(const char *path) {
	const char *pBaseName = stripPath(path);
	const char *pDot = strrchr(pBaseName, '.');

	if (!pDot)
		return nullptr;

	for (const char *pNext = strrchr(pDot + 1, '.'); pNext; pNext = strrchr(pNext + 1, '.'))
		pDot = pNext;

	return pDot + 1;
}

char *StdLbm::stripExtension(char *path) {
	const char *pExtension = getExtension(path);

	if (!pExtension)
		return nullptr;

	size_t offset = pExtension - path;

	path[offset - 1] = 0;

	return path + offset;
}

char *StdLbm::copyBaseName(char *dst, int size, const char *path) {
	if (size < 1) {
		warning("StdLbm::copyBaseName(): invalid buffer size");
		return nullptr;
	}

	const char *pBaseName = stripPath(path);

	strncpy(dst, pBaseName, size - 1);
	dst[size - 1] = 0;

	return stripExtension(dst);
}

StdBitmapStruct *StdLbm::loadFailed(Common::SeekableReadStream *pFile, StdBitmapStruct *pBitmap) {
	delete pFile;
	_stdBitmap->unloadBitmap(pBitmap);

	return nullptr;
}

void StdLbm::readBmhd(Common::ReadStream *pStream, StdLbmBmhd *pBmhd) {
	pBmhd->width = pStream->readUint16LE();
	pBmhd->height = pStream->readUint16LE();
	pBmhd->xOrigin = pStream->readSint16LE();
	pBmhd->yOrigin = pStream->readSint16LE();
	pBmhd->planes = pStream->readByte();
	pBmhd->masking = pStream->readByte();
	pBmhd->compression = pStream->readByte();
	pBmhd->pad1 = pStream->readByte();
	pBmhd->transparentColor = pStream->readUint16LE();
	pBmhd->xAspect = pStream->readByte();
	pBmhd->yAspect = pStream->readByte();
	pBmhd->pageWidth = pStream->readUint16LE();
	pBmhd->pageHeight = pStream->readUint16LE();
}

void StdLbm::writeBmhd(Common::WriteStream *pStream, const StdLbmBmhd *pBmhd) {
	pStream->writeUint16LE(pBmhd->width);
	pStream->writeUint16LE(pBmhd->height);
	pStream->writeSint16LE(pBmhd->xOrigin);
	pStream->writeSint16LE(pBmhd->yOrigin);
	pStream->writeByte(pBmhd->planes);
	pStream->writeByte(pBmhd->masking);
	pStream->writeByte(pBmhd->compression);
	pStream->writeByte(pBmhd->pad1);
	pStream->writeUint16LE(pBmhd->transparentColor);
	pStream->writeByte(pBmhd->xAspect);
	pStream->writeByte(pBmhd->yAspect);
	pStream->writeUint16LE(pBmhd->pageWidth);
	pStream->writeUint16LE(pBmhd->pageHeight);
}

} // End of namespace Grim
