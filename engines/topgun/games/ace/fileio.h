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

#ifndef TOPGUN_GAMES_ACE_FILEIO_H
#define TOPGUN_GAMES_ACE_FILEIO_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/str-array.h"
#include "common/str.h"

#include "topgun/plugin.h"

namespace Topgun {

enum {
	kFileIoPathSize = 256,
	kFileIoStringSlotSize = 128,
	kFileIoStringTransferLimit = 0x7F,
	kFileIoConstantStringFlag = 0x8000
};

enum FileIoOpenFlag {
	kFileIoOpenRead = 0x01,
	kFileIoOpenWrite = 0x02,
	kFileIoOpenCreateNew = 0x10,
	kFileIoOpenCreate = 0x20,
	kFileIoOpenMustExist = 0x40,
	kFileIoOpenUnknown80 = 0x80,
	kFileIoOpenCreateAlternate = 0x100,
	kFileIoOpenDelete = 0x200
};

class FileIoPlugin : public Plugin {
public:
	FileIoPlugin(PluginHost *host);

	bool init() override;
	void free() override;
	int findFunction(const Common::String &name) const override;
	uint16 callFunction(int function, const uint16 *arguments, uint16 count) override;

private:
	PluginHost *_host;

	// Paths: they live as long as the library stays loaded
	Common::String _dataPath;
	Common::String _defaultPath;

	// State of the find in progress: one per scene
	Common::StringArray _findNames;
	uint32 _findIndex;
	bool _findActive;

	uint16 readItemsToVars(uint16 handle, uint16 address, uint16 count, uint32 itemSize);
	uint16 writeItemsFromVars(uint16 handle, uint16 address, uint16 count, uint32 itemSize);
	uint16 fileClose(uint16 handle);
	uint16 fileGetC(uint16 handle, uint16 address, uint16 count);
	uint16 fileGetL(uint16 handle, uint16 address, uint16 count);
	uint16 fileGetS(uint16 handle, uint16 address, uint16 count);
	uint16 fileLength(uint16 handle);
	uint16 fileOpen(uint16 nameRef, uint16 flags);
	uint16 filePutC(uint16 handle, uint16 address, uint16 count);
	uint16 filePutL(uint16 handle, uint16 address, uint16 count);
	uint16 filePutS(uint16 handle, uint16 address, uint16 count);
	uint16 fileRead(uint16 handle, uint16 stringRef, uint16 count);
	uint16 fileWrite(uint16 handle, uint16 stringRef, uint16 count);
	uint16 fileReadLine(uint16 handle, uint16 stringRef);
	uint16 fileWriteLine(uint16 handle, uint16 stringRef);
	uint16 fileSeek(uint16 handle, uint16 offset, uint16 origin);
	uint16 fileFindFirst(uint16 patternRef, uint16 resultRef, uint16 usePath);
	uint16 fileFindNext(uint16 resultRef);
	uint16 fileFindClose();
	void initDefaultPath();
	uint16 fileSetDataPath(uint16 pathRef);
	uint16 fileSetDataPathRel(uint16 relativeRef);
	uint16 fileDataOpen(uint16 nameRef, uint16 flags);
	uint16 fileDirCreate(uint16 pathRef);
	uint16 fileDirCreateRel(uint16 relativeRef);
	uint16 fileDirRemove(uint16 pathRef);
	uint16 fileDirRemoveRel(uint16 relativeRef);
	uint16 fileGetTempDir(uint16 resultRef);
	uint16 freeCtx();
	uint16 initCtx();
};

} // End of namespace Topgun

#endif // TOPGUN_GAMES_ACE_FILEIO_H
