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

#ifndef TOPGUN_LIBRARIES_AUDIO_AUDIO_V1_2_1_H
#define TOPGUN_LIBRARIES_AUDIO_AUDIO_V1_2_1_H

#include "topgun/libraries/audio/audio_v1_2.h"

// Library
#define TG_AUDIO_V1_2_1_VERSION                   0x0400
#define TG_AUDIO_V1_2_1_MAX_MIDI_OBJECTS          20

// Source procedures
#define TG_AUDIO_V1_2_1_PROC_NONE                 0
#define TG_AUDIO_V1_2_1_PROC_RESOURCE_WAVE        1
#define TG_AUDIO_V1_2_1_PROC_MOVIE_AUDIO          2
#define TG_AUDIO_V1_2_1_PROC_RESOURCE_MIDI        3

// Default wave format: 22050 Hz, mono, 16 bit
#define TG_AUDIO_V1_2_1_DEFAULT_FORMAT_TAG        0x40
#define TG_AUDIO_V1_2_1_DEFAULT_SAMPLE_RATE       22050
#define TG_AUDIO_V1_2_1_DEFAULT_BYTES_PER_SECOND  44100
#define TG_AUDIO_V1_2_1_DEFAULT_BLOCK_ALIGN       2
#define TG_AUDIO_V1_2_1_DEFAULT_BITS_PER_SAMPLE   16

// Sound creation header
#define TG_AUDIO_V1_2_1_INFO_OFFSET_LENGTH        0x00
#define TG_AUDIO_V1_2_1_INFO_OFFSET_PRELOAD       0x04
#define TG_AUDIO_V1_2_1_INFO_OFFSET_EXEMPT        0x05
#define TG_AUDIO_V1_2_1_INFO_OFFSET_ADPCM         0x06
#define TG_AUDIO_V1_2_1_INFO_OFFSET_PRIORITY      0x08
#define TG_AUDIO_V1_2_1_INFO_OFFSET_NOTIFY        0x0A
#define TG_AUDIO_V1_2_1_INFO_OFFSET_FORMAT        0x0C

// Play parameters block (0x18 bytes)
#define TG_AUDIO_V1_2_1_PLAY_OFFSET_LOOPS         0x00
#define TG_AUDIO_V1_2_1_PLAY_OFFSET_SET_VOLUME    0x02
#define TG_AUDIO_V1_2_1_PLAY_OFFSET_NOTIFY        0x03
#define TG_AUDIO_V1_2_1_PLAY_OFFSET_NO_FLUSH      0x04
#define TG_AUDIO_V1_2_1_PLAY_OFFSET_VOLUME        0x05
#define TG_AUDIO_V1_2_1_PLAY_OFFSET_FADE_IN       0x06
#define TG_AUDIO_V1_2_1_PLAY_OFFSET_FADE_OUT      0x0A
#define TG_AUDIO_V1_2_1_PLAY_OFFSET_RANDOM_MIN    0x0E
#define TG_AUDIO_V1_2_1_PLAY_OFFSET_RANDOM_MAX    0x12
#define TG_AUDIO_V1_2_1_PLAY_OFFSET_NOTIFY_PARAM  0x16

// MIDI creation header
#define TG_AUDIO_V1_2_1_MIDI_HEADER_SIZE          4
#define TG_AUDIO_V1_2_1_MIDI_HEADER_OFFSET_SIZE   0
#define TG_AUDIO_V1_2_1_MIDI_HEADER_OFFSET_NOTIFY 2

// MIDI play parameters block (6 bytes)
#define TG_AUDIO_V1_2_1_MIDI_PLAY_PARAMS_SIZE     6
#define TG_AUDIO_V1_2_1_MIDI_PLAY_OFFSET_LOOPS    0
#define TG_AUDIO_V1_2_1_MIDI_PLAY_OFFSET_NOTIFY   2
#define TG_AUDIO_V1_2_1_MIDI_PLAY_OFFSET_PARAM    4

// MIDI setting block (10 bytes)
#define TG_AUDIO_V1_2_1_MIDI_SET_OFFSET_MUTE      0
#define TG_AUDIO_V1_2_1_MIDI_SET_OFFSET_TEMPO     1
#define TG_AUDIO_V1_2_1_MIDI_SET_OFFSET_CHANNELS  2
#define TG_AUDIO_V1_2_1_MIDI_SET_OFFSET_MUTE_BITS 4
#define TG_AUDIO_V1_2_1_MIDI_SET_OFFSET_DENOMINATOR 6
#define TG_AUDIO_V1_2_1_MIDI_SET_OFFSET_NUMERATOR 8

// MIDI events
#define TG_AUDIO_V1_2_1_MIDI_NUM_CHANNELS         16
#define TG_AUDIO_V1_2_1_MIDI_DELAY_FLAGS          0xF000
#define TG_AUDIO_V1_2_1_MIDI_DELAY_VALUE_MASK     0x7FFFFFFF
#define TG_AUDIO_V1_2_1_MIDI_MIN_DELAY            3
#define TG_AUDIO_V1_2_1_MIDI_NOTE_ON              0x90
#define TG_AUDIO_V1_2_1_MIDI_DRUM_CHANNEL         9
#define TG_AUDIO_V1_2_1_MIDI_REMAPPED_CHANNEL     0x0F
#define TG_AUDIO_V1_2_1_MIDI_ALL_NOTES_OFF        0x7B
#define TG_AUDIO_V1_2_1_MIDI_CONTROL_CHANGE       0xB0

namespace Topgun {

class MidiSequencer;
class TopgunEngine_v1_2_1;

struct MidiCreateInfo_v1_2_1 {
	byte header[TG_AUDIO_V1_2_1_MIDI_HEADER_SIZE];
	uint16 readProc;
	uint16 closeProc;
	uint16 handle;

	MidiCreateInfo_v1_2_1() : readProc(0), closeProc(0), handle(0) {
		for (uint32 i = 0; i < TG_AUDIO_V1_2_1_MIDI_HEADER_SIZE; i++)
			header[i] = 0;
	}
};

class AudioLibrary_v1_2_1 : public AudioLibrary_v1_2 {
public:
	AudioLibrary_v1_2_1(TopgunEngine_v1_2_1 *engine);
	~AudioLibrary_v1_2_1() override;

	// Library
	uint16 audioInitLibrary(uint16 hwnd, uint16 version);
	void audioExitLibrary() override;
	void audioService() override;
	void audioActivate(uint16 on);
	void audioPause(uint16 paused);
	void audioMute(int16 flag);
	void audioResetLibrary();

	// Wave sounds
	uint32 audioGetWaveVolume();

	// MIDI
	uint16 audioCreateMidi(MidiCreateInfo_v1_2_1 *info);
	uint16 audioDestroyMidi(uint16 midi);
	uint16 audioPlayMidi(uint16 midi, const byte *params);
	void audioSetMidi(uint16 midi, const byte *params);
	void audioStopMidi(uint16 midi);
	uint32 audioGetMidiVolume();
	void audioSetMidiVolume(uint32 volume);
	void applyMidiDeviceVolume();

protected:
	struct MidiObject {
		uint16 handle;
		byte *data;
		uint16 index;
		uint16 count;
		uint16 readProc;
		uint16 closeProc;
		uint16 callbackHandle;
		uint16 notifyWParam;
		uint16 notifyLParam;
		uint16 tempoNumerator;
		uint16 tempoDenominator;
		int16 loops;
		bool notifyArmed;
		bool channelMuted[TG_AUDIO_V1_2_1_MIDI_NUM_CHANNELS];

		MidiObject() : handle(0), data(nullptr), index(0), count(0), readProc(0), closeProc(0), callbackHandle(0),
			notifyWParam(0), notifyLParam(0), tempoNumerator(0), tempoDenominator(0), loops(0), notifyArmed(false) {
			for (uint32 i = 0; i < TG_AUDIO_V1_2_1_MIDI_NUM_CHANNELS; i++)
				channelMuted[i] = false;
		}
	};

	// Library
	bool allocAudioContext();
	void freeAudioContext();
	void initializeAudioSubsystem();
	void readAudioIniSettings();
	void writeAudioIniInt(const char *key, uint16 value);
	void writeAudioIniString(const char *key, const char *value);
	void saveMidiVolumeToIni();

	// CD audio
	uint16 closeCDAudioDevice() override;

	// Wave device
	void resetWaveQueueAndEnableAllWaves(uint16 on) override;
	void resetWaveOutQueue() override;
	void retireBuffer() override;
	bool finishWaveOutOpen() override;
	bool openWaveOutDevice(const WaveFormat *format);
	bool openWaveDevice(const WaveFormat *format);
	void waveMuteImpl();
	void pauseAllPlayback(uint16 paused);
	void pumpAudioPlayback() override;
	void updateWaveEnabledFlags(uint16 on);

	// Wave sounds
	bool initWaveObjectFromDescriptor(const SoundCreateInfo_v1_2 *info, SoundObject *sound) override;
	bool startWavePlayback(SoundObject *sound, const byte *params) override;
	void scheduleRandomRepeatPlayback(SoundObject *sound, const byte *params);
	void stopAllNonExemptWaves();
	void activateWaveObject(SoundObject *sound);
	void armWaveNotificationEndTime(SoundObject *sound);

	// MIDI objects
	MidiObject *allocMidiObject();
	void destroyMidiObject(MidiObject *midi);
	void removeMidiFromArray(MidiObject *midi);
	bool initMidiObjectDataSource(const MidiCreateInfo_v1_2_1 *info, MidiObject *midi);
	bool invokeAndClearCloseCallback(MidiObject *midi);
	void destroyAllMidiObjects();
	MidiObject *validateMidiHandle(uint16 handle);

	// MIDI device
	void probeMidiOutCapabilities();
	bool openMidiOutDevice();
	void setMidiOutVolume(uint32 volume);
	void closeMidiOutDevice();
	void muteMidiOutput();
	bool openAndConfigureMidiOut();
	void pauseOrResumeMidiPlayback(uint16 paused);
	void resetMidiOutDevice();
	void sendMidiAllNotesOff(byte channel);

	// MIDI playback
	bool startMidiSequence(const byte *params, MidiObject *midi);
	void postMidiCompletionMessage(MidiObject *midi);
	void stopMidiSequence(MidiObject *midi);
	bool midiSequenceEndOrLoop(MidiObject *midi);
	void midiSequencerStep(MidiObject *midi);
	void midiClearCurrentAndKillTimer();
	void audioTimerProc();
	void midiServiceTick();

	bool _paused = false;
	int16 _maxPriority = 0;
	uint32 _waveMuteSavedVolume = 0;
	bool _remapMidiDrums = false;
	uint16 _iniMidiVolume = 0;

	// MIDI
	MidiObject *_midiObjects[TG_AUDIO_V1_2_1_MAX_MIDI_OBJECTS] = {};
	uint16 _midiCount = 0;
	uint16 _nextMidiHandle = 1;
	MidiObject *_currentMidi = nullptr;
	MidiSequencer *_midiOut = nullptr;
	bool _noMidiDevice = false;
	uint32 _midiDeviceVolume = 0xFFFFFFFF;
	uint32 _midiOriginalVolume = 0;
	uint32 _midiUserVolume = 0;
	uint32 _midiMuteSavedVolume = 0;
	bool _midiUserVolumeSet = false;
	bool _midiTimerArmed = false;
	uint32 _midiTimerExpiry = 0;
	bool _midiCatchUp = false;
	uint32 _midiCatchUpTime = 0;

private:
	TopgunEngine_v1_2_1 *engine() const;
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_AUDIO_AUDIO_V1_2_1_H
