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

#ifndef GRIM_EMI_CMUSICPS2_H
#define GRIM_EMI_CMUSICPS2_H

#include "common/scummsys.h"

namespace Grim {

class CWaveSound;
struct EMIMilesSample;

class CMusicPs2 {
public:
	enum {
		kStateCount = 126,
		kMaxFileName = 130,
		kMaxMissMessages = 5
	};

	enum {
		kFadeNone = 0,
		kFadeIn = 1,
		kFadeOut = 2
	};

	enum {
		kStreamHandle = 1,
		kStreamDone = 2
	};

	struct MusicState {
		int sync;
		byte episode;
		int lastPositionMs;
		bool looped;
		bool ended;
		float fadeRate;
		float fadeVolume;
		int fadeState;
		int16 loopCount;
		char fileName[kMaxFileName];
		int handle;
		float trim;
		float startOffsetMs;
	};

	struct StateEntry {
		const char *fileName;
		int sync;
		byte episode;
		uint trim;
	};

	CMusicPs2();
	~CMusicPs2();

	void *operator new(size_t size);
	void operator delete(void *block);

	void initStates();
	void resetStartOffsets();
	void closeStreams();

	static void constructState(MusicState *state);
	static void initState(MusicState *state, const char *fileName, const int &sync, byte episode, uint trim);
	static bool openMusicTable();
	static void closeMusicTable();
	static void setMusicTableVolume(float volume);
	static void startState(const int &id, const int &startMs, const bool &immediate);
	static bool consumeStateFlags(const int &id, bool &looped, bool &ended);
	static void updateStates();
	static int getStatePositionMs(const int &id, const bool &relative);
	static void closeAllStates();
	static bool isSameFile(int id, int otherId);
	static const char *findAudioFile(const char *fileName);
	static void clearAudioSearchDirs();
	static int remapState(const int &id);
	static int toStreamVolume(float volume);
	static void resetModule();

	static bool openStream(const char *fileName, int startMs, int loopCount, float volume);
	static void closeStream();
	static void setStreamVolume(int volume);
	static int getStreamMsPosition();
	static int getStreamStatus();
	static uint getStreamPosition();

	int16 count;
	MusicState *states;
	bool loaded;

	static const StateEntry stateTable[kStateCount];

	static CMusicPs2 *activeTable;
	static int16 instanceCount;
	static float masterVolume;
	static float currentFadeRate;
	static int pendingState;
	static int pendingStartMs;
	static bool pendingImmediate;
	static int fadeOutGate;
	static uint firstUpdateMs;
	static uint lastUpdateMs;
	static int searchDirCount;
	static int findMissCount;

	static EMIMilesSample *streamSample;
	static CWaveSound *streamSound;
	static int streamChannels;
	static int streamRate;
};

} // End of namespace Grim

#endif
