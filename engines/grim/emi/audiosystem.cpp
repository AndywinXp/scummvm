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

#include "engines/grim/emi/audiosystem.h"
#include "engines/grim/emi/caudiochannel.h"
#include "engines/grim/emi/caudiodevice.h"
#include "engines/grim/emi/cnode.h"
#include "engines/grim/emi/csoundcue.h"
#include "engines/grim/emi/cwaveaudiodevice.h"
#include "engines/grim/emi/cwavesound.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/grim.h"

namespace Grim {

static const int kDeviceChannelCount = 14;
static const uint16 kInitialDevicePreference = 16;

int16 AudioSystem::refCount = 0;
AudioSystem::LipCallback *AudioSystem::lipCallback = nullptr;
int AudioSystem::debugLevel = 0;
bool AudioSystem::audioAvailable = false;
bool AudioSystem::digitalHandleReleased = false;
CAudioDevice *AudioSystem::device = nullptr;
int AudioSystem::reverbPreset = 0;
float AudioSystem::reverbValues[5] = { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f };
byte AudioSystem::reverbEnabled = 1;
int AudioSystem::sonyReverbType = 0;
int AudioSystem::sonyReverbDepth = 0;
int AudioSystem::sonyReverbDelay = 0;
int AudioSystem::sonyReverbFeedback = 0;
CNodeRef AudioSystem::listener(nullptr);
float AudioSystem::masterGain = 1.0f;
float AudioSystem::categoryGain[3] = { 1.0f, 1.0f, 1.0f };
uint16 AudioSystem::devicePreference = kInitialDevicePreference;

AudioSystem::AudioSystem() {
	if (refCount <= 0) {
		audioAvailable = false;
		digitalHandleReleased = false;
		device = nullptr;
		reverbPreset = 0;

		for (int i = 0; i < 5; i++)
			reverbValues[i] = -1.0f;

		sonyReverbType = 0;
		sonyReverbDepth = 0;
		sonyReverbDelay = 0;
		sonyReverbFeedback = 0;

		listener.setNode(nullptr);

		audioAvailable = createAudioDevice();

		if (audioAvailable) {
			uint16 preference = kInitialDevicePreference;

			setAudioDevicePreference(&preference);
			setMasterGain(1.0f);
		}
	} else {
		refCount++;
	}
}

AudioSystem::~AudioSystem() {
	refCount--;

	if (refCount <= 0) {
		destroyAudioDevice();

		audioAvailable = false;
		digitalHandleReleased = false;
		device = nullptr;
		reverbPreset = 0;

		for (int i = 0; i < 5; i++)
			reverbValues[i] = -1.0f;

		sonyReverbType = 0;
		sonyReverbDepth = 0;
		sonyReverbDelay = 0;
		sonyReverbFeedback = 0;

		listener.setNode(nullptr);
	}
}

void *AudioSystem::operator new(size_t size) {
	return gpMemoryServices->mallocImpl(size);
}

void AudioSystem::operator delete(void *block) {
	gpMemoryServices->freeImpl(block);
}

bool AudioSystem::createAudioDevice() {
	device = new CWaveAudioDevice(kDeviceChannelCount);

	return device && CWaveAudioDevice::getMilesDriver();
}

void AudioSystem::destroyAudioDevice() {
	if (device) {
		delete device;
		device = nullptr;
	}
}

void AudioSystem::updateAudioDevice() {
	if (audioAvailable) {
		if (device)
			device->update();
	}
}

CAudioChannel *AudioSystem::allocateAudioChannel(CSoundCue *cue) {
	if (!cue)
		return nullptr;

	if (!audioAvailable)
		return nullptr;

	if (cue->getSoundType() == CWaveSound::kTypeWave) {
		uint flags = cue->flags;

		if ((flags & CSoundCue::kFlagForceChannel) != 0 || (flags & CSoundCue::kFlagPositional) == 0) {
			if (device)
				return device->allocateChannel(cue);
		}
	}

	return nullptr;
}

void AudioSystem::setAudioListener(CNode *node) {
	listener.setNode(node);

	if (device)
		device->updateChannelsSpatialization();
}

bool AudioSystem::isCueInRange(CSoundCue *cue) {
	uint flags = cue->flags;
	CVector cuePosition;

	if ((flags & CSoundCue::kFlagAlwaysInRange) != 0 || (flags & CSoundCue::kFlagPositional) == 0)
		return true;

	CNode *listenerNode = listener.node;

	if (!listenerNode)
		return false;

	cue->getPosition(&cuePosition);

	if ((listenerNode->nodeFlags & CNode::kWorldValid) == 0)
		listenerNode->updateWorldTransform();

	double deltaZ = (double)cuePosition.z - listenerNode->world.pos.z;
	double deltaY = (double)cuePosition.y - listenerNode->world.pos.y;
	double deltaX = (double)cuePosition.x - listenerNode->world.pos.x;
	float distanceSquared = (float)(deltaY * deltaY + deltaZ * deltaZ + deltaX * deltaX);
	float cutoffFar = cue->getCutoffFar();

	return (double)cue->getCutoffFar() * cutoffFar >= (double)distanceSquared;
}

float AudioSystem::setMasterGain(float gain) {
	if (gain >= 1.0f)
		masterGain = 1.0f;
	else if (!(gain > 0.0f))
		masterGain = 0.0f;
	else
		masterGain = gain;

	if (device)
		device->setMasterVolume(masterGain);

	return masterGain;
}

float AudioSystem::getMasterGain() {
	return masterGain;
}

float AudioSystem::setCategoryGain(int category, float gain) {
	double clamped;

	if (gain >= 1.0f)
		clamped = 1.0;
	else if (!(gain > 0.0f))
		clamped = 0.0;
	else
		clamped = gain;

	switch (category) {
	case 1:
		categoryGain[0] = (float)clamped;

		break;

	case 2:
		categoryGain[1] = (float)clamped;

		break;

	case 3:
		categoryGain[2] = (float)clamped;

		break;

	default:
		break;
	}

	if (device)
		device->setCategoryVolume(category);

	return gain;
}

float AudioSystem::getCategoryGain(int category) {
	switch (category) {
	case 1:
		return categoryGain[0];

	case 2:
		return categoryGain[1];

	case 3:
		return categoryGain[2];

	default:
		break;
	}

	return 1.0f;
}

void AudioSystem::setGlobalReverbEnabled(int enabled) {
	if (g_grim->getGamePlatform() == Common::kPlatformPS2) {
		if (reverbEnabled != (byte)enabled) {
			reverbEnabled = (byte)enabled;

			if (device) {
				CWaveAudioDevice *waveDevice = static_cast<CWaveAudioDevice *>(device);

				if ((byte)enabled)
					waveDevice->setSonyRoomType(sonyReverbType, sonyReverbDepth, sonyReverbDelay, sonyReverbFeedback);
				else
					waveDevice->setSonyRoomType(0, 0, 0, 0);
			}
		}

		return;
	}

	if (reverbEnabled != (byte)enabled) {
		reverbEnabled = (byte)enabled;

		if (device) {
			if ((byte)enabled) {
				device->setReverb(&reverbPreset, &reverbValues[0], &reverbValues[1], &reverbValues[2], &reverbValues[3], &reverbValues[4]);
			} else {
				int disabledPreset = 0;
				float disabledValue = -1.0f;

				device->setReverb(&disabledPreset, &disabledValue, &disabledValue, &disabledValue, &disabledValue, &disabledValue);
			}
		}
	}
}

bool AudioSystem::setGlobalReverb(int preset, const float *first, const float *second, const float *third, const float *fourth, const float *fifth) {
	reverbPreset = preset;
	reverbValues[0] = *first;
	reverbValues[1] = *second;
	reverbValues[2] = *third;
	reverbValues[3] = *fourth;
	reverbValues[4] = *fifth;

	if (!reverbEnabled)
		return false;

	if (device)
		return device->setReverb(&reverbPreset, first, second, third, fourth, fifth);

	return false;
}

bool AudioSystem::getGlobalReverb(int *preset, float *first, float *second, float *third, float *fourth, float *fifth) {
	if (reverbEnabled) {
		*preset = reverbPreset;
		*first = reverbValues[0];
		*second = reverbValues[1];
		*third = reverbValues[2];
		*fourth = reverbValues[3];
		*fifth = reverbValues[4];

		return true;
	}

	*preset = 0;
	*first = -1.0f;
	*second = -1.0f;
	*third = -1.0f;
	*fourth = -1.0f;
	*fifth = -1.0f;

	return false;
}

bool AudioSystem::setSonyReverb(int type, int depth, int delay, int feedback) {
	sonyReverbType = type;
	sonyReverbDepth = depth;
	sonyReverbDelay = delay;
	sonyReverbFeedback = feedback;

	if (!reverbEnabled) {
		if (gpConsoleServices)
			gpConsoleServices->logError("SetSonyReverb !mReverbEnabled\n");

		return false;
	}

	if (!device) {
		if (gpConsoleServices)
			gpConsoleServices->logError("SetSonyReverb !smpWaveAudioDevice\n");

		return false;
	}

	return static_cast<CWaveAudioDevice *>(device)->setSonyRoomType(type, depth, delay, feedback);
}

bool AudioSystem::setAudioDevicePreference(const uint16 *preference) {
	devicePreference = *preference;

	return device && device->setMixAhead(preference);
}

void AudioSystem::stopAllChannels() {
	if (device)
		device->stopAll();
}

bool AudioSystem::saveAudioSettings(tSaveRestoreAccess *access) {
	access(&devicePreference, sizeof(devicePreference));
	access(&masterGain, sizeof(masterGain));
	access(&categoryGain[0], sizeof(float));
	access(&categoryGain[1], sizeof(float));
	access(&categoryGain[2], sizeof(float));

	// PS2 keeps the PC layout and stores the four Sony values in the preset and the first floats
	if (g_grim->getGamePlatform() == Common::kPlatformPS2) {
		int type = sonyReverbType;
		float values[5];

		values[0] = (float)sonyReverbDepth;
		values[1] = (float)sonyReverbDelay;
		values[2] = (float)sonyReverbFeedback;
		values[3] = -1.0f;
		values[4] = -1.0f;

		access(&type, sizeof(type));

		for (int i = 0; i < 5; i++)
			access(&values[i], sizeof(float));

		return true;
	}

	access(&reverbPreset, sizeof(reverbPreset));

	for (int i = 0; i < 5; i++)
		access(&reverbValues[i], sizeof(float));

	return true;
}

bool AudioSystem::restoreAudioSettings(tSaveRestoreAccess *access) {
	uint16 preference = 0;
	float restoredValue = 0.0f;
	int restoredPreset = 0;
	float restoredReverb[5];

	access(&preference, sizeof(preference));
	setAudioDevicePreference(&preference);

	access(&restoredValue, sizeof(restoredValue));
	setMasterGain(restoredValue);

	access(&restoredValue, sizeof(restoredValue));
	setCategoryGain(1, restoredValue);

	access(&restoredValue, sizeof(restoredValue));
	setCategoryGain(2, restoredValue);

	access(&restoredValue, sizeof(restoredValue));
	setCategoryGain(3, restoredValue);

	access(&restoredPreset, sizeof(restoredPreset));

	for (int i = 0; i < 5; i++)
		access(&restoredReverb[i], sizeof(float));

	if (g_grim->getGamePlatform() == Common::kPlatformPS2) {
		setSonyReverb(restoredPreset, (int)restoredReverb[0], (int)restoredReverb[1], (int)restoredReverb[2]);

		return true;
	}

	setGlobalReverb(restoredPreset, &restoredReverb[0], &restoredReverb[1], &restoredReverb[2], &restoredReverb[3], &restoredReverb[4]);

	return true;
}

} // End of namespace Grim
