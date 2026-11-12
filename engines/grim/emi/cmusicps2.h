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
class EMIMilesDriver;
struct EMIMilesSample;
class Sounds;

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

	explicit CMusicPs2(Sounds *sounds);
	~CMusicPs2();

	void *operator new(size_t size);
	void operator delete(void *block);

	void initStates();
	void resetStartOffsets();
	void closeStreams();

	static void constructState(MusicState *state);
	void initState(MusicState *state, const char *fileName, const int &sync, byte episode, uint trim);
	bool openMusicTable();
	void closeMusicTable();
	void setMusicTableVolume(float volume);
	void startState(const int &id, const int &startMs, const bool &immediate);
	bool consumeStateFlags(const int &id, bool &looped, bool &ended);
	void updateStates();
	int getStatePositionMs(const int &id, const bool &relative);
	void closeAllStates();
	bool isSameFile(int id, int otherId);
	const char *findAudioFile(const char *fileName);
	void clearAudioSearchDirs();
	static int remapState(const int &id);
	static int toStreamVolume(float volume);

	bool openStream(const char *fileName, int startMs, int loopCount, float volume);
	void closeStream();
	void setStreamVolume(int volume);
	int getStreamMsPosition();
	int getStreamStatus();
	uint getStreamPosition();

	int16 count;
	MusicState *states;
	bool loaded;

	static const StateEntry stateTable[kStateCount];

private:
	EMIMilesDriver *getMilesDriver();

	Sounds *_sounds;
	float _masterVolume;
	float _currentFadeRate;
	int _pendingState;
	int _pendingStartMs;
	bool _pendingImmediate;
	int _fadeOutGate;
	uint _firstUpdateMs;
	uint _lastUpdateMs;
	int _searchDirCount;
	int _findMissCount;

	EMIMilesSample *_streamSample;
	CWaveSound *_streamSound;
	int _streamChannels;
	int _streamRate;
};

} // End of namespace Grim

#endif
