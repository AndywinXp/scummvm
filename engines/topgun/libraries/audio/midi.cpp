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
#include "common/file.h"
#include "common/path.h"
#include "common/textconsole.h"

#include "topgun/libraries/audio/audio.h"
#include "topgun/libraries/audio/midi.h"

namespace Topgun {

static const char *const kFireMidiFile = "efxfire.mid";

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
	if (kFireMidiFile[0] != 0)
		playMidiFile(kFireMidiFile);
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

} // End of namespace Topgun
