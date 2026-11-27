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
#include "common/textconsole.h"

#include "topgun/games/ace/dynalloc.h"

namespace Topgun {

enum {
	kFunctionDynAllocBuf,
	kFunctionDynFreeBuf,
	kFunctionDynFreeSystem,
	kFunctionDynGetByte,
	kFunctionDynPutByte,
	kFunctionDynGetShort,
	kFunctionDynPutShort,
	kFunctionDynGetLong,
	kFunctionDynPutLong,
	kFunctionDynReadFile,
	kFunctionDynWriteFile,
	kFunctionCount
};

DynAllocPlugin::DynAllocPlugin(PluginHost *host) : _host(host) {
}

DynAllocPlugin::~DynAllocPlugin() {
	dynFreeSystem();
}

bool DynAllocPlugin::init() {
	_buffers.clear();
	return true;
}

void DynAllocPlugin::free() {
	dynFreeSystem();
}

int DynAllocPlugin::findFunction(const Common::String &name) const {
	if (name.equalsIgnoreCase("DYNALLOCBUF"))
		return kFunctionDynAllocBuf;
	if (name.equalsIgnoreCase("DYNFREEBUF"))
		return kFunctionDynFreeBuf;
	if (name.equalsIgnoreCase("DYNFREESYSTEM"))
		return kFunctionDynFreeSystem;
	if (name.equalsIgnoreCase("DYNGETBYTE"))
		return kFunctionDynGetByte;
	if (name.equalsIgnoreCase("DYNPUTBYTE"))
		return kFunctionDynPutByte;
	if (name.equalsIgnoreCase("DYNGETSHORT"))
		return kFunctionDynGetShort;
	if (name.equalsIgnoreCase("DYNPUTSHORT"))
		return kFunctionDynPutShort;
	if (name.equalsIgnoreCase("DYNGETLONG"))
		return kFunctionDynGetLong;
	if (name.equalsIgnoreCase("DYNPUTLONG"))
		return kFunctionDynPutLong;
	if (name.equalsIgnoreCase("DYNREADFILE"))
		return kFunctionDynReadFile;
	if (name.equalsIgnoreCase("DYNWRITEFILE"))
		return kFunctionDynWriteFile;

	return -1;
}

uint16 DynAllocPlugin::callFunction(int function, const uint16 *arguments, uint16 count) {
	if (function < 0 || function >= kFunctionCount) {
		warning("DynAllocPlugin::callFunction(): invalid function %d", function);
		return 0;
	}

	uint16 expected = 4;
	switch (function) {
	case kFunctionDynFreeSystem:
		expected = 0;
		break;
	case kFunctionDynAllocBuf:
	case kFunctionDynFreeBuf:
		expected = 1;
		break;
	}

	if (count < expected)
		warning("DynAllocPlugin::callFunction(): function %d expects %d arguments, got %d", function, expected, count);

	uint16 values[4];
	for (int i = 0; i < 4; i++)
		values[i] = (i < count) ? arguments[i] : 0;

	switch (function) {
	case kFunctionDynAllocBuf:
		return dynAllocBuf(values[0]);
	case kFunctionDynFreeBuf:
		return dynFreeBuf(values[0]);
	case kFunctionDynFreeSystem:
		return dynFreeSystem();
	case kFunctionDynGetByte:
		return dynGetByte(values[0], values[1], values[2], values[3]);
	case kFunctionDynPutByte:
		return dynPutByte(values[0], values[1], values[2], values[3]);
	case kFunctionDynGetShort:
		return dynGetShort(values[0], values[1], values[2], values[3]);
	case kFunctionDynPutShort:
		return dynPutShort(values[0], values[1], values[2], values[3]);
	case kFunctionDynGetLong:
		return dynGetLong(values[0], values[1], values[2], values[3]);
	case kFunctionDynPutLong:
		return dynPutLong(values[0], values[1], values[2], values[3]);
	case kFunctionDynReadFile:
		return dynReadFile(values[0], values[1], values[2], values[3]);
	case kFunctionDynWriteFile:
		return dynWriteFile(values[0], values[1], values[2], values[3]);
	}

	return 0;
}

uint16 DynAllocPlugin::dynAllocBuf(uint16 size) {
	uint32 count = _buffers.size();

	if (count >= kDynAllocFailed) {
		warning("DynAllocPlugin::dynAllocBuf(): the buffer table is full");
		return kDynAllocFailed;
	}

	for (uint32 i = 0; i < count; i++) {
		if (_buffers[i].size == 0) {
			byte *data = nullptr;
			if (size != 0)
				data = new byte[size]();

			_buffers[i].data = data;

			if (data) {
				_buffers[i].size = size;
				return i;
			}
		}
	}

	// A failed allocation still leaves the new entry in the table
	_buffers.push_back(DynBuffer());

	byte *data = nullptr;
	if (size != 0)
		data = new byte[size]();

	_buffers[count].data = data;

	if (!data) {
		warning("DynAllocPlugin::dynAllocBuf(): cannot allocate %d bytes", size);
		return kDynAllocFailed;
	}

	_buffers[count].size = size;
	return count;
}

uint16 DynAllocPlugin::dynFreeBuf(uint16 buffer) {
	if (buffer >= _buffers.size()) {
		warning("DynAllocPlugin::dynFreeBuf(): invalid buffer %d", buffer);
		return 0;
	}

	DynBuffer &entry = _buffers[buffer];

	if (entry.size != 0) {
		delete[] entry.data;
		entry.size = 0;
		entry.data = nullptr;
	}

	return 0;
}

uint16 DynAllocPlugin::dynFreeSystem() {
	for (uint32 i = 0; i < _buffers.size(); i++) {
		if (_buffers[i].size != 0)
			delete[] _buffers[i].data;
	}

	_buffers.clear();
	return 0;
}

uint16 DynAllocPlugin::dynGetByte(uint16 buffer, uint16 offset, uint16 address, uint16 count) {
	if (buffer >= _buffers.size()) {
		warning("DynAllocPlugin::dynGetByte(): invalid buffer %d", buffer);
		return 0;
	}

	const DynBuffer &entry = _buffers[buffer];
	if (entry.size == 0)
		return buffer;

	for (uint16 i = 0; i < count; i++) {
		if (offset >= entry.size) {
			warning("DynAllocPlugin::dynGetByte(): offset %d is outside the buffer", offset);
			return 0;
		}

		_host->writeAddress(address, entry.data[offset]);
		offset++;
		address += 2;
	}

	return 0;
}

uint16 DynAllocPlugin::dynGetLong(uint16 buffer, uint16 offset, uint16 address, uint16 count) {
	if (buffer >= _buffers.size()) {
		warning("DynAllocPlugin::dynGetLong(): invalid buffer %d", buffer);
		return 0;
	}

	const DynBuffer &entry = _buffers[buffer];
	if (entry.size == 0)
		return buffer;

	uint16 position = offset << 2;

	for (uint16 i = 0; i < count; i++) {
		if ((uint32)position + 2 > entry.size) {
			warning("DynAllocPlugin::dynGetLong(): position %d is outside the buffer", position);
			return 0;
		}

		_host->writeAddress(address, READ_LE_UINT16(entry.data + position));
		position += 4;
		address += 2;
	}

	return 0;
}

uint16 DynAllocPlugin::dynGetShort(uint16 buffer, uint16 offset, uint16 address, uint16 count) {
	if (buffer >= _buffers.size()) {
		warning("DynAllocPlugin::dynGetShort(): invalid buffer %d", buffer);
		return 0;
	}

	const DynBuffer &entry = _buffers[buffer];
	if (entry.size == 0)
		return buffer;

	uint16 position = offset << 1;

	for (uint16 i = 0; i < count; i++) {
		if ((uint32)position + 2 > entry.size) {
			warning("DynAllocPlugin::dynGetShort(): position %d is outside the buffer", position);
			return 0;
		}

		_host->writeAddress(address, READ_LE_UINT16(entry.data + position));
		position += 2;
		address += 2;
	}

	return 0;
}

uint16 DynAllocPlugin::dynPutByte(uint16 buffer, uint16 offset, uint16 address, uint16 count) {
	if (buffer >= _buffers.size()) {
		warning("DynAllocPlugin::dynPutByte(): invalid buffer %d", buffer);
		return 0;
	}

	DynBuffer &entry = _buffers[buffer];
	if (entry.size == 0)
		return buffer;

	for (uint16 i = 0; i < count; i++) {
		if (offset >= entry.size) {
			warning("DynAllocPlugin::dynPutByte(): offset %d is outside the buffer", offset);
			return 0;
		}

		entry.data[offset] = (byte)_host->readAddress(address);
		offset++;
		address += 2;
	}

	return 0;
}

uint16 DynAllocPlugin::dynPutLong(uint16 buffer, uint16 offset, uint16 address, uint16 count) {
	if (buffer >= _buffers.size()) {
		warning("DynAllocPlugin::dynPutLong(): invalid buffer %d", buffer);
		return 0;
	}

	DynBuffer &entry = _buffers[buffer];
	if (entry.size == 0)
		return buffer;

	uint16 position = offset << 2;

	for (uint16 i = 0; i < count; i++) {
		if ((uint32)position + 4 > entry.size) {
			warning("DynAllocPlugin::dynPutLong(): position %d is outside the buffer", position);
			return 0;
		}

		WRITE_LE_UINT16(entry.data + position, _host->readAddress(address));
		WRITE_LE_UINT16(entry.data + position + 2, 0);
		position += 4;
		address += 2;
	}

	return 0;
}

uint16 DynAllocPlugin::dynPutShort(uint16 buffer, uint16 offset, uint16 address, uint16 count) {
	if (buffer >= _buffers.size()) {
		warning("DynAllocPlugin::dynPutShort(): invalid buffer %d", buffer);
		return 0;
	}

	DynBuffer &entry = _buffers[buffer];
	if (entry.size == 0)
		return buffer;

	uint16 position = offset << 1;

	for (uint16 i = 0; i < count; i++) {
		if ((uint32)position + 2 > entry.size) {
			warning("DynAllocPlugin::dynPutShort(): position %d is outside the buffer", position);
			return 0;
		}

		// The destination position is not advanced: every word is stored in the same place
		WRITE_LE_UINT16(entry.data + position, _host->readAddress(address));
		address += 2;
	}

	return 0;
}

uint16 DynAllocPlugin::dynReadFile(uint16 handle, uint16 buffer, uint16 offset, uint16 count) {
	if (buffer >= _buffers.size()) {
		warning("DynAllocPlugin::dynReadFile(): invalid buffer %d", buffer);
		return 0;
	}

	DynBuffer &entry = _buffers[buffer];
	if (entry.size == 0)
		return 0;

	if ((uint32)offset + count > entry.size) {
		warning("DynAllocPlugin::dynReadFile(): reading %d bytes at %d goes outside the buffer", count, offset);

		if (offset >= entry.size)
			return 0;

		count = entry.size - offset;
	}

	return (uint16)_host->mmioRead(handle, entry.data + offset, count);
}

uint16 DynAllocPlugin::dynWriteFile(uint16 handle, uint16 buffer, uint16 offset, uint16 count) {
	if (buffer >= _buffers.size()) {
		warning("DynAllocPlugin::dynWriteFile(): invalid buffer %d", buffer);
		return 0;
	}

	DynBuffer &entry = _buffers[buffer];
	if (entry.size == 0)
		return 0;

	if ((uint32)offset + count > entry.size) {
		warning("DynAllocPlugin::dynWriteFile(): writing %d bytes at %d goes outside the buffer", count, offset);

		if (offset >= entry.size)
			return 0;

		count = entry.size - offset;
	}

	return (uint16)_host->mmioWrite(handle, entry.data + offset, count);
}

} // End of namespace Topgun
