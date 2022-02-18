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

#ifndef GRIM_IMUSE_SNDMGR_H
#define GRIM_IMUSE_SNDMGR_H

#include "audio/mixer.h"
#include "audio/audiostream.h"

namespace Grim {

class McmpMgr;

class ImuseSndMgr {
public:

// MAX_IMUSE_SOUNDS needs to be hardcoded, ask aquadran
#define MAX_IMUSE_SOUNDS    32

// The numbering below fixes talking to Domino in his office
// and it also allows Manny to get the info for Mercedes
// Colomar, without this the game hangs at these points!
#define IMUSE_VOLGRP_BGND   0
#define IMUSE_VOLGRP_SFX    1
#define IMUSE_VOLGRP_VOICE  2
#define IMUSE_VOLGRP_MUSIC  3
#define IMUSE_VOLGRP_ACTION 4

private:
	struct Region {
		int32 offset;       // offset of region
		int32 length;       // lenght of region
	};

	struct Jump {
		int32 offset;       // jump offset position
		int32 dest;         // jump to dest position
		byte hookId;        // id of hook
		int16 fadeDelay;    // fade delay in ms
	};

public:

	struct SoundDesc {
		uint16 freq;        // frequency
		byte channels;      // stereo or mono
		byte bits;          // 8, 12, 16
		int numJumps;       // number of Jumps
		int numRegions;     // number of Regions
		Region *region;
		Jump *jump;
		bool endFlag;
		bool inUse;
		char name[32];
		McmpMgr *mcmpMgr;
		int type;
		int volGroupId;
		bool mcmpData;
		uint32 headerSize;
		Common::SeekableReadStream *inStream;
	};

	struct SoundInfo {
		char soundFilename[32];
		int luaSoundId;
		int tsOfAllocation;
		uint8 *soundUncompData;
		uint8 *soundAddrData;
		uint8 *soundMap;
		int uncompResourceSize;
		int flag;
		int priority;
	};

	struct VIMAHeader {
		uint8 chanOneStepIndexHint;
		uint8 chanTwoStepIndexHint;
		int16 chanOnePCMHint;
		int16 chanTwoPCMHint;
	};


	struct SndResource {
		int resId;
		int32 resSize;
		int32 fileCurOffset;
		int compBlocksNum;
		uint8 *mcmpBlockPtr;
		int32 **compBlockPtr;
		int32 *decompBlockSizes;
		int32 *compBlockOffsets;
		int32 *compBlockSizes;
		int32 *vimaOffsets;
		int curProcessedCompBlock;
		int32 curDecompBlockSize;
		uint8 *decompDataBuffer;
		VIMAHeader vimaHeader;
		char filename[80];
		Common::SeekableReadStream *fileStream;
	};

private:

	SoundDesc _Oldsounds[MAX_IMUSE_SOUNDS];
	SoundInfo _iMUSESounds[MAX_IMUSE_SOUNDS];
	SndResource _resources[MAX_IMUSE_SOUNDS];
	Common::Mutex _mutex;
	bool _demo;
	Imuse *_engine;
	int _currentSoundSlot;
	int _nextResIndex;

	bool checkForProperHandle(SoundDesc *soundDesc);
	SoundDesc *allocSlot();
	void parseSoundHeader(SoundDesc *sound, int &headerSize);
	void countElements(SoundDesc *sound);
	int nukeOldestSound();
	intptr openSound(const char *soundName);
	void closeSound(intptr resPtrOrId);
	int getExistingResource(const char *soundName);
	void soundSeek(intptr resPtrOrId, int pos, int mode);
	int soundTell(intptr resPtrOrId);
	void resetLipsyncState(int soundId);
	void registerLipsync(const char *lipFilename, int soundId);
	bool isPointerToResource(intptr resPtrOrId);
	void vimaClearHeader(VIMAHeader *header);
	int getCompBlockIdFromOffset(SndResource *resPtr, int32 offset);

public:

	ImuseSndMgr(bool demo, Imuse *engine);
	~ImuseSndMgr();

	SoundDesc *openSound(const char *soundName, int volGroupId);
	void closeSound(SoundDesc *sound);
	SoundDesc *cloneSound(SoundDesc *sound);

	int getFreq(SoundDesc *sound);
	int getBits(SoundDesc *sound);
	int getChannels(SoundDesc *sound);
	bool isEndOfRegion(SoundDesc *sound, int region);
	int getNumRegions(SoundDesc *sound);
	int getNumJumps(SoundDesc *sound);
	int getRegionOffset(SoundDesc *sound, int region);
	int getRegionLength(SoundDesc *sound, int region);
	int getJumpIdByRegionAndHookId(SoundDesc *sound, int region, int hookId);
	int getRegionIdByJumpId(SoundDesc *sound, int jumpId);
	int getJumpHookId(SoundDesc *sound, int number);
	int getJumpFade(SoundDesc *sound, int number);
	int getSoundId(const char *soundName);
	uint8 *getExtFormatBuffer(uint8 *srcBuf, int &formatId, int &sampleRate, int &wordSize, int &nChans, int &swapFlag, int32 &audioLength);
	void createSoundHeader(int32 *fakeWavHeader, int sampleRate, int wordSize, int nChans, int32 audioLength);
	void copyDataFromResource(intptr resPtrOrId, uint8 *destBuf, int32 size);
	int32 getDataFromRegion(SoundDesc *sound, int region, byte **buf, int32 offset, int32 size);
};

} // end of namespace Grim

#endif
