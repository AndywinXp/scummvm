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

#include "audio/mixer.h"

#include "engines/grim/emi/audiosystem.h"
#include "engines/grim/emi/csoundcue.h"
#include "engines/grim/emi/cwaveaudiodevice.h"
#include "engines/grim/emi/cwavechannel.h"
#include "engines/grim/emi/cwavesound.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/movie.h"
#include "engines/grim/emi/streamscript.h"

namespace Grim {

static const int kMaxSampleVolume = 127;
static const uint kReacquireIntervalMs = 5000;

bool CWaveAudioDevice::digitalDriverOpen = false;
bool CWaveAudioDevice::handleReleased = false;
int CWaveAudioDevice::masterVolume = kMaxSampleVolume;
uint16 CWaveAudioDevice::mixAheadPreference = 0;
int CWaveAudioDevice::busyChannelCount = 0;

CWaveAudioDevice::CWaveAudioDevice(int channelCount) {
	lost = false;
	numChannels = 0;
	lastCheckMs = 0;
	digitalDriverOpen = false;

	Audio::Mixer *mixer = g_system->getMixer();

	if (mixer && mixer->isReady())
		digitalDriverOpen = true;

	if (!digitalDriverOpen)
		return;

	StreamScript::setStreamingGate(getMilesDigitalDriver);
	Movie::setMovieAudioGate(getMilesDigitalDriver);

	int created = 0;

	for (int i = 0; i < channelCount; i++) {
		CWaveChannel *channel = new CWaveChannel();

		if (!channel->hasSample) {
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

	digitalDriverOpen = false;
	handleReleased = false;
}

void *CWaveAudioDevice::operator new(size_t size) {
	return gpMemoryServices->malloc(size);
}

void CWaveAudioDevice::operator delete(void *block) {
	gpMemoryServices->free(block);
}

int CWaveAudioDevice::getType() {
	return CWaveSound::kTypeWave;
}

void CWaveAudioDevice::releaseDigitalHandle() {
	if (!digitalDriverOpen)
		return;

	handleReleased = true;

	for (uint i = 0; i < channels.size(); i++)
		channels[i]->releaseDigitalOutput();
}

void CWaveAudioDevice::reacquireDigitalHandle() {
	if (!digitalDriverOpen)
		return;

	lost = false;
	handleReleased = false;

	for (uint i = 0; i < channels.size(); i++)
		channels[i]->reacquireDigitalOutput();
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

	busyChannelCount = 0;

	for (uint i = 0; i < channels.size(); i++) {
		if (channels[i]) {
			if (channels[i]->update() != 1)
				busyChannelCount++;
		}
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
	if (!digitalDriverOpen)
		return false;

	mixAheadPreference = *mixAhead;

	return true;
}

float CWaveAudioDevice::setMasterVolume(float volume) {
	if (volume >= 1.0f)
		volume = 1.0f;
	else if (!(volume > 0.0f))
		volume = 0.0f;

	if (!digitalDriverOpen)
		return -1.0f;

	masterVolume = (int)((double)volume * 127.0);

	for (uint i = 0; i < channels.size(); i++)
		channels[i]->refreshMixerVolume();

	return volume;
}

float CWaveAudioDevice::getMasterVolume() {
	if (!digitalDriverOpen)
		return -1.0f;

	double result = (double)masterVolume;

	if (result != 0.0)
		return (float)(result * (double)0.0078740157f);

	return (float)result;
}

float CWaveAudioDevice::setCategoryVolume(int category) {
	float gain = AudioSystem::getCategoryGain(category);

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

int CWaveAudioDevice::getCPUPercent() {
	return 0;
}

int CWaveAudioDevice::getLatency() {
	return 0;
}

int CWaveAudioDevice::getActiveSampleCount() {
	if (!digitalDriverOpen)
		return 0;

	int count = 0;

	for (uint i = 0; i < channels.size(); i++) {
		if (channels[i]->isSampleActive())
			count++;
	}

	return count;
}

int CWaveAudioDevice::getMilesDigitalDriver() {
	return digitalDriverOpen ? 1 : 0;
}

} // End of namespace Grim
