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

#include "engines/grim/emi/sound/audiosystem.h"
#include "engines/grim/emi/sound/csoundcue.h"
#include "engines/grim/emi/sound/cwaveaudiodevice.h"
#include "engines/grim/emi/sound/cwavechannel.h"
#include "engines/grim/emi/sound/cwavesound.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/sound/miles_audio/driver.h"

namespace Grim {

static const int kMaxSampleVolume = 127;
static const uint kSilentTailHeadroom = 4;
static const int kSilentTailLoopCount = (int)(2.5 / (double)0.0005f + 0.5);
static const uint kMaxAilValue = 0x80;
static const float kSpuRegisterScale = 16383.0f;
static const float kSpuVoiceStreamGain = 0.55f;

CWaveChannel::CWaveChannel(CWaveAudioDevice *device) : CAudioChannel(device->getEngine()), _device(device), _driver(device->getMilesDriver()) {
	_started = false;
	_sample = nullptr;
	_cue = nullptr;
	_spuVolume = 0.0f;
	_spuBalance = 0.0f;

	if (_driver) {
		_sample = _driver->allocateSample();

		if (_sample)
			_driver->initSample(_sample);
	}
}

CWaveChannel::~CWaveChannel() {
	if (_sample) {
		_driver->releaseSample(_sample);
		_sample = nullptr;
	}
}

void *CWaveChannel::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void CWaveChannel::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

int CWaveChannel::update() {
	int status = getStatus();

	if (status == 2) {
		if (_started) {
			if (_cue) {
				_cue->_state = CSoundCue::kStateFinished;
				release();
			}
		}
	}

	return status;
}

bool CWaveChannel::assignToSample(CSoundCue *newCue) {
	if (!_sample)
		warning("CWaveChannel::assignToSample(): the sample is missing");

	if (_cue)
		warning("CWaveChannel::assignToSample(): the channel already has a cue");

	if (!_sample)
		return false;

	if (!newCue)
		return false;

	CWaveSound *waveSound = newCue->_sound;

	if (!waveSound) {
		const char *failedName = "<file_error>";

		if (newCue->_soundHandle._resource)
			failedName = newCue->_soundHandle._resource->_name.c_str();

		warning("CWaveChannel::assignToSample(): can't load sound file <%s>", failedName);

		return false;
	}

	if (waveSound->getType() == CWaveSound::kTypeWave) {
		_started = false;
		_driver->setSampleFilter(_sample, false);
		_driver->initSample(_sample);
		_driver->setSampleAddress(_sample, waveSound->_samples, waveSound->_allocatedSize);

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

		_driver->setSampleType(_sample, format);

		double rate = waveSound->_sampleRate;

		if (!(rate >= 0.0 && rate <= 2147483647.0)) {
			warning("CWaveChannel::assignToSample(): invalid sample rate");
			rate = 0.0;
		}

		setPlaybackRate((int)rate);
		_cue = newCue;
		applyCueSettings(newCue);
	}

	return true;
}

void CWaveChannel::release() {
	if (_cue) {
		_cue->_channel = nullptr;
		_cue = nullptr;
	}

	if (_sample) {
		_driver->setSampleFilter(_sample, false);
		_driver->initSample(_sample);
	}

	_started = false;
}

void CWaveChannel::start() {
	if (_sample) {
		if (_cue) {
			_driver->startSample(_sample);
			_cue->_state = CSoundCue::kStatePlaying;
			_started = true;
		}
	}
}

void CWaveChannel::stop() {
	if (!_sample)
		warning("CWaveChannel::stop(): the sample is missing");

	if (_sample) {
		_driver->endSample(_sample);

		if (_cue)
			_cue->_state = CSoundCue::kStateStopped;

		release();
	}
}

void CWaveChannel::pause() {
	if (_sample) {
		_paused = _driver->getSampleStatus(_sample) == EMIMilesDriver::kSamplePlaying || _paused;
		_driver->stopSample(_sample);

		if (_cue)
			_cue->_state = CSoundCue::kStatePaused;
	}
}

void CWaveChannel::resume() {
	if (_sample) {
		if (_paused) {
			_driver->resumeSample(_sample);
			_paused = false;

			if (_cue)
				_cue->_state = CSoundCue::kStatePlaying;
		}
	}
}

int CWaveChannel::getStatus() {
	if (!_sample)
		return 0;

	switch (_driver->getSampleStatus(_sample)) {
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
	if (!_sample)
		return -1;

	int currentMs = 0;

	_driver->getSampleMsPosition(_sample, totalMs, &currentMs);

	return currentMs;
}

void CWaveChannel::setPositionMs(int milliseconds) {
	if (_sample)
		_driver->setSampleMsPosition(_sample, milliseconds);
}

void CWaveChannel::setVolume(float volume) {
	if (!_sample)
		return;

	if (_engine->getGamePlatform() == Common::kPlatformPS2) {
		if (!_cue)
			return;

		float limited = volume < 1.0f ? volume : 1.0f;

		if (limited <= 0.0f)
			limited = 0.0f;

		int ailVolume = (int)(limited * 127.0f);

		if (_cue->_category == 1)
			setSpuSampleVolume(ailVolume);
		else
			setSpuStreamVolume(ailVolume);

		return;
	}

	float clamped;

	if (volume >= 1.0f)
		clamped = 1.0f;
	else if (!(volume > 0.0f))
		clamped = 0.0f;
	else
		clamped = volume;

	_driver->setSampleVolume(_sample, (int)((double)clamped * (double)kMaxSampleVolume));
}

void CWaveChannel::setPlaybackRate(int rate) {
	if (_sample)
		_driver->setSampleRate(_sample, rate);
}

void CWaveChannel::setPan(float pan) {
	if (!_sample)
		return;

	if (_engine->getGamePlatform() == Common::kPlatformPS2) {
		if (!_cue)
			return;

		float limited = pan < 1.0f ? pan : 1.0f;

		if (limited <= -1.0f)
			limited = -1.0f;

		float ailPan;

		if (limited < 0.0f)
			ailPan = 64.0f + 64.0f * limited;
		else
			ailPan = 63.0f + 64.0f * limited;

		// A voice stream ignores the pan
		if (_cue->_category == 1)
			setSpuSamplePan((int)ailPan);

		return;
	}

	float clamped;

	if (pan >= 1.0f)
		clamped = 1.0f;
	else if (!(pan > -1.0f))
		clamped = -1.0f;
	else
		clamped = pan;

	_driver->setSamplePan(_sample, (int)((double)clamped * 64.0 + 64.0));
}

bool CWaveChannel::setReverb(int preset, float time, float mix, float unusedFirst, float damping, float unusedSecond) {
	if (!_sample)
		return false;

	if (!(time >= 0.01f && time <= 8000.0f))
		time = -1.0f;

	if (!(mix >= 0.0f && mix <= 1.0f))
		mix = -1.0f;

	if (!(damping >= 20.0f && damping <= 20000.0f))
		damping = -1.0f;

	if (preset != 0 || !(time < 0.0f) || !(mix < 0.0f) || !(damping < 0.0f)) {
		_driver->setSampleFilter(_sample, true);
		_driver->setSampleFilterEnvironment(_sample, (float)preset);

		if (time >= 0.0f)
			_driver->setSampleFilterTime(_sample, time);

		if (mix >= 0.0f)
			_driver->setSampleFilterMix(_sample, mix);

		if (damping >= 0.0f)
			_driver->setSampleFilterDamping(_sample, damping);
	} else {
		_driver->setSampleFilter(_sample, false);
	}

	return true;
}

bool CWaveChannel::hasFiniteLoopCount() {
	if (!_sample)
		return false;

	int count = _driver->getSampleLoopCount(_sample);

	return count > 1 && count < kSilentTailLoopCount;
}

void CWaveChannel::setLoopCount(int count) {
	if (_sample)
		_driver->setSampleLoopCount(_sample, count);
}

void CWaveChannel::setCutoff(float nearDistance, float farDistance) {
}

void CWaveChannel::applyCueSettings(CSoundCue *newCue) {
	if (!newCue || !_sample)
		return;

	// PS2 only sets the loop flag; the SPU jumps to the loop address given by the ADPCM flags
	if (_engine->getGamePlatform() == Common::kPlatformPS2) {
		if ((newCue->_flags & CSoundCue::kFlagLoop) != 0) {
			setLoopCount(0);

			if (newCue->_sound)
				_driver->setSampleLoopBlock(_sample, (int)newCue->_sound->_spuLoopStart, -1);
		}

		return;
	}

	setVolume(newCue->getVolume());
	setPan(newCue->getPan());

	// The demo has no reverb, cutoff or silent tail: a looping cue gets its loop block, any other plays once
	if (_engine->isDemo()) {
		if ((newCue->_flags & CSoundCue::kFlagLoop) != 0) {
			setLoopCount(0);

			if (newCue->_sound)
				_driver->setSampleLoopBlock(_sample, (int)newCue->_sound->_loopStart, (int)newCue->_sound->_loopEnd);
		}

		return;
	}

	int preset = 0;
	float reverbFirst = 0.0f;
	float reverbSecond = 0.0f;
	float reverbThird = 0.0f;
	float reverbFourth = 0.0f;
	float reverbFifth = 0.0f;

	_device->getAudioSystem()->getGlobalReverb(&preset, &reverbFirst, &reverbSecond, &reverbThird, &reverbFourth, &reverbFifth);
	setReverb(preset, reverbFirst, reverbSecond, reverbThird, reverbFourth, reverbFifth);
	setCutoff(newCue->getCutoffNear(), newCue->getCutoffFar());

	CWaveSound *waveSound = newCue->_sound;

	if ((newCue->_flags & CSoundCue::kFlagLoop) != 0) {
		setLoopCount(0);

		if (waveSound)
			_driver->setSampleLoopBlock(_sample, (int)waveSound->_loopStart, (int)waveSound->_loopEnd);
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

			if (!waveSound->_samples || waveSound->_allocatedSize == 0) {
				warning("CWaveChannel::applyCueSettings(): the sound has no samples");
				setLoopCount(1);

				return;
			}

			double scaledRate = waveSound->_sampleRate * (double)0.0005f;
			int lastByte = (int)waveSound->_allocatedSize - 1;
			int silentStart = lastByte - (int16)(int)(scaledRate * (double)frameBytes + 0.5) - (int)kSilentTailHeadroom;

			if (silentStart < 0)
				silentStart = 0;

			if (silentStart < lastByte)
				memset(waveSound->_samples + silentStart, silence, lastByte - silentStart);

			_driver->setSampleLoopBlock(_sample, silentStart + (int)kSilentTailHeadroom, lastByte - 1);
		}
	}
}

void CWaveChannel::updateSpatialization() {
	if (_cue) {
		if (_cue->_position._valid)
			_cue->updateSpatialization();
	}
}

void CWaveChannel::setSpuSampleVolume(int volume) {
	if ((uint)volume >= kMaxAilValue) {
		warning("CWaveChannel::setSpuSampleVolume(): volume %d out of range", volume);

		return;
	}

	int left = 0;
	int right = 0;

	_spuVolume = (float)volume / 127.0f;

	if (getAttenuatedVolumes(_spuVolume, _spuBalance, &left, &right))
		_driver->setSampleSpuVolume(_sample, left, right);
}

void CWaveChannel::setSpuSamplePan(int pan) {
	if ((uint)pan >= kMaxAilValue) {
		warning("CWaveChannel::setSpuSamplePan(): pan %d out of range", pan);

		return;
	}

	int left = 0;
	int right = 0;

	_spuBalance = ((float)pan - 64.0f) / 64.0f;

	// In mono output every pan is centered
	if (!_device->getSpuStereo())
		_spuBalance = 0.0f;

	if (getAttenuatedVolumes(_spuVolume, _spuBalance, &left, &right))
		_driver->setSampleSpuVolume(_sample, left, right);
}

void CWaveChannel::setSpuStreamVolume(int volume) {
	if ((uint)volume >= kMaxAilValue) {
		warning("CWaveChannel::setSpuStreamVolume(): volume %d out of range", volume);

		return;
	}

	float scaled = kSpuRegisterScale * (kSpuVoiceStreamGain * ((float)volume / 127.0f));
	int level = scaled > 0.0f ? (int)scaled : 0;

	_driver->setSampleSpuVolume(_sample, level, level);
}

void CWaveChannel::endSpuSample() {
	// On PS2, voices are IOP streams, not SPU samples
	if (!_sample || !_started || (_cue && _cue->_category == 2))
		return;

	_driver->endSample(_sample);
}

bool CWaveChannel::getAttenuatedVolumes(float volume, float balance, int *left, int *right) {
	if (!(balance <= 1.0f && balance >= -1.0f)) {
		warning("CWaveChannel::getAttenuatedVolumes(): balance %f out of range", balance);

		return false;
	}

	if (!(volume <= 1.0f && volume >= 0.0f)) {
		warning("CWaveChannel::getAttenuatedVolumes(): volume %f out of range", volume);

		return false;
	}

	float leftFactor = 1.0f;
	float rightFactor = 1.0f;

	if (balance < 0.0f)
		rightFactor = 1.0f + balance;
	else if (balance > 0.0f)
		leftFactor = 1.0f - balance;

	float scaledLeft = kSpuRegisterScale * (leftFactor * volume);
	float scaledRight = kSpuRegisterScale * (rightFactor * volume);

	*left = scaledLeft > 0.0f ? (int)scaledLeft : 0;
	*right = scaledRight > 0.0f ? (int)scaledRight : 0;

	return true;
}

} // End of namespace Grim
