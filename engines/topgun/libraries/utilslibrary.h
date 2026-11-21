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

#ifndef TOPGUN_LIBRARIES_UTILSLIBRARY_H
#define TOPGUN_LIBRARIES_UTILSLIBRARY_H

#include "common/scummsys.h"
#include "common/str.h"

namespace Topgun {

class TopgunEngine;

class UtilsLibrary {
public:
	UtilsLibrary(TopgunEngine *engine);
	~UtilsLibrary();

	void copyHugeBytes(byte *destination, const byte *source, uint32 count);
	uint16 shiInvokeErrorHandler1(void *pointer, uint16 errorCode);
	void setHugeBytes(byte *destination, uint16 fill, uint32 count);
	char *copyString(const char *source);
	void debugStr(const char *format, ...);
	uint32 memMgrSize();
	byte *tgMalloc(uint32 size, uint16 zeroInit);
	byte *tgPtrCpy(const byte *block);
	byte *tgRealloc(byte *block, uint32 newSize, uint16 zeroInit);
	byte *tgFree(byte *block);
	Common::String makeFullPath(const Common::String &directory, const Common::String &file);
	Common::String makeModulePath(uint16 instance, const Common::String &file);
	void getPathAndFile(const Common::String &fullPath, Common::String &path, Common::String &file);
	Common::String skipPath(const Common::String &path);
	uint32 memMgrPreAllocate(uint32 bytes);
	bool fileExist(const Common::String &path);
	void memSetNotify();
	uint16 smartHeapErrHdlr(void *errorInfo);

private:
	TopgunEngine *_engine;
	bool _notifyRegistered = false;
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_UTILSLIBRARY_H
