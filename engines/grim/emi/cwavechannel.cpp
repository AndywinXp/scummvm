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

#include "common/system.h"
#include "common/textconsole.h"

#include "audio/decoders/raw.h"

#include "engines/grim/emi/audiosystem.h"
#include "engines/grim/emi/csoundcue.h"
#include "engines/grim/emi/cwaveaudiodevice.h"
#include "engines/grim/emi/cwavechannel.h"
#include "engines/grim/emi/cwavesound.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

static const int kMaxSampleVolume = 127;
static const int kCenterPan = 64;
static const int kDefaultSampleRate = 11025;
static const int kPanScaleLimit = 128;
static const uint kSilentTailHeadroom = 4;

int CWaveChannel::silentTailLoopCount = (int)(2.5 / (double)0.0005f + 0.5);

CWaveChannel::CWaveChannel() {
	started = false;
	hasSample = false;
	cue = nullptr;
	sampleStatus = kSampleFree;
	sampleAddress = nullptr;
	sampleSize = 0;
	sampleStereo = false;
	sample16Bit = true;
	sampleRate = 0;
	sampleVolume = kMaxSampleVolume;
	samplePan = kCenterPan;
	sampleLoopCount = 1;
	sampleLoopStart = 0;
	sampleLoopEnd = 0xFFFFFFFF;
	silentTail = false;
	samplePosition = 0;
	runStartMs = 0;
	reverbActive = false;
	reverbEnvironment = 0;
	reverbTime = -1.0f;
	reverbMix = -1.0f;
	reverbDamping = -1.0f;

	if (CWaveAudioDevice::digitalDriverOpen) {
		hasSample = true;
		initSample();
	}
}

CWaveChannel::~CWaveChannel() {
	if (hasSample)
		releaseSample();
}

void *CWaveChannel::operator new(size_t size) {
	return gpMemoryServices->malloc(size);
}

void CWaveChannel::operator delete(void *block) {
	gpMemoryServices->free(block);
}

int CWaveChannel::update() {
	int status = getStatus();

	if (status == 2) {
		if (started) {
			if (cue) {
				cue->state = CSoundCue::kStateFinished;
				release();
			}
		}
	}

	return status;
}

bool CWaveChannel::assignToSample(CSoundCue *newCue) {
	if (!hasSample)
		warning("CWaveChannel::assignToSample(): the sample is missing");

	if (cue)
		warning("CWaveChannel::assignToSample(): the channel already has a cue");

	if (!hasSample)
		return false;

	if (!newCue)
		return false;

	CWaveSound *waveSound = newCue->sound;

	if (!waveSound) {
		const char *failedName = "<file_error>";

		if (newCue->soundHandle.resource)
			failedName = newCue->soundHandle.resource->name.c_str();

		warning("CWaveChannel::assignToSample(): can't load sound file <%s>", failedName);

		return false;
	}

	if (waveSound->getType() == CWaveSound::kTypeWave) {
		started = false;
		reverbActive = false;
		initSample();
		setSampleAddress(waveSound->samples, waveSound->allocatedSize);

		int format = 1;

		switch (waveSound->getSampleFormat()) {
		case 1:
			format = 0;

			break;

		case 2:
			format = 2;

			break;

		case 3:
			format = 1;

			break;

		case 4:
			format = 3;

			break;

		default:
			break;
		}

		setSampleType(format);

		double rate = waveSound->sampleRate;

		if (!(rate >= 0.0 && rate <= 2147483647.0)) {
			warning("CWaveChannel::assignToSample(): invalid sample rate");
			rate = 0.0;
		}

		setPlaybackRate((int)rate);
		cue = newCue;
		applyCueSettings(newCue);
	}

	return true;
}

void CWaveChannel::release() {
	if (cue) {
		cue->channel = nullptr;
		cue = nullptr;
	}

	if (hasSample) {
		reverbActive = false;
		initSample();
	}

	started = false;
}

void CWaveChannel::start() {
	if (hasSample) {
		if (cue) {
			startSample();
			cue->state = CSoundCue::kStatePlaying;
			started = true;
		}
	}
}

void CWaveChannel::stop() {
	if (!hasSample)
		warning("CWaveChannel::stop(): the sample is missing");

	if (hasSample) {
		endSample();

		if (cue)
			cue->state = CSoundCue::kStateStopped;

		release();
	}
}

void CWaveChannel::pause() {
	if (hasSample) {
		paused = getSampleStatus() == kSamplePlaying || paused;
		stopSample();

		if (cue)
			cue->state = CSoundCue::kStatePaused;
	}
}

void CWaveChannel::resume() {
	if (hasSample) {
		if (paused) {
			resumeSample();
			paused = false;

			if (cue)
				cue->state = CSoundCue::kStatePlaying;
		}
	}
}

int CWaveChannel::getStatus() {
	if (!hasSample)
		return 0;

	switch (getSampleStatus()) {
	case kSampleFree:
		return 1;

	case kSampleDone:
		return 2;

	case kSamplePlaying:
		return 3;

	case kSamplePlayingReleased:
		return 4;

	case kSampleStopped:
		return 5;

	default:
		break;
	}

	return 0;
}

int CWaveChannel::getPositionMs(int *totalMs) {
	if (!hasSample)
		return -1;

	if (totalMs)
		*totalMs = (int)getSampleLengthMs();

	return (int)getSamplePositionMs();
}

void CWaveChannel::setPositionMs(int milliseconds) {
	if (hasSample)
		setSampleMsPosition(milliseconds);
}

void CWaveChannel::setVolume(float volume) {
	if (!hasSample)
		return;

	float clamped;

	if (volume >= 1.0f)
		clamped = 1.0f;
	else if (!(volume > 0.0f))
		clamped = 0.0f;
	else
		clamped = volume;

	sampleVolume = (int)((double)clamped * 127.0);
	refreshMixerVolume();
}

void CWaveChannel::setPlaybackRate(int rate) {
	if (hasSample)
		sampleRate = rate;
}

void CWaveChannel::setPan(float pan) {
	if (!hasSample)
		return;

	float clamped;

	if (pan >= 1.0f)
		clamped = 1.0f;
	else if (!(pan > -1.0f))
		clamped = -1.0f;
	else
		clamped = pan;

	samplePan = (int)((double)clamped * 64.0 + 64.0);

	if (samplePan > kMaxSampleVolume)
		samplePan = kMaxSampleVolume;

	refreshMixerVolume();
}

bool CWaveChannel::setReverb(int preset, float time, float mix, float unusedFirst, float damping, float unusedSecond) {
	if (!hasSample)
		return false;

	if (!(time >= 0.01f && time <= 8000.0f))
		time = -1.0f;

	if (!(mix >= 0.0f && mix <= 1.0f))
		mix = -1.0f;

	if (!(damping >= 20.0f && damping <= 20000.0f))
		damping = -1.0f;

	if (preset != 0 || !(time < 0.0f) || !(mix < 0.0f) || !(damping < 0.0f)) {
		reverbActive = true;
		reverbEnvironment = preset;

		if (time >= 0.0f)
			reverbTime = time;

		if (mix >= 0.0f)
			reverbMix = mix;

		if (damping >= 0.0f)
			reverbDamping = damping;
	} else {
		reverbActive = false;
	}

	return true;
}

bool CWaveChannel::hasFiniteLoopCount() {
	if (!hasSample)
		return false;

	int count = getSampleLoopCount();

	return count > 1 && count < silentTailLoopCount;
}

void CWaveChannel::setLoopCount(int count) {
	if (!hasSample)
		return;

	if (count < 0) {
		warning("CWaveChannel::setLoopCount(): invalid loop count %d", count);
		count = 1;
	}

	rebaseSample();
	sampleLoopCount = count;
}

void CWaveChannel::setCutoff(float nearDistance, float farDistance) {
}

void CWaveChannel::applyCueSettings(CSoundCue *newCue) {
	if (!newCue || !hasSample)
		return;

	setVolume(newCue->getVolume());
	setPan(newCue->getPan());

	int preset = 0;
	float reverbFirst = 0.0f;
	float reverbSecond = 0.0f;
	float reverbThird = 0.0f;
	float reverbFourth = 0.0f;
	float reverbFifth = 0.0f;

	AudioSystem::getGlobalReverb(&preset, &reverbFirst, &reverbSecond, &reverbThird, &reverbFourth, &reverbFifth);
	setReverb(preset, reverbFirst, reverbSecond, reverbThird, reverbFourth, reverbFifth);
	setCutoff(newCue->getCutoffNear(), newCue->getCutoffFar());

	CWaveSound *waveSound = newCue->sound;

	if ((newCue->flags & CSoundCue::kFlagLoop) != 0) {
		setLoopCount(0);
		silentTail = false;

		if (waveSound)
			setSampleLoopBlock(waveSound->loopStart, waveSound->loopEnd);
	} else {
		setLoopCount(silentTailLoopCount);
		silentTail = true;

		if (waveSound) {
			int frameBytes = 0;
			byte silence = 0;

			switch (waveSound->getSampleFormat()) {
			case 1:
				frameBytes = 1;
				silence = 0x80;

				break;

			case 2:
				frameBytes = 2;
				silence = 0x80;

				break;

			case 3:
				frameBytes = 2;

				break;

			case 4:
				frameBytes = 4;

				break;

			default:
				break;
			}

			if (frameBytes == 0) {
				warning("CWaveChannel::applyCueSettings(): illegal sample format");
				setLoopCount(1);
				silentTail = false;

				return;
			}

			if (!waveSound->samples || waveSound->allocatedSize == 0) {
				warning("CWaveChannel::applyCueSettings(): the sound has no samples");
				setLoopCount(1);
				silentTail = false;

				return;
			}

			double scaledRate = waveSound->sampleRate * (double)0.0005f;
			int lastByte = (int)waveSound->allocatedSize - 1;
			int silentStart = lastByte - (int16)(int)(scaledRate * (double)frameBytes + 0.5) - (int)kSilentTailHeadroom;

			if (silentStart < 0)
				silentStart = 0;

			if (silentStart < lastByte)
				memset(waveSound->samples + silentStart, silence, lastByte - silentStart);

			setSampleLoopBlock(silentStart + kSilentTailHeadroom, lastByte - 1);
		}
	}
}

void CWaveChannel::updateSpatialization() {
	if (cue) {
		if (cue->position.valid)
			cue->updateSpatialization();
	}
}

void CWaveChannel::refreshMixerVolume() {
	Audio::Mixer *mixer = g_system->getMixer();

	if (!mixer || !mixer->isSoundHandleActive(soundHandle))
		return;

	mixer->setChannelVolume(soundHandle, (byte)getMixerVolume());
	mixer->setChannelBalance(soundHandle, (int8)getMixerBalance());
}

bool CWaveChannel::isSampleActive() {
	int status = getSampleStatus();

	return status == kSamplePlaying || status == kSamplePlayingReleased;
}

void CWaveChannel::releaseDigitalOutput() {
	rebaseSample();

	if (sampleStatus != kSamplePlaying)
		return;

	stopMixerStream();

	sampleStatus = kSamplePlayingReleased;
}

void CWaveChannel::reacquireDigitalOutput() {
	if (sampleStatus != kSamplePlayingReleased)
		return;

	sampleStatus = kSamplePlaying;
	runStartMs = g_system->getMillis();

	stopMixerStream();
	playMixerStream();
}

void CWaveChannel::initSample() {
	stopMixerStream();

	sampleStatus = kSampleDone;
	sampleAddress = nullptr;
	sampleSize = 0;
	sampleStereo = false;
	sample16Bit = false;
	sampleRate = kDefaultSampleRate;
	sampleVolume = kMaxSampleVolume;
	samplePan = kCenterPan;
	sampleLoopCount = 1;
	sampleLoopStart = 0;
	sampleLoopEnd = 0xFFFFFFFF;
	silentTail = false;
	samplePosition = 0;
	runStartMs = 0;
}

void CWaveChannel::releaseSample() {
	stopMixerStream();

	sampleStatus = kSampleFree;
	hasSample = false;
}

void CWaveChannel::setSampleAddress(const byte *address, uint size) {
	sampleAddress = address;
	sampleSize = size;
}

void CWaveChannel::setSampleType(int format) {
	sampleStereo = (format & 2) != 0;
	sample16Bit = (format & 1) != 0;
}

void CWaveChannel::setSampleLoopBlock(uint start, uint end) {
	rebaseSample();

	// The disasm rounds the start to the nearest frame
	uint frameSize = getFrameSize();
	uint64 rounded = ((uint64)start + frameSize / 2) / frameSize * frameSize;

	if (rounded > 0xFFFFFFFF)
		rounded = 0xFFFFFFFF;

	sampleLoopStart = (uint)rounded;
	sampleLoopEnd = end;
}

void CWaveChannel::startSample() {
	if (sampleStatus == kSampleFree)
		return;

	if (!sampleAddress || sampleSize == 0)
		return;

	rebaseSample();

	stopMixerStream();

	samplePosition = 0;

	if (CWaveAudioDevice::handleReleased) {
		sampleStatus = kSamplePlayingReleased;

		return;
	}

	sampleStatus = kSamplePlaying;
	runStartMs = g_system->getMillis();

	playMixerStream();
}

void CWaveChannel::stopSample() {
	if (getSampleStatus() != kSamplePlaying)
		return;

	rebaseSample();

	if (sampleStatus != kSamplePlaying)
		return;

	sampleStatus = kSampleStopped;

	pauseMixerStream(true);
}

void CWaveChannel::resumeSample() {
	if (sampleStatus == kSampleStopped) {
		if (CWaveAudioDevice::handleReleased) {
			sampleStatus = kSamplePlayingReleased;
		} else {
			sampleStatus = kSamplePlaying;
			runStartMs = g_system->getMillis();

			pauseMixerStream(false);
		}
	} else if (sampleStatus == kSampleDone) {
		if (!sampleAddress || sampleSize == 0)
			return;

		if (CWaveAudioDevice::handleReleased) {
			sampleStatus = kSamplePlayingReleased;
		} else {
			sampleStatus = kSamplePlaying;
			runStartMs = g_system->getMillis();

			playMixerStream();
		}
	}
}

void CWaveChannel::endSample() {
	if (sampleStatus == kSampleFree || sampleStatus == kSampleDone)
		return;

	rebaseSample();
	stopMixerStream();

	sampleStatus = kSampleDone;
}

int CWaveChannel::getSampleStatus() {
	if (sampleStatus == kSamplePlaying) {
		uint position = 0;
		int loopCount = 0;
		bool finished = false;

		getSampleState(getElapsedBytes(), &position, &loopCount, &finished);

		if (finished)
			finishSample();
	}

	return sampleStatus;
}

int CWaveChannel::getSampleLoopCount() {
	if (getSampleStatus() != kSamplePlaying)
		return sampleLoopCount;

	uint position = 0;
	int loopCount = 0;
	bool finished = false;

	getSampleState(getElapsedBytes(), &position, &loopCount, &finished);

	return loopCount;
}

void CWaveChannel::setSampleMsPosition(int milliseconds) {
	if (milliseconds < 0)
		milliseconds = 0;

	uint bytes = msToBytes((uint)milliseconds);

	if (bytes > sampleSize) {
		warning("CWaveChannel::setSampleMsPosition(): position %d ms is past the end of the sample", milliseconds);
		bytes = sampleSize;
	}

	rebaseSample();

	if (sampleStatus == kSamplePlaying || sampleStatus == kSampleStopped) {
		stopMixerStream();

		samplePosition = bytes;

		if (sampleStatus == kSamplePlaying)
			runStartMs = g_system->getMillis();

		playMixerStream();

		if (sampleStatus == kSampleStopped)
			pauseMixerStream(true);
	} else {
		samplePosition = bytes;
	}
}

uint CWaveChannel::getSamplePositionMs() {
	uint bytes = samplePosition;

	if (getSampleStatus() == kSamplePlaying) {
		int loopCount = 0;
		bool finished = false;

		getSampleState(getElapsedBytes(), &bytes, &loopCount, &finished);
	}

	if (bytes > sampleSize)
		bytes = sampleSize;

	return bytesToMs(bytes);
}

uint CWaveChannel::getSampleLengthMs() {
	return bytesToMs(sampleSize);
}

void CWaveChannel::getSampleState(uint64 elapsedBytes, uint *position, int *loopCount, bool *finished) {
	uint blockStart = 0;
	uint blockEnd = 0;

	getLoopBlock(&blockStart, &blockEnd);

	*finished = false;

	if (blockEnd <= blockStart) {
		*position = sampleSize;
		*loopCount = 1;
		*finished = true;

		return;
	}

	// The disasm ignores the loop end when the count is one
	uint segmentEnd = sampleLoopCount != 1 ? blockEnd : sampleSize;
	uint64 firstSegment = 0;

	if (samplePosition < segmentEnd)
		firstSegment = segmentEnd - samplePosition;

	if (elapsedBytes < firstSegment) {
		*position = samplePosition + (uint)elapsedBytes;
		*loopCount = sampleLoopCount;

		return;
	}

	if (sampleLoopCount == 1) {
		*position = sampleSize;
		*loopCount = 1;
		*finished = true;

		return;
	}

	uint64 elapsed = elapsedBytes - firstSegment;
	uint64 cycle = blockEnd - blockStart;

	if (sampleLoopCount == 0) {
		*position = blockStart + (uint)(elapsed % cycle);
		*loopCount = 0;

		return;
	}

	uint64 middle = (uint64)(sampleLoopCount - 2) * cycle;

	if (elapsed < middle) {
		*position = blockStart + (uint)(elapsed % cycle);
		*loopCount = sampleLoopCount - 1 - (int)(elapsed / cycle);

		return;
	}

	uint64 lastPosition = blockStart + (elapsed - middle);

	*loopCount = 1;

	if (lastPosition >= sampleSize) {
		*position = sampleSize;
		*finished = true;
	} else {
		*position = (uint)lastPosition;
	}
}

void CWaveChannel::rebaseSample() {
	if (sampleStatus != kSamplePlaying)
		return;

	uint position = 0;
	int loopCount = 0;
	bool finished = false;

	getSampleState(getElapsedBytes(), &position, &loopCount, &finished);

	if (finished) {
		finishSample();

		return;
	}

	samplePosition = position;
	sampleLoopCount = loopCount;
	runStartMs = g_system->getMillis();
}

void CWaveChannel::finishSample() {
	stopMixerStream();

	sampleStatus = kSampleDone;
	samplePosition = sampleSize;
	sampleLoopCount = 1;
}

uint64 CWaveChannel::getElapsedBytes() {
	if (sampleStatus != kSamplePlaying || sampleRate <= 0)
		return 0;

	uint elapsedMs = g_system->getMillis() - runStartMs;
	uint64 frames = (uint64)elapsedMs * (uint)sampleRate / 1000;

	return frames * getFrameSize();
}

int CWaveChannel::getPanScale(int index) {
	if (index <= 62)
		return index * 2;

	return kPanScaleLimit;
}

int CWaveChannel::panToBalance(int pan) {
	if (pan < 0)
		pan = 0;

	if (pan > kMaxSampleVolume)
		pan = kMaxSampleVolume;

	int left = getPanScale(kMaxSampleVolume - pan);
	int right = getPanScale(pan);

	if (left > right)
		return -((127 * (left - right) + left / 2) / left);

	if (right > left)
		return (127 * (right - left) + right / 2) / right;

	return 0;
}

int CWaveChannel::volumeToMixer(int volume) {
	if (volume < 0)
		volume = 0;

	if (volume > kMaxSampleVolume)
		volume = kMaxSampleVolume;

	int effective = volume * CWaveAudioDevice::masterVolume / kMaxSampleVolume;

	if (effective < 0)
		effective = 0;

	if (effective > kMaxSampleVolume)
		effective = kMaxSampleVolume;

	int scale = 16 * effective;

	if (scale >= 2032)
		scale = 2047;

	return (scale * Audio::Mixer::kMaxChannelVolume + 1024) / 2048;
}

int CWaveChannel::getMixerVolume() {
	return volumeToMixer(sampleVolume);
}

int CWaveChannel::getMixerBalance() {
	return panToBalance(samplePan);
}

uint CWaveChannel::getFrameSize() {
	uint frameSize = sample16Bit ? 2 : 1;

	if (sampleStereo)
		frameSize *= 2;

	return frameSize;
}

uint CWaveChannel::getBytesPerSecond() {
	if (sampleRate <= 0)
		return 0;

	return (uint)sampleRate * getFrameSize();
}

uint CWaveChannel::msToBytes(uint milliseconds) {
	if (sampleRate <= 0)
		return 0;

	uint frameSize = getFrameSize();
	uint64 bytes = (uint64)((double)((uint)sampleRate * frameSize) * (double)milliseconds * 0.001);

	bytes = (bytes + frameSize / 2) / frameSize * frameSize;

	if (bytes > 0xFFFFFFFF)
		return 0xFFFFFFFF;

	return (uint)bytes;
}

uint CWaveChannel::bytesToMs(uint bytes) {
	uint bytesPerSecond = getBytesPerSecond();

	if (bytesPerSecond == 0)
		return 0;

	return (uint)((uint64)bytes * 1000 / bytesPerSecond);
}

void CWaveChannel::getLoopBlock(uint *start, uint *end) {
	uint frameSize = getFrameSize();
	uint blockStart = sampleLoopStart;
	uint blockEnd = sampleLoopEnd;

	if (blockEnd >= sampleSize)
		blockEnd = sampleSize;
	else
		blockEnd -= blockEnd % frameSize;

	if (blockStart >= sampleSize || blockEnd <= blockStart) {
		blockStart = 0;
		blockEnd = sampleSize;
	}

	*start = blockStart;
	*end = blockEnd;
}

void CWaveChannel::playMixerStream() {
	Audio::Mixer *mixer = g_system->getMixer();

	if (!mixer || !sampleAddress || sampleSize == 0)
		return;

	if (sampleRate <= 0) {
		warning("CWaveChannel::playMixerStream(): invalid playback rate %d", sampleRate);

		return;
	}

	uint offset = samplePosition;

	if (offset >= sampleSize)
		return;

	byte rawFlags = 0;

	if (sample16Bit)
		rawFlags |= Audio::FLAG_16BITS | Audio::FLAG_LITTLE_ENDIAN;
	else
		rawFlags |= Audio::FLAG_UNSIGNED;

	if (sampleStereo)
		rawFlags |= Audio::FLAG_STEREO;

	Audio::AudioStream *stream = nullptr;

	if (silentTail || sampleLoopCount == 1)
		stream = Audio::makeRawStream(sampleAddress + offset, sampleSize - offset, sampleRate, rawFlags, DisposeAfterUse::NO);
	else
		stream = makeLoopingStream(offset, rawFlags);

	if (!stream)
		return;

	Audio::Mixer::SoundType type = Audio::Mixer::kSFXSoundType;

	if (cue) {
		if (cue->category == 2)
			type = Audio::Mixer::kSpeechSoundType;
		else if (cue->category == 3)
			type = Audio::Mixer::kMusicSoundType;
	}

	mixer->playStream(type, &soundHandle, stream, -1, (byte)getMixerVolume(), (int8)getMixerBalance(), DisposeAfterUse::YES);
}

void CWaveChannel::stopMixerStream() {
	Audio::Mixer *mixer = g_system->getMixer();

	if (mixer)
		mixer->stopHandle(soundHandle);

	soundHandle = Audio::SoundHandle();
}

void CWaveChannel::pauseMixerStream(bool pauseIt) {
	Audio::Mixer *mixer = g_system->getMixer();

	if (mixer)
		mixer->pauseHandle(soundHandle, pauseIt);
}

Audio::AudioStream *CWaveChannel::makeLoopingStream(uint offset, byte rawFlags) {
	uint blockStart = 0;
	uint blockEnd = 0;

	getLoopBlock(&blockStart, &blockEnd);

	Audio::QueuingAudioStream *queue = Audio::makeQueuingAudioStream(sampleRate, sampleStereo);

	if (offset < blockEnd) {
		Audio::SeekableAudioStream *first = Audio::makeRawStream(sampleAddress + offset, blockEnd - offset, sampleRate, rawFlags, DisposeAfterUse::NO);

		queue->queueAudioStream(first, DisposeAfterUse::YES);
	}

	if (sampleLoopCount == 0) {
		Audio::SeekableAudioStream *block = Audio::makeRawStream(sampleAddress + blockStart, blockEnd - blockStart, sampleRate, rawFlags, DisposeAfterUse::NO);
		Audio::AudioStream *forever = Audio::makeLoopingAudioStream(block, 0);

		queue->queueAudioStream(forever, DisposeAfterUse::YES);
	} else {
		uint repeats = (uint)sampleLoopCount - 2;

		if (repeats > 0) {
			Audio::SeekableAudioStream *block = Audio::makeRawStream(sampleAddress + blockStart, blockEnd - blockStart, sampleRate, rawFlags, DisposeAfterUse::NO);
			Audio::AudioStream *looped = Audio::makeLoopingAudioStream(block, repeats);

			queue->queueAudioStream(looped, DisposeAfterUse::YES);
		}

		Audio::SeekableAudioStream *last = Audio::makeRawStream(sampleAddress + blockStart, sampleSize - blockStart, sampleRate, rawFlags, DisposeAfterUse::NO);

		queue->queueAudioStream(last, DisposeAfterUse::YES);
	}

	queue->finish();

	return queue;
}

} // End of namespace Grim
