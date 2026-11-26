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

#include "audio/mididrv.h"
#include "audio/midiparser.h"
#include "common/config-manager.h"
#include "common/endian.h"
#include "common/file.h"
#include "common/path.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "topgun/libraries/audio/audio.h"
#include "topgun/libraries/audio/audio_v1_2_3.h"
#include "topgun/libraries/audio/midi.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_3.h"

namespace Topgun {

MidiSequencer::MidiSequencer() {
	MidiDriver::DeviceHandle device = MidiDriver::detectDevice(MDT_MIDI | MDT_ADLIB | MDT_PREFER_GM);

	_driver = MidiDriver::createMidi(device);
	assert(_driver);

	int result = _driver->open();
	if (result == 0) {
		_driver->sendGMReset();
		_driver->setTimerCallback(this, &timerCallback);
	}
}

void MidiSequencer::sendToChannel(byte channel, uint32 b) {
	if (!_channelsTable[channel]) {
		_channelsTable[channel] = (channel == 9) ? _driver->getPercussionChannel() : _driver->allocateChannel();

		if (_channelsTable[channel])
			_channelsTable[channel]->volume(_channelsVolume[channel] * _masterVolume / 255);
	}

	if (_channelsTable[channel])
		_channelsTable[channel]->send(b);
}

bool MidiSequencer::playFile(const Common::String &fileName) {
	Common::StackLock lock(_mutex);

	stop();

	Common::File file;
	if (!file.open(Common::Path(fileName, '/')))
		return false;

	int size = file.size();
	byte *buffer = (byte *)malloc(size);

	file.read(buffer, size);
	file.close();

	bool started = playData(buffer, size);

	free(buffer);
	return started;
}

bool MidiSequencer::playData(const byte *data, uint32 size) {
	Common::StackLock lock(_mutex);

	stop();

	_midiData = (byte *)malloc(size);
	memcpy(_midiData, data, size);

	MidiParser *parser = MidiParser::createParser_SMF();

	if (!parser->loadMusic(_midiData, size)) {
		delete parser;
		free(_midiData);
		_midiData = nullptr;
		return false;
	}

	parser->setTrack(0);
	parser->setMidiDriver(this);
	parser->setTimerRate(_driver->getBaseTempo());
	parser->property(MidiParser::mpCenterPitchWheelOnUnload, 1);

	_parser = parser;

	setVolume(ConfMan.getBool("mute") ? 0 : 255);

	_isLooping = false;
	_isPlaying = true;
	return true;
}

void AudioLibrary::playDefaultMidiFile() {
	playMidiFile("efxfire.mid");
}

bool AudioLibrary::sendMidiDeviceCommand(uint16 message, uint32 flags, DeviceParms &parms) {
	if (message == TG_AUDIO_DEV_OPEN) {
		// The element is either a file or a block of memory
		if (!(flags & TG_AUDIO_DEV_OPEN_ELEMENT) || (!parms.midiData && !Common::File::exists(Common::Path(parms.fileName, '/'))))
			return false;

		if (MidiDriver::getMusicType(MidiDriver::detectDevice(MDT_MIDI | MDT_ADLIB | MDT_PREFER_GM)) == MT_NULL)
			return false;

		_midiFileName = parms.fileName;
		_sequencer = new MidiSequencer();
		parms.deviceId = 1;
		return true;
	}

	if (_seqDeviceId == 0)
		return false;

	switch (message) {
	case TG_AUDIO_DEV_PLAY:
		if (parms.midiData)
			return _sequencer->playData(parms.midiData, parms.midiSize);

		return _sequencer->playFile(_midiFileName);

	case TG_AUDIO_DEV_STOP:
		_sequencer->stop();
		return true;

	case TG_AUDIO_DEV_CLOSE:
		delete _sequencer;
		_sequencer = nullptr;
		return true;

	default:
		return false;
	}
}

uint16 AudioLibrary::playMidiFile(const Common::String &fileName) {
	stopAndCloseMidi();

	DeviceParms openParms;

	openParms.fileName = fileName;

	if (!sendMidiDeviceCommand(TG_AUDIO_DEV_OPEN, TG_AUDIO_DEV_OPEN_TYPE | TG_AUDIO_DEV_OPEN_ELEMENT, openParms))
		return 0;

	_seqDeviceId = openParms.deviceId;

	DeviceParms playParms;

	return sendMidiDeviceCommand(TG_AUDIO_DEV_PLAY, 0, playParms) ? 1 : 0;
}

void AudioLibrary::stopAndCloseMidi() {
	if (_seqDeviceId != 0) {
		DeviceParms parms;

		sendMidiDeviceCommand(TG_AUDIO_DEV_STOP, 0, parms);
		sendMidiDeviceCommand(TG_AUDIO_DEV_CLOSE, 0, parms);
		_seqDeviceId = 0;
	}
}

void AudioLibrary::shutdownMidi() {
	stopAndCloseMidi();
}

AudioLibrary_v1_2::MidiObject *AudioLibrary_v1_2::allocMidiObject() {
	if (_midiCount >= TG_AUDIO_V1_2_2_MAX_MIDI_OBJECTS)
		return nullptr;

	MidiObject *midi = new MidiObject();

	do {
		midi->handle = _nextMidiHandle;
		_nextMidiHandle = (_nextMidiHandle == 0xFFFF) ? 1 : _nextMidiHandle + 1;
	} while (validateMidiHandle(midi->handle));

	_midiObjects[_midiCount] = midi;
	_midiCount++;
	return midi;
}

void AudioLibrary_v1_2::destroyMidiObject(MidiObject *midi) {
	if (_currentMidi == midi)
		stopMidiSequence(midi);

	if (_engine->_version == kVersionV1_2_2_1 && !midi->isStatic)
		_streamedMidiCount--;

	removeMidiFromArray(midi);
}

void AudioLibrary_v1_2::removeMidiFromArray(MidiObject *midi) {
	uint16 index = 0;

	while (index < _midiCount && _midiObjects[index] != midi)
		index++;

	if (_midiCount <= index)
		return;

	invokeAndClearCloseCallback(midi);
	engine()->utilsLibrary()->tgFree(midi->data);
	delete midi;

	for (uint16 i = index; i + 1 < _midiCount; i++)
		_midiObjects[i] = _midiObjects[i + 1];

	_midiCount--;
	_midiObjects[_midiCount] = nullptr;
}

bool AudioLibrary_v1_2::initMidiObjectDataSource(const MidiCreateInfo_v1_2_3 *info, MidiObject *midi) {
	midi->callbackHandle = info->handle;
	midi->readProc = info->readProc;
	midi->rewindProc = info->rewindProc;
	midi->closeProc = info->closeProc;

	if (_engine->_version < kVersionV1_2_2_1) {
		midi->notifyWParam = READ_LE_UINT16(info->header + TG_AUDIO_V1_2_2_MIDI_HEADER_OFFSET_NOTIFY);
		midi->size = READ_LE_UINT32(info->header + TG_AUDIO_V1_2_2_MIDI_HEADER_OFFSET_SIZE);
		return true;
	}

	if (_engine->_version == kVersionV1_2_2_1) {
		midi->notifyWParam = READ_LE_UINT16(info->header + TG_AUDIO_V1_2_2_1_MIDI_HEADER_OFFSET_NOTIFY);
		midi->size = READ_LE_UINT32(info->header + TG_AUDIO_V1_2_2_1_MIDI_HEADER_OFFSET_SIZE);
		midi->enabled = (!_mute && _cdDeviceId == 0);
		midi->isStatic = (info->header[TG_AUDIO_V1_2_2_1_MIDI_HEADER_OFFSET_FLAGS] & TG_AUDIO_V1_2_2_1_MIDI_HEADER_FLAG_STATIC) != 0;

		if (_cdDeviceId != 0)
			stopCDIfOwned();

		if (midi->size <= TG_AUDIO_V1_2_2_1_MIDI_BUFFER_SIZE)
			midi->isStatic = true;

		if (midi->isStatic) {
			midi->bufferSize = midi->size;
		} else {
			midi->bufferSize = TG_AUDIO_V1_2_2_1_MIDI_BUFFER_SIZE;
			midi->lowWater = TG_AUDIO_V1_2_2_1_MIDI_LOW_WATER;
			_streamedMidiCount++;
		}

		midi->data = engine()->utilsLibrary()->tgMalloc(midi->bufferSize, 0);

		if (!midi->data)
			return false;

		if (!compactAndRefillMidiBuffer(midi))
			return false;

		if (midi->isStatic)
			invokeAndClearCloseCallback(midi);

		return true;
	}

	uint32 size;

	if (_engine->_version >= kVersionV1_2_6) {
		midi->notifyWParam = READ_LE_UINT16(info->header + TG_AUDIO_V1_2_2_MIDI_HEADER_OFFSET_NOTIFY);
		size = READ_LE_UINT32(info->header + TG_AUDIO_V1_2_2_MIDI_HEADER_OFFSET_SIZE);
	} else {
		midi->notifyWParam = READ_LE_UINT16(info->header + TG_AUDIO_V1_2_2_2_MIDI_HEADER_OFFSET_NOTIFY);
		size = READ_LE_UINT16(info->header + TG_AUDIO_V1_2_2_2_MIDI_HEADER_OFFSET_SIZE);
	}

	if (_cdDeviceId != 0)
		stopCDIfOwned();

	midi->count = (uint16)(size >> 2);
	midi->data = engine()->utilsLibrary()->tgMalloc(size, 0);

	if (!midi->data)
		return false;

	if (engine()->audioSourceRead(midi->readProc, midi->callbackHandle, midi->data, size) != size)
		return false;

	return invokeAndClearCloseCallback(midi);
}

bool AudioLibrary_v1_2::invokeAndClearCloseCallback(MidiObject *midi) {
	if (midi->closeProc != 0) {
		if (_engine->_version >= kVersionV1_2_2_1 && _cdDeviceId != 0)
			stopCDIfOwned();

		engine()->audioSourceClose(midi->closeProc, midi->callbackHandle);
		midi->closeProc = 0;
	}

	return true;
}

uint16 AudioLibrary_v1_2::rewindMidiSource(MidiObject *midi) {
	if (midi->rewindProc == 0)
		return 0;

	return (uint16)engine()->audioSourceRewind(midi->rewindProc, midi->callbackHandle);
}

uint16 AudioLibrary_v1_2::compactAndRefillMidiBuffer(MidiObject *midi) {
	if (midi->readPos != 0) {
		if (midi->available != 0)
			engine()->utilsLibrary()->copyHugeBytes(midi->data, midi->data + midi->readPos, midi->available);

		midi->readPos = 0;
	}

	uint32 wanted = midi->bufferSize - midi->available;

	if (wanted == 0)
		return 0;

	uint32 remaining = midi->size - midi->readFromSource;

	if (remaining <= wanted) {
		midi->endOfSource = true;
		wanted = remaining;
	}

	uint32 delivered = engine()->audioSourceRead(midi->readProc, midi->callbackHandle, midi->data + midi->available, wanted);

	if (delivered != wanted)
		return 0;

	midi->available += wanted;
	midi->readFromSource += wanted;

	if (!midi->isStatic && midi->endOfSource && midi->loops != 0) {
		midi->readFromSource = 0;
		midi->endOfSource = false;
		return rewindMidiSource(midi);
	}

	return 1;
}

void AudioLibrary_v1_2::rewindMidiObject(MidiObject *midi) {
	if (midi->isStatic) {
		midi->available = midi->bufferSize;
		midi->consumed = 0;
		midi->readPos = 0;
		return;
	}

	if (midi->consumed == 0)
		return;

	if (_cdDeviceId != 0)
		stopCDIfOwned();

	midi->readFromSource = 0;
	midi->consumed = 0;
	midi->available = 0;
	midi->endOfSource = false;
	rewindMidiSource(midi);
	compactAndRefillMidiBuffer(midi);
}

void AudioLibrary_v1_2::destroyAllMidiObjects() {
	uint16 index = 0;

	while (index < _midiCount) {
		destroyMidiObject(_midiObjects[index]);
		index++;
	}

	if (_midiOut)
		closeMidiOutDevice();
}

AudioLibrary_v1_2::MidiObject *AudioLibrary_v1_2::validateMidiHandle(uint16 handle) {
	for (uint16 i = 0; i < _midiCount; i++) {
		if (_midiObjects[i]->handle == handle)
			return _midiObjects[i];
	}

	return nullptr;
}

void AudioLibrary_v1_2::probeMidiOutCapabilities() {
	MidiDriver::DeviceHandle device = MidiDriver::detectDevice(MDT_MIDI | MDT_ADLIB | MDT_PREFER_GM);

	if (MidiDriver::getMusicType(device) == MT_NULL)
		_noMidiDevice = true;
}

bool AudioLibrary_v1_2::openMidiOutDevice() {
	if (_noMidiDevice)
		return false;

	if (!_midiOut) {
		_midiOut = new MidiSequencer();
		_midiOriginalVolume = _midiDeviceVolume;
		applyMidiDeviceVolume();
	}

	return true;
}

void AudioLibrary_v1_2::setMidiOutVolume(uint32 volume) {
	if (_engine->_version >= kVersionV1_2_6) {
		if (!_midiOut)
			return;

		_midiUserVolume = volume;
	}

	_midiDeviceVolume = volume;
	applyMidiDeviceVolume();
}

void AudioLibrary_v1_2::applyMidiDeviceVolume() {
	if (_engine->_version < kVersionV1_2_2_1) {
		if (_sequencer)
			_sequencer->setVolume(ConfMan.getBool("mute") ? 0 : 255);

		return;
	}

	if (!_midiOut)
		return;

	uint32 loudest = MAX<uint32>(_midiDeviceVolume & 0xFFFF, _midiDeviceVolume >> 16);
	int level = (int)((loudest * 255 + 32767) / 65535);
	_midiOut->setVolume(ConfMan.getBool("mute") ? 0 : level);
}

void AudioLibrary_v1_2::closeMidiOutDevice() {
	if (!_midiOut)
		return;

	if (_engine->_version >= kVersionV1_2_2_2)
		static_cast<AudioLibrary_v1_2_3 *>(this)->midiClearCurrentAndKillTimer();

	resetMidiOutDevice();

	if (_engine->_version >= kVersionV1_2_6 || _midiOriginalVolume != _midiUserVolume)
		setMidiOutVolume(_midiOriginalVolume);

	delete _midiOut;
	_midiOut = nullptr;
}

void AudioLibrary_v1_2_3::muteMidiOutput() {
	if (_mute) {
		_midiMuteSavedVolume = audioGetMidiVolume();

		if (_engine->_version >= kVersionV1_2_6)
			setMidiOutVolume(0);
		else
			audioSetMidiVolume(0);
	} else {
		if (_engine->_version >= kVersionV1_2_6)
			setMidiOutVolume(_midiMuteSavedVolume);
		else
			audioSetMidiVolume(_midiMuteSavedVolume);
	}
}

bool AudioLibrary_v1_2::openAndConfigureMidiOut() {
	if (!openMidiOutDevice())
		return false;

	if (_engine->_version >= kVersionV1_2_6)
		static_cast<AudioLibrary_v1_2_3 *>(this)->_midiTimerArmed = false;

	if (_engine->_version >= kVersionV1_2_3) {
		resetMidiOutDevice();

		uint16 unsetVolume = TG_AUDIO_V1_2_3_INI_MIDI_VOLUME_UNSET;

		if (_engine->_version == kVersionV1_2_8)
			unsetVolume = TG_AUDIO_V1_2_8_INI_MIDI_VOLUME_UNSET;

		uint16 iniVolume = static_cast<AudioLibrary_v1_2_3 *>(this)->_iniMidiVolume;

		if (iniVolume != unsetVolume) {
			_midiUserVolume = ((uint32)iniVolume << 16) | iniVolume;
			_midiUserVolumeSet = true;
		}
	}

	if (_engine->_version >= kVersionV1_2_6) {
		if (_midiUserVolumeSet)
			setMidiOutVolume(_midiUserVolume);

		if (_mute)
			static_cast<AudioLibrary_v1_2_3 *>(this)->muteMidiOutput();
	} else if (_midiUserVolumeSet && _midiOriginalVolume != _midiUserVolume) {
		setMidiOutVolume(_midiUserVolume);
	}

	return true;
}

void AudioLibrary_v1_2_3::pauseOrResumeMidiPlayback(uint16 paused) {
	if (paused == 0) {
		midiSequencerStep(_currentMidi);
		return;
	}

	_midiTimerArmed = false;
	resetMidiOutDevice();
}

void AudioLibrary_v1_2::resetMidiOutDevice() {
	if (!_midiOut)
		return;

	for (byte channel = 0; channel < TG_AUDIO_V1_2_2_1_MIDI_NUM_CHANNELS; channel++)
		sendMidiAllNotesOff(channel);
}

void AudioLibrary_v1_2::sendMidiAllNotesOff(byte channel) {
	if (!_midiOut)
		return;

	_midiOut->send((uint32)(channel | TG_AUDIO_V1_2_2_1_MIDI_CONTROL_CHANGE) | (TG_AUDIO_V1_2_2_1_MIDI_ALL_NOTES_OFF << 8));

	// Resetting the device turns off the notes on every channel
	if (_engine->_version == kVersionV1_2_2_1) {
		for (byte i = 0; i < TG_AUDIO_V1_2_2_1_MIDI_NUM_CHANNELS; i++)
			_midiOut->send((uint32)(i | TG_AUDIO_V1_2_2_1_MIDI_CONTROL_CHANGE) | (TG_AUDIO_V1_2_2_1_MIDI_ALL_NOTES_OFF << 8));
	}
}

uint16 AudioLibrary_v1_2::audioCreateMidi(MidiCreateInfo_v1_2_3 *info) {
	if (!info)
		return 0;

	if (info->readProc == 0)
		return 0;

	MidiObject *midi = allocMidiObject();

	if (!midi && info->closeProc != 0)
		engine()->audioSourceClose(info->closeProc, info->handle);

	if (!midi)
		return 0;

	if (!initMidiObjectDataSource(info, midi)) {
		removeMidiFromArray(midi);
		return 0;
	}

	return midi->handle;
}

uint16 AudioLibrary_v1_2::audioDestroyMidi(uint16 handle) {
	if (handle == 0)
		return 0;

	MidiObject *midi = validateMidiHandle(handle);

	if (midi)
		destroyMidiObject(midi);

	return 0;
}

uint16 AudioLibrary_v1_2::audioPlayMidi(uint16 handle, const byte *params) {
	MidiObject *midi = validateMidiHandle(handle);

	if (_engine->_version >= kVersionV1_2_2_1) {
		if (!midi)
			return 0;

		byte defaultParams[TG_AUDIO_V1_2_2_1_MIDI_PLAY_PARAMS_SIZE] = {};

		if (!params)
			params = defaultParams;

		if (!startMidiSequence(params, midi)) {
			if (_engine->_version >= kVersionV1_2_6)
				midi->notifyArmed = false;
			else
				postMidiCompletionMessage(midi);
		}

		return 1;
	}

	if (!midi) {
		warning("AudioLibrary_v1_2::audioPlayMidi(): unknown MIDI %d", handle);
		return 0;
	}

	// The only task there is owns the device, so the owner check always passes
	if (_seqDeviceId != 0) {
		stopAndCloseMidi();
		_currentMidi = nullptr;
	}

	UtilsLibrary *utilsLibrary = engine()->utilsLibrary();
	byte *buffer = utilsLibrary->tgMalloc(midi->size, 0);

	if (!buffer)
		return 0;

	if (midi->rewindProc != 0)
		engine()->audioSourceRewind(midi->rewindProc, midi->callbackHandle);

	if (engine()->audioSourceRead(midi->readProc, midi->callbackHandle, buffer, midi->size) != midi->size) {
		utilsLibrary->tgFree(buffer);
		return 0;
	}

	// The device takes the data from memory instead of a temporary file
	DeviceParms openParms;

	openParms.midiData = buffer;
	openParms.midiSize = midi->size;

	bool opened = sendMidiDeviceCommand(TG_AUDIO_DEV_OPEN, TG_AUDIO_DEV_OPEN_TYPE | TG_AUDIO_DEV_OPEN_ELEMENT, openParms);

	if (opened) {
		_seqDeviceId = openParms.deviceId;

		DeviceParms playParms;

		playParms.midiData = buffer;
		playParms.midiSize = midi->size;
		sendMidiDeviceCommand(TG_AUDIO_DEV_PLAY, 0, playParms);
	}

	utilsLibrary->tgFree(buffer);

	if (!opened)
		return 0;

	_currentMidi = midi;
	return 1;
}

uint16 AudioLibrary_v1_2::audioStopMidi(uint16 handle) {
	if (_engine->_version >= kVersionV1_2_2_1) {
		MidiObject *current = (handle != 0) ? validateMidiHandle(handle) : _currentMidi;

		if (current)
			stopMidiSequence(current);

		return 0;
	}

	if (_seqDeviceId == 0)
		return 0;

	MidiObject *midi = validateMidiHandle(handle);

	if (midi)
		stopMidiSequence(midi);

	return (_seqDeviceId != 0) ? 1 : 0;
}

void AudioLibrary_v1_2::stopMidiSequence(MidiObject *midi) {
	if (_engine->_version < kVersionV1_2_2_1) {
		if (_currentMidi == midi && _seqDeviceId != 0) {
			stopAndCloseMidi();
			_currentMidi = nullptr;
		}

		return;
	}

	midi->notifyArmed = false;

	if (_currentMidi == midi) {
		if (_engine->_version >= kVersionV1_2_2_2)
			static_cast<AudioLibrary_v1_2_3 *>(this)->midiClearCurrentAndKillTimer();

		if (_engine->_version == kVersionV1_2_2_2)
			closeMidiOutDevice();
		else
			resetMidiOutDevice();

		postMidiCompletionMessage(midi);
	}
}

uint32 AudioLibrary_v1_2::audioGetMidiVolume() {
	if (!_midiOut && !openMidiOutDevice())
		return 0;

	if (_midiUserVolumeSet)
		return _midiUserVolume;

	return _midiOriginalVolume;
}

void AudioLibrary_v1_2::audioSetMidiVolume(uint32 volume) {
	_midiUserVolume = volume;
	_midiUserVolumeSet = true;

	if (_midiOut)
		setMidiOutVolume(volume);

	if (_engine->_version >= kVersionV1_2_3) {
		AudioLibrary_v1_2_3 *newer = static_cast<AudioLibrary_v1_2_3 *>(this);

		newer->_iniMidiVolume = (uint16)volume;
		newer->saveMidiVolumeToIni();
	}
}

void AudioLibrary_v1_2::audioSetMidi(uint16 handle, const byte *params) {
	MidiObject *midi = (handle != 0) ? validateMidiHandle(handle) : _currentMidi;

	if (!midi)
		return;

	if (!params) {
		warning("AudioLibrary_v1_2::audioSetMidi(): no parameter block");
		return;
	}

	bool setTempo;
	bool setMutes;

	if (_engine->_version < kVersionV1_2_2_2) {
		setTempo = (params[TG_AUDIO_V1_2_2_1_MIDI_SET_OFFSET_FLAGS] & TG_AUDIO_V1_2_2_1_MIDI_SET_FLAG_TEMPO) != 0;
		setMutes = (params[TG_AUDIO_V1_2_2_1_MIDI_SET_OFFSET_FLAGS] & TG_AUDIO_V1_2_2_1_MIDI_SET_FLAG_MUTE) != 0;
	} else {
		setTempo = (params[TG_AUDIO_V1_2_2_2_MIDI_SET_OFFSET_TEMPO] != 0);
		setMutes = (params[TG_AUDIO_V1_2_2_2_MIDI_SET_OFFSET_MUTE] != 0);
	}

	if (setTempo) {
		midi->tempoDenominator = READ_LE_UINT16(params + TG_AUDIO_V1_2_3_MIDI_SET_OFFSET_DENOMINATOR);
		midi->tempoNumerator = READ_LE_UINT16(params + TG_AUDIO_V1_2_3_MIDI_SET_OFFSET_NUMERATOR);
	}

	if (!setMutes)
		return;

	uint16 channelMask = READ_LE_UINT16(params + TG_AUDIO_V1_2_2_1_MIDI_SET_OFFSET_CHANNELS);
	uint16 muteBits = READ_LE_UINT16(params + TG_AUDIO_V1_2_2_1_MIDI_SET_OFFSET_MUTE_BITS);

	for (byte channel = 0; channel < TG_AUDIO_V1_2_2_1_MIDI_NUM_CHANNELS; channel++) {
		if ((channelMask & 1) != 0) {
			bool muted = ((muteBits & 1) != 0);

			if (muted != midi->channelMuted[channel]) {
				midi->channelMuted[channel] = muted;

				if (muted)
					sendMidiAllNotesOff(channel);
			}
		}

		channelMask >>= 1;
		muteBits >>= 1;
	}
}

void AudioLibrary_v1_2::audioBreakMidiLoops(uint16 handle) {
	MidiObject *midi = validateMidiHandle(handle);

	if (midi)
		midi->loops = 0;
}

bool AudioLibrary_v1_2::startMidiSequence(const byte *params, MidiObject *midi) {
	if (_engine->_version >= kVersionV1_2_6_1) {
		if (_currentMidi)
			stopMidiSequence(_currentMidi);

		if (!openAndConfigureMidiOut())
			return false;
	}

	midi->loops = (int16)READ_LE_UINT16(params + TG_AUDIO_V1_2_2_1_MIDI_PLAY_OFFSET_LOOPS);
	midi->initialLoops = midi->loops;

	if (_engine->_version < kVersionV1_2_2_2)
		midi->notifyArmed = (params[TG_AUDIO_V1_2_2_1_MIDI_PLAY_OFFSET_NOTIFY] & TG_AUDIO_V1_2_2_1_MIDI_PLAY_NOTIFY_MASK) != 0;
	else
		midi->notifyArmed = (params[TG_AUDIO_V1_2_2_1_MIDI_PLAY_OFFSET_NOTIFY] != 0);

	if (midi->notifyArmed)
		midi->notifyLParam = READ_LE_UINT16(params + TG_AUDIO_V1_2_3_MIDI_PLAY_OFFSET_PARAM);
	else
		midi->notifyLParam = 0;

	if (_engine->_version < kVersionV1_2_2_2) {
		if (_mute)
			return false;

		if (!openAndConfigureMidiOut())
			return false;

		rewindMidiObject(midi);

		if (_cdDeviceId != 0 && !midi->isStatic)
			stopCDIfOwned();

		if (_currentMidi)
			stopMidiSequence(_currentMidi);

		midi->loops = midi->initialLoops;
		_currentMidi = midi;
		return true;
	}

	midi->index = 0;

	if (_engine->_version < kVersionV1_2_6_1) {
		if (_currentMidi)
			stopMidiSequence(_currentMidi);

		if (!openAndConfigureMidiOut())
			return false;
	}

	_currentMidi = midi;

	bool stepPaused = false;

	if (_engine->_version == kVersionV1_2_8)
		stepPaused = (static_cast<AudioLibrary_v1_2_3 *>(this)->_midiPauseCount != 0);
	else if (_engine->_version >= kVersionV1_2_3)
		stepPaused = _paused;

	if (!stepPaused)
		midiSequencerStep(midi);

	return true;
}

void AudioLibrary_v1_2::postMidiCompletionMessage(MidiObject *midi) {
	if (_engine->_version == kVersionV1_2_2_1 && _currentMidi == midi)
		_currentMidi = nullptr;

	if (midi->notifyArmed && _hwnd != 0) {
		midi->notifyArmed = false;
		engine()->postMessage(TG_AUDIO_V1_2_NOTIFY_MESSAGE, midi->notifyWParam, midi->notifyLParam);
	}
}

bool AudioLibrary_v1_2::midiSequenceEndOrLoop(MidiObject *midi) {
	if (midi->loops == 0) {
		postMidiCompletionMessage(midi);

		if (_engine->_version >= kVersionV1_2_2_3)
			_currentMidi = nullptr;

		return false;
	}

	if (midi->loops > 0)
		midi->loops--;

	if (_engine->_version < kVersionV1_2_2_2) {
		if (midi->isStatic) {
			midi->available = midi->bufferSize;
			midi->readPos = 0;
		}

		midi->consumed = 0;
	} else {
		midi->index = 0;
	}

	return true;
}

void AudioLibrary_v1_2::midiSequencerStep(MidiObject *midi) {
	if (midi->consumed >= midi->size) {
		if (!midiSequenceEndOrLoop(midi))
			return;
	}

	const byte *data = midi->data + midi->readPos;
	uint32 remaining = midi->available >> 2;

	while (remaining != 0) {
		remaining--;

		uint32 event = READ_LE_UINT32(data);

		data += 4;
		midi->consumed += 4;
		midi->readPos += 4;
		midi->available -= 4;

		if (((event >> 16) & TG_AUDIO_V1_2_2_1_MIDI_DELAY_FLAGS) != 0) {
			uint32 delay = event & TG_AUDIO_V1_2_2_1_MIDI_DELAY_VALUE_MASK;

			if (midi->tempoDenominator != 0)
				delay = (delay * midi->tempoNumerator) / midi->tempoDenominator;

			midi->nextEventTime = _midiServiceTime + delay;

			if (midi->nextEventTime >= _midiServiceTime)
				return;

			continue;
		}

		byte channel = (byte)(event & 0x0F);

		if (midi->channelMuted[channel] && (event & 0xF0) == TG_AUDIO_V1_2_2_1_MIDI_NOTE_ON)
			continue;

		_midiOut->send(event);
	}
}

void AudioLibrary_v1_2::refillStreamedMidiObjects() {
	for (uint16 i = 0; i < _midiCount; i++) {
		MidiObject *midi = _midiObjects[i];

		if (midi->enabled && !midi->isStatic && !midi->endOfSource && midi->available <= midi->lowWater)
			compactAndRefillMidiBuffer(midi);
	}
}

void AudioLibrary_v1_2::pollMidiPlayback() {
	MidiObject *midi = _currentMidi;

	if (_midiOut && midi && !_paused) {
		_midiServiceTime = _engine->getEngineMillis();

		if (midi->nextEventTime <= _midiServiceTime) {
			midiSequencerStep(midi);
			return;
		}
	}

	if (_streamedMidiCount != 0 && _cdDeviceId == 0)
		refillStreamedMidiObjects();
}

void AudioLibrary_v1_2::updateMidiEnabledFlags(uint16 on) {
	if (_mute)
		on = 0;

	for (uint16 i = 0; i < _midiCount; i++) {
		MidiObject *midi = _midiObjects[i];

		if (on == 0)
			midi->enabled = false;
		else if (_cdDeviceId == 0)
			midi->enabled = true;
		else
			midi->enabled = midi->isStatic;
	}
}

void AudioLibrary_v1_2_3::midiSequencerStep(MidiObject *midi) {
	for (;;) {
		if (midi->index >= midi->count) {
			if (!midiSequenceEndOrLoop(midi))
				return;
		}

		uint32 event = READ_LE_UINT32(midi->data + midi->index * 4);

		midi->index++;

		if (((event >> 16) & TG_AUDIO_V1_2_2_1_MIDI_DELAY_FLAGS) != 0) {
			uint32 delay = event & TG_AUDIO_V1_2_2_1_MIDI_DELAY_VALUE_MASK;

			if (midi->tempoDenominator != 0)
				delay = (delay * midi->tempoNumerator) / midi->tempoDenominator;

			uint16 timerDelay = (uint16)delay;
			uint16 highWord = (uint16)(delay >> 16);

			if (highWord == 0 && timerDelay == 0)
				continue;

			if (_engine->_version >= kVersionV1_2_2_3 && highWord == 0 && timerDelay < TG_AUDIO_V1_2_2_3_MIDI_MIN_DELAY)
				timerDelay = TG_AUDIO_V1_2_2_3_MIDI_MIN_DELAY;

			uint32 now = _midiCatchUp ? _midiCatchUpTime : _engine->getEngineMillis();

			if (_midiTimerArmed && _engine->_version < kVersionV1_2_3)
				_midiLostTimers.push_back(_midiTimerExpiry);

			_midiTimerArmed = true;
			_midiTimerExpiry = now + timerDelay;
			return;
		}

		if ((event & 0x0F) == TG_AUDIO_V1_2_2_2_MIDI_DRUM_CHANNEL && _remapMidiDrums)
			event |= TG_AUDIO_V1_2_2_2_MIDI_REMAPPED_CHANNEL;

		byte channel = (byte)(event & 0x0F);

		if (midi->channelMuted[channel] && (event & 0xF0) == TG_AUDIO_V1_2_2_1_MIDI_NOTE_ON)
			continue;

		_midiOut->send(event);
	}
}

void AudioLibrary_v1_2_3::midiClearCurrentAndKillTimer() {
	_currentMidi = nullptr;
	_midiTimerArmed = false;
}

void AudioLibrary_v1_2_3::audioTimerProc() {
	_midiTimerArmed = false;

	if (_currentMidi && _midiOut)
		midiSequencerStep(_currentMidi);
}

// The original arms a multimedia timer for every MIDI delay; here the delays expire from audioService()
void AudioLibrary_v1_2_3::midiServiceTick() {
	for (;;) {
		uint32 now = _engine->getEngineMillis();
		bool timerDue = (_midiTimerArmed && now >= _midiTimerExpiry);
		int32 lost = -1;

		for (uint32 i = 0; i < _midiLostTimers.size(); i++) {
			if (now >= _midiLostTimers[i] && (lost < 0 || _midiLostTimers[i] < _midiLostTimers[lost]))
				lost = i;
		}

		if (lost >= 0 && (!timerDue || _midiLostTimers[lost] < _midiTimerExpiry)) {
			_midiCatchUpTime = _midiLostTimers[lost];
			_midiLostTimers.remove_at(lost);

			if (_midiTimerArmed)
				_midiLostTimers.push_back(_midiTimerExpiry);
		} else if (timerDue) {
			_midiCatchUpTime = _midiTimerExpiry;
		} else {
			break;
		}

		_midiCatchUp = true;
		audioTimerProc();
		_midiCatchUp = false;
	}
}

} // End of namespace Topgun
