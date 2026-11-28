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

#ifndef TOPGUN_GAMES_ACE_DYNALLOC_H
#define TOPGUN_GAMES_ACE_DYNALLOC_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/str.h"

#include "topgun/plugin.h"

namespace Topgun {

enum {
	kDynAllocFailed = 0xFFFF,
	// Stands for the data segment selector that the original returns
	kDynAllocNonZeroResult = 1
};

struct DynBuffer {
	uint16 size;
	byte *data;

	DynBuffer() : size(0), data(nullptr) {}
};

class DynAllocPlugin : public Plugin {
public:
	DynAllocPlugin(PluginHost *host);
	~DynAllocPlugin();

	bool init() override;
	void free() override;
	int findFunction(const Common::String &name) const override;
	uint16 callFunction(int function, const uint16 *arguments, uint16 count) override;

private:
	PluginHost *_host;
	Common::Array<DynBuffer> _buffers;

	uint16 dynAllocBuf(uint16 size);
	uint16 dynFreeBuf(uint16 buffer);
	uint16 dynFreeSystem();
	uint16 dynGetByte(uint16 buffer, uint16 offset, uint16 address, uint16 count);
	uint16 dynGetLong(uint16 buffer, uint16 offset, uint16 address, uint16 count);
	uint16 dynGetShort(uint16 buffer, uint16 offset, uint16 address, uint16 count);
	uint16 dynPutByte(uint16 buffer, uint16 offset, uint16 address, uint16 count);
	uint16 dynPutLong(uint16 buffer, uint16 offset, uint16 address, uint16 count);
	uint16 dynPutShort(uint16 buffer, uint16 offset, uint16 address, uint16 count);
	uint16 dynReadFile(uint16 handle, uint16 buffer, uint16 offset, uint16 count);
	uint16 dynWriteFile(uint16 handle, uint16 buffer, uint16 offset, uint16 count);
};

} // End of namespace Topgun

#endif // TOPGUN_GAMES_ACE_DYNALLOC_H
