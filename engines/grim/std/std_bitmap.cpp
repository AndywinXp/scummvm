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
#include "engines/grim/std/std_gif.h"
#include "engines/grim/std/std_lbm.h"
#include "engines/grim/std/std_pcx.h"
#include "engines/grim/std/std_pic.h"
#include "engines/grim/std/std_sys.h"

namespace Grim {

static const uint ditherTable[16] = {
	0x001F, 0xC01F, 0x301F, 0xF01F,
	0x801F, 0x401F, 0xB01F, 0x701F,
	0x201F, 0xE01F, 0x101F, 0xD01F,
	0xA01F, 0x601F, 0x901F, 0x501F
};

static bool isValidShift(int shift) {
	return shift >= 0 && shift < 32;
}

StdBitmap::StdBitmap(StdDisplay *stdDisplay, StdFile *stdFile, SysStruct *hs) : _stdDisplay(stdDisplay), _stdFile(stdFile), _hs(hs),
		_stdLbm(nullptr), _stdPcx(nullptr), _stdGif(nullptr), _stdPic(nullptr), _stdBmp(nullptr), _ditherCounter(0) {
}

StdBitmapStruct *StdBitmap::zbmLoad(const char *path, int videoMemory, int noZ) {
	StdBitmapStruct *bitmap = new StdBitmapStruct();

	if (readData(path, bitmap, videoMemory, noZ))
		return bitmap;

	delete bitmap;

	return nullptr;
}

int StdBitmap::readData(const char *fileName, StdBitmapStruct *bitmap, int videoMemory, int noZ) {
	Common::SeekableReadStream *file = _hs->openFile(fileName);

	if (!file)
		return 0;

	int result = loadBitmapData(file, bitmap, videoMemory, noZ);

	strncpy(bitmap->name, _stdFile->fromPath(fileName), 31);
	bitmap->name[31] = 0;
	delete file;

	return result;
}

int StdBitmap::loadBitmapData(Common::SeekableReadStream *file, StdBitmapStruct *bitmap, int videoMemory, int noZ) {
	StdBitmapFileHeader hdr;

	file->read(hdr.magic, 4);
	hdr.version = file->readSint32LE();
	hdr.encoding = file->readSint32LE();
	hdr.flags = file->readUint32LE();
	hdr.numFrames = file->readSint32LE();
	hdr.xPos = file->readSint32LE();
	hdr.yPos = file->readSint32LE();
	hdr.colorKey = file->readSint32LE();
	readColorFormat(file, &hdr.format);
	file->read(hdr.pad, 40);

	if (file->eos()) {
		warning("StdBitmap::loadBitmapData(): file shorter than the bitmap header");
		return 0;
	}

	if (memcmp(hdr.magic, "BM  ", 4))
		return 0;

	if (hdr.version != 70)
		return 0;

	if ((int64)hdr.numFrames * 8 > file->size() - file->pos()) {
		warning("StdBitmap::loadBitmapData(): frame count larger than the file");
		return 0;
	}

	if (!prepareBitmaps(bitmap, hdr.numFrames, nullptr, hdr.encoding, hdr.flags, &hdr.format))
		return 0;

	bitmap->xPos = hdr.xPos;
	bitmap->colorKey = hdr.colorKey;
	bitmap->yPos = hdr.yPos;

	StdVBufferTexFmt format;
	memset(&format, 0, sizeof(format));

	for (uint index = 0; index < bitmap->numFrames; index++) {
		int frameWidth = file->readSint32LE();
		uint frameHeight = file->readUint32LE();

		if (frameWidth < 0 || (int)frameHeight < 0 || (int64)frameWidth * (int64)frameHeight * (int64)((uint)bitmap->format.bpp >> 3) > 0x7FFFFFFF) {
			warning("StdBitmap::loadBitmapData(): invalid frame size");
			return 0;
		}

		format.height = frameHeight;
		format.width = frameWidth;
		format.format = bitmap->format;

		StdVBuffer *frame = _stdDisplay->vBufferNew(&format, videoMemory, noZ, nullptr);

		if (!frame)
			return 0;

		addFrame(bitmap, index, frame);
		_stdDisplay->vBufferLock(frame);

		if (bitmap->encoding == 0) {
			byte *rowPointer = frame->pixels;
			uint rowBytes = frame->tex.width * ((uint)frame->tex.format.bpp >> 3);

			for (uint row = 0; row < frameHeight; row++) {
				file->read(rowPointer, rowBytes);
				rowPointer += frame->tex.pitchInBytes;
			}
		} else if (bitmap->encoding == 1) {
			byte *out = frame->pixels;

			if (frame->tex.sizeInBytes) {
				do {
					byte value = file->readByte();

					if ((value & 0xC0) == 0xC0) {
						int count = value & 0x3F;
						byte fill = file->readByte();

						if (count > frame->tex.sizeInBytes - (int)(out - frame->pixels)) {
							warning("StdBitmap::loadBitmapData(): run goes past the end of the frame");
							count = frame->tex.sizeInBytes - (int)(out - frame->pixels);
						}

						if (count) {
							memset(out, fill, count);
							out += count;
						}
					} else {
						*out++ = value;
					}
				} while ((uint)(out - frame->pixels) < (uint)frame->tex.sizeInBytes);
			}
		} else if (bitmap->encoding == 3) {
			uint compressedSize = file->readUint32LE();

			if (compressedSize < (uint)frame->tex.sizeInBytes) {
				byte *compressed = new byte[compressedSize];

				file->read(compressed, compressedSize);
				frame->type = 2;
				frame->tex.sizeInBytes = compressedSize;
				delete[] frame->pixels;
				frame->pixels = compressed;
			}
		}

		_stdDisplay->vBufferUnlock(frame);

		if (bitmap->flags & 1)
			_stdDisplay->vBufferSetColorKey(frame, bitmap->colorKey);
	}

	if (bitmap->flags & 2) {
		bitmap->palette = new byte[768];
		file->read(bitmap->palette, 768);
	}

	return 1;
}

void StdBitmap::unloadBitmap(StdBitmapStruct *&bitmap) {
	freeEntry(bitmap);
	delete bitmap;
	bitmap = nullptr;
}

void StdBitmap::freeEntry(StdBitmapStruct *bitmap) {
	if (bitmap->frames) {
		for (uint i = 0; i < bitmap->numFrames; i++) {
			if (bitmap->frames[i])
				_stdDisplay->vBufferFree(bitmap->frames[i]);
		}

		delete[] bitmap->frames;
		bitmap->frames = nullptr;
	}

	if (bitmap->palette) {
		delete[] bitmap->palette;
		bitmap->palette = nullptr;
	}
}

int StdBitmap::appendToFile(Common::WriteStream *stream, StdBitmapStruct *bitmap) {
	StdBitmapFileHeader hdr;

	memset(&hdr, 0, sizeof(hdr));
	strncpy(hdr.magic, "BM  ", 4);
	hdr.version = 70;
	hdr.encoding = bitmap->encoding;
	hdr.flags = bitmap->flags;
	hdr.numFrames = bitmap->numFrames;
	hdr.xPos = bitmap->xPos;
	hdr.yPos = bitmap->yPos;
	hdr.colorKey = bitmap->colorKey;
	hdr.format = bitmap->format;

	stream->write(hdr.magic, 4);
	stream->writeSint32LE(hdr.version);
	stream->writeSint32LE(hdr.encoding);
	stream->writeUint32LE(hdr.flags);
	stream->writeSint32LE(hdr.numFrames);
	stream->writeSint32LE(hdr.xPos);
	stream->writeSint32LE(hdr.yPos);
	stream->writeSint32LE(hdr.colorKey);
	writeColorFormat(stream, &hdr.format);
	stream->write(hdr.pad, 40);

	if (stream->err())
		return 0;

	for (uint i = 0; i < bitmap->numFrames; i++) {
		StdVBuffer *frame = bitmap->frames[i];
		int width = frame->tex.width;
		uint height = frame->tex.height;

		stream->writeSint32LE(width);
		stream->writeUint32LE(height);

		if (stream->err())
			return 0;

		if (hdr.encoding == 0) {
			_stdDisplay->vBufferLock(frame);

			byte *pixels = frame->pixels;
			uint rowBytes = ((uint)frame->tex.format.bpp >> 3) * width;

			for (uint row = 0; row < height; row++) {
				if (stream->write(pixels, rowBytes) != rowBytes)
					return 0;

				pixels += frame->tex.pitchInBytes;
			}

			_stdDisplay->vBufferUnlock(frame);
		} else if (hdr.encoding == 1) {
			byte *pixels = frame->pixels;

			for (uint y = 0; y < (uint)frame->tex.height; y++) {
				uint rowWidth = frame->tex.width;
				byte *rowPointer = pixels + y * rowWidth;
				uint x = 0;

				while (x < rowWidth) {
					byte value = rowPointer[x];
					byte run = 1;

					while (run < 63 && x + run < rowWidth && rowPointer[x + run] == value)
						run++;

					if (run > 1 || value > 0xBF)
						stream->writeByte(run | 0xC0);

					stream->writeByte(value);
					x += run;
				}
			}
		} else if (hdr.encoding == 3) {
			_stdDisplay->vBufferLock(frame);

			uint sizeInBytes = frame->tex.sizeInBytes;

			stream->writeUint32LE(sizeInBytes);
			stream->write(frame->pixels, sizeInBytes);
			_stdDisplay->vBufferUnlock(frame);
		}
	}

	if (bitmap->flags & 2) {
		if (stream->write(bitmap->palette, 768) != 768)
			return 0;
	}

	return 1;
}

int StdBitmap::write(const char *path, StdBitmapStruct *bitmap) {
	Common::DumpFile file;

	if (!file.open(path))
		return 0;

	if (appendToFile(&file, bitmap)) {
		file.close();
		return 1;
	}

	file.close();

	return 0;
}

void StdBitmap::convertColorFormat(StdColorFormat *format, StdBitmapStruct *bitmap) {
	if (!memcmp(format, &bitmap->format, sizeof(StdColorFormat)))
		return;

	if (!bitmap->format.colorMode && !format->colorMode)
		return;

	for (int i = 0; i < (int)bitmap->numFrames; i++) {
		if (bitmap->frames[i]->type != 2) {
			bitmap->frames[i] = _stdDisplay->vBufferConvertColorFormat(format, bitmap->frames[i]);

			if (!bitmap->frames[i])
				error("Unable to allocate a new frame when converting image from 24 to 16bpp.");

			if (bitmap->flags & 1)
				_stdDisplay->vBufferSetColorKey(bitmap->frames[i], bitmap->frames[i]->colorKey);
		}
	}

	if (bitmap->flags & 1)
		bitmap->colorKey = convertColor(format, bitmap->colorKey, &bitmap->format);

	bitmap->format = *format;
}

StdBitmapStruct *StdBitmap::vBufferToBitmap(StdVBuffer *src, int videoMemory, int noZ) {
	StdBitmapStruct *bitmap = new StdBitmapStruct();

	bitmap->encoding = 0;

	if (src->paletteRGB)
		bitmap->flags = 2;
	else
		bitmap->flags = 0;

	bitmap->numFrames = 1;
	bitmap->curFrame = 0;
	bitmap->xPos = 0;
	bitmap->yPos = 0;
	bitmap->frames = new StdVBuffer *[1];
	bitmap->frames[0] = _stdDisplay->vBufferNew(&src->tex, videoMemory, noZ, nullptr);

	if (!bitmap->frames[0])
		return nullptr;

	_stdDisplay->vBufferCopy(bitmap->frames[0], src, 0, 0, nullptr, 0);
	_stdDisplay->vBufferUnlock(src);
	bitmap->format = bitmap->frames[0]->tex.format;

	if (bitmap->flags & 2) {
		bitmap->palette = new byte[768];
		memcpy(bitmap->palette, src->paletteRGB, 768);
	} else {
		bitmap->palette = nullptr;
	}

	return bitmap;
}

int StdBitmap::prepareBitmaps(StdBitmapStruct *bitmap, int numFrames, byte *palette, int encoding, uint flags, const StdColorFormat *format) {
	memset(bitmap, 0, sizeof(StdBitmapStruct));

	if (numFrames < 0) {
		warning("StdBitmap::prepareBitmaps(): negative frame count");
		return 0;
	}

	bitmap->frames = new StdVBuffer *[numFrames]();
	bitmap->flags = flags;
	bitmap->encoding = encoding;
	bitmap->format = *format;
	bitmap->numFrames = numFrames;
	bitmap->palette = palette;

	return 1;
}

void StdBitmap::addFrame(StdBitmapStruct *bitmap, int index, StdVBuffer *frame) {
	bitmap->frames[index] = frame;
}

int StdBitmap::memUsage(StdBitmapStruct *bitmap) {
	int size = 0;

	for (int i = 0; i < (int)bitmap->numFrames; i++) {
		if (!bitmap->frames[i]->videoMemory)
			size += bitmap->frames[i]->tex.sizeInBytes;
	}

	return size + 124;
}

void StdBitmap::deCompress(StdBitmapStruct *bitmap) {
	for (int i = 0; i < (int)bitmap->numFrames; i++)
		_stdDisplay->vBufferDeCompress(bitmap->frames[i]);

	bitmap->encoding = 0;
}

void StdBitmap::applyGamma(byte *dst, const byte *src, int count, double gamma) {
	for (int i = 0; i < count; i++) {
		for (int component = 0; component < 3; component++) {
			double value = pow((double)*src * 0.00392156862745098, gamma) * 255.0;

			if (value > 255.0)
				value = 255.0;

			if (value != value) {
				warning("StdBitmap::applyGamma(): gamma produced an invalid value");
				value = 0.0;
			}

			*dst = (byte)(int)value;
			src++;
			dst++;
		}
	}
}

int StdBitmap::buildPaletteOpaque(const byte *palette, uint16 *table, const StdColorFormat *format) {
	if (!isValidShift(format->redLoss) || !isValidShift(format->greenLoss) || !isValidShift(format->blueLoss) ||
		!isValidShift(format->redShift) || !isValidShift(format->greenShift) || !isValidShift(format->blueShift)) {
		warning("StdBitmap::buildPaletteOpaque(): invalid color format");
		return 0;
	}

	for (int i = 0; i < 256; i++) {
		uint red = palette[0] >> format->redLoss;
		uint green = palette[1] >> format->greenLoss;
		uint blue = palette[2] >> format->blueLoss;

		palette += 3;
		table[i] = (uint16)((blue << format->blueShift) | (green << format->greenShift) | (red << format->redShift));
	}

	return 1;
}

int StdBitmap::buildPaletteAlpha(const byte *palette, uint16 *table, const StdColorFormat *format) {
	if (!isValidShift(format->redLoss) || !isValidShift(format->greenLoss) || !isValidShift(format->blueLoss) ||
		!isValidShift(format->redShift) || !isValidShift(format->greenShift) || !isValidShift(format->blueShift) ||
		!isValidShift(format->alphaLoss) || !isValidShift(format->alphaShift)) {
		warning("StdBitmap::buildPaletteAlpha(): invalid color format");
		return 0;
	}

	for (int i = 0; i < 256; i++) {
		uint red = palette[0] >> format->redLoss;
		uint green = palette[1] >> format->greenLoss;
		uint blue = palette[2] >> format->blueLoss;
		byte alpha = 0;

		if (i != 0)
			alpha = 0xFF;

		palette += 3;
		table[i] = (uint16)((blue << format->blueShift) | (green << format->greenShift) | (red << format->redShift) | (((uint)alpha >> format->alphaLoss) << format->alphaShift));
	}

	return 1;
}

int StdBitmap::buildPaletteColorKey(const byte *palette, uint16 *table, const StdColorFormat *format, byte alpha) {
	if (!isValidShift(format->redLoss) || !isValidShift(format->greenLoss) || !isValidShift(format->blueLoss) ||
		!isValidShift(format->redShift) || !isValidShift(format->greenShift) || !isValidShift(format->blueShift) ||
		!isValidShift(format->alphaLoss) || !isValidShift(format->alphaShift)) {
		warning("StdBitmap::buildPaletteColorKey(): invalid color format");
		return 0;
	}

	for (int i = 0; i < 256; i++) {
		uint red = palette[0] >> format->redLoss;
		uint green = palette[1] >> format->greenLoss;
		uint blue = palette[2] >> format->blueLoss;
		uint entryAlpha = 0;

		if (i != 0)
			entryAlpha = alpha;

		palette += 3;
		table[i] = (uint16)((blue << format->blueShift) | (green << format->greenShift) | (red << format->redShift) | ((entryAlpha >> format->alphaLoss) << format->alphaShift));
	}

	return 1;
}

void StdBitmap::convertPixelRow(byte *dst, StdColorFormat *dstFormat, const byte *src, StdColorFormat *srcFormat, int width) {
	_ditherCounter++;

	bool rgb24To16 = srcFormat->bpp == 24 && dstFormat->bpp == 16;
	bool rgb16To16 = srcFormat->bpp == 16 && dstFormat->bpp == 16;
	bool rgb16To24 = srcFormat->bpp == 16 && dstFormat->bpp == 24;

	if (!rgb24To16 && !rgb16To16 && !rgb16To24)
		return;

	if (width <= 0) {
		warning("StdBitmap::convertPixelRow(): invalid width");
		return;
	}

	if (srcFormat->redBits < 1 || srcFormat->redBits > 30 || srcFormat->greenBits < 1 || srcFormat->greenBits > 30 || srcFormat->blueBits < 1 || srcFormat->blueBits > 30 ||
		dstFormat->redBits < 0 || dstFormat->redBits > 15 || dstFormat->greenBits < 0 || dstFormat->greenBits > 15 || dstFormat->blueBits < 0 || dstFormat->blueBits > 15) {
		warning("StdBitmap::convertPixelRow(): invalid color bits");
		return;
	}

	uint redScale = (int)((double)((1 << dstFormat->redBits) - 1) * (1.0 / (double)((1 << srcFormat->redBits) - 1)) * 65536.0);
	uint greenScale = (int)((double)((1 << dstFormat->greenBits) - 1) * (1.0 / (double)((1 << srcFormat->greenBits) - 1)) * 65536.0);
	uint blueScale = (int)((double)((1 << dstFormat->blueBits) - 1) * (1.0 / (double)((1 << srcFormat->blueBits) - 1)) * 65536.0);
	int redShift = dstFormat->redShift;
	int greenShift = dstFormat->greenShift;
	const byte *in = src;
	byte *out = dst;
	int remaining = width;

	if (rgb24To16) {
		if (!isValidShift(redShift) || !isValidShift(greenShift)) {
			warning("StdBitmap::convertPixelRow(): invalid color shift");
			return;
		}

		const uint *ditherPointer = &ditherTable[(_ditherCounter & 3) * 4];

		do {
			uint dither = ditherPointer[remaining & 3];
			uint red = (in[0] * redScale + dither) >> 16;
			uint green = (in[1] * greenScale + dither) >> 16;
			uint blue = (in[2] * blueScale + dither) >> 16;

			WRITE_LE_UINT16(out, (uint16)((red << redShift) | (green << greenShift) | blue));
			in += 3;
			out += 2;
			remaining--;
		} while (remaining);
	} else if (rgb16To16) {
		if (!isValidShift(redShift) || !isValidShift(greenShift) || !isValidShift(srcFormat->redShift) || !isValidShift(srcFormat->greenShift)) {
			warning("StdBitmap::convertPixelRow(): invalid color shift");
			return;
		}

		uint redMask = 0xFFFFFFFF >> (32 - srcFormat->redBits);
		uint greenMask = 0xFFFFFFFF >> (32 - srcFormat->greenBits);
		uint blueMask = 0xFFFFFFFF >> (32 - srcFormat->blueBits);

		do {
			uint pixel = READ_LE_UINT16(in);
			uint red = (redScale * (redMask & (pixel >> srcFormat->redShift))) >> 16;
			uint green = (greenScale * (greenMask & (pixel >> srcFormat->greenShift))) >> 16;
			uint blue = (blueScale * (blueMask & pixel)) >> 16;

			WRITE_LE_UINT16(out, (uint16)((red << redShift) | (green << greenShift) | blue));
			in += 2;
			out += 2;
			remaining--;
		} while (remaining);
	} else {
		if (!isValidShift(srcFormat->redShift) || !isValidShift(srcFormat->greenShift) ||
			!isValidShift(srcFormat->redLoss) || !isValidShift(srcFormat->greenLoss) || !isValidShift(srcFormat->blueLoss)) {
			warning("StdBitmap::convertPixelRow(): invalid color shift");
			return;
		}

		uint redMask = 0xFFFFFFFF >> (32 - srcFormat->redBits);
		uint greenMask = 0xFFFFFFFF >> (32 - srcFormat->greenBits);
		uint blueMask = 0xFFFFFFFF >> (32 - srcFormat->blueBits);

		do {
			uint pixel = READ_LE_UINT16(in);
			byte red = (byte)(pixel >> srcFormat->redShift) & (byte)redMask;
			byte green = (byte)(pixel >> srcFormat->greenShift) & (byte)greenMask;
			byte blue = (byte)pixel & (byte)blueMask;

			out[0] = (byte)(red << srcFormat->redLoss);
			out[1] = (byte)(green << srcFormat->greenLoss);
			out[2] = (byte)(blue << srcFormat->blueLoss);
			in += 2;
			out += 3;
			remaining--;
		} while (remaining);
	}
}

int StdBitmap::convertColor(StdColorFormat *dstFormat, int color, StdColorFormat *srcFormat) {
	byte srcColor[4];
	byte dstColor[4];

	WRITE_LE_UINT32(srcColor, color);
	memset(dstColor, 0, sizeof(dstColor));
	convertPixelRow(dstColor, dstFormat, srcColor, srcFormat, 1);

	return READ_LE_INT32(dstColor);
}

StdBitmapStruct *StdBitmap::loadImage(Common::String &path, int videoMemory, int noZ) {
	StdBitmapStruct *bitmap = loadLbm(path, videoMemory, noZ);

	if (bitmap)
		deCompress(bitmap);

	return bitmap;
}

StdBitmapStruct *StdBitmap::loadLbm(Common::String &path, int videoMemory, int noZ) {
	// TODO: verify the supported games need the .lbm/.pcx/.gif/.pic/.bmp loaders
	Common::String extension;
	size_t dot = path.findLastOf('.');

	if (dot != Common::String::npos) {
		extension = path.substr(dot, 4);
	} else {
		path += ".lbm";
		extension = ".lbm";
	}

	if (extension.equalsIgnoreCase(".lbm"))
		return _stdLbm->load(path.c_str(), videoMemory, noZ);

	if (extension.equalsIgnoreCase(".pcx"))
		return _stdPcx->load(path.c_str(), videoMemory, noZ);

	if (extension.equalsIgnoreCase(".gif"))
		return _stdGif->load(path.c_str(), videoMemory, noZ);

	if (extension.equalsIgnoreCase(".bm") || extension.equalsIgnoreCase(".zbm"))
		return zbmLoad(path.c_str(), videoMemory, noZ);

	if (extension.equalsIgnoreCase(".pic"))
		return _stdPic->load(path.c_str(), videoMemory, noZ);

	if (extension.equalsIgnoreCase(".bmp"))
		return _stdBmp->load(path.c_str(), videoMemory, noZ);

	return nullptr;
}

int StdBitmap::loadLbm2(Common::String &path, StdBitmapStruct *bitmap) {
	Common::String extension;
	const char *extensionPointer = _stdLbm->getExtension(path.c_str());

	if (!extensionPointer) {
		path += ".lbm";
		extension = ".lbm";
	} else {
		extension = Common::String(extensionPointer).substr(0, 4);
	}

	if (!extension.equalsIgnoreCase("bm") && !extension.equalsIgnoreCase("zbm"))
		deCompress(bitmap);

	if (extension.equalsIgnoreCase("lbm"))
		return _stdLbm->write(path.c_str(), bitmap) != 0;

	if (extension.equalsIgnoreCase("pcx"))
		return _stdPcx->write(path.c_str(), bitmap) != 0;

	if (extension.equalsIgnoreCase("gif"))
		return _stdGif->write(path.c_str(), bitmap) != 0;

	if (extension.equalsIgnoreCase("bm"))
		return write(path.c_str(), bitmap) != 0;

	if (extension.equalsIgnoreCase("zbm")) {
		bitmap->format.colorMode = 5;

		for (int i = 0; i < (int)bitmap->numFrames; i++)
			bitmap->frames[i]->tex.format.colorMode = 5;

		return write(path.c_str(), bitmap) != 0;
	}

	return extension.equalsIgnoreCase("bmp") && _stdBmp->write(path.c_str(), bitmap) != 0;
}

void StdBitmap::readColorFormat(Common::ReadStream *stream, StdColorFormat *format) {
	format->colorMode = stream->readSint32LE();
	format->bpp = stream->readSint32LE();
	format->redBits = stream->readSint32LE();
	format->greenBits = stream->readSint32LE();
	format->blueBits = stream->readSint32LE();
	format->redShift = stream->readSint32LE();
	format->greenShift = stream->readSint32LE();
	format->blueShift = stream->readSint32LE();
	format->redLoss = stream->readSint32LE();
	format->greenLoss = stream->readSint32LE();
	format->blueLoss = stream->readSint32LE();
	format->alphaBits = stream->readSint32LE();
	format->alphaShift = stream->readSint32LE();
	format->alphaLoss = stream->readSint32LE();
}

void StdBitmap::writeColorFormat(Common::WriteStream *stream, const StdColorFormat *format) {
	stream->writeSint32LE(format->colorMode);
	stream->writeSint32LE(format->bpp);
	stream->writeSint32LE(format->redBits);
	stream->writeSint32LE(format->greenBits);
	stream->writeSint32LE(format->blueBits);
	stream->writeSint32LE(format->redShift);
	stream->writeSint32LE(format->greenShift);
	stream->writeSint32LE(format->blueShift);
	stream->writeSint32LE(format->redLoss);
	stream->writeSint32LE(format->greenLoss);
	stream->writeSint32LE(format->blueLoss);
	stream->writeSint32LE(format->alphaBits);
	stream->writeSint32LE(format->alphaShift);
	stream->writeSint32LE(format->alphaLoss);
}

} // End of namespace Grim
