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

	_midiData = (byte *)malloc(size);
	file.read(_midiData, size);
	file.close();

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

	syncVolume();

	_isLooping = false;
	_isPlaying = true;
	return true;
}

void AudioLibrary::playDefaultMidiFile() {
	playMidiFile("efxfire.mid");
}

bool AudioLibrary::sendMidiDeviceCommand(uint16 message, uint32 flags, DeviceParms &parms) {
	if (message == TG_AUDIO_DEV_OPEN) {
		if (!(flags & TG_AUDIO_DEV_OPEN_ELEMENT) || !Common::File::exists(Common::Path(parms.fileName, '/')))
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

AudioLibrary_v1_2_3::MidiObject *AudioLibrary_v1_2_3::allocMidiObject() {
	if (_midiCount >= TG_AUDIO_V1_2_3_MAX_MIDI_OBJECTS)
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

void AudioLibrary_v1_2_3::destroyMidiObject(MidiObject *midi) {
	if (_currentMidi == midi)
		stopMidiSequence(midi);

	removeMidiFromArray(midi);
}

void AudioLibrary_v1_2_3::removeMidiFromArray(MidiObject *midi) {
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

bool AudioLibrary_v1_2_3::initMidiObjectDataSource(const MidiCreateInfo_v1_2_3 *info, MidiObject *midi) {
	midi->callbackHandle = info->handle;
	midi->readProc = info->readProc;
	midi->closeProc = info->closeProc;
	midi->notifyWParam = READ_LE_UINT16(info->header + TG_AUDIO_V1_2_3_MIDI_HEADER_OFFSET_NOTIFY);

	if (_cdDeviceId != 0)
		stopCDIfOwned();

	uint16 size = READ_LE_UINT16(info->header + TG_AUDIO_V1_2_3_MIDI_HEADER_OFFSET_SIZE);

	midi->count = size >> 2;
	midi->data = engine()->utilsLibrary()->tgMalloc(size, 0);

	if (!midi->data)
		return false;

	if (engine()->audioSourceRead(midi->readProc, midi->callbackHandle, midi->data, size) != size)
		return false;

	return invokeAndClearCloseCallback(midi);
}

bool AudioLibrary_v1_2_3::invokeAndClearCloseCallback(MidiObject *midi) {
	if (midi->closeProc != 0) {
		if (_cdDeviceId != 0)
			stopCDIfOwned();

		engine()->audioSourceClose(midi->closeProc, midi->callbackHandle);
		midi->closeProc = 0;
	}

	return true;
}

void AudioLibrary_v1_2_3::destroyAllMidiObjects() {
	uint16 index = 0;

	while (index < _midiCount) {
		destroyMidiObject(_midiObjects[index]);
		index++;
	}

	if (_midiOut)
		closeMidiOutDevice();
}

AudioLibrary_v1_2_3::MidiObject *AudioLibrary_v1_2_3::validateMidiHandle(uint16 handle) {
	for (uint16 i = 0; i < _midiCount; i++) {
		if (_midiObjects[i]->handle == handle)
			return _midiObjects[i];
	}

	return nullptr;
}

void AudioLibrary_v1_2_3::probeMidiOutCapabilities() {
	MidiDriver::DeviceHandle device = MidiDriver::detectDevice(MDT_MIDI | MDT_ADLIB | MDT_PREFER_GM);

	if (MidiDriver::getMusicType(device) == MT_NULL)
		_noMidiDevice = true;
}

bool AudioLibrary_v1_2_3::openMidiOutDevice() {
	if (_noMidiDevice)
		return false;

	if (!_midiOut) {
		_midiOut = new MidiSequencer();
		_midiOriginalVolume = _midiDeviceVolume;
		applyMidiDeviceVolume();
	}

	return true;
}

void AudioLibrary_v1_2_3::setMidiOutVolume(uint32 volume) {
	_midiDeviceVolume = volume;
	applyMidiDeviceVolume();
}

void AudioLibrary_v1_2_3::applyMidiDeviceVolume() {
	if (!_midiOut)
		return;

	uint32 loudest = MAX<uint32>(_midiDeviceVolume & 0xFFFF, _midiDeviceVolume >> 16);
	int level = (int)((loudest * 255 + 32767) / 65535);
	int musicVolume = ConfMan.getBool("mute") ? 0 : ConfMan.getInt("music_volume");

	_midiOut->setVolume(level * musicVolume / 255);
}

void AudioLibrary_v1_2_3::closeMidiOutDevice() {
	if (!_midiOut)
		return;

	midiClearCurrentAndKillTimer();
	resetMidiOutDevice();

	if (_midiOriginalVolume != _midiUserVolume)
		setMidiOutVolume(_midiOriginalVolume);

	delete _midiOut;
	_midiOut = nullptr;
}

void AudioLibrary_v1_2_3::muteMidiOutput() {
	if (_mute) {
		_midiMuteSavedVolume = audioGetMidiVolume();
		audioSetMidiVolume(0);
	} else {
		audioSetMidiVolume(_midiMuteSavedVolume);
	}
}

bool AudioLibrary_v1_2_3::openAndConfigureMidiOut() {
	if (!openMidiOutDevice())
		return false;

	resetMidiOutDevice();

	if (_iniMidiVolume != 0) {
		_midiUserVolume = ((uint32)_iniMidiVolume << 16) | _iniMidiVolume;
		_midiUserVolumeSet = true;
	}

	if (_midiUserVolumeSet && _midiOriginalVolume != _midiUserVolume)
		setMidiOutVolume(_midiUserVolume);

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

void AudioLibrary_v1_2_3::resetMidiOutDevice() {
	if (!_midiOut)
		return;

	for (byte channel = 0; channel < TG_AUDIO_V1_2_3_MIDI_NUM_CHANNELS; channel++)
		sendMidiAllNotesOff(channel);
}

void AudioLibrary_v1_2_3::sendMidiAllNotesOff(byte channel) {
	_midiOut->send((uint32)(channel | TG_AUDIO_V1_2_3_MIDI_CONTROL_CHANGE) | (TG_AUDIO_V1_2_3_MIDI_ALL_NOTES_OFF << 8));
}

uint16 AudioLibrary_v1_2_3::audioCreateMidi(MidiCreateInfo_v1_2_3 *info) {
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

uint16 AudioLibrary_v1_2_3::audioDestroyMidi(uint16 handle) {
	if (handle == 0)
		return 0;

	MidiObject *midi = validateMidiHandle(handle);

	if (midi)
		destroyMidiObject(midi);

	return 0;
}

uint32 AudioLibrary_v1_2_3::audioGetMidiVolume() {
	if (!_midiOut && !openMidiOutDevice())
		return 0;

	if (_midiUserVolumeSet)
		return _midiUserVolume;

	return _midiOriginalVolume;
}

void AudioLibrary_v1_2_3::audioSetMidiVolume(uint32 volume) {
	_midiUserVolume = volume;
	_midiUserVolumeSet = true;

	if (_midiOut)
		setMidiOutVolume(volume);

	_iniMidiVolume = (uint16)volume;
	saveMidiVolumeToIni();
}

uint16 AudioLibrary_v1_2_3::audioPlayMidi(uint16 handle, const byte *params) {
	MidiObject *midi = validateMidiHandle(handle);

	if (!midi)
		return 0;

	byte defaultParams[TG_AUDIO_V1_2_3_MIDI_PLAY_PARAMS_SIZE] = {};

	if (!params)
		params = defaultParams;

	if (!startMidiSequence(params, midi))
		postMidiCompletionMessage(midi);

	return 1;
}

void AudioLibrary_v1_2_3::audioSetMidi(uint16 handle, const byte *params) {
	MidiObject *midi = (handle != 0) ? validateMidiHandle(handle) : _currentMidi;

	if (!midi)
		return;

	if (!params) {
		warning("AudioLibrary_v1_2_3::audioSetMidi(): no parameter block");
		return;
	}

	if (params[TG_AUDIO_V1_2_3_MIDI_SET_OFFSET_TEMPO] != 0) {
		midi->tempoDenominator = READ_LE_UINT16(params + TG_AUDIO_V1_2_3_MIDI_SET_OFFSET_DENOMINATOR);
		midi->tempoNumerator = READ_LE_UINT16(params + TG_AUDIO_V1_2_3_MIDI_SET_OFFSET_NUMERATOR);
	}

	if (params[TG_AUDIO_V1_2_3_MIDI_SET_OFFSET_MUTE] == 0)
		return;

	uint16 channelMask = READ_LE_UINT16(params + TG_AUDIO_V1_2_3_MIDI_SET_OFFSET_CHANNELS);
	uint16 muteBits = READ_LE_UINT16(params + TG_AUDIO_V1_2_3_MIDI_SET_OFFSET_MUTE_BITS);

	for (byte channel = 0; channel < TG_AUDIO_V1_2_3_MIDI_NUM_CHANNELS; channel++) {
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

void AudioLibrary_v1_2_3::audioStopMidi(uint16 handle) {
	MidiObject *midi = (handle != 0) ? validateMidiHandle(handle) : _currentMidi;

	if (midi)
		stopMidiSequence(midi);
}

bool AudioLibrary_v1_2_3::startMidiSequence(const byte *params, MidiObject *midi) {
	midi->loops = (int16)READ_LE_UINT16(params + TG_AUDIO_V1_2_3_MIDI_PLAY_OFFSET_LOOPS);
	midi->notifyArmed = (params[TG_AUDIO_V1_2_3_MIDI_PLAY_OFFSET_NOTIFY] != 0);

	if (midi->notifyArmed)
		midi->notifyLParam = READ_LE_UINT16(params + TG_AUDIO_V1_2_3_MIDI_PLAY_OFFSET_PARAM);
	else
		midi->notifyLParam = 0;

	midi->index = 0;

	if (_currentMidi)
		stopMidiSequence(_currentMidi);

	if (!openAndConfigureMidiOut())
		return false;

	_currentMidi = midi;

	if (!_paused)
		midiSequencerStep(midi);

	return true;
}

void AudioLibrary_v1_2_3::postMidiCompletionMessage(MidiObject *midi) {
	if (midi->notifyArmed && _hwnd != 0) {
		midi->notifyArmed = false;
		engine()->postMessage(TG_AUDIO_V1_2_3_NOTIFY_MESSAGE, midi->notifyWParam, midi->notifyLParam);
	}
}

void AudioLibrary_v1_2_3::stopMidiSequence(MidiObject *midi) {
	midi->notifyArmed = false;

	if (_currentMidi == midi) {
		midiClearCurrentAndKillTimer();
		resetMidiOutDevice();
		postMidiCompletionMessage(midi);
	}
}

bool AudioLibrary_v1_2_3::midiSequenceEndOrLoop(MidiObject *midi) {
	if (midi->loops == 0) {
		postMidiCompletionMessage(midi);
		_currentMidi = nullptr;
		return false;
	}

	if (midi->loops > 0)
		midi->loops--;

	midi->index = 0;
	return true;
}

void AudioLibrary_v1_2_3::midiSequencerStep(MidiObject *midi) {
	for (;;) {
		if (midi->index >= midi->count) {
			if (!midiSequenceEndOrLoop(midi))
				return;
		}

		uint32 event = READ_LE_UINT32(midi->data + midi->index * 4);

		midi->index++;

		if (((event >> 16) & TG_AUDIO_V1_2_3_MIDI_DELAY_FLAGS) != 0) {
			uint32 delay = event & TG_AUDIO_V1_2_3_MIDI_DELAY_VALUE_MASK;

			if (midi->tempoDenominator != 0)
				delay = (delay * midi->tempoNumerator) / midi->tempoDenominator;

			uint16 timerDelay = (uint16)delay;
			uint16 highWord = (uint16)(delay >> 16);

			if (highWord == 0 && timerDelay == 0)
				continue;

			if (highWord == 0 && timerDelay < TG_AUDIO_V1_2_3_MIDI_MIN_DELAY)
				timerDelay = TG_AUDIO_V1_2_3_MIDI_MIN_DELAY;

			uint32 now = _midiCatchUp ? _midiCatchUpTime : _engine->getEngineMillis();

			_midiTimerArmed = true;
			_midiTimerExpiry = now + timerDelay;
			return;
		}

		if ((event & 0x0F) == TG_AUDIO_V1_2_3_MIDI_DRUM_CHANNEL && _remapMidiDrums)
			event |= TG_AUDIO_V1_2_3_MIDI_REMAPPED_CHANNEL;

		byte channel = (byte)(event & 0x0F);

		if (midi->channelMuted[channel] && (event & 0xF0) == TG_AUDIO_V1_2_3_MIDI_NOTE_ON)
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
	while (_midiTimerArmed && _engine->getEngineMillis() >= _midiTimerExpiry) {
		_midiCatchUp = true;
		_midiCatchUpTime = _midiTimerExpiry;
		audioTimerProc();
		_midiCatchUp = false;
	}
}

} // End of namespace Topgun
