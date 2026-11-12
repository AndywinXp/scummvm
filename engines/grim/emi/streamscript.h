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

#ifndef GRIM_EMI_STREAMSCRIPT_H
#define GRIM_EMI_STREAMSCRIPT_H

#include "common/scummsys.h"

#include "engines/grim/emi/lfile.h"

namespace Common {
class FSDirectory;
class SeekableReadStream;
}

namespace Grim {

class CueEntry;
class EMIMilesDriver;
class Sounds;

class StreamScript {
public:
	explicit StreamScript(Sounds *sounds);
	~StreamScript();

	void *operator new(size_t size);
	void operator delete(void *block);

	void closeStreams();

	bool openStreamScript(const char *scriptFileName);
	void closeStreamScript();
	void setStreamScriptVolume(float volume);
	void startStream(const uint &id, const int &startMs, const bool &immediate);
	bool consumeStreamFlags(const uint &id, bool &looped, bool &ended);
	void updateStreams();
	void pauseStreams();
	void resumeStreams();
	int getStreamPositionMs(const uint &id, const bool &relative);
	void closeAllStreams();
	int isStreamingEnabled();
	EMIMilesDriver *getMilesDriver();

	Common::SeekableReadStream *openStream(const char *path);
	Common::SeekableReadStream *openAudioFile(const char *fileName);
	LString findAudioFile(const char *fileName);
	bool hasAudioFile(const char *fileName);
	void pushAudioSearchDir(const char *directory);
	void clearAudioSearchDirs();

	bool openMusicScript(const char *scriptFileName);
	void setMusicStreamVolume(float volume);
	void startMusicStream(const uint &id, const int &startMs, const bool &immediate);
	bool consumeMusicStreamFlags(const uint &id, bool &looped, bool &ended);
	int getMusicStreamPositionMs(const uint &id, const bool &relative);

	LString fileName;
	CueEntry *head;
	bool loaded;

	static const int kMaxSearchDirs = 16;

private:
	Sounds *_sounds;
	uint _firstUpdateMs;
	uint _lastUpdateMs;
	float _masterVolume;
	float _currentFadeRate;
	LString _searchDirs[kMaxSearchDirs];
	int _searchDirCount;
	int _openMissCount;
	int _findMissCount;
	Common::FSDirectory *_gameDirectory;
};

} // End of namespace Grim

#endif
