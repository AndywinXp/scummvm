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

#include "common/endian.h"
#include "common/file.h"
#include "common/path.h"
#include "common/str.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "engines/grim/emi/cmusicps2.h"
#include "engines/grim/emi/cscxfile.h"
#include "engines/grim/emi/cwaveaudiodevice.h"
#include "engines/grim/emi/cwavesound.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lfile.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/miles_audio/driver.h"

namespace Grim {

static const float kEpisodeFadeRate = 1.0f;
static const float kStateFadeRate = 2.0f;
static const float kTrimScale = 0.007874f;
static const float kElapsedScale = 0.001f;
static const float kMusicStreamGain = 0.45f;
static const float kSpuRegisterScale = 16383.0f;
static const float kStreamBlockScale = 1.75f;
static const uint kMaxAilValue = 0x80;
static const int kMaxAilVolume = 127;
static const int kMaxSpuRegister = 0x4000;
static const uint32 kStartOffsetScale = 768000;
static const uint32 kStartOffsetDivisor = 28000;
static const uint kScxLoopOffset = 0x0C;
static const uint kScxHeaderReadSize = 0x10;

const CMusicPs2::StateEntry CMusicPs2::stateTable[CMusicPs2::kStateCount] = {
	{ "", 0, 0, 0 },
	{ "1115.scx", 1, 0, 127 },
	{ "1170.scx", 2, 0, 127 },
	{ "1170.scx", 2, 0, 127 },
	{ "1170.scx", 2, 0, 127 },
	{ "1165.scx", 3, 0, 127 },
	{ "1145.scx", 4, 0, 127 },
	{ "1145.scx", 4, 0, 127 },
	{ "1115.scx", 1, 0, 127 },
	{ "1115.scx", 1, 0, 127 },
	{ "7200.scx", 0, 1, 127 },
	{ "1210.scx", 0, 1, 127 },
	{ "1180.scx", 0, 0, 127 },
	{ "1110.scx", 0, 0, 127 },
	{ "1115.scx", 1, 0, 127 },
	{ "1105.scx", 0, 0, 127 },
	{ "1145.scx", 4, 0, 127 },
	{ "1150.scx", 0, 0, 127 },
	{ "1100.scx", 0, 0, 127 },
	{ "1120.scx", 5, 0, 127 },
	{ "1120.scx", 5, 0, 127 },
	{ "1120.scx", 5, 0, 127 },
	{ "1165.scx", 3, 0, 127 },
	{ "1155.scx", 0, 0, 127 },
	{ "1160.scx", 0, 0, 127 },
	{ "1140.scx", 0, 0, 127 },
	{ "1140.scx", 0, 0, 127 },
	{ "1170.scx", 2, 0, 127 },
	{ "1175.scx", 2, 0, 127 },
	{ "1205.scx", 0, 1, 127 },
	{ "1000.scx", 0, 0, 127 },
	{ "1185.scx", 0, 0, 127 },
	{ "2127.scx", 0, 0, 127 },
	{ "2119.scx", 0, 0, 127 },
	{ "2208.scx", 0, 1, 127 },
	{ "2195.scx", 0, 0, 127 },
	{ "2190.scx", 0, 0, 127 },
	{ "2185.scx", 0, 0, 127 },
	{ "2175.scx", 1, 0, 127 },
	{ "2170.scx", 0, 0, 127 },
	{ "2165.scx", 0, 0, 127 },
	{ "2160.scx", 0, 0, 127 },
	{ "2155.scx", 0, 0, 127 },
	{ "2120.scx", 0, 0, 127 },
	{ "2150.scx", 0, 0, 127 },
	{ "2145.scx", 0, 0, 127 },
	{ "2105.scx", 2, 0, 127 },
	{ "2115.scx", 0, 0, 127 },
	{ "2125.scx", 0, 0, 127 },
	{ "2130.scx", 0, 0, 127 },
	{ "2100.scx", 0, 0, 127 },
	{ "2140.scx", 0, 0, 127 },
	{ "2200.scx", 0, 1, 127 },
	{ "2116.scx", 0, 0, 127 },
	{ "2207.scx", 0, 1, 127 },
	{ "2107.scx", 0, 0, 127 },
	{ "2215.scx", 0, 1, 127 },
	{ "2220.scx", 0, 1, 127 },
	{ "2225.scx", 0, 1, 127 },
	{ "2210.scx", 0, 1, 127 },
	{ "2135.scx", 0, 0, 127 },
	{ "2105.scx", 2, 0, 127 },
	{ "2108.scx", 0, 0, 127 },
	{ "2117.scx", 0, 0, 127 },
	{ "2118.scx", 0, 0, 127 },
	{ "2175.scx", 1, 0, 127 },
	{ "4120.scx", 0, 0, 127 },
	{ "3100.scx", 1, 0, 127 },
	{ "4115.scx", 0, 0, 127 },
	{ "4100.scx", 2, 0, 127 },
	{ "3150.scx", 0, 0, 127 },
	{ "3145.scx", 0, 0, 127 },
	{ "4110.scx", 0, 0, 127 },
	{ "3140.scx", 0, 0, 127 },
	{ "3135.scx", 3, 0, 127 },
	{ "3120.scx", 3, 0, 127 },
	{ "3130.scx", 4, 0, 127 },
	{ "3115.scx", 4, 0, 127 },
	{ "3100.scx", 1, 0, 127 },
	{ "3125.scx", 5, 0, 127 },
	{ "3110.scx", 5, 0, 127 },
	{ "3105.scx", 6, 0, 127 },
	{ "3210.scx", 0, 1, 127 },
	{ "3200.scx", 0, 1, 127 },
	{ "3205.scx", 0, 1, 127 },
	{ "3147.scx", 0, 0, 127 },
	{ "4215.scx", 0, 1, 127 },
	{ "4105.scx", 0, 0, 127 },
	{ "3106.scx", 6, 0, 127 },
	{ "3107.scx", 6, 0, 127 },
	{ "4100.scx", 2, 0, 127 },
	{ "5145.scx", 1, 0, 127 },
	{ "5140.scx", 2, 0, 127 },
	{ "5140.scx", 2, 0, 127 },
	{ "5135.scx", 3, 0, 127 },
	{ "5135.scx", 3, 0, 127 },
	{ "5135.scx", 3, 0, 127 },
	{ "5170.scx", 0, 0, 127 },
	{ "5205.scx", 0, 1, 127 },
	{ "5120.scx", 0, 0, 127 },
	{ "5215.scx", 0, 1, 127 },
	{ "5230.scx", 0, 1, 127 },
	{ "5225.scx", 0, 1, 127 },
	{ "5117.scx", 0, 0, 127 },
	{ "5115.scx", 0, 0, 127 },
	{ "5220.scx", 0, 1, 127 },
	{ "6105.scx", 0, 0, 127 },
	{ "6100.scx", 0, 0, 127 },
	{ "5165.scx", 0, 0, 127 },
	{ "5160.scx", 0, 0, 127 },
	{ "5200.scx", 0, 1, 127 },
	{ "5140.scx", 2, 0, 127 },
	{ "5135.scx", 3, 0, 127 },
	{ "5155.scx", 0, 0, 127 },
	{ "5150.scx", 0, 0, 127 },
	{ "5130.scx", 0, 0, 127 },
	{ "5125.scx", 0, 0, 127 },
	{ "5110.scx", 0, 0, 127 },
	{ "5105.scx", 1, 0, 127 },
	{ "5100.scx", 0, 0, 127 },
	{ "6110.scx", 0, 0, 127 },
	{ "5106.scx", 0, 0, 127 },
	{ "7210.scx", 0, 1, 127 },
	{ "1200.scx", 0, 1, 127 },
	{ "1195.scx", 0, 0, 127 },
	{ "1215.scx", 0, 1, 127 }
};

CMusicPs2 *CMusicPs2::activeTable = nullptr;
int16 CMusicPs2::instanceCount = 0;
float CMusicPs2::masterVolume = 1.0f;
float CMusicPs2::currentFadeRate = 1.0f;
int CMusicPs2::pendingState = -1;
int CMusicPs2::pendingStartMs = 0;
bool CMusicPs2::pendingImmediate = false;
int CMusicPs2::fadeOutGate = 0;
uint CMusicPs2::firstUpdateMs = 0;
uint CMusicPs2::lastUpdateMs = 0;
int CMusicPs2::searchDirCount = 0;
int CMusicPs2::findMissCount = 0;
EMIMilesSample *CMusicPs2::streamSample = nullptr;
CWaveSound *CMusicPs2::streamSound = nullptr;
int CMusicPs2::streamChannels = 0;
int CMusicPs2::streamRate = 0;

CMusicPs2::CMusicPs2() {
	count = 0;
	states = nullptr;
	loaded = false;

	instanceCount++;

	if (instanceCount < 2) {
		loaded = false;
		states = nullptr;

		initStates();
		resetStartOffsets();

		loaded = true;
	}
}

CMusicPs2::~CMusicPs2() {
	instanceCount--;

	if (instanceCount <= 0) {
		closeStreams();

		if (states) {
			gpMemoryServices->free(states);
			states = nullptr;
		}

		loaded = false;
	}
}

void *CMusicPs2::operator new(size_t size) {
	return gpMemoryServices->malloc(size);
}

void CMusicPs2::operator delete(void *block) {
	gpMemoryServices->free(block);
}

void CMusicPs2::initStates() {
	count = kStateCount;
	states = (MusicState *)gpMemoryServices->malloc(count * sizeof(MusicState));

	if (!states) {
		warning("CMusicPs2::initStates(): out of memory");
		count = 0;

		return;
	}

	for (int i = 0; i < count; i++)
		constructState(&states[i]);

	for (int i = 0; i < count; i++)
		initState(&states[i], stateTable[i].fileName, stateTable[i].sync, stateTable[i].episode, stateTable[i].trim);
}

void CMusicPs2::resetStartOffsets() {
	if (!states)
		return;

	for (int i = 1; i < count; i++)
		states[i].startOffsetMs = 0.0f;
}

void CMusicPs2::closeStreams() {
	if (!loaded)
		return;

	for (int i = 0; i < count; i++) {
		MusicState *state = &states[i];

		if (state->handle) {
			closeStream();
			state->handle = 0;
			state->fadeState = kFadeNone;
		}
	}
}

void CMusicPs2::constructState(MusicState *state) {
	state->sync = 0;
	state->episode = 0;
	state->lastPositionMs = 0;
	state->looped = false;
	state->ended = false;
	state->fadeRate = 0.0f;
	state->fadeVolume = 0.0f;
	state->fadeState = kFadeNone;
	state->loopCount = 0;
	state->fileName[0] = '\0';
	state->handle = 0;
	state->trim = 1.0f;
	state->startOffsetMs = 0.0f;
}

void CMusicPs2::initState(MusicState *state, const char *fileName, const int &sync, byte episode, uint trim) {
	state->sync = sync;
	state->episode = episode;
	state->lastPositionMs = 0;
	state->looped = false;
	state->ended = false;
	state->fadeRate = episode ? kEpisodeFadeRate : kStateFadeRate;
	state->fadeVolume = 0.0f;
	state->fadeState = kFadeNone;

	Common::strlcpy(state->fileName, findAudioFile(fileName), kMaxFileName);

	state->trim = kTrimScale * (float)trim;
	state->loopCount = episode;
}

bool CMusicPs2::openMusicTable() {
	if (activeTable)
		return false;

	CMusicPs2 *table = new CMusicPs2();

	activeTable = table;

	if (!table->loaded) {
		delete table;
		activeTable = nullptr;

		return false;
	}

	return true;
}

void CMusicPs2::closeMusicTable() {
	if (activeTable) {
		delete activeTable;
		activeTable = nullptr;
	}
}

void CMusicPs2::setMusicTableVolume(float volume) {
	float clamped = volume;

	if (!(volume >= 0.0f))
		clamped = 0.0f;
	else if (volume > 1.0f)
		clamped = 1.0f;

	CMusicPs2 *table = activeTable;

	if (!table)
		return;

	masterVolume = clamped;

	for (int16 i = 0; i < table->count; i++) {
		MusicState *state = &table->states[i];

		if (state->handle)
			setStreamVolume(toStreamVolume(state->trim * (state->fadeVolume * masterVolume)));
	}
}

void CMusicPs2::startState(const int &id, const int &startMs, const bool &immediate) {
	CMusicPs2 *table = activeTable;

	if (!table)
		return;

	int target = remapState(id);
	MusicState *keptState = nullptr;
	bool otherOpen = false;

	pendingState = -1;

	for (int16 i = 0; i < table->count; i++) {
		MusicState *state = &table->states[i];

		if (!state->handle || i == target)
			continue;

		MusicState *closedState = nullptr;

		otherOpen = true;

		if (keptState) {
			closedState = keptState;

			if (keptState->fadeVolume < state->fadeVolume)
				keptState = state;
			else
				closedState = state;

			closedState->fadeVolume = 0.0f;
			closeStream();
			closedState->handle = 0;
			closedState->fadeState = kFadeNone;
		} else {
			keptState = state;
		}

		if (closedState != state)
			state->fadeState = kFadeOut;
	}

	// The next state waits for the fade-out
	if (otherOpen) {
		pendingState = target;
		pendingStartMs = startMs;
		pendingImmediate = immediate;

		return;
	}

	if (target <= 0 || table->count < target)
		return;

	if (target == table->count) {
		warning("CMusicPs2::startState(): state %d past end of table", target);

		return;
	}

	MusicState *state = &table->states[target];

	if (!state->handle) {
		state->handle = kStreamHandle;
		state->lastPositionMs = 0;
		state->looped = false;
		state->ended = false;
		currentFadeRate = state->fadeRate;

		if (startMs > 0)
			state->lastPositionMs = startMs;

		if (immediate)
			state->fadeVolume = 1.0f;
		else if (otherOpen || startMs >= 0)
			state->fadeVolume = 0.0f;
		else
			state->fadeVolume = 1.0f;

		float volume = state->trim * (state->fadeVolume * masterVolume);

		if (openStream(state->fileName, startMs > 0 ? startMs : 0, state->loopCount, volume)) {
			if (gpConsoleServices)
				gpConsoleServices->logError("AIL_open_stream: FAILED\n");
		}
	}

	state->fadeState = kFadeIn;
}

bool CMusicPs2::consumeStateFlags(const int &id, bool &looped, bool &ended) {
	CMusicPs2 *table = activeTable;

	if (!table)
		return false;

	int target = remapState(id);

	looped = false;
	ended = false;

	if (target <= 0 || table->count < target)
		return false;

	if (target == table->count) {
		warning("CMusicPs2::consumeStateFlags(): state %d past end of table", target);

		return false;
	}

	MusicState *state = &table->states[target];

	looped = state->looped;
	ended = state->ended;
	state->looped = false;
	state->ended = false;

	return true;
}

void CMusicPs2::updateStates() {
	uint now = g_system->getMillis();

	if (firstUpdateMs == 0) {
		firstUpdateMs = now;
		lastUpdateMs = now;

		return;
	}

	uint previous = lastUpdateMs;

	lastUpdateMs = now;

	float elapsed = kElapsedScale * (float)(int)(now - previous);
	CMusicPs2 *table = activeTable;

	if (!table)
		return;

	int fadingOut = 0;

	for (int16 i = 0; i < table->count; i++) {
		MusicState *state = &table->states[i];

		if (!state->handle)
			continue;

		if (!state->looped) {
			int position = getStreamMsPosition();

			if (position < state->lastPositionMs)
				state->looped = true;

			state->lastPositionMs = position;
		}

		if (!state->ended && getStreamStatus() == kStreamDone)
			state->ended = true;

		if (state->fadeState == kFadeNone)
			continue;

		if (state->fadeState == kFadeIn) {
			state->fadeVolume = state->fadeVolume + currentFadeRate * elapsed;
		} else {
			fadingOut = kFadeOut;

			// The fade-out starts one update late
			if (!fadeOutGate)
				continue;

			state->fadeVolume = state->fadeVolume - currentFadeRate * elapsed;
		}

		if (state->fadeVolume <= 0.0f) {
			state->fadeVolume = 0.0f;
			closeStream();
			state->handle = 0;
			state->fadeState = kFadeNone;

			startState(pendingState, pendingStartMs, pendingImmediate);

			continue;
		}

		if (!(state->fadeVolume <= 1.0f)) {
			state->fadeVolume = 1.0f;
			state->fadeState = kFadeNone;
		}

		setStreamVolume(toStreamVolume(state->trim * (state->fadeVolume * masterVolume)));
	}

	fadeOutGate = fadingOut;
}

int CMusicPs2::getStatePositionMs(const int &id, const bool &relative) {
	CMusicPs2 *table = activeTable;

	if (!table)
		return 0;

	int target = remapState(id);
	int position = -1;

	if (target <= 0 || table->count < target)
		return 0;

	if (target == table->count) {
		warning("CMusicPs2::getStatePositionMs(): state %d past end of table", target);

		return 0;
	}

	MusicState *state = &table->states[target];

	if (state->handle) {
		position = getStreamMsPosition();

		if (relative) {
			position = (int)(((float)position - state->startOffsetMs) + 0.5f);

			if (position < 0)
				position = 0;
		}
	}

	return position;
}

void CMusicPs2::closeAllStates() {
	CMusicPs2 *table = activeTable;

	if (!table)
		return;

	for (int16 i = 0; i < table->count; i++) {
		MusicState *state = &table->states[i];

		if (state->handle) {
			closeStream();
			state->handle = 0;
			state->fadeState = kFadeNone;
		}
	}
}

bool CMusicPs2::isSameFile(int id, int otherId) {
	CMusicPs2 *table = activeTable;

	if (!table)
		return true;

	if (id == otherId)
		return true;

	if (id < 0 || id >= table->count || otherId < 0 || otherId >= table->count) {
		warning("CMusicPs2::isSameFile(): state %d or %d out of table", id, otherId);

		return false;
	}

	return scumm_stricmp(table->states[id].fileName, table->states[otherId].fileName) == 0;
}

const char *CMusicPs2::findAudioFile(const char *fileName) {
	if (!*fileName)
		return fileName;

	if (Common::File::exists(Common::Path(fileName)))
		return fileName;

	int misses = findMissCount;

	findMissCount++;

	if (misses < kMaxMissMessages && gpConsoleServices)
		gpConsoleServices->logError("Error: File '%s' could not be found.\n", fileName);

	return fileName;
}

void CMusicPs2::clearAudioSearchDirs() {
	searchDirCount = 0;
}

int CMusicPs2::remapState(const int &id) {
	switch (id) {
	case 2:
	case 3:
	case 4:
		return 27;

	case 6:
	case 7:
		return 16;

	case 1:
	case 9:
		return 14;

	case 20:
	case 21:
		return 19;

	case 5:
		return 22;

	case 25:
		return 26;

	case 78:
		return 67;

	case 90:
		return 69;

	case 92:
	case 93:
		return 111;

	case 94:
	case 95:
	case 96:
		return 112;

	default:
		break;
	}

	return id;
}

int CMusicPs2::toStreamVolume(float volume) {
	return (int)((float)kMaxAilVolume * volume + 0.5f);
}

void CMusicPs2::resetModule() {
	closeMusicTable();
	closeStream();

	instanceCount = 0;
	masterVolume = 1.0f;
	currentFadeRate = 1.0f;
	pendingState = -1;
	pendingStartMs = 0;
	pendingImmediate = false;
	fadeOutGate = 0;
	firstUpdateMs = 0;
	lastUpdateMs = 0;
	searchDirCount = 0;
	findMissCount = 0;
}

bool CMusicPs2::openStream(const char *fileName, int startMs, int loopCount, float volume) {
	// Wraps above 5592 ms, like the original
	uint32 byteOffset = kStartOffsetScale * (uint32)startMs / kStartOffsetDivisor;
	float streamVolume = kMusicStreamGain * volume;

	if (*fileName == '@')
		fileName++;

	LFile *file = gpFileServices->open(fileName, "rb");

	if (!file)
		return true;

	byte header[kScxHeaderReadSize];
	uint32 loopOffset = 0;

	if (file->read(header, kScxHeaderReadSize) == kScxHeaderReadSize)
		loopOffset = READ_LE_UINT32(header + kScxLoopOffset);

	file->seek(0, SEEK_SET);

	CScxFile scxFile;
	CWaveSound *sound = new CWaveSound();
	bool parsed = scxFile.read(file, sound);

	gpFileServices->close(file, true);

	if (!parsed) {
		warning("CMusicPs2::openStream(): can't parse %s", fileName);
		delete sound;

		return true;
	}

	if (!(streamVolume <= 1.0f && streamVolume >= 0.0f)) {
		warning("CMusicPs2::openStream(): invalid volume %f", streamVolume);
		delete sound;

		return true;
	}

	float scaled = kSpuRegisterScale * streamVolume;
	int level = scaled > 0.0f ? (int)scaled : 0;

	if (level >= kMaxSpuRegister) {
		warning("CMusicPs2::openStream(): invalid register volume 0x%08x", level);
		delete sound;

		return true;
	}

	EMIMilesDriver *driver = CWaveAudioDevice::getMilesDriver();
	EMIMilesSample *sample = driver ? driver->allocateSample() : nullptr;

	if (!sample) {
		warning("CMusicPs2::openStream(): no free sample for %s", fileName);
		delete sound;

		return true;
	}

	// The IOP replaces a stream that is already open on the music channel
	closeStream();

	int channels = scxFile.numChannels;
	int frameBytes = channels * (int)sizeof(int16);
	int format = EMIMilesDriver::kFormat16Bit;

	if (channels == 2)
		format |= EMIMilesDriver::kFormatStereo;

	streamSample = sample;
	streamSound = sound;
	streamChannels = channels;
	streamRate = (int)scxFile.rate;

	driver->initSample(sample);

	// The music stream plays on core 1, which has no reverb
	driver->setSampleEffectSend(sample, false);
	driver->setSampleAddress(sample, sound->samples, sound->dataSize);
	driver->setSampleType(sample, format);
	driver->setSampleRate(sample, streamRate);

	// The IOP loops at the header offset and ignores the ADPCM flags
	if (loopCount == 1) {
		driver->setSampleLoopCount(sample, 1);
	} else {
		uint loopStart = 0;
		int loopEnd = -1;
		uint dataEnd = scxFile.vagSize[0] / CScxFile::kAdpcmBlockSize * CScxFile::kAdpcmBlockSamples * frameBytes;

		if (loopOffset >= CScxFile::kVagHeaderSize)
			loopStart = (loopOffset - CScxFile::kVagHeaderSize) / CScxFile::kAdpcmBlockSize * CScxFile::kAdpcmBlockSamples * frameBytes;

		if (dataEnd < sound->dataSize)
			loopEnd = (int)dataEnd;

		driver->setSampleLoopCount(sample, loopCount);
		driver->setSampleLoopBlock(sample, (int)loopStart, loopEnd);
	}

	uint channelOffset = byteOffset / channels;
	uint startPosition = 0;

	if (channelOffset >= CScxFile::kVagHeaderSize)
		startPosition = (channelOffset - CScxFile::kVagHeaderSize) / CScxFile::kAdpcmBlockSize * CScxFile::kAdpcmBlockSamples * frameBytes;

	if (startPosition >= sound->dataSize) {
		warning("CMusicPs2::openStream(): start offset %u past end of %s", byteOffset, fileName);
		startPosition = 0;
	}

	driver->setSampleSpuVolume(sample, level, level);
	driver->setSamplePosition(sample, (int)startPosition);

	// Resuming keeps the position, while starting would rewind
	driver->resumeSample(sample);

	return false;
}

void CMusicPs2::closeStream() {
	if (streamSample) {
		EMIMilesDriver *driver = CWaveAudioDevice::getMilesDriver();

		if (driver)
			driver->releaseSample(streamSample);

		streamSample = nullptr;
	}

	if (streamSound) {
		delete streamSound;
		streamSound = nullptr;
	}

	streamChannels = 0;
	streamRate = 0;
}

void CMusicPs2::setStreamVolume(int volume) {
	if ((uint)volume >= kMaxAilValue) {
		if (gpConsoleServices)
			gpConsoleServices->logError("AIL_set_stream_volume: volume out of range\n");

		return;
	}

	float streamVolume = kMusicStreamGain * ((float)volume / (float)kMaxAilVolume);

	if (!(streamVolume >= 0.0f && streamVolume <= 1.0f)) {
		if (gpConsoleServices)
			gpConsoleServices->logError("AIL_set_stream_volume: volume out of range: %f\n", streamVolume);

		return;
	}

	float scaled = kSpuRegisterScale * streamVolume;
	int level = scaled > 0.0f ? (int)scaled : 0;

	if (level >= kMaxSpuRegister) {
		warning("CMusicPs2::setStreamVolume(): invalid register volume 0x%08x", level);

		return;
	}

	EMIMilesDriver *driver = CWaveAudioDevice::getMilesDriver();

	if (driver && streamSample)
		driver->setSampleSpuVolume(streamSample, level, level);
}

int CMusicPs2::getStreamMsPosition() {
	EMIMilesDriver *driver = CWaveAudioDevice::getMilesDriver();

	if (!driver || !streamSample || !streamChannels || !streamRate)
		return 0;

	uint frames = (uint)driver->getSamplePosition(streamSample) / (uint)(streamChannels * (int)sizeof(int16));
	uint channelBytes = CScxFile::kVagHeaderSize + frames * CScxFile::kAdpcmBlockSize / CScxFile::kAdpcmBlockSamples;
	uint position = channelBytes * (uint)streamChannels;
	float milliseconds = (1000.0f / (float)streamRate) * (((float)position - 1.0f) * kStreamBlockScale);
	uint result = milliseconds > 0.0f ? (uint)(int)milliseconds : 0;

	return (int)(result / (uint)streamChannels);
}

int CMusicPs2::getStreamStatus() {
	EMIMilesDriver *driver = CWaveAudioDevice::getMilesDriver();

	if (!driver || !streamSample)
		return 0;

	return driver->getSampleStatus(streamSample);
}

uint CMusicPs2::getStreamPosition() {
	EMIMilesDriver *driver = CWaveAudioDevice::getMilesDriver();

	if (!driver || !streamSample || !streamChannels)
		return 0;

	uint frames = (uint)driver->getSamplePosition(streamSample) / (uint)(streamChannels * (int)sizeof(int16));
	uint channelBytes = CScxFile::kVagHeaderSize + frames * CScxFile::kAdpcmBlockSize / CScxFile::kAdpcmBlockSamples;

	return channelBytes * (uint)streamChannels;
}

} // End of namespace Grim
