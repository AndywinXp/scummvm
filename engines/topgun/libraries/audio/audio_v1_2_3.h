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

#ifndef TOPGUN_LIBRARIES_AUDIO_AUDIO_V1_2_3_H
#define TOPGUN_LIBRARIES_AUDIO_AUDIO_V1_2_3_H

#include "topgun/libraries/audio/audio_v1_2.h"

// Library
#define TG_AUDIO_V1_2_3_VERSION                   0x0400

// INI value of MidiVolume that leaves the device volume alone
#define TG_AUDIO_V1_2_3_INI_MIDI_VOLUME_UNSET     0
#define TG_AUDIO_V1_2_6_INI_MIDI_VOLUME_UNSET     1

// Source procedures
#define TG_AUDIO_V1_2_3_PROC_NONE                 0
#define TG_AUDIO_V1_2_3_PROC_RESOURCE_WAVE        1
#define TG_AUDIO_V1_2_3_PROC_MOVIE_AUDIO          2
#define TG_AUDIO_V1_2_3_PROC_RESOURCE_MIDI        3

// Default wave format: 22050 Hz, mono, 16 bit
#define TG_AUDIO_V1_2_3_DEFAULT_FORMAT_TAG        0x40
#define TG_AUDIO_V1_2_3_DEFAULT_SAMPLE_RATE       22050
#define TG_AUDIO_V1_2_3_DEFAULT_BYTES_PER_SECOND  44100
#define TG_AUDIO_V1_2_3_DEFAULT_BLOCK_ALIGN       2
#define TG_AUDIO_V1_2_3_DEFAULT_BITS_PER_SAMPLE   16

// Sound creation header
#define TG_AUDIO_V1_2_3_INFO_OFFSET_LENGTH        0x00
#define TG_AUDIO_V1_2_3_INFO_OFFSET_PRELOAD       0x04
#define TG_AUDIO_V1_2_3_INFO_OFFSET_EXEMPT        0x05
#define TG_AUDIO_V1_2_3_INFO_OFFSET_ADPCM         0x06
#define TG_AUDIO_V1_2_3_INFO_OFFSET_PRIORITY      0x08
#define TG_AUDIO_V1_2_3_INFO_OFFSET_NOTIFY        0x0A
#define TG_AUDIO_V1_2_3_INFO_OFFSET_FORMAT        0x0C

// Play parameters block (0x18 bytes)
#define TG_AUDIO_V1_2_3_PLAY_OFFSET_LOOPS         0x00
#define TG_AUDIO_V1_2_3_PLAY_OFFSET_SET_VOLUME    0x02
#define TG_AUDIO_V1_2_3_PLAY_OFFSET_NOTIFY        0x03
#define TG_AUDIO_V1_2_3_PLAY_OFFSET_NO_FLUSH      0x04
#define TG_AUDIO_V1_2_3_PLAY_OFFSET_VOLUME        0x05
#define TG_AUDIO_V1_2_3_PLAY_OFFSET_FADE_IN       0x06
#define TG_AUDIO_V1_2_3_PLAY_OFFSET_FADE_OUT      0x0A
#define TG_AUDIO_V1_2_3_PLAY_OFFSET_RANDOM_MIN    0x0E
#define TG_AUDIO_V1_2_3_PLAY_OFFSET_RANDOM_MAX    0x12
#define TG_AUDIO_V1_2_3_PLAY_OFFSET_NOTIFY_PARAM  0x16

// MIDI creation header
#define TG_AUDIO_V1_2_3_MIDI_HEADER_SIZE          4
#define TG_AUDIO_V1_2_3_MIDI_HEADER_OFFSET_SIZE   0
#define TG_AUDIO_V1_2_3_MIDI_HEADER_OFFSET_NOTIFY 2

// MIDI play parameters block (6 bytes)
#define TG_AUDIO_V1_2_3_MIDI_PLAY_PARAMS_SIZE     6
#define TG_AUDIO_V1_2_3_MIDI_PLAY_OFFSET_LOOPS    0
#define TG_AUDIO_V1_2_3_MIDI_PLAY_OFFSET_NOTIFY   2
#define TG_AUDIO_V1_2_3_MIDI_PLAY_OFFSET_PARAM    4

// MIDI setting block (10 bytes)
#define TG_AUDIO_V1_2_3_MIDI_SET_OFFSET_MUTE      0
#define TG_AUDIO_V1_2_3_MIDI_SET_OFFSET_TEMPO     1
#define TG_AUDIO_V1_2_3_MIDI_SET_OFFSET_CHANNELS  2
#define TG_AUDIO_V1_2_3_MIDI_SET_OFFSET_MUTE_BITS 4
#define TG_AUDIO_V1_2_3_MIDI_SET_OFFSET_DENOMINATOR 6
#define TG_AUDIO_V1_2_3_MIDI_SET_OFFSET_NUMERATOR 8

// MIDI events
#define TG_AUDIO_V1_2_3_MIDI_DELAY_FLAGS          0xF000
#define TG_AUDIO_V1_2_3_MIDI_DELAY_VALUE_MASK     0x7FFFFFFF
#define TG_AUDIO_V1_2_3_MIDI_MIN_DELAY            3
#define TG_AUDIO_V1_2_3_MIDI_NOTE_ON              0x90
#define TG_AUDIO_V1_2_3_MIDI_DRUM_CHANNEL         9
#define TG_AUDIO_V1_2_3_MIDI_REMAPPED_CHANNEL     0x0F
#define TG_AUDIO_V1_2_3_MIDI_ALL_NOTES_OFF        0x7B
#define TG_AUDIO_V1_2_3_MIDI_CONTROL_CHANGE       0xB0

namespace Topgun {

class MidiSequencer;
class TopgunEngine_v1_2_3;

class AudioLibrary_v1_2_3 : public AudioLibrary_v1_2 {
public:
	AudioLibrary_v1_2_3(TopgunEngine_v1_2_3 *engine);
	~AudioLibrary_v1_2_3() override;

	// Library
	uint16 audioInitLibrary(uint16 hwnd, uint16 version);
	void audioExitLibrary() override;
	void audioService() override;
	void audioActivate(uint16 on);
	void audioPause(uint16 paused) override;
	void audioPauseMidi(uint16 paused);
	void audioMute(int16 flag);
	void audioResetLibrary();

	// CD audio
	uint16 audioSetCdVolume(uint32 volume) override;

	// Wave sounds
	uint32 audioGetWaveVolume() override;
	void audioSetWaveVolume(uint32 volume) override;

	// MIDI
	uint16 audioPlayMidi(uint16 midi, const byte *params) override;
	void audioSetMidi(uint16 midi, const byte *params);
	uint16 audioStopMidi(uint16 midi) override;
	uint32 audioGetMidiVolume();
	void audioSetMidiVolume(uint32 volume);
	void applyMidiDeviceVolume() override;

protected:
	// Library
	bool allocAudioContext();
	void initializeAudioSubsystem();
	void readAudioIniSettings();
	void writeAudioIniInt(const char *key, uint16 value);
	void writeAudioIniString(const char *key, const char *value);
	void saveCdVolumeToIni();
	void saveMidiVolumeToIni();
	void saveWaveVolumeToIni();

	// CD audio
	bool openCDAudioDevice() override;
	uint16 closeCDAudioDevice() override;

	// Wave device
	void resetWaveQueueAndEnableAllWaves(uint16 on) override;
	void resetWaveOutQueue() override;
	void retireBuffer() override;
	bool finishWaveOutOpen() override;
	bool openWaveOutDevice(const WaveFormat *format);
	bool openWaveDevice(const WaveFormat *format);
	void waveMuteImpl();
	void pumpAudioPlayback() override;
	void updateWaveEnabledFlags(uint16 on);

	// Wave sounds
	bool initWaveObjectFromDescriptor(const SoundCreateInfo_v1_2 *info, SoundObject *sound) override;
	bool startWavePlayback(SoundObject *sound, const byte *params) override;
	void stopAllNonExemptWaves();
	void activateWaveObject(SoundObject *sound) override;
	void armWaveNotificationEndTime(SoundObject *sound);

	// MIDI objects
	void destroyAllMidiObjects();

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
	void stopMidiSequence(MidiObject *midi) override;
	bool midiSequenceEndOrLoop(MidiObject *midi);
	void midiSequencerStep(MidiObject *midi);
	void midiClearCurrentAndKillTimer();
	void audioTimerProc();
	void midiServiceTick();

	bool _paused = false;
	uint32 _waveMuteSavedVolume = 0;
	bool _remapMidiDrums = false;
	uint16 _iniCdVolume = 0;
	uint16 _iniMidiVolume = 0;
	uint16 _iniWaveVolume = 0;
	uint16 _midiPauseCount = 0;

	// MIDI
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
	TopgunEngine_v1_2_3 *engine() const;
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_AUDIO_AUDIO_V1_2_3_H
