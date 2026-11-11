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

class StreamScript {
public:
	StreamScript(const char *scriptFileName);
	~StreamScript();

	void *operator new(size_t size);
	void operator delete(void *block);

	void closeStreams();

	static bool openStreamScript(const char *scriptFileName);
	static void closeStreamScript();
	static void setStreamScriptVolume(float volume);
	static void startStream(const uint &id, const int &startMs, const bool &immediate);
	static bool consumeStreamFlags(const uint &id, bool &looped, bool &ended);
	static void updateStreams();
	static void pauseStreams();
	static void resumeStreams();
	static int getStreamPositionMs(const uint &id, const bool &relative);
	static void closeAllStreams();
	static void setStreamingGate(int (*gate)());
	static int isStreamingEnabled();

	static Common::SeekableReadStream *openStream(const char *path);
	static Common::SeekableReadStream *openAudioFile(const char *fileName);
	static LString findAudioFile(const char *fileName);
	static void pushAudioSearchDir(const char *directory);
	static void clearAudioSearchDirs();

	static bool openMusicScript(const char *scriptFileName);
	static void setMusicStreamVolume(float volume);
	static void startMusicStream(const uint &id, const int &startMs, const bool &immediate);
	static bool consumeMusicStreamFlags(const uint &id, bool &looped, bool &ended);
	static int getMusicStreamPositionMs(const uint &id, const bool &relative);

	LString fileName;
	CueEntry *head;
	bool loaded;

	static StreamScript *activeScript;
	static int (*streamingGate)();
	static int16 instanceCount;
	static uint firstUpdateMs;
	static uint lastUpdateMs;
	static float masterVolume;
	static float currentFadeRate;
	static float fadeStep;

	static const int kMaxSearchDirs = 16;
	static LString searchDirs[kMaxSearchDirs];
	static int searchDirCount;
	static int openMissCount;
	static int findMissCount;
	static Common::FSDirectory *gameDirectory;
};

} // End of namespace Grim

#endif
