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
static const byte aKeepMask[8] = { 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F };
static const byte aSetMask[8] = { 0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80 };
static const int interlaceStart[4] = { 0, 4, 2, 1 };
static const int interlaceStep[4] = { 8, 8, 4, 2 };

StdGif::StdGif(StdDisplay *stdDisplay, StdFile *stdFile, StdBitmap *stdBitmap, SysStruct *pHS) : _stdDisplay(stdDisplay), _stdFile(stdFile), _stdBitmap(stdBitmap), _pHS(pHS),
		_oldCode(0), _initCodeSize(0), _firstFree(0), _nextFree(0), _clearCode(0), _endCode(0), _curCodeSize(0), _firstChar(0), _bFresh(0), _bitOffset(0), _codeMask(0), _bitBuffer(0),
		_blockPos(0), _blockSize(0), _aBlockBuf(), _pStack(_aStack), _aStack(), _aPrefix(), _aSuffix(),
		_encOutCount(0), _encPrefix(0), _encHash(0), _encFreeEnt(0), _encHashShift(0), _aHashTable(), _aHashCode() {
}

StdBitmapStruct *StdGif::load(const char *path, int bVideoMemory, int bNoZ) {
	Common::SeekableReadStream *pFile = _pHS->openFile(path);

	if (!pFile)
		return nullptr;

	StdBitmapStruct *pBitmap = new StdBitmapStruct();
	byte signature[3];

	memset(signature, 0, sizeof(signature));
	pFile->read(signature, 3);

	if (toupper(signature[0]) != 'G' || toupper(signature[1]) != 'I' || toupper(signature[2]) != 'F')
		return loadFailed(pFile, pBitmap);

	byte version[3];

	memset(version, 0, sizeof(version));
	pFile->read(version, 3);
	pFile->skip(4);

	byte screenFlags = pFile->readByte();

	pFile->skip(2);

	if (screenFlags & 0x80) {
		int numColors = 1 << ((screenFlags & 7) + 1);

		pBitmap->pPalette = new byte[768];
		pFile->read(pBitmap->pPalette, 3 * numColors);
	}

	int separator;

	if (version[0] == '8' && version[1] == '9' && toupper(version[2]) == 'A') {
		separator = (signed char)pFile->readByte();

		while (separator == '!') {
			byte data;

			do {
				data = pFile->readByte();
			} while (data);

			separator = (signed char)pFile->readByte();
		}
	} else {
		separator = (signed char)pFile->readByte();
	}

	if (separator != ',')
		return loadFailed(pFile, pBitmap);

	byte descriptor[9];

	memset(descriptor, 0, sizeof(descriptor));
	pFile->read(descriptor, 9);

	int left = READ_LE_INT16(descriptor);
	int imageWidth = READ_LE_INT16(descriptor + 4);
	int imageHeight = READ_LE_INT16(descriptor + 6);
	byte imageFlags = descriptor[8];

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
	format.width = imageWidth;
	format.height = imageHeight;
	format.format.colorMode = 0;
	format.format.bpp = 8;

	if (left < 0 || imageWidth < 0 || imageHeight < 0 || (int64)imageWidth * (int64)imageHeight > 0x7FFFFFFF) {
		warning("StdGif::load(): invalid image descriptor");
		return loadFailed(pFile, pBitmap);
	}

	pBitmap->apFrames[0] = _stdDisplay->vBufferNew(&format, bVideoMemory, bNoZ, nullptr);

	if (!pBitmap->apFrames[0])
		return loadFailed(pFile, pBitmap);

	if (imageFlags & 0x80) {
		int numColors = 1 << ((imageFlags & 7) + 1);

		if (!pBitmap->pPalette)
			pBitmap->pPalette = new byte[768];

		pFile->read(pBitmap->pPalette, 3 * numColors);
	}

	StdVBuffer *pFrame = pBitmap->apFrames[0];
	byte *pRowBuffer = new byte[left + pFrame->tex.width];
	byte minCodeSize = pFile->readByte();

	if (lzwInitDecoder((signed char)minCodeSize)) {
		delete[] pRowBuffer;
		pRowBuffer = nullptr;

		return loadFailed(pFile, pBitmap);
	}

	int row = 0;
	int step;

	if (imageFlags & 2)
		step = 8;
	else
		step = 1;

	_stdDisplay->vBufferLock(pFrame);

	int pass = 0;
	int rowsDecoded = 0;
	int width = pFrame->tex.width;
	int height = pFrame->tex.height;

	if (height > 0) {
		do {
			int x = 0;

			if (left + width > 0) {
				do {
					pRowBuffer[x] = (byte)lzwDecodeByte(pFile);
					x++;
				} while (x < left + width);
			}

			if (row == height) {
				warning("StdGif::load(): row past the end of the image");
			} else if (row < height) {
				for (int i = 0; i < width; i++)
					pFrame->pPixels[width * row + i] = pRowBuffer[i + left];
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

	_stdDisplay->vBufferUnlock(pFrame);
	delete[] pRowBuffer;
	pRowBuffer = nullptr;
	pFile->readByte();
	delete pFile;

	return pBitmap;
}

int StdGif::write(const char *path, StdBitmapStruct *pBitmap) {
	if (!pBitmap->pPalette) {
		warning("StdGif::write(): bitmap without a palette");
		return 0;
	}

	Common::DumpFile file;

	if (!file.open(path))
		return 0;

	StdVBuffer *pFrame = pBitmap->apFrames[0];

	file.write("GIF", 3);
	file.write("89a", 3);
	file.writeSint16LE((int16)pFrame->tex.width);
	file.writeSint16LE((int16)pFrame->tex.height);
	file.writeByte(0xF7);
	file.writeByte(0);
	file.writeByte(0);
	file.write(pBitmap->pPalette, 768);
	file.writeByte(',');
	file.writeSint16LE(0);
	file.writeSint16LE(0);
	file.writeSint16LE((int16)pFrame->tex.width);
	file.writeSint16LE((int16)pFrame->tex.height);

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

	byte *pRow = new byte[pFrame->tex.width];

	_stdDisplay->vBufferLock(pFrame);

	for (int y = 0; y < pFrame->tex.height; y++) {
		memcpy(pRow, pFrame->pPixels + pFrame->tex.width * y, pFrame->tex.width);

		for (int x = 0; x < pFrame->tex.width; x++) {
			if (lzwCompressRun(&file, pRow[x])) {
				delete[] pRow;
				pRow = nullptr;
				file.close();

				return 0;
			}
		}
	}

	_stdDisplay->vBufferUnlock(pFrame);
	lzwFinish(&file);

	byte terminator = 0;

	if (file.write(&terminator, 1) == 1) {
		terminator = 59;

		if (file.write(&terminator, 1) == 1) {
			delete[] pRow;
			pRow = nullptr;
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
	_bFresh = 0;
	_clearCode = 1 << minCodeSize;
	_endCode = _clearCode + 1;
	_firstFree = _clearCode + 2;
	_curCodeSize = minCodeSize + 1;
	_codeMask = (1 << _initCodeSize) - 1;
	memset(_aPrefix, 0, 0x400);

	for (int i = 255; i >= 0; i--)
		_aSuffix[i] = (byte)i;

	_nextFree = _firstFree;
	_pStack = _aStack;
	_blockPos = _blockSize;
	_bitOffset = 0;

	return 0;
}

int StdGif::lzwDecodeByte(Common::SeekableReadStream *pFile) {
	if (_pStack != _aStack) {
		_pStack--;

		return *_pStack;
	}

	int code = lzwGetCode(pFile);

	if (code == _clearCode) {
		memset(_aPrefix, 0, 0x400);
		_bFresh = 1;
		_nextFree = _firstFree - 1;
		code = lzwGetCode(pFile);
	}

	if (code == _endCode)
		return -1;

	int newCode = code;

	if (code >= _nextFree) {
		*_pStack = (byte)_firstChar;
		_pStack++;
		code = _oldCode;
	}

	while (code >= _endCode) {
		if (_pStack - _aStack >= (int)sizeof(_aStack) - 2) {
			warning("StdGif::lzwDecodeByte(): code chain too long");
			break;
		}

		*_pStack = _aSuffix[code];
		_pStack++;
		code = _aPrefix[code];
	}

	_firstChar = _aSuffix[code];
	*_pStack = (byte)_firstChar;
	_pStack++;

	if (_nextFree < tableLimit) {
		_aPrefix[_nextFree] = (uint16)_oldCode;
		_aSuffix[_nextFree] = (byte)_firstChar;
		_nextFree++;
	}

	_oldCode = newCode;
	_pStack--;

	return *_pStack;
}

int StdGif::lzwGetCode(Common::SeekableReadStream *pFile) {
	if (_nextFree > _codeMask) {
		_curCodeSize++;

		if (_curCodeSize == 12)
			_codeMask = tableLimit;
		else
			_codeMask = (1 << _curCodeSize) - 1;
	}

	if (_bFresh > 0) {
		_curCodeSize = _initCodeSize;
		_bFresh = 0;
		_codeMask = (1 << _initCodeSize) - 1;
	}

	byte current;

	if (_bitOffset == 0) {
		current = (byte)readBlockByte(pFile);
		_bitBuffer = current;
	} else {
		current = _bitBuffer;
	}

	int bitInByte = _bitOffset & 7;
	int shift = 8 - bitInByte;
	uint code = (byte)(current >> bitInByte);
	int remaining = bitInByte + _curCodeSize - 8;

	if (remaining >= 8) {
		byte next = (byte)readBlockByte(pFile);

		_bitBuffer = next;
		code |= (uint)next << shift;
		shift += 8;
		remaining -= 8;
	}

	if (remaining > 0) {
		byte next = (byte)readBlockByte(pFile);

		_bitBuffer = next;
		code |= (uint)(next & aKeepMask[remaining]) << shift;
	}

	_bitOffset = (_curCodeSize + _bitOffset) & 7;

	return code & ((1 << _curCodeSize) - 1);
}

int StdGif::readBlockByte(Common::SeekableReadStream *pFile) {
	int pos = _blockPos;

	if (_blockPos == _blockSize) {
		byte size;

		_blockSize = 0;

		if (pFile->read(&size, 1) != 1)
			return -1;

		_blockSize = size;

		if (pFile->read(_aBlockBuf, _blockSize) != (uint32)_blockSize)
			return -1;

		pos = 0;
	}

	if (pos >= (int)sizeof(_aBlockBuf)) {
		warning("StdGif::readBlockByte(): read past the end of the block");
		return -1;
	}

	_blockPos = pos + 1;

	return _aBlockBuf[pos];
}

int StdGif::lzwCompress(Common::WriteStream *pStream, int minCodeSize) {
	_encOutCount = 0;
	_bitOffset = 0;
	_bFresh = 0;
	_initCodeSize = minCodeSize + 1;
	_curCodeSize = minCodeSize + 1;
	_clearCode = 1 << minCodeSize;
	_endCode = _clearCode + 1;
	_firstFree = _clearCode + 2;
	_nextFree = _firstFree;
	_codeMask = (1 << _initCodeSize) - 1;

	for (int i = 0; i < hashSize; i++)
		_aHashTable[i] = -1;

	_encFreeEnt = 1;

	byte codeSize = (byte)minCodeSize;

	if (pStream->write(&codeSize, 1) != 1)
		return 1;

	lzwEncodeCode(pStream, _clearCode);

	return _encOutCount;
}

void StdGif::lzwEncodeCode(Common::WriteStream *pStream, int code) {
	int bitInByte = _bitOffset & 7;
	int index = (_bitOffset >> 3) + 1;

	_bitOffset += _curCodeSize;
	_aBlockBuf[index] = (byte)((_aBlockBuf[index] & aKeepMask[bitInByte]) | (aSetMask[bitInByte] & (byte)((byte)code << bitInByte)));
	index++;

	int remaining = bitInByte + _curCodeSize - 8;
	int bits = (int16)code >> (8 - bitInByte);

	if (remaining >= 8) {
		_aBlockBuf[index] = (byte)bits;
		index++;
		bits >>= 8;
		remaining -= 8;
	}

	if (index == 256) {
		writeBlock(pStream, 255);
		_bitOffset -= 2040;
		index = 1;
	}

	if (remaining > 0) {
		if (index == 255) {
			writeBlock(pStream, 254);
			_bitOffset -= 2032;
			index = 1;
		}

		_aBlockBuf[index] = (byte)bits;
	}

	if (_bitOffset == 2032) {
		writeBlock(pStream, 254);
		_bitOffset = 0;
	}

	if (_nextFree > _codeMask || _bFresh > 0) {
		if (_bFresh) {
			_curCodeSize = _initCodeSize;
			_bFresh = 0;
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

int StdGif::lzwCompressRun(Common::WriteStream *pStream, byte pixel) {
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

	if (_aHashTable[index] == fcode) {
		_encPrefix = _aHashCode[index];

		return 0;
	}

	if (_aHashTable[index] >= 0) {
		int displacement = hashSize - index;

		if (index == 0)
			displacement = 1;

		for (;;) {
			index -= displacement;

			if (index < 0)
				index += hashSize;

			if (_aHashTable[index] == fcode) {
				_encPrefix = _aHashCode[index];

				return 0;
			}

			if (_aHashTable[index] <= 0)
				break;
		}
	}

	lzwEncodeCode(pStream, _encPrefix);

	int nextFree = _nextFree;

	_encPrefix = pixel;

	if (nextFree >= tableLimit) {
		lzwInitEncoder(pStream);

		return _encOutCount;
	}

	_aHashCode[index] = (uint16)nextFree;
	_aHashTable[index] = _encHash;
	_nextFree = nextFree + 1;

	return _encOutCount;
}

void StdGif::lzwInitEncoder(Common::WriteStream *pStream) {
	_bFresh = 1;

	for (int i = 0; i < hashSize; i++)
		_aHashTable[i] = -1;

	_nextFree = _firstFree;
	lzwEncodeCode(pStream, _clearCode);
}

int StdGif::lzwFinish(Common::WriteStream *pStream) {
	_encOutCount = 0;
	lzwEncodeCode(pStream, _encPrefix);
	lzwEncodeCode(pStream, _endCode);

	if (_bitOffset != 1)
		writeBlock(pStream, (_bitOffset + 7) >> 3);

	return _encOutCount;
}

uint32 StdGif::writeBlock(Common::WriteStream *pStream, int count) {
	_aBlockBuf[0] = (byte)count;

	uint32 result = pStream->write(_aBlockBuf, count + 1);

	if (result != (uint32)(count + 1))
		_encOutCount++;

	return result;
}

StdBitmapStruct *StdGif::loadFailed(Common::SeekableReadStream *pFile, StdBitmapStruct *pBitmap) {
	delete pFile;
	_stdBitmap->unloadBitmap(pBitmap);

	return nullptr;
}

} // End of namespace Grim
