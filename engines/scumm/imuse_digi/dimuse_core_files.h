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
#include "scumm/file.h"
#include "scumm/imuse_digi/dimuse_bndmgr.h"
#include "scumm/imuse_digi/dimuse_sndmgr.h"

namespace Scumm {

class IMuseDigiFilesHandler {

private:
	IMuseDigital *_engine;
	ImuseDigiSndMgr *_sound;
	ScummEngine_v7 *_vm;
	Common::Mutex _mutex;
	IMuseDigiSndBuffer _soundBuffers[4];
	char _currentSpeechFilename[60];
	ScummFile *_ftSpeechFile;
	int _ftSpeechSubFileOffset;
	int _ftSpeechFileSize;
	int _ftSpeechFileCurPos;

	void getFilenameFromSoundId(int soundId, char *fileName, size_t size);
public:
	IMuseDigiFilesHandler(IMuseDigital *engine, ScummEngine_v7 *vm);
	~IMuseDigiFilesHandler();

	uint8 *getSoundAddrData(int soundId);
	int getNextSound(int soundId);
	int seek(int soundId, int offset, int mode, int bufId);
	int read(int soundId, uint8 *buf, int size, int bufId);
	IMuseDigiSndBuffer *getBufInfo(int bufId);
	int openSound(int soundId);
	void closeSound(int soundId);
	void closeAllSounds();
	void allocSoundBuffer(int bufId, int size, int loadSize, int criticalSize);
	void deallocSoundBuffer(int bufId);
	void flushSounds();
	int setCurrentSpeechFilename(const char *fileName);
	void setCurrentFtSpeechFile(ScummFile *file, unsigned int offset, unsigned int size);
	void closeSoundImmediatelyById(int soundId);
	void saveLoad(Common::Serializer &ser);
};

} // End of namespace Scumm
#endif
