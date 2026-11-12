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

#include "engines/grim/emi/sound/audiosystem.h"
#include "engines/grim/emi/sound/caudiochannel.h"
#include "engines/grim/emi/sound/caudiodevice.h"
#include "engines/grim/emi/nodes/cnode.h"
#include "engines/grim/emi/sound/csoundcue.h"
#include "engines/grim/emi/sound/cwaveaudiodevice.h"
#include "engines/grim/emi/sound/cwavesound.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lconsoleservices.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/sound/miles_audio/driver.h"
#include "engines/grim/emi/sound/sounds.h"

namespace Grim {

static const int kDeviceChannelCount = 14;
static const uint16 kInitialDevicePreference = 16;

AudioSystem::AudioSystem(Sounds *sounds) : _refCount(0), _debugLevel(0), _audioAvailable(false), _digitalHandleReleased(false), _device(nullptr), _reverbPreset(0), _reverbEnabled(1), _sonyReverbType(0), _sonyReverbDepth(0), _sonyReverbDelay(0), _sonyReverbFeedback(0), _listener(nullptr), _masterGain(1.0f), _devicePreference(kInitialDevicePreference), _sounds(sounds), _engine(sounds->getEngine()) {
	for (int i = 0; i < 5; i++)
		_reverbValues[i] = -1.0f;

	for (int i = 0; i < 3; i++)
		_categoryGain[i] = 1.0f;

	if (_refCount <= 0) {
		_audioAvailable = false;
		_digitalHandleReleased = false;
		_device = nullptr;
		_reverbPreset = 0;

		for (int i = 0; i < 5; i++)
			_reverbValues[i] = -1.0f;

		_sonyReverbType = 0;
		_sonyReverbDepth = 0;
		_sonyReverbDelay = 0;
		_sonyReverbFeedback = 0;

		_listener.setNode(nullptr);

		_audioAvailable = createAudioDevice();

		if (_audioAvailable) {
			uint16 preference = kInitialDevicePreference;

			setAudioDevicePreference(&preference);
			setMasterGain(1.0f);
		}
	} else {
		_refCount++;
	}
}

AudioSystem::~AudioSystem() {
	_refCount--;

	if (_refCount <= 0) {
		destroyAudioDevice();

		_audioAvailable = false;
		_digitalHandleReleased = false;
		_device = nullptr;
		_reverbPreset = 0;

		for (int i = 0; i < 5; i++)
			_reverbValues[i] = -1.0f;

		_sonyReverbType = 0;
		_sonyReverbDepth = 0;
		_sonyReverbDelay = 0;
		_sonyReverbFeedback = 0;

		_listener.setNode(nullptr);
	}
}

void *AudioSystem::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void AudioSystem::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

bool AudioSystem::createAudioDevice() {
	_device = new CWaveAudioDevice(_sounds, this, kDeviceChannelCount);

	return _device && getMilesDriver();
}

EMIMilesDriver *AudioSystem::getMilesDriver() {
	if (!_device || _device->getType() != CWaveSound::kTypeWave)
		return nullptr;

	return static_cast<CWaveAudioDevice *>(_device)->getMilesDriver();
}

void AudioSystem::destroyAudioDevice() {
	if (_device) {
		delete _device;
		_device = nullptr;
	}
}

void AudioSystem::updateAudioDevice() {
	if (_audioAvailable) {
		if (_device)
			_device->update();
	}
}

CAudioChannel *AudioSystem::allocateAudioChannel(CSoundCue *cue) {
	if (!cue)
		return nullptr;

	if (!_audioAvailable)
		return nullptr;

	if (cue->getSoundType() == CWaveSound::kTypeWave) {
		uint flags = cue->_flags;

		if ((flags & CSoundCue::kFlagForceChannel) != 0 || (flags & CSoundCue::kFlagPositional) == 0) {
			if (_device)
				return _device->allocateChannel(cue);
		}
	}

	return nullptr;
}

void AudioSystem::setAudioListener(CNode *node) {
	_listener.setNode(node);

	if (_device)
		_device->updateChannelsSpatialization();
}

bool AudioSystem::isCueInRange(CSoundCue *cue) {
	uint flags = cue->_flags;
	CVector cuePosition;

	if ((flags & CSoundCue::kFlagAlwaysInRange) != 0 || (flags & CSoundCue::kFlagPositional) == 0)
		return true;

	CNode *listenerNode = _listener._node;

	if (!listenerNode)
		return false;

	cue->getPosition(&cuePosition);

	if ((listenerNode->_nodeFlags & CNode::kWorldValid) == 0)
		listenerNode->updateWorldTransform();

	double deltaZ = (double)cuePosition._z - listenerNode->_world._pos._z;
	double deltaY = (double)cuePosition._y - listenerNode->_world._pos._y;
	double deltaX = (double)cuePosition._x - listenerNode->_world._pos._x;
	float distanceSquared = (float)(deltaY * deltaY + deltaZ * deltaZ + deltaX * deltaX);
	float cutoffFar = cue->getCutoffFar();

	return (double)cue->getCutoffFar() * cutoffFar >= (double)distanceSquared;
}

float AudioSystem::setMasterGain(float gain) {
	if (gain >= 1.0f)
		_masterGain = 1.0f;
	else if (!(gain > 0.0f))
		_masterGain = 0.0f;
	else
		_masterGain = gain;

	if (_device)
		_device->setMasterVolume(_masterGain);

	return _masterGain;
}

float AudioSystem::getMasterGain() {
	return _masterGain;
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
		_categoryGain[0] = (float)clamped;

		break;

	case 2:
		_categoryGain[1] = (float)clamped;

		break;

	case 3:
		_categoryGain[2] = (float)clamped;

		break;

	default:
		break;
	}

	if (_device)
		_device->setCategoryVolume(category);

	return gain;
}

float AudioSystem::getCategoryGain(int category) {
	switch (category) {
	case 1:
		return _categoryGain[0];

	case 2:
		return _categoryGain[1];

	case 3:
		return _categoryGain[2];

	default:
		break;
	}

	return 1.0f;
}

void AudioSystem::setGlobalReverbEnabled(int enabled) {
	if (_engine->getGamePlatform() == Common::kPlatformPS2) {
		if (_reverbEnabled != (byte)enabled) {
			_reverbEnabled = (byte)enabled;

			if (_device) {
				CWaveAudioDevice *waveDevice = static_cast<CWaveAudioDevice *>(_device);

				if ((byte)enabled)
					waveDevice->setSonyRoomType(_sonyReverbType, _sonyReverbDepth, _sonyReverbDelay, _sonyReverbFeedback);
				else
					waveDevice->setSonyRoomType(0, 0, 0, 0);
			}
		}

		return;
	}

	if (_reverbEnabled != (byte)enabled) {
		_reverbEnabled = (byte)enabled;

		if (_device) {
			if ((byte)enabled) {
				_device->setReverb(&_reverbPreset, &_reverbValues[0], &_reverbValues[1], &_reverbValues[2], &_reverbValues[3], &_reverbValues[4]);
			} else {
				int disabledPreset = 0;
				float disabledValue = -1.0f;

				_device->setReverb(&disabledPreset, &disabledValue, &disabledValue, &disabledValue, &disabledValue, &disabledValue);
			}
		}
	}
}

bool AudioSystem::setGlobalReverb(int preset, const float *first, const float *second, const float *third, const float *fourth, const float *fifth) {
	_reverbPreset = preset;
	_reverbValues[0] = *first;
	_reverbValues[1] = *second;
	_reverbValues[2] = *third;
	_reverbValues[3] = *fourth;
	_reverbValues[4] = *fifth;

	if (!_reverbEnabled)
		return false;

	if (_device)
		return _device->setReverb(&_reverbPreset, first, second, third, fourth, fifth);

	return false;
}

bool AudioSystem::getGlobalReverb(int *preset, float *first, float *second, float *third, float *fourth, float *fifth) {
	if (_reverbEnabled) {
		*preset = _reverbPreset;
		*first = _reverbValues[0];
		*second = _reverbValues[1];
		*third = _reverbValues[2];
		*fourth = _reverbValues[3];
		*fifth = _reverbValues[4];

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
	_sonyReverbType = type;
	_sonyReverbDepth = depth;
	_sonyReverbDelay = delay;
	_sonyReverbFeedback = feedback;

	LConsoleServices *console = _engine->getConsoleServices();

	if (!_reverbEnabled) {
		if (console)
			console->logError("SetSonyReverb !mReverbEnabled\n");

		return false;
	}

	if (!_device) {
		if (console)
			console->logError("SetSonyReverb !smpWaveAudioDevice\n");

		return false;
	}

	return static_cast<CWaveAudioDevice *>(_device)->setSonyRoomType(type, depth, delay, feedback);
}

bool AudioSystem::setAudioDevicePreference(const uint16 *preference) {
	_devicePreference = *preference;

	return _device && _device->setMixAhead(preference);
}

void AudioSystem::stopAllChannels() {
	if (_device)
		_device->stopAll();
}

bool AudioSystem::saveAudioSettings(SaveRestoreAccess *access) {
	access(&_devicePreference, sizeof(_devicePreference));
	access(&_masterGain, sizeof(_masterGain));
	access(&_categoryGain[0], sizeof(float));
	access(&_categoryGain[1], sizeof(float));
	access(&_categoryGain[2], sizeof(float));

	// PS2 keeps the PC layout and stores the four Sony values in the preset and the first floats
	if (_engine->getGamePlatform() == Common::kPlatformPS2) {
		int type = _sonyReverbType;
		float values[5];

		values[0] = (float)_sonyReverbDepth;
		values[1] = (float)_sonyReverbDelay;
		values[2] = (float)_sonyReverbFeedback;
		values[3] = -1.0f;
		values[4] = -1.0f;

		access(&type, sizeof(type));

		for (int i = 0; i < 5; i++)
			access(&values[i], sizeof(float));

		return true;
	}

	access(&_reverbPreset, sizeof(_reverbPreset));

	for (int i = 0; i < 5; i++)
		access(&_reverbValues[i], sizeof(float));

	return true;
}

bool AudioSystem::restoreAudioSettings(SaveRestoreAccess *access) {
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

	if (_engine->getGamePlatform() == Common::kPlatformPS2) {
		setSonyReverb(restoredPreset, (int)restoredReverb[0], (int)restoredReverb[1], (int)restoredReverb[2]);

		return true;
	}

	setGlobalReverb(restoredPreset, &restoredReverb[0], &restoredReverb[1], &restoredReverb[2], &restoredReverb[3], &restoredReverb[4]);

	return true;
}

} // End of namespace Grim
