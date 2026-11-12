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

#include "engines/grim/emi/sound/audiosystem.h"
#include "engines/grim/emi/sound/cmusicps2.h"
#include "engines/grim/emi/sound/cscxfile.h"
#include "engines/grim/emi/sound/cwavesound.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lconsoleservices.h"
#include "engines/grim/emi/files/lfile.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/sound/miles_audio/driver.h"
#include "engines/grim/emi/sound/sounds.h"

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

const CMusicPs2::StateEntry CMusicPs2::kStateTable[CMusicPs2::kStateCount] = {
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

CMusicPs2::CMusicPs2(Sounds *sounds) {
	_sounds = sounds;
	_count = 0;
	_states = nullptr;
	_loaded = false;

	_masterVolume = 1.0f;
	_currentFadeRate = 1.0f;
	_pendingState = -1;
	_pendingStartMs = 0;
	_pendingImmediate = false;
	_fadeOutGate = 0;
	_firstUpdateMs = 0;
	_lastUpdateMs = 0;
	_searchDirCount = 0;
	_findMissCount = 0;
	_streamSample = nullptr;
	_streamSound = nullptr;
	_streamChannels = 0;
	_streamRate = 0;
}

CMusicPs2::~CMusicPs2() {
	closeMusicTable();
	closeStream();
}

void *CMusicPs2::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void CMusicPs2::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

void CMusicPs2::initStates() {
	_count = kStateCount;
	_states = (MusicState *)_sounds->getEngine()->getMemoryServices()->mallocImpl(_count * sizeof(MusicState));

	if (!_states) {
		warning("CMusicPs2::initStates(): out of memory");
		_count = 0;

		return;
	}

	for (int i = 0; i < _count; i++)
		constructState(&_states[i]);

	for (int i = 0; i < _count; i++)
		initState(&_states[i], kStateTable[i].fileName, kStateTable[i].sync, kStateTable[i].episode, kStateTable[i].trim);
}

void CMusicPs2::resetStartOffsets() {
	if (!_states)
		return;

	for (int i = 1; i < _count; i++)
		_states[i].startOffsetMs = 0.0f;
}

void CMusicPs2::closeStreams() {
	if (!_loaded)
		return;

	for (int i = 0; i < _count; i++) {
		MusicState *state = &_states[i];

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
	if (_loaded)
		return false;

	initStates();
	resetStartOffsets();

	_loaded = true;

	return true;
}

void CMusicPs2::closeMusicTable() {
	if (!_loaded)
		return;

	closeStreams();

	if (_states) {
		_sounds->getEngine()->getMemoryServices()->freeImpl(_states);
		_states = nullptr;
	}

	_count = 0;
	_loaded = false;
}

void CMusicPs2::setMusicTableVolume(float volume) {
	float clamped = volume;

	if (!(volume >= 0.0f))
		clamped = 0.0f;
	else if (volume > 1.0f)
		clamped = 1.0f;

	if (!_loaded)
		return;

	_masterVolume = clamped;

	for (int16 i = 0; i < _count; i++) {
		MusicState *state = &_states[i];

		if (state->handle)
			setStreamVolume(toStreamVolume(state->trim * (state->fadeVolume * _masterVolume)));
	}
}

void CMusicPs2::startState(const int &id, const int &startMs, const bool &immediate) {
	if (!_loaded)
		return;

	int target = remapState(id);
	MusicState *keptState = nullptr;
	bool otherOpen = false;

	_pendingState = -1;

	for (int16 i = 0; i < _count; i++) {
		MusicState *state = &_states[i];

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
		_pendingState = target;
		_pendingStartMs = startMs;
		_pendingImmediate = immediate;

		return;
	}

	if (target <= 0 || _count < target)
		return;

	if (target == _count) {
		warning("CMusicPs2::startState(): state %d past end of table", target);

		return;
	}

	MusicState *state = &_states[target];

	if (!state->handle) {
		state->handle = kStreamHandle;
		state->lastPositionMs = 0;
		state->looped = false;
		state->ended = false;
		_currentFadeRate = state->fadeRate;

		if (startMs > 0)
			state->lastPositionMs = startMs;

		if (immediate)
			state->fadeVolume = 1.0f;
		else if (otherOpen || startMs >= 0)
			state->fadeVolume = 0.0f;
		else
			state->fadeVolume = 1.0f;

		float volume = state->trim * (state->fadeVolume * _masterVolume);

		if (openStream(state->fileName, startMs > 0 ? startMs : 0, state->loopCount, volume)) {
			LConsoleServices *console = _sounds->getEngine()->getConsoleServices();

			if (console)
				console->logError("AIL_open_stream: FAILED\n");
		}
	}

	state->fadeState = kFadeIn;
}

bool CMusicPs2::consumeStateFlags(const int &id, bool &looped, bool &ended) {
	if (!_loaded)
		return false;

	int target = remapState(id);

	looped = false;
	ended = false;

	if (target <= 0 || _count < target)
		return false;

	if (target == _count) {
		warning("CMusicPs2::consumeStateFlags(): state %d past end of table", target);

		return false;
	}

	MusicState *state = &_states[target];

	looped = state->looped;
	ended = state->ended;
	state->looped = false;
	state->ended = false;

	return true;
}

void CMusicPs2::updateStates() {
	uint now = g_system->getMillis();

	if (_firstUpdateMs == 0) {
		_firstUpdateMs = now;
		_lastUpdateMs = now;

		return;
	}

	uint previous = _lastUpdateMs;

	_lastUpdateMs = now;

	float elapsed = kElapsedScale * (float)(int)(now - previous);

	if (!_loaded)
		return;

	int fadingOut = 0;

	for (int16 i = 0; i < _count; i++) {
		MusicState *state = &_states[i];

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
			state->fadeVolume = state->fadeVolume + _currentFadeRate * elapsed;
		} else {
			fadingOut = kFadeOut;

			// The fade-out starts one update late
			if (!_fadeOutGate)
				continue;

			state->fadeVolume = state->fadeVolume - _currentFadeRate * elapsed;
		}

		if (state->fadeVolume <= 0.0f) {
			state->fadeVolume = 0.0f;
			closeStream();
			state->handle = 0;
			state->fadeState = kFadeNone;

			startState(_pendingState, _pendingStartMs, _pendingImmediate);

			continue;
		}

		if (!(state->fadeVolume <= 1.0f)) {
			state->fadeVolume = 1.0f;
			state->fadeState = kFadeNone;
		}

		setStreamVolume(toStreamVolume(state->trim * (state->fadeVolume * _masterVolume)));
	}

	_fadeOutGate = fadingOut;
}

int CMusicPs2::getStatePositionMs(const int &id, const bool &relative) {
	if (!_loaded)
		return 0;

	int target = remapState(id);
	int position = -1;

	if (target <= 0 || _count < target)
		return 0;

	if (target == _count) {
		warning("CMusicPs2::getStatePositionMs(): state %d past end of table", target);

		return 0;
	}

	MusicState *state = &_states[target];

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
	if (!_loaded)
		return;

	for (int16 i = 0; i < _count; i++) {
		MusicState *state = &_states[i];

		if (state->handle) {
			closeStream();
			state->handle = 0;
			state->fadeState = kFadeNone;
		}
	}
}

bool CMusicPs2::isSameFile(int id, int otherId) {
	if (!_loaded)
		return true;

	if (id == otherId)
		return true;

	if (id < 0 || id >= _count || otherId < 0 || otherId >= _count) {
		warning("CMusicPs2::isSameFile(): state %d or %d out of table", id, otherId);

		return false;
	}

	return scumm_stricmp(_states[id].fileName, _states[otherId].fileName) == 0;
}

const char *CMusicPs2::findAudioFile(const char *fileName) {
	if (!*fileName)
		return fileName;

	if (Common::File::exists(Common::Path(fileName)))
		return fileName;

	int misses = _findMissCount;

	_findMissCount++;

	LConsoleServices *console = _sounds->getEngine()->getConsoleServices();

	if (misses < kMaxMissMessages && console)
		console->logError("Error: File '%s' could not be found.\n", fileName);

	return fileName;
}

void CMusicPs2::clearAudioSearchDirs() {
	_searchDirCount = 0;
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

bool CMusicPs2::openStream(const char *fileName, int startMs, int loopCount, float volume) {
	// Wraps above 5592 ms, like the original
	uint32 byteOffset = kStartOffsetScale * (uint32)startMs / kStartOffsetDivisor;
	float streamVolume = kMusicStreamGain * volume;

	if (*fileName == '@')
		fileName++;

	EmiEngine *engine = _sounds->getEngine();
	LFileServices *fileServices = engine->getFileServices();
	LFile *file = fileServices->open(fileName, "rb");

	if (!file)
		return true;

	byte header[kScxHeaderReadSize];
	uint32 loopOffset = 0;

	if (file->read(header, kScxHeaderReadSize) == kScxHeaderReadSize)
		loopOffset = READ_LE_UINT32(header + kScxLoopOffset);

	file->seek(0, SEEK_SET);

	CScxFile scxFile;
	CWaveSound *sound = new CWaveSound();

	sound->bindEngine(engine);

	bool parsed = scxFile.read(file, sound);

	fileServices->close(file, true);

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

	EMIMilesDriver *driver = getMilesDriver();
	EMIMilesSample *sample = driver ? driver->allocateSample() : nullptr;

	if (!sample) {
		warning("CMusicPs2::openStream(): no free sample for %s", fileName);
		delete sound;

		return true;
	}

	// The IOP replaces a stream that is already open on the music channel
	closeStream();

	int channels = scxFile._numChannels;
	int frameBytes = channels * (int)sizeof(int16);
	int format = EMIMilesDriver::kFormat16Bit;

	if (channels == 2)
		format |= EMIMilesDriver::kFormatStereo;

	_streamSample = sample;
	_streamSound = sound;
	_streamChannels = channels;
	_streamRate = (int)scxFile._rate;

	driver->initSample(sample);

	// The music stream plays on core 1, which has no reverb
	driver->setSampleEffectSend(sample, false);
	driver->setSampleAddress(sample, sound->_samples, sound->_dataSize);
	driver->setSampleType(sample, format);
	driver->setSampleRate(sample, _streamRate);

	// The IOP loops at the header offset and ignores the ADPCM flags
	if (loopCount == 1) {
		driver->setSampleLoopCount(sample, 1);
	} else {
		uint loopStart = 0;
		int loopEnd = -1;
		uint dataEnd = scxFile._vagSize[0] / CScxFile::kAdpcmBlockSize * CScxFile::kAdpcmBlockSamples * frameBytes;

		if (loopOffset >= CScxFile::kVagHeaderSize)
			loopStart = (loopOffset - CScxFile::kVagHeaderSize) / CScxFile::kAdpcmBlockSize * CScxFile::kAdpcmBlockSamples * frameBytes;

		if (dataEnd < sound->_dataSize)
			loopEnd = (int)dataEnd;

		driver->setSampleLoopCount(sample, loopCount);
		driver->setSampleLoopBlock(sample, (int)loopStart, loopEnd);
	}

	uint channelOffset = byteOffset / channels;
	uint startPosition = 0;

	if (channelOffset >= CScxFile::kVagHeaderSize)
		startPosition = (channelOffset - CScxFile::kVagHeaderSize) / CScxFile::kAdpcmBlockSize * CScxFile::kAdpcmBlockSamples * frameBytes;

	if (startPosition >= sound->_dataSize) {
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
	if (_streamSample) {
		EMIMilesDriver *driver = getMilesDriver();

		if (driver)
			driver->releaseSample(_streamSample);

		_streamSample = nullptr;
	}

	if (_streamSound) {
		delete _streamSound;
		_streamSound = nullptr;
	}

	_streamChannels = 0;
	_streamRate = 0;
}

void CMusicPs2::setStreamVolume(int volume) {
	LConsoleServices *console = _sounds->getEngine()->getConsoleServices();

	if ((uint)volume >= kMaxAilValue) {
		if (console)
			console->logError("AIL_set_stream_volume: volume out of range\n");

		return;
	}

	float streamVolume = kMusicStreamGain * ((float)volume / (float)kMaxAilVolume);

	if (!(streamVolume >= 0.0f && streamVolume <= 1.0f)) {
		if (console)
			console->logError("AIL_set_stream_volume: volume out of range: %f\n", streamVolume);

		return;
	}

	float scaled = kSpuRegisterScale * streamVolume;
	int level = scaled > 0.0f ? (int)scaled : 0;

	if (level >= kMaxSpuRegister) {
		warning("CMusicPs2::setStreamVolume(): invalid register volume 0x%08x", level);

		return;
	}

	EMIMilesDriver *driver = getMilesDriver();

	if (driver && _streamSample)
		driver->setSampleSpuVolume(_streamSample, level, level);
}

int CMusicPs2::getStreamMsPosition() {
	EMIMilesDriver *driver = getMilesDriver();

	if (!driver || !_streamSample || !_streamChannels || !_streamRate)
		return 0;

	uint frames = (uint)driver->getSamplePosition(_streamSample) / (uint)(_streamChannels * (int)sizeof(int16));
	uint channelBytes = CScxFile::kVagHeaderSize + frames * CScxFile::kAdpcmBlockSize / CScxFile::kAdpcmBlockSamples;
	uint position = channelBytes * (uint)_streamChannels;
	float milliseconds = (1000.0f / (float)_streamRate) * (((float)position - 1.0f) * kStreamBlockScale);
	uint result = milliseconds > 0.0f ? (uint)(int)milliseconds : 0;

	return (int)(result / (uint)_streamChannels);
}

int CMusicPs2::getStreamStatus() {
	EMIMilesDriver *driver = getMilesDriver();

	if (!driver || !_streamSample)
		return 0;

	return driver->getSampleStatus(_streamSample);
}

uint CMusicPs2::getStreamPosition() {
	EMIMilesDriver *driver = getMilesDriver();

	if (!driver || !_streamSample || !_streamChannels)
		return 0;

	uint frames = (uint)driver->getSamplePosition(_streamSample) / (uint)(_streamChannels * (int)sizeof(int16));
	uint channelBytes = CScxFile::kVagHeaderSize + frames * CScxFile::kAdpcmBlockSize / CScxFile::kAdpcmBlockSamples;

	return channelBytes * (uint)_streamChannels;
}

EMIMilesDriver *CMusicPs2::getMilesDriver() {
	AudioSystem *audioSystem = _sounds->getAudioSystem();

	if (!audioSystem)
		return nullptr;

	return audioSystem->getMilesDriver();
}

} // End of namespace Grim
