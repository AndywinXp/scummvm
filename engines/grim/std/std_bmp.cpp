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
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/std/std_bitmap.h"
#include "engines/grim/std/std_bmp.h"
#include "engines/grim/std/std_file.h"
#include "engines/grim/std/std_sys.h"

namespace Grim {

StdBmp::StdBmp(StdDisplay *stdDisplay, StdFile *stdFile, SysStruct *hs) : _stdDisplay(stdDisplay), _stdFile(stdFile), _hs(hs) {
}

StdBitmapStruct *StdBmp::load(const char *path, int videoMemory, int noZ) {
	StdBitmapStruct *bitmap = new StdBitmapStruct();

	if (loadEntry(path, bitmap, videoMemory, noZ))
		return bitmap;

	return nullptr;
}

int StdBmp::loadEntry(const char *path, StdBitmapStruct *bitmap, int videoMemory, int noZ) {
	byte *palette = nullptr;
	Common::SeekableReadStream *file = _hs->openFile(path);

	if (!file)
		return 0;

	byte fileHeader[14];
	byte infoHeader[40];

	memset(fileHeader, 0, sizeof(fileHeader));
	memset(infoHeader, 0, sizeof(infoHeader));

	uint32 numRead = file->read(fileHeader, 14);

	if (file->read(infoHeader, 40) + numRead != 54)
		return 0;

	uint16 bfType = READ_LE_UINT16(fileHeader);
	uint32 biSize = READ_LE_UINT32(infoHeader);
	int32 biWidth = READ_LE_INT32(infoHeader + 4);
	int32 biHeight = READ_LE_INT32(infoHeader + 8);
	uint16 biBitCount = READ_LE_UINT16(infoHeader + 14);

	if (bfType != 19778)
		return 0;

	if (biSize != 40)
		return 0;

	if (biBitCount < 8) {
		uint32 paletteSize = 4 * (1 << biBitCount);

		palette = new byte[paletteSize + 44];

		if (file->read(palette, paletteSize) != paletteSize)
			error("Unable to read the palette.  Your file is corrupted.");
	}

	memset(bitmap, 0, sizeof(StdBitmapStruct));
	strncpy(bitmap->name, _stdFile->fromPath(path), 31);
	bitmap->name[31] = 0;
	bitmap->encoding = 0;
	bitmap->flags = 0;
	bitmap->palette = palette;
	bitmap->numFrames = 1;
	bitmap->curFrame = 0;

	switch (biBitCount) {
	case 8:
		bitmap->flags = 2;
		bitmap->format.bpp = biBitCount;
		bitmap->format.colorMode = 0;
		break;
	case 16:
		error("16 bit-per-pixel BMP not yet supported.");
		break;
	case 24:
		bitmap->format.colorMode = 1;
		bitmap->format.bpp = 24;
		bitmap->format.redBits = 8;
		bitmap->format.greenBits = 8;
		bitmap->format.blueBits = 8;
		bitmap->format.redShift = 16;
		bitmap->format.greenShift = 8;
		bitmap->format.blueShift = 0;
		break;
	case 32:
		error("32bit-per-pixel BMP not yet supported.");
		break;
	default:
		error("This BMP file uses a bit-depth which I do not support yet.");
		break;
	}

	bitmap->frames = new StdVBuffer *[bitmap->numFrames]();

	if (biWidth < 0 || biHeight == (int32)0x80000000) {
		warning("StdBmp::loadEntry(): invalid image size");
		return 0;
	}

	StdVBufferTexFmt format;

	memset(&format, 0, sizeof(format));
	format.format = bitmap->format;
	format.width = biWidth;

	if (biHeight < 0)
		format.height = -biHeight;
	else
		format.height = biHeight;

	if ((int64)format.width * (int64)format.height * (int64)(bitmap->format.bpp >> 3) > 0x7FFFFFFF) {
		warning("StdBmp::loadEntry(): invalid image size");
		return 0;
	}

	bitmap->frames[0] = _stdDisplay->vBufferNew(&format, videoMemory, noZ, palette);

	if (!bitmap->frames[0])
		error("Unable to allocate memory.");

	if (!_stdDisplay->vBufferLock(bitmap->frames[0]))
		error("Unable to lock VBuffer memory.");

	StdVBuffer *frame = bitmap->frames[0];
	int height = frame->tex.height;
	byte *dstRow = frame->pixels;
	int pitchInBytes = frame->tex.pitchInBytes;
	byte *pixels = dstRow;

	for (int row = 0; row < height; row++) {
		if (file->read(pixels, frame->tex.pitchInBytes) != (uint32)frame->tex.pitchInBytes)
			error("Unable to read all the data from your BMP file.  It may be corrupted.");

		file->seek(((pitchInBytes + 3) & ~3) - pitchInBytes, SEEK_CUR);

		if (frame->tex.format.bpp == 24) {
			byte *pixel = pixels;

			for (int x = 0; x < frame->tex.width; x++) {
				byte swap = pixel[0];

				pixel[0] = pixel[2];
				pixel[2] = swap;
				pixel += 3;
			}
		}

		pixels += pitchInBytes;
	}

	if (biHeight > 0) {
		for (int row = 0; row < height / 2; row++) {
			byte *top = dstRow + row * pitchInBytes;
			byte *bottom = dstRow + (height - 1 - row) * pitchInBytes;

			for (int x = 0; x < pitchInBytes; x++) {
				byte swap = top[x];

				top[x] = bottom[x];
				bottom[x] = swap;
			}
		}
	}

	_stdDisplay->vBufferUnlock(bitmap->frames[0]);
	delete file;

	return 1;
}

int StdBmp::write(const char *path, StdBitmapStruct *bitmap) {
	StdVBuffer *frame = bitmap->frames[0];
	int paletteSize;

	if (frame->tex.format.colorMode) {
		if (frame->tex.format.bpp != 24)
			error("Only 24bpp supported");

		paletteSize = 0;
	} else {
		if (frame->tex.format.bpp < 1 || frame->tex.format.bpp > 8) {
			warning("StdBmp::write(): unsupported bits per pixel for a palettized image");
			return 0;
		}

		paletteSize = 4 * (1 << frame->tex.format.bpp);
	}

	uint32 imageSize = (uint32)frame->tex.format.bpp * (uint32)frame->tex.height * (uint32)frame->tex.width;
	byte fileHeader[14];
	byte infoHeader[40];

	imageSize >>= 3;
	WRITE_LE_UINT16(fileHeader, 19778);
	WRITE_LE_UINT32(fileHeader + 2, imageSize + paletteSize + 54);
	WRITE_LE_UINT16(fileHeader + 6, 0);
	WRITE_LE_UINT16(fileHeader + 8, 0);
	WRITE_LE_UINT32(fileHeader + 10, paletteSize + 54);
	memset(infoHeader, 0, sizeof(infoHeader));
	WRITE_LE_UINT32(infoHeader, 40);
	WRITE_LE_INT32(infoHeader + 4, frame->tex.width);
	WRITE_LE_INT32(infoHeader + 8, frame->tex.height);
	WRITE_LE_UINT16(infoHeader + 12, 1);
	WRITE_LE_UINT16(infoHeader + 14, frame->tex.format.bpp);
	WRITE_LE_UINT32(infoHeader + 16, 0);
	WRITE_LE_UINT32(infoHeader + 20, imageSize);
	WRITE_LE_UINT32(infoHeader + 24, 2834);
	WRITE_LE_UINT32(infoHeader + 28, 2834);

	Common::DumpFile file;

	if (!file.open(path))
		return 0;

	bool written = file.write(fileHeader, 14) == 14 && file.write(infoHeader, 40) == 40;

	if (written && paletteSize > 0) {
		int copySize = paletteSize;

		if (!bitmap->palette) {
			warning("StdBmp::write(): bitmap without a palette");
			copySize = 0;
		} else if (copySize > 768) {
			warning("StdBmp::write(): palette smaller than the file palette");
			copySize = 768;
		}

		if (copySize > 0)
			written = file.write(bitmap->palette, copySize) == (uint32)copySize;

		for (int i = copySize; written && i < paletteSize; i++)
			file.writeByte(0);
	}

	if (written) {
		int rowBytes = ((uint32)frame->tex.format.bpp * (uint32)frame->tex.width) >> 3;

		for (int y = frame->tex.height - 1; y >= 0; y--) {
			byte *row = frame->pixels + frame->tex.pitchInBytes * y;

			if (frame->tex.format.bpp == 24) {
				byte *pixel = row;

				for (int x = 0; x < frame->tex.width; x++) {
					byte swap = pixel[0];

					pixel[0] = pixel[2];
					pixel[2] = swap;
					pixel += 3;
				}
			}

			if (file.write(row, rowBytes) != (uint32)rowBytes) {
				file.close();
				return 0;
			}
		}

		file.close();

		return 1;
	}

	file.close();

	return 0;
}

} // End of namespace Grim
