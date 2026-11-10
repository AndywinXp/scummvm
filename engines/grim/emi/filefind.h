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

#ifndef GRIM_EMI_FILEFIND_H
#define GRIM_EMI_FILEFIND_H

#include "common/fs.h"

namespace Grim {

enum {
	kFileFindDirectory = 0x10,
	kFileFindNormal = 0x80
};

class FileFind {
public:
	FileFind(const char *pattern, const char *directory);

	void *operator new(size_t size);
	void operator delete(void *block);

	void close();
	bool next(char *name, uint *bufferSize, uint *attributes);

private:
	Common::FSList _entries;
	uint _current;
};

} // End of namespace Grim

#endif
