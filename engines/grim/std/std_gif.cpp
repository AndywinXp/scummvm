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
#include "engines/grim/std/std_file.h"
#include "engines/grim/std/std_gif.h"
#include "engines/grim/std/std_sys.h"

namespace Grim {

static const int tableLimit = 4096;
static const int hashSize = 5003;
static const byte keepMask[8] = { 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F };
static const byte setMask[8] = { 0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80 };
static const int interlaceStart[4] = { 0, 4, 2, 1 };
static const int interlaceStep[4] = { 8, 8, 4, 2 };

StdGif::StdGif(StdDisplay *stdDisplay, StdFile *stdFile, StdBitmap *stdBitmap, SysStruct *hs) : _stdDisplay(stdDisplay), _stdFile(stdFile), _stdBitmap(stdBitmap), _hs(hs),
		_oldCode(0), _initCodeSize(0), _firstFree(0), _nextFree(0), _clearCode(0), _endCode(0), _curCodeSize(0), _firstChar(0), _fresh(0), _bitOffset(0), _codeMask(0), _bitBuffer(0),
		_blockPos(0), _blockSize(0), _blockBuf(), _stack(_stackArray), _stackArray(), _prefix(), _suffix(),
		_encOutCount(0), _encPrefix(0), _encHash(0), _encFreeEnt(0), _encHashShift(0), _hashTable(), _hashCode() {
}

StdBitmapStruct *StdGif::load(const char *path, int videoMemory, int noZ) {
	Common::SeekableReadStream *file = _hs->openFile(path);

	if (!file)
		return nullptr;

	StdBitmapStruct *bitmap = new StdBitmapStruct();
	byte signature[3];

	memset(signature, 0, sizeof(signature));
	file->read(signature, 3);

	if (toupper(signature[0]) != 'G' || toupper(signature[1]) != 'I' || toupper(signature[2]) != 'F')
		return loadFailed(file, bitmap);

	byte version[3];

	memset(version, 0, sizeof(version));
	file->read(version, 3);
	file->skip(4);

	byte screenFlags = file->readByte();

	file->skip(2);

	if (screenFlags & 0x80) {
		int numColors = 1 << ((screenFlags & 7) + 1);

		bitmap->palette = new byte[768];
		file->read(bitmap->palette, 3 * numColors);
	}

	int separator;

	if (version[0] == '8' && version[1] == '9' && toupper(version[2]) == 'A') {
		separator = (signed char)file->readByte();

		while (separator == '!') {
			byte data;

			do {
				data = file->readByte();
			} while (data);

			separator = (signed char)file->readByte();
		}
	} else {
		separator = (signed char)file->readByte();
	}

	if (separator != ',')
		return loadFailed(file, bitmap);

	byte descriptor[9];

	memset(descriptor, 0, sizeof(descriptor));
	file->read(descriptor, 9);

	int left = READ_LE_INT16(descriptor);
	int imageWidth = READ_LE_INT16(descriptor + 4);
	int imageHeight = READ_LE_INT16(descriptor + 6);
	byte imageFlags = descriptor[8];

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
	format.width = imageWidth;
	format.height = imageHeight;
	format.format.colorMode = 0;
	format.format.bpp = 8;

	if (left < 0 || imageWidth < 0 || imageHeight < 0 || (int64)imageWidth * (int64)imageHeight > 0x7FFFFFFF) {
		warning("StdGif::load(): invalid image descriptor");
		return loadFailed(file, bitmap);
	}

	bitmap->frames[0] = _stdDisplay->vBufferNew(&format, videoMemory, noZ, nullptr);

	if (!bitmap->frames[0])
		return loadFailed(file, bitmap);

	if (imageFlags & 0x80) {
		int numColors = 1 << ((imageFlags & 7) + 1);

		if (!bitmap->palette)
			bitmap->palette = new byte[768];

		file->read(bitmap->palette, 3 * numColors);
	}

	StdVBuffer *frame = bitmap->frames[0];
	byte *rowBuffer = new byte[left + frame->tex.width];
	byte minCodeSize = file->readByte();

	if (lzwInitDecoder((signed char)minCodeSize)) {
		delete[] rowBuffer;
		rowBuffer = nullptr;

		return loadFailed(file, bitmap);
	}

	int row = 0;
	int step;

	if (imageFlags & 2)
		step = 8;
	else
		step = 1;

	_stdDisplay->vBufferLock(frame);

	int pass = 0;
	int rowsDecoded = 0;
	int width = frame->tex.width;
	int height = frame->tex.height;

	if (height > 0) {
		do {
			int x = 0;

			if (left + width > 0) {
				do {
					rowBuffer[x] = (byte)lzwDecodeByte(file);
					x++;
				} while (x < left + width);
			}

			if (row == height) {
				warning("StdGif::load(): row past the end of the image");
			} else if (row < height) {
				for (int i = 0; i < width; i++)
					frame->pixels[width * row + i] = rowBuffer[i + left];
			}

			row += step;

			if (row >= height) {
				pass++;

				if (pass < 4) {
					row = interlaceStart[pass];
					step = interlaceStep[pass];
				}
			}

			rowsDecoded++;
		} while (rowsDecoded < height);
	}

	_stdDisplay->vBufferUnlock(frame);
	delete[] rowBuffer;
	rowBuffer = nullptr;
	file->readByte();
	delete file;

	return bitmap;
}

int StdGif::write(const char *path, StdBitmapStruct *bitmap) {
	if (!bitmap->palette) {
		warning("StdGif::write(): bitmap without a palette");
		return 0;
	}

	Common::DumpFile file;

	if (!file.open(path))
		return 0;

	StdVBuffer *frame = bitmap->frames[0];

	file.write("GIF", 3);
	file.write("89a", 3);
	file.writeSint16LE((int16)frame->tex.width);
	file.writeSint16LE((int16)frame->tex.height);
	file.writeByte(0xF7);
	file.writeByte(0);
	file.writeByte(0);
	file.write(bitmap->palette, 768);
	file.writeByte(',');
	file.writeSint16LE(0);
	file.writeSint16LE(0);
	file.writeSint16LE((int16)frame->tex.width);
	file.writeSint16LE((int16)frame->tex.height);

	byte imageFlags = 0;

	file.writeByte(imageFlags);

	int numColors = 1 << ((imageFlags & 7) + 1);
	int codeSize = 1;
	int codeBits = 0;

	if (numColors > 1) {
		do {
			codeSize *= 2;
			codeBits++;
		} while (codeSize < numColors);
	}

	if (codeBits == 1)
		codeBits = 2;

	if (lzwCompress(&file, codeBits)) {
		file.close();
		return 0;
	}

	byte *row = new byte[frame->tex.width];

	_stdDisplay->vBufferLock(frame);

	for (int y = 0; y < frame->tex.height; y++) {
		memcpy(row, frame->pixels + frame->tex.width * y, frame->tex.width);

		for (int x = 0; x < frame->tex.width; x++) {
			if (lzwCompressRun(&file, row[x])) {
				delete[] row;
				row = nullptr;
				file.close();

				return 0;
			}
		}
	}

	_stdDisplay->vBufferUnlock(frame);
	lzwFinish(&file);

	byte terminator = 0;

	if (file.write(&terminator, 1) == 1) {
		terminator = 59;

		if (file.write(&terminator, 1) == 1) {
			delete[] row;
			row = nullptr;
			file.close();

			return 1;
		}
	}

	file.close();

	return 0;
}

int StdGif::lzwInitDecoder(int minCodeSize) {
	if (minCodeSize < 0 || minCodeSize > 10) {
		warning("StdGif::lzwInitDecoder(): invalid minimum code size");
		return -1;
	}

	_initCodeSize = minCodeSize + 1;
	_fresh = 0;
	_clearCode = 1 << minCodeSize;
	_endCode = _clearCode + 1;
	_firstFree = _clearCode + 2;
	_curCodeSize = minCodeSize + 1;
	_codeMask = (1 << _initCodeSize) - 1;
	memset(_prefix, 0, 0x400);

	for (int i = 255; i >= 0; i--)
		_suffix[i] = (byte)i;

	_nextFree = _firstFree;
	_stack = _stackArray;
	_blockPos = _blockSize;
	_bitOffset = 0;

	return 0;
}

int StdGif::lzwDecodeByte(Common::SeekableReadStream *file) {
	if (_stack != _stackArray) {
		_stack--;

		return *_stack;
	}

	int code = lzwGetCode(file);

	if (code == _clearCode) {
		memset(_prefix, 0, 0x400);
		_fresh = 1;
		_nextFree = _firstFree - 1;
		code = lzwGetCode(file);
	}

	if (code == _endCode)
		return -1;

	int newCode = code;

	if (code >= _nextFree) {
		*_stack = (byte)_firstChar;
		_stack++;
		code = _oldCode;
	}

	while (code >= _endCode) {
		if (_stack - _stackArray >= (int)sizeof(_stackArray) - 2) {
			warning("StdGif::lzwDecodeByte(): code chain too long");
			break;
		}

		*_stack = _suffix[code];
		_stack++;
		code = _prefix[code];
	}

	_firstChar = _suffix[code];
	*_stack = (byte)_firstChar;
	_stack++;

	if (_nextFree < tableLimit) {
		_prefix[_nextFree] = (uint16)_oldCode;
		_suffix[_nextFree] = (byte)_firstChar;
		_nextFree++;
	}

	_oldCode = newCode;
	_stack--;

	return *_stack;
}

int StdGif::lzwGetCode(Common::SeekableReadStream *file) {
	if (_nextFree > _codeMask) {
		_curCodeSize++;

		if (_curCodeSize == 12)
			_codeMask = tableLimit;
		else
			_codeMask = (1 << _curCodeSize) - 1;
	}

	if (_fresh > 0) {
		_curCodeSize = _initCodeSize;
		_fresh = 0;
		_codeMask = (1 << _initCodeSize) - 1;
	}

	byte current;

	if (_bitOffset == 0) {
		current = (byte)readBlockByte(file);
		_bitBuffer = current;
	} else {
		current = _bitBuffer;
	}

	int bitInByte = _bitOffset & 7;
	int shift = 8 - bitInByte;
	uint code = (byte)(current >> bitInByte);
	int remaining = bitInByte + _curCodeSize - 8;

	if (remaining >= 8) {
		byte next = (byte)readBlockByte(file);

		_bitBuffer = next;
		code |= (uint)next << shift;
		shift += 8;
		remaining -= 8;
	}

	if (remaining > 0) {
		byte next = (byte)readBlockByte(file);

		_bitBuffer = next;
		code |= (uint)(next & keepMask[remaining]) << shift;
	}

	_bitOffset = (_curCodeSize + _bitOffset) & 7;

	return code & ((1 << _curCodeSize) - 1);
}

int StdGif::readBlockByte(Common::SeekableReadStream *file) {
	int pos = _blockPos;

	if (_blockPos == _blockSize) {
		byte size;

		_blockSize = 0;

		if (file->read(&size, 1) != 1)
			return -1;

		_blockSize = size;

		if (file->read(_blockBuf, _blockSize) != (uint32)_blockSize)
			return -1;

		pos = 0;
	}

	if (pos >= (int)sizeof(_blockBuf)) {
		warning("StdGif::readBlockByte(): read past the end of the block");
		return -1;
	}

	_blockPos = pos + 1;

	return _blockBuf[pos];
}

int StdGif::lzwCompress(Common::WriteStream *stream, int minCodeSize) {
	_encOutCount = 0;
	_bitOffset = 0;
	_fresh = 0;
	_initCodeSize = minCodeSize + 1;
	_curCodeSize = minCodeSize + 1;
	_clearCode = 1 << minCodeSize;
	_endCode = _clearCode + 1;
	_firstFree = _clearCode + 2;
	_nextFree = _firstFree;
	_codeMask = (1 << _initCodeSize) - 1;

	for (int i = 0; i < hashSize; i++)
		_hashTable[i] = -1;

	_encFreeEnt = 1;

	byte codeSize = (byte)minCodeSize;

	if (stream->write(&codeSize, 1) != 1)
		return 1;

	lzwEncodeCode(stream, _clearCode);

	return _encOutCount;
}

void StdGif::lzwEncodeCode(Common::WriteStream *stream, int code) {
	int bitInByte = _bitOffset & 7;
	int index = (_bitOffset >> 3) + 1;

	_bitOffset += _curCodeSize;
	_blockBuf[index] = (byte)((_blockBuf[index] & keepMask[bitInByte]) | (setMask[bitInByte] & (byte)((byte)code << bitInByte)));
	index++;

	int remaining = bitInByte + _curCodeSize - 8;
	int bits = (int16)code >> (8 - bitInByte);

	if (remaining >= 8) {
		_blockBuf[index] = (byte)bits;
		index++;
		bits >>= 8;
		remaining -= 8;
	}

	if (index == 256) {
		writeBlock(stream, 255);
		_bitOffset -= 2040;
		index = 1;
	}

	if (remaining > 0) {
		if (index == 255) {
			writeBlock(stream, 254);
			_bitOffset -= 2032;
			index = 1;
		}

		_blockBuf[index] = (byte)bits;
	}

	if (_bitOffset == 2032) {
		writeBlock(stream, 254);
		_bitOffset = 0;
	}

	if (_nextFree > _codeMask || _fresh > 0) {
		if (_fresh) {
			_curCodeSize = _initCodeSize;
			_fresh = 0;
			_codeMask = (1 << _initCodeSize) - 1;
		} else {
			_curCodeSize++;

			if (_curCodeSize == 12)
				_codeMask = tableLimit;
			else
				_codeMask = (1 << _curCodeSize) - 1;
		}
	}
}

int StdGif::lzwCompressRun(Common::WriteStream *stream, byte pixel) {
	_encOutCount = 0;

	if (_encFreeEnt) {
		_encPrefix = pixel;

		int count = 0;
		int value = hashSize;

		do {
			value += value;
			count++;
		} while (value < 0x10000);

		_encHash = value;
		_encFreeEnt = 0;
		_encHashShift = 8 - count;

		return 0;
	}

	int fcode = (pixel << 12) + _encPrefix;
	int index = (int16)(((uint)pixel << _encHashShift) ^ (uint)_encPrefix);

	_encHash = fcode;

	if (_hashTable[index] == fcode) {
		_encPrefix = _hashCode[index];

		return 0;
	}

	if (_hashTable[index] >= 0) {
		int displacement = hashSize - index;

		if (index == 0)
			displacement = 1;

		for (;;) {
			index -= displacement;

			if (index < 0)
				index += hashSize;

			if (_hashTable[index] == fcode) {
				_encPrefix = _hashCode[index];

				return 0;
			}

			if (_hashTable[index] <= 0)
				break;
		}
	}

	lzwEncodeCode(stream, _encPrefix);

	int nextFree = _nextFree;

	_encPrefix = pixel;

	if (nextFree >= tableLimit) {
		lzwInitEncoder(stream);

		return _encOutCount;
	}

	_hashCode[index] = (uint16)nextFree;
	_hashTable[index] = _encHash;
	_nextFree = nextFree + 1;

	return _encOutCount;
}

void StdGif::lzwInitEncoder(Common::WriteStream *stream) {
	_fresh = 1;

	for (int i = 0; i < hashSize; i++)
		_hashTable[i] = -1;

	_nextFree = _firstFree;
	lzwEncodeCode(stream, _clearCode);
}

int StdGif::lzwFinish(Common::WriteStream *stream) {
	_encOutCount = 0;
	lzwEncodeCode(stream, _encPrefix);
	lzwEncodeCode(stream, _endCode);

	if (_bitOffset != 1)
		writeBlock(stream, (_bitOffset + 7) >> 3);

	return _encOutCount;
}

uint32 StdGif::writeBlock(Common::WriteStream *stream, int count) {
	_blockBuf[0] = (byte)count;

	uint32 result = stream->write(_blockBuf, count + 1);

	if (result != (uint32)(count + 1))
		_encOutCount++;

	return result;
}

StdBitmapStruct *StdGif::loadFailed(Common::SeekableReadStream *file, StdBitmapStruct *bitmap) {
	delete file;
	_stdBitmap->unloadBitmap(bitmap);

	return nullptr;
}

} // End of namespace Grim
