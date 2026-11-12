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

StdLbm::StdLbm(StdDisplay *stdDisplay, StdFile *stdFile, StdBitmap *stdBitmap, SysStruct *hs) : _stdDisplay(stdDisplay), _stdFile(stdFile), _stdBitmap(stdBitmap), _hs(hs),
		_packLastByte(0), _packSize(0), _packRunPtr(nullptr), _packOut(nullptr), _packState(0), _packRepeat(0) {
}

StdBitmapStruct *StdLbm::load(const char *path, int videoMemory, int noZ) {
	Common::SeekableReadStream *file = _hs->openFile(path);

	if (!file)
		return nullptr;

	StdBitmapStruct *bitmap = new StdBitmapStruct();
	byte formHeader[12];

	memset(formHeader, 0, sizeof(formHeader));
	file->read(formHeader, 12);

	if (memcmp(formHeader, "FORM", 4))
		return loadFailed(file, bitmap);

	file->seek(0);

	bool found = false;
	int bmhdSize = 0;
	int value = file->readByte();

	do {
		if (file->eos())
			break;

		if (value == 'B') {
			value = file->readByte();

			if (value == 'M') {
				value = file->readByte();

				if (value == 'H') {
					value = file->readByte();

					if (value == 'D') {
						byte chunkHeader[8];

						memset(chunkHeader, 0, sizeof(chunkHeader));
						file->seek(file->pos() - 4);
						file->read(chunkHeader, 8);
						bmhdSize = swapLong(READ_LE_UINT32(chunkHeader + 4));
						found = true;
					}
				}
			}
		} else {
			value = file->readByte();
		}
	} while (!found);

	if (!found)
		return nullptr;

	byte bmhdBytes[20];

	memset(bmhdBytes, 0, sizeof(bmhdBytes));
	file->read(bmhdBytes, 20);

	Common::MemoryReadStream bmhdStream(bmhdBytes, sizeof(bmhdBytes));
	StdLbmBmhd bmhd;

	readBmhd(&bmhdStream, &bmhd);
	swapBMHD(&bmhd);

	if (bmhdSize % 2)
		file->readByte();

	if (bmhdSize % 2)
		file->readByte();

	found = false;
	int bodySize = 0;

	do {
		if (file->eos())
			break;

		if (file->readByte() == 'B' && file->readByte() == 'O' && file->readByte() == 'D' && file->readByte() == 'Y') {
			byte chunkHeader[8];

			memset(chunkHeader, 0, sizeof(chunkHeader));
			file->seek(file->pos() - 4);
			file->read(chunkHeader, 8);
			bodySize = swapLong(READ_LE_UINT32(chunkHeader + 4));
			found = true;
		}
	} while (!found);

	if (!found)
		return nullptr;

	strncpy(bitmap->name, _stdFile->fromPath(path), 31);
	bitmap->name[31] = 0;
	bitmap->encoding = 0;
	bitmap->flags = 2;
	bitmap->numFrames = 1;
	bitmap->curFrame = 0;
	bitmap->format.colorMode = 0;
	bitmap->format.bpp = 8;
	bitmap->frames = new StdVBuffer *[1]();

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
		return loadFailed(file, bitmap);
	}

	bitmap->frames[0] = _stdDisplay->vBufferNew(&format, videoMemory, noZ, nullptr);

	if (!bitmap->frames[0])
		return loadFailed(file, bitmap);

	_stdDisplay->vBufferLock(bitmap->frames[0]);

	byte *pixels = bitmap->frames[0]->pixels;
	int sizeInBytes = bitmap->frames[0]->tex.sizeInBytes;

	if (bmhd.compression == 0) {
		if (bodySize > 0) {
			int copySize = bodySize;

			if (copySize > sizeInBytes) {
				warning("StdLbm::load(): body larger than the image");
				copySize = sizeInBytes;
			}

			file->read(pixels, copySize);
		}
	} else if (bmhd.compression == 1) {
		int pos = 0;

		for (int row = 0; row < bmhd.height; row++) {
			int rowCount = 0;

			do {
				signed char control = (signed char)file->readByte();

				if (control < 0) {
					int count = 1 - control;
					byte fill = file->readByte();
					int fillCount = count;

					if (fillCount > sizeInBytes - pos) {
						warning("StdLbm::load(): run goes past the end of the image");
						fillCount = sizeInBytes - pos;

						if (fillCount < 0)
							fillCount = 0;
					}

					if (fillCount > 0)
						memset(pixels + pos, fill, fillCount);

					pos += count;
					rowCount += count;
				} else {
					int count = control + 1;

					rowCount += count;

					for (int i = 0; i < count; i++) {
						byte literal = file->readByte();

						if (pos < sizeInBytes)
							pixels[pos] = literal;

						pos++;
					}
				}
			} while (rowCount < bmhd.width);
		}
	}

	_stdDisplay->vBufferUnlock(bitmap->frames[0]);
	bitmap->palette = new byte[768];
	file->seek(0);

	found = false;

	int paletteSize = 0;

	do {
		if (file->eos())
			break;

		if (file->readByte() == 'C' && file->readByte() == 'M' && file->readByte() == 'A' && file->readByte() == 'P') {
			byte chunkHeader[8];

			memset(chunkHeader, 0, sizeof(chunkHeader));
			file->seek(file->pos() - 4);
			file->read(chunkHeader, 8);
			paletteSize = swapLong(READ_LE_UINT32(chunkHeader + 4));
			found = true;
		}
	} while (!found);

	if (!found)
		return nullptr;

	if ((uint)paletteSize > 768) {
		warning("StdLbm::load(): palette larger than 768 bytes");
		paletteSize = 768;
	}

	file->read(bitmap->palette, paletteSize);
	delete file;

	return bitmap;
}

int StdLbm::write(const char *path, StdBitmapStruct *bitmap) {
	if (!bitmap->palette) {
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
	file.write(bitmap->palette, 768);

	int64 bodyStart = file.pos();

	file.write("BODYxxxx", 8);

	StdVBuffer *frame = bitmap->frames[0];

	_stdDisplay->vBufferLock(frame);

	const byte *pixels = frame->pixels;
	int width = frame->tex.width;
	byte *packBuffer = new byte[2 * width + 2];

	for (int row = 0; row < frame->tex.height; row++) {
		packRow(&file, pixels, width, packBuffer);
		pixels += width;
	}

	delete[] packBuffer;
	packBuffer = nullptr;
	_stdDisplay->vBufferUnlock(bitmap->frames[0]);

	int64 bodyEnd = file.pos();
	uint bodySize = swapLong((uint)(bodyEnd - bodyStart - 8));

	file.seek(bodyStart + 4);
	file.writeUint32LE(bodySize);
	file.seek(0, SEEK_END);

	uint formSize = swapLong((uint)(file.pos() - 8));

	frame = bitmap->frames[0];
	bmhd.width = (uint16)frame->tex.width;
	bmhd.height = (uint16)frame->tex.height;
	bmhd.xOrigin = 0;
	bmhd.yOrigin = 0;
	bmhd.planes = 8;
	bmhd.masking = 0;
	bmhd.compression = 1;
	bmhd.transparentColor = 0;
	bmhd.xAspect = 6;
	bmhd.yAspect = 5;
	bmhd.pageWidth = (uint16)frame->tex.width;
	bmhd.pageHeight = (uint16)frame->tex.height;
	file.seek(4);
	file.writeUint32LE(formSize);
	file.seek(20);
	swapBMHD(&bmhd);
	writeBmhd(&file, &bmhd);
	file.seek(0, SEEK_END);
	file.close();

	return 1;
}

uint32 StdLbm::packRow(Common::WriteStream *stream, const byte *pixels, int width, byte *buffer) {
	_packSize = 0;
	_packOut = buffer;
	_packState = 3;

	for (int i = 0; i < width; i++)
		packByte(pixels[i]);

	if (_packState == 2) {
		*_packOut = 0;
		_packOut++;
		*_packOut = _packLastByte;
		_packSize += 2;
	}

	return stream->write(buffer, _packSize);
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

void StdLbm::swapBMHD(StdLbmBmhd *bmhd) {
	bmhd->width = swapShort(bmhd->width);
	bmhd->height = swapShort(bmhd->height);
	bmhd->xOrigin = (int16)swapShort((uint16)bmhd->xOrigin);
	bmhd->yOrigin = (int16)swapShort((uint16)bmhd->yOrigin);
	bmhd->transparentColor = swapShort(bmhd->transparentColor);
	bmhd->pageWidth = swapShort(bmhd->pageWidth);
	bmhd->pageHeight = swapShort(bmhd->pageHeight);
}

uint16 StdLbm::swapShort(uint16 value) {
	return SWAP_BYTES_16(value);
}

const char *StdLbm::stripPath(const char *path) {
	const char *slash = strrchr(path, '\\');

	if (!slash)
		return path;

	if (*slash == '\\') {
		do {
			slash++;
		} while (*slash == '\\');
	}

	return slash;
}

const char *StdLbm::getExtension(const char *path) {
	const char *baseName = stripPath(path);
	const char *dot = strrchr(baseName, '.');

	if (!dot)
		return nullptr;

	for (const char *next = strrchr(dot + 1, '.'); next; next = strrchr(next + 1, '.'))
		dot = next;

	return dot + 1;
}

char *StdLbm::stripExtension(char *path) {
	const char *extension = getExtension(path);

	if (!extension)
		return nullptr;

	size_t offset = extension - path;

	path[offset - 1] = 0;

	return path + offset;
}

char *StdLbm::copyBaseName(char *dst, int size, const char *path) {
	if (size < 1) {
		warning("StdLbm::copyBaseName(): invalid buffer size");
		return nullptr;
	}

	const char *baseName = stripPath(path);

	strncpy(dst, baseName, size - 1);
	dst[size - 1] = 0;

	return stripExtension(dst);
}

StdBitmapStruct *StdLbm::loadFailed(Common::SeekableReadStream *file, StdBitmapStruct *bitmap) {
	delete file;
	_stdBitmap->unloadBitmap(bitmap);

	return nullptr;
}

void StdLbm::readBmhd(Common::ReadStream *stream, StdLbmBmhd *bmhd) {
	bmhd->width = stream->readUint16LE();
	bmhd->height = stream->readUint16LE();
	bmhd->xOrigin = stream->readSint16LE();
	bmhd->yOrigin = stream->readSint16LE();
	bmhd->planes = stream->readByte();
	bmhd->masking = stream->readByte();
	bmhd->compression = stream->readByte();
	bmhd->pad1 = stream->readByte();
	bmhd->transparentColor = stream->readUint16LE();
	bmhd->xAspect = stream->readByte();
	bmhd->yAspect = stream->readByte();
	bmhd->pageWidth = stream->readUint16LE();
	bmhd->pageHeight = stream->readUint16LE();
}

void StdLbm::writeBmhd(Common::WriteStream *stream, const StdLbmBmhd *bmhd) {
	stream->writeUint16LE(bmhd->width);
	stream->writeUint16LE(bmhd->height);
	stream->writeSint16LE(bmhd->xOrigin);
	stream->writeSint16LE(bmhd->yOrigin);
	stream->writeByte(bmhd->planes);
	stream->writeByte(bmhd->masking);
	stream->writeByte(bmhd->compression);
	stream->writeByte(bmhd->pad1);
	stream->writeUint16LE(bmhd->transparentColor);
	stream->writeByte(bmhd->xAspect);
	stream->writeByte(bmhd->yAspect);
	stream->writeUint16LE(bmhd->pageWidth);
	stream->writeUint16LE(bmhd->pageHeight);
}

} // End of namespace Grim
