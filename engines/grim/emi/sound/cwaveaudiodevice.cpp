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

#include "engines/grim/emi/sound/audiosystem.h"
#include "engines/grim/emi/sound/csoundcue.h"
#include "engines/grim/emi/sound/cwaveaudiodevice.h"
#include "engines/grim/emi/sound/cwavechannel.h"
#include "engines/grim/emi/sound/cwavesound.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lconsoleservices.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/sound/miles_audio/driver.h"
#include "engines/grim/emi/sound/sounds.h"

namespace Grim {

static const int kMaxSampleVolume = 127;
static const uint kReacquireIntervalMs = 5000;
static const int kSonyRoomTypeCount = 10;
static const int kSonyMaxValue = 100;

CWaveAudioDevice::CWaveAudioDevice(Sounds *sounds, AudioSystem *audioSystem, int channelCount) : CAudioDevice(sounds->getEngine()), _audioSystem(audioSystem), _spuStereo(true), _driver(nullptr) {
	lost = false;
	numChannels = 0;
	lastCheckMs = 0;

	EMIMilesDriver *driver = new EMIMilesDriver(_engine->getMemoryServices(), _engine->getGamePlatform() == Common::kPlatformPS2);

	if (!driver->open(g_system->getMixer())) {
		delete driver;

		return;
	}

	_driver = driver;

	int created = 0;

	for (int i = 0; i < channelCount; i++) {
		CWaveChannel *channel = new CWaveChannel(this);

		if (!channel->hSample) {
			delete channel;

			break;
		}

		channels.push_back(channel);
		created++;
	}

	numChannels = created;
}

CWaveAudioDevice::~CWaveAudioDevice() {
	while (!channels.empty()) {
		CWaveChannel *channel = channels.back();

		channels.pop_back();

		if (channel)
			delete channel;
	}

	if (_driver) {
		delete _driver;
		_driver = nullptr;
	}
}

void *CWaveAudioDevice::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void CWaveAudioDevice::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

int CWaveAudioDevice::getType() {
	return CWaveSound::kTypeWave;
}

void CWaveAudioDevice::releaseDigitalHandle() {
	if (_driver)
		_driver->releaseHandle();
}

void CWaveAudioDevice::reacquireDigitalHandle() {
	if (_driver) {
		_driver->reacquireHandle();
		lost = false;
	}
}

void CWaveAudioDevice::pauseAll() {
	for (uint i = 0; i < channels.size(); i++)
		channels[i]->pause();
}

void CWaveAudioDevice::resumeAll() {
	for (uint i = 0; i < channels.size(); i++)
		channels[i]->resume();
}

void CWaveAudioDevice::stopAll() {
	for (uint i = 0; i < channels.size(); i++)
		channels[i]->stop();
}

void CWaveAudioDevice::update() {
	if (lost) {
		int now = (int)g_system->getMillis();

		if ((uint)(now - lastCheckMs) > kReacquireIntervalMs) {
			reacquireDigitalHandle();
			lastCheckMs = now;
		}
	}

	for (uint i = 0; i < channels.size(); i++) {
		if (channels[i])
			channels[i]->update();
	}
}

void CWaveAudioDevice::updateChannelsSpatialization() {
	for (uint i = 0; i < channels.size(); i++) {
		if (channels[i])
			channels[i]->updateSpatialization();
	}
}

CAudioChannel *CWaveAudioDevice::allocateChannel(CSoundCue *cue) {
	CWaveChannel *freeChannel = nullptr;
	CWaveChannel *lowerChannel = nullptr;

	if (!cue)
		return nullptr;

	if (channels.empty())
		return nullptr;

	uint index = 0;

	while (!freeChannel) {
		CWaveChannel *candidate = channels[index];
		CSoundCue *candidateCue = candidate->cue;

		if (candidateCue) {
			float newPriority = cue->getPriority();

			if (candidateCue->getPriority() < (double)newPriority)
				lowerChannel = candidate;
		} else {
			freeChannel = candidate;
		}

		index++;

		if (index == channels.size()) {
			if (!freeChannel && lowerChannel) {
				CSoundCue *lowerCue = lowerChannel->cue;

				if (lowerCue) {
					lowerChannel->stop();
					lowerCue->state = CSoundCue::kStateWaiting;
				}

				return lowerChannel;
			}

			return freeChannel;
		}
	}

	return freeChannel;
}

int CWaveAudioDevice::getChannelCount() {
	return (int)channels.size();
}

CAudioChannel *CWaveAudioDevice::getChannel(uint index) {
	if (!channels.empty() && index < channels.size())
		return channels[index];

	return nullptr;
}

bool CWaveAudioDevice::setMixAhead(const uint16 *mixAhead) {
	if (!_driver)
		return false;

	_driver->setMixAhead(*mixAhead);

	return true;
}

float CWaveAudioDevice::setMasterVolume(float volume) {
	if (volume >= 1.0f)
		volume = 1.0f;
	else if (!(volume > 0.0f))
		volume = 0.0f;

	if (!_driver)
		return -1.0f;

	_driver->setMasterVolume((int)((double)volume * (double)kMaxSampleVolume));

	return volume;
}

float CWaveAudioDevice::getMasterVolume() {
	if (!_driver)
		return -1.0f;

	double result = (double)_driver->getMasterVolume();

	if (result != 0.0)
		return (float)(result * (double)0.0078740157f);

	return (float)result;
}

float CWaveAudioDevice::setCategoryVolume(int category) {
	float gain = _audioSystem->getCategoryGain(category);

	if (category != 1 && category != 2 && category != 3)
		return gain;

	for (uint i = 0; i < channels.size(); i++) {
		if (channels[i]->getStatus() != 1) {
			CSoundCue *channelCue = channels[i]->cue;

			if (channelCue) {
				float cueVolume = channelCue->getVolume();

				if (channelCue->category == category)
					channels[i]->setVolume(cueVolume * gain);
			}
		}
	}

	return gain;
}

bool CWaveAudioDevice::setReverb(const int *preset, const float *time, const float *mix, const float *unusedFirst, const float *damping, const float *unusedSecond) {
	return true;
}

bool CWaveAudioDevice::setSonyRoomType(int type, int depth, int delay, int feedback) {
	LConsoleServices *console = _engine->getConsoleServices();

	if (type < 0 || type >= kSonyRoomTypeCount) {
		if (console)
			console->logError("SetSonyRoomType: Type out of range (%d)\n", type);

		return false;
	}

	if (depth < 0 || depth > kSonyMaxValue) {
		if (console)
			console->logError("SetSonyRoomType: Depth out of range (%d)\n", depth);

		return false;
	}

	if (delay < 0 || delay > kSonyMaxValue) {
		if (console)
			console->logError("SetSonyRoomType: Delay out of range (%d)\n", delay);

		return false;
	}

	// The original checks the depth instead of the feedback for negative values
	if (depth < 0 || feedback > kSonyMaxValue) {
		if (console)
			console->logError("SetSonyRoomType: Feedback out of range (%d)\n", feedback);

		return false;
	}

	if (!_driver)
		return false;

	float depthValue = (float)depth / 100.0f;

	if (_driver->setSpuReverb(type, 0, depthValue, depthValue, (float)delay / 100.0f, (float)feedback / 100.0f)) {
		if (console)
			console->logError("Cannot set reverb.\n");

		return false;
	}

	return true;
}

void CWaveAudioDevice::setSpuStereo(bool stereo) {
	_spuStereo = stereo;
}

bool CWaveAudioDevice::getSpuStereo() const {
	return _spuStereo;
}

AudioSystem *CWaveAudioDevice::getAudioSystem() const {
	return _audioSystem;
}

int CWaveAudioDevice::getCPUPercent() {
	return 0;
}

int CWaveAudioDevice::getLatency() {
	return 0;
}

int CWaveAudioDevice::getActiveSampleCount() {
	if (_driver)
		return _driver->getActiveSampleCount();

	return 0;
}

int CWaveAudioDevice::getMilesDigitalDriver() {
	return getMilesDriver() ? 1 : 0;
}

EMIMilesDriver *CWaveAudioDevice::getMilesDriver() {
	return _driver;
}

} // End of namespace Grim
