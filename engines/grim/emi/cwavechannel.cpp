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

#include "common/textconsole.h"

#include "engines/grim/emi/audiosystem.h"
#include "engines/grim/emi/csoundcue.h"
#include "engines/grim/emi/cwavechannel.h"
#include "engines/grim/emi/cwavesound.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/miles_audio/driver.h"

namespace Grim {

static const int kMaxSampleVolume = 127;
static const uint kSilentTailHeadroom = 4;
static const int kSilentTailLoopCount = (int)(2.5 / (double)0.0005f + 0.5);

CWaveChannel::CWaveChannel(EMIMilesDriver *driver) {
	started = false;
	hSample = nullptr;
	cue = nullptr;
	_driver = driver;

	if (_driver) {
		hSample = _driver->allocateSample();

		if (hSample)
			_driver->initSample(hSample);
	}
}

CWaveChannel::~CWaveChannel() {
	if (hSample) {
		_driver->releaseSample(hSample);
		hSample = nullptr;
	}
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
	if (!hSample)
		warning("CWaveChannel::assignToSample(): the sample is missing");

	if (cue)
		warning("CWaveChannel::assignToSample(): the channel already has a cue");

	if (!hSample)
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
		_driver->setSampleFilter(hSample, false);
		_driver->initSample(hSample);
		_driver->setSampleAddress(hSample, waveSound->samples, waveSound->allocatedSize);

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

		_driver->setSampleType(hSample, format);

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

	if (hSample) {
		_driver->setSampleFilter(hSample, false);
		_driver->initSample(hSample);
	}

	started = false;
}

void CWaveChannel::start() {
	if (hSample) {
		if (cue) {
			_driver->startSample(hSample);
			cue->state = CSoundCue::kStatePlaying;
			started = true;
		}
	}
}

void CWaveChannel::stop() {
	if (!hSample)
		warning("CWaveChannel::stop(): the sample is missing");

	if (hSample) {
		_driver->endSample(hSample);

		if (cue)
			cue->state = CSoundCue::kStateStopped;

		release();
	}
}

void CWaveChannel::pause() {
	if (hSample) {
		paused = _driver->getSampleStatus(hSample) == EMIMilesDriver::kSamplePlaying || paused;
		_driver->stopSample(hSample);

		if (cue)
			cue->state = CSoundCue::kStatePaused;
	}
}

void CWaveChannel::resume() {
	if (hSample) {
		if (paused) {
			_driver->resumeSample(hSample);
			paused = false;

			if (cue)
				cue->state = CSoundCue::kStatePlaying;
		}
	}
}

int CWaveChannel::getStatus() {
	if (!hSample)
		return 0;

	switch (_driver->getSampleStatus(hSample)) {
	case EMIMilesDriver::kSampleFree:
		return 1;

	case EMIMilesDriver::kSampleDone:
		return 2;

	case EMIMilesDriver::kSamplePlaying:
		return 3;

	case EMIMilesDriver::kSamplePlayingReleased:
		return 4;

	case EMIMilesDriver::kSampleStopped:
		return 5;

	default:
		break;
	}

	return 0;
}

int CWaveChannel::getPositionMs(int *totalMs) {
	if (!hSample)
		return -1;

	int currentMs = 0;

	_driver->getSampleMsPosition(hSample, totalMs, &currentMs);

	return currentMs;
}

void CWaveChannel::setPositionMs(int milliseconds) {
	if (hSample)
		_driver->setSampleMsPosition(hSample, milliseconds);
}

void CWaveChannel::setVolume(float volume) {
	if (!hSample)
		return;

	float clamped;

	if (volume >= 1.0f)
		clamped = 1.0f;
	else if (!(volume > 0.0f))
		clamped = 0.0f;
	else
		clamped = volume;

	_driver->setSampleVolume(hSample, (int)((double)clamped * (double)kMaxSampleVolume));
}

void CWaveChannel::setPlaybackRate(int rate) {
	if (hSample)
		_driver->setSampleRate(hSample, rate);
}

void CWaveChannel::setPan(float pan) {
	if (!hSample)
		return;

	float clamped;

	if (pan >= 1.0f)
		clamped = 1.0f;
	else if (!(pan > -1.0f))
		clamped = -1.0f;
	else
		clamped = pan;

	_driver->setSamplePan(hSample, (int)((double)clamped * 64.0 + 64.0));
}

bool CWaveChannel::setReverb(int preset, float time, float mix, float unusedFirst, float damping, float unusedSecond) {
	if (!hSample)
		return false;

	if (!(time >= 0.01f && time <= 8000.0f))
		time = -1.0f;

	if (!(mix >= 0.0f && mix <= 1.0f))
		mix = -1.0f;

	if (!(damping >= 20.0f && damping <= 20000.0f))
		damping = -1.0f;

	if (preset != 0 || !(time < 0.0f) || !(mix < 0.0f) || !(damping < 0.0f)) {
		_driver->setSampleFilter(hSample, true);
		_driver->setSampleFilterEnvironment(hSample, (float)preset);

		if (time >= 0.0f)
			_driver->setSampleFilterTime(hSample, time);

		if (mix >= 0.0f)
			_driver->setSampleFilterMix(hSample, mix);

		if (damping >= 0.0f)
			_driver->setSampleFilterDamping(hSample, damping);
	} else {
		_driver->setSampleFilter(hSample, false);
	}

	return true;
}

bool CWaveChannel::hasFiniteLoopCount() {
	if (!hSample)
		return false;

	int count = _driver->getSampleLoopCount(hSample);

	return count > 1 && count < kSilentTailLoopCount;
}

void CWaveChannel::setLoopCount(int count) {
	if (hSample)
		_driver->setSampleLoopCount(hSample, count);
}

void CWaveChannel::setCutoff(float nearDistance, float farDistance) {
}

void CWaveChannel::applyCueSettings(CSoundCue *newCue) {
	if (!newCue || !hSample)
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

		if (waveSound)
			_driver->setSampleLoopBlock(hSample, (int)waveSound->loopStart, (int)waveSound->loopEnd);
	} else {
		setLoopCount(kSilentTailLoopCount);

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

				return;
			}

			if (!waveSound->samples || waveSound->allocatedSize == 0) {
				warning("CWaveChannel::applyCueSettings(): the sound has no samples");
				setLoopCount(1);

				return;
			}

			double scaledRate = waveSound->sampleRate * (double)0.0005f;
			int lastByte = (int)waveSound->allocatedSize - 1;
			int silentStart = lastByte - (int16)(int)(scaledRate * (double)frameBytes + 0.5) - (int)kSilentTailHeadroom;

			if (silentStart < 0)
				silentStart = 0;

			if (silentStart < lastByte)
				memset(waveSound->samples + silentStart, silence, lastByte - silentStart);

			_driver->setSampleLoopBlock(hSample, silentStart + (int)kSilentTailHeadroom, lastByte - 1);
		}
	}
}

void CWaveChannel::updateSpatialization() {
	if (cue) {
		if (cue->position.valid)
			cue->updateSpatialization();
	}
}

} // End of namespace Grim
