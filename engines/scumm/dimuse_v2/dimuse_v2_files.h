/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#if !defined(SCUMM_IMUSE_DIGI_V2_FILES_H) && defined(ENABLE_SCUMM_7_8)
#define SCUMM_IMUSE_DIGI_V2_FILES_H

#include "common/scummsys.h"
#include "common/textconsole.h"
#include "common/util.h"
#include "scumm/resource.h"
#include "scumm/dimuse_v1/dimuse_bndmgr.h"
#include "scumm/dimuse_v1/dimuse_sndmgr.h"

namespace Scumm {

class DiMUSEFilesHandler {

private:
	DiMUSE_v2 *_engine;
	DiMUSESndMgr *_sound;
	ScummEngine_v7 *_vm;
	Common::Mutex _mutex;
	DiMUSESoundBuffer _soundBuffers[4];
	char _currentSpeechFile[60];

	void getFilenameFromSoundId(int soundId, char *fileName, size_t size);
public:
	DiMUSEFilesHandler(DiMUSE_v2 *engine, ScummEngine_v7 *vm);
	~DiMUSEFilesHandler();

	int init();
	int deinit();
	uint8 *getSoundAddrData(int soundId);
	uint8 *fetchMap(int soundId);
	int getNextSound(int soundId);
	int checkIdInRange(int soundId);
	int seek(int soundId, int offset, int mode, int bufId);
	int read(int soundId, uint8 *buf, int size, int bufId);
	DiMUSESoundBuffer *getBufInfo(int bufId);
	int openSound(int soundId);
	void closeSound(int soundId);
	void closeAllSounds();
	void diMUSEAllocSoundBuffer(int bufId, int size, int loadSize, int criticalSize);
	void diMUSEDeallocSoundBuffer(int bufId);
	void flushSounds();
	int setCurrentSpeechFile(const char *fileName);
	void closeSoundImmediatelyById(int soundId);
	void saveLoad(Common::Serializer &ser);
};

} // End of namespace Scumm
#endif
