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

#ifndef TOPGUN_LIBRARIES_AUDIO_AUDIO_V1_2_H
#define TOPGUN_LIBRARIES_AUDIO_AUDIO_V1_2_H

#include "topgun/libraries/audio/audio.h"

// Library
#define TG_AUDIO_V1_2_VERSION                 0x0200
#define TG_AUDIO_V1_2_MAX_SOUNDS              50
#define TG_AUDIO_V1_2_SOUND_TABLE_SIZE        100
#define TG_AUDIO_V1_2_MIN_WAVE_BUFFERS        4
#define TG_AUDIO_V1_2_MAX_WAVE_BUFFERS        8
#define TG_AUDIO_V1_2_DEFAULT_WAVE_BUFFERS    8
#define TG_AUDIO_V1_2_DEFAULT_FORMAT_MASK     0xFF
#define TG_AUDIO_V1_2_TASK_ID                 1
#define TG_AUDIO_V1_2_NOTIFY_MESSAGE          0x52C

// Wave device
#define TG_WAVEOUT_V1_2_BUFFER_SIZE           0x2000
#define TG_WAVEOUT_V1_2_HEADER_PREPARED       0x02
#define TG_WAVEOUT_V1_2_SUPPORT_SYNC          0x10
#define TG_WAVEOUT_V1_2_ALL_FORMATS           0x0FFF

// Sound creation block
#define TG_AUDIO_V1_2_PROC_NONE               0
#define TG_AUDIO_V1_2_PROC_RESOURCE_WAVE      1
#define TG_AUDIO_V1_2_PROC_MOVIE_AUDIO        2
#define TG_AUDIO_V1_2_CREATE_HEADER_SIZE      0x18
#define TG_AUDIO_V1_2_INFO_OFFSET_LENGTH      0x10
#define TG_AUDIO_V1_2_INFO_OFFSET_ID          0x14
#define TG_AUDIO_V1_2_INFO_OFFSET_FLAGS       0x16
#define TG_AUDIO_V1_2_INFO_FLAG_OVERLAY       0x01
#define TG_AUDIO_V1_2_INFO_FLAG_ADPCM         0x02
#define TG_AUDIO_V1_2_INFO_FLAG_PRELOAD       0x08

// Play parameters block (0x16 bytes)
#define TG_AUDIO_V1_2_PLAY_PARAMS_SIZE        0x16
#define TG_AUDIO_V1_2_PLAY_OFFSET_LOOPS       0x00
#define TG_AUDIO_V1_2_PLAY_OFFSET_FLAGS       0x02
#define TG_AUDIO_V1_2_PLAY_OFFSET_FADE_IN     0x04
#define TG_AUDIO_V1_2_PLAY_OFFSET_FADE_OUT    0x08
#define TG_AUDIO_V1_2_PLAY_OFFSET_RANDOM_MIN  0x0C
#define TG_AUDIO_V1_2_PLAY_OFFSET_RANDOM_MAX  0x10
#define TG_AUDIO_V1_2_PLAY_OFFSET_NOTIFY      0x14
#define TG_AUDIO_V1_2_PLAY_FLAG_NOTIFY        0x02
#define TG_AUDIO_V1_2_PLAY_FLAG_KEEP_OTHERS   0x04

// Play flags of the later versions
#define TG_AUDIO_V1_2_1_PLAY_FLAG_SET_VOLUME  0x01
#define TG_AUDIO_V1_2_1_PLAY_VOLUME_SHIFT     3
#define TG_AUDIO_V1_2_1_PLAY_VOLUME_MASK      0x0F

// Sound information block (0x1A bytes)
#define TG_AUDIO_V1_2_SOUND_INFO_SIZE         0x1A
#define TG_AUDIO_V1_2_SOUND_INFO_LENGTH       0x10
#define TG_AUDIO_V1_2_SOUND_INFO_DECODED      0x14
#define TG_AUDIO_V1_2_SOUND_INFO_VOLUME       0x18
#define TG_AUDIO_V1_2_SOUND_INFO_ACTIVE       0x19

// IMA ADPCM
#define TG_AUDIO_V1_2_ADPCM_MAX_INDEX         0x2C0

// Device format of the wave volume query (German and French TuneLand)
#define TG_AUDIO_V1_2_2_DEFAULT_FORMAT_TAG         1
#define TG_AUDIO_V1_2_2_DEFAULT_SAMPLE_RATE        11025
#define TG_AUDIO_V1_2_2_DEFAULT_BYTES_PER_SECOND   22050
#define TG_AUDIO_V1_2_2_DEFAULT_BLOCK_ALIGN        2
#define TG_AUDIO_V1_2_2_DEFAULT_BITS_PER_SAMPLE    16

// Peter and the Wolf
#define TG_AUDIO_V1_2_3_VERSION                    0x0300
#define TG_AUDIO_V1_2_3_MAX_MIDI_OBJECTS           20
#define TG_AUDIO_V1_2_3_MIDI_HEADER_SIZE           6
#define TG_AUDIO_V1_2_3_MIDI_HEADER_OFFSET_SIZE    0
#define TG_AUDIO_V1_2_3_MIDI_HEADER_OFFSET_NOTIFY  4

// Sound creation block
#define TG_AUDIO_V1_2_3_1_INFO_OFFSET_LENGTH       0x00
#define TG_AUDIO_V1_2_3_1_INFO_OFFSET_FLAGS        0x04
#define TG_AUDIO_V1_2_3_1_INFO_OFFSET_ID           0x06
#define TG_AUDIO_V1_2_3_1_INFO_OFFSET_FORMAT       0x08
#define TG_AUDIO_V1_2_3_1_INFO_FLAG_PRELOAD        0x01
#define TG_AUDIO_V1_2_3_1_INFO_FLAG_OVERLAY        0x02
#define TG_AUDIO_V1_2_3_1_INFO_FLAG_ADPCM          0x04

// MIDI creation header
#define TG_AUDIO_V1_2_3_1_MIDI_HEADER_SIZE         8
#define TG_AUDIO_V1_2_3_1_MIDI_HEADER_OFFSET_SIZE  0
#define TG_AUDIO_V1_2_3_1_MIDI_HEADER_OFFSET_FLAGS 4
#define TG_AUDIO_V1_2_3_1_MIDI_HEADER_OFFSET_NOTIFY 6
#define TG_AUDIO_V1_2_3_1_MIDI_HEADER_FLAG_STATIC  0x01

#define TG_AUDIO_V1_2_3_1_MIDI_BUFFER_SIZE         0x8000
#define TG_AUDIO_V1_2_3_1_MIDI_LOW_WATER           0x6000

// MIDI play parameters
#define TG_AUDIO_V1_2_3_1_MIDI_PLAY_PARAMS_SIZE    8
#define TG_AUDIO_V1_2_3_1_MIDI_PLAY_OFFSET_LOOPS   0
#define TG_AUDIO_V1_2_3_1_MIDI_PLAY_OFFSET_NOTIFY  2
#define TG_AUDIO_V1_2_3_1_MIDI_PLAY_NOTIFY_MASK    0x01

// MIDI settings block (10 bytes)
#define TG_AUDIO_V1_2_3_1_MIDI_SET_OFFSET_FLAGS    0
#define TG_AUDIO_V1_2_3_1_MIDI_SET_FLAG_MUTE       0x01
#define TG_AUDIO_V1_2_3_1_MIDI_SET_FLAG_TEMPO      0x02
#define TG_AUDIO_V1_2_3_1_MIDI_SET_OFFSET_CHANNELS 2
#define TG_AUDIO_V1_2_3_1_MIDI_SET_OFFSET_MUTE_BITS 4

// MIDI events
#define TG_AUDIO_V1_2_3_1_MIDI_NUM_CHANNELS        16
#define TG_AUDIO_V1_2_3_1_MIDI_DELAY_FLAGS         0xF000
#define TG_AUDIO_V1_2_3_1_MIDI_DELAY_VALUE_MASK    0x7FFFFFFF
#define TG_AUDIO_V1_2_3_1_MIDI_NOTE_ON             0x90
#define TG_AUDIO_V1_2_3_1_MIDI_ALL_NOTES_OFF       0x7B
#define TG_AUDIO_V1_2_3_1_MIDI_CONTROL_CHANGE      0xB0

#define TG_AUDIO_V1_2_3_3_MAX_SOUNDS          100
#define TG_AUDIO_V1_2_4_CREATE_HEADER_SIZE    0x1C
#define TG_AUDIO_V1_2_4_PLAY_PARAMS_SIZE      0x18

namespace Topgun {

class TopgunEngine_v1_2;

struct AdpcmState {
	int32 predictorLeft;
	int32 predictorRight;
	uint16 indexLeft;
	uint16 indexRight;

	AdpcmState() : predictorLeft(0), predictorRight(0), indexLeft(0), indexRight(0) {}
};

struct SoundCreateInfo_v1_2 {
	byte header[TG_AUDIO_V1_2_4_CREATE_HEADER_SIZE];
	uint16 readProc;
	uint16 rewindProc;
	uint16 closeProc;
	uint16 handle;

	SoundCreateInfo_v1_2() : readProc(0), rewindProc(0), closeProc(0), handle(0) {
		for (uint32 i = 0; i < TG_AUDIO_V1_2_4_CREATE_HEADER_SIZE; i++)
			header[i] = 0;
	}
};

struct MidiCreateInfo_v1_2_4 {
	byte header[TG_AUDIO_V1_2_3_1_MIDI_HEADER_SIZE];
	uint16 readProc;
	uint16 rewindProc;
	uint16 closeProc;
	uint16 handle;

	MidiCreateInfo_v1_2_4() : readProc(0), rewindProc(0), closeProc(0), handle(0) {
		for (uint32 i = 0; i < TG_AUDIO_V1_2_3_1_MIDI_HEADER_SIZE; i++)
			header[i] = 0;
	}
};

class AudioLibrary_v1_2 : public AudioLibrary {
public:
	AudioLibrary_v1_2(TopgunEngine_v1_2 *engine);
	~AudioLibrary_v1_2() override;

	// Library
	uint16 audioInitLibrary(uint16 hwnd, uint16 version);
	void audioExitLibrary() override;
	void audioService() override;
	void audioActivate(uint16 on);
	virtual void audioPause(uint16 paused);

	// Wave device
	void audioCloseWave();
	uint16 audioOpenWave(const WaveFormat *format);
	uint16 audioMasterFadeIn(uint32 milliseconds);
	uint16 audioMasterFadeOut(uint32 milliseconds);
	uint16 audioGetWaveDeviceInfo(uint32 *info);
	virtual uint32 audioGetWaveVolume();
	virtual void audioSetWaveVolume(uint32 volume);
	void audioSetWaveFormats(uint16 mask);
	void audioMute(int16 muted);

	// MIDI
	uint16 audioCreateMidi(MidiCreateInfo_v1_2_4 *info);
	uint16 audioDestroyMidi(uint16 midi);
	uint16 audioPlayMidi(uint16 midi, const byte *params);
	uint16 audioStopMidi(uint16 midi);
	void audioSetMidi(uint16 midi, const byte *params);
	void audioBreakMidiLoops(uint16 midi);
	uint32 audioGetMidiVolume();
	void audioSetMidiVolume(uint32 volume);
	void applyMidiDeviceVolume();

	// Master volume
	uint32 audioGetMasterVolume();
	void audioSetMasterVolume(uint32 volume);

	// Wave sounds
	uint16 audioCreateWave(SoundCreateInfo_v1_2 *info);
	uint16 audioDestroyWave(uint16 sound);
	uint16 audioPlayWave(uint16 sound, const byte *params);
	void audioStopAllWaves(const byte *params);
	uint16 audioStopWave(uint16 sound, const byte *params);
	uint16 audioGetWaveSoundInfo(uint16 sound, byte *info);
	uint16 audioSetWaveSoundVolume(uint16 sound, uint32 volume);
	void audioBreakWaveLoops(uint16 sound);
	void audioClearNotifications();

	// CD audio
	uint16 audioPlayCdFadeIn(uint32 milliseconds, uint32 from, uint32 to);
	uint16 audioCdFadeOut(uint32 milliseconds);
	uint32 audioGetCdPosition();
	uint8 audioGetCdTrackCount();
	uint32 audioGetCdVolume();
	uint16 audioIsCdPlaying();
	uint16 audioPlayCd(uint32 from, uint32 to);
	uint16 audioPlayCdTrack(uint16 track);
	virtual uint16 audioSetCdVolume(uint32 volume);
	uint16 audioStopCd();

protected:
	struct SoundObject {
		uint16 handle;
		byte *data;
		uint32 available;
		uint32 readPos;
		uint32 size;
		uint32 lowWater;
		uint32 sourceLength;
		uint32 consumed;
		uint32 readFromSource;
		byte *chunk;
		uint32 chunkLength;
		uint32 fadeInReload;
		uint32 fadeInCount;
		uint32 fadeOutReload;
		uint32 fadeOutCount;
		uint32 nextTrigger;
		uint16 readProc;
		uint16 rewindProc;
		uint16 closeProc;
		WaveFormat format;
		AdpcmState adpcm;
		uint16 callbackHandle;
		uint16 notifyWParam;
		uint16 notifyLParam;
		int16 loops;
		bool enabled;
		bool notifyArmed;
		bool active;
		bool isStatic;
		bool needsConversion;
		bool endOfSource;
		bool overlay;
		bool randomMode;
		bool fadingIn;
		bool fadingOut;
		bool adpcmEncoded;
		byte adpcmShift;
		byte volume;
		byte currentVolume;
		byte randomMin;
		byte randomRange;
		ConvertState convert;

		// Fields of the later versions
		uint32 notifyEndTime;
		int16 initialLoops;
		int16 priority;
		bool notifyPending;
		bool exempt;

		virtual ~SoundObject() {}

		SoundObject() : handle(0), data(nullptr), available(0), readPos(0), size(0), lowWater(0), sourceLength(0),
			consumed(0), readFromSource(0), chunk(nullptr), chunkLength(0), fadeInReload(0), fadeInCount(0),
			fadeOutReload(0), fadeOutCount(0), nextTrigger(0), readProc(0), rewindProc(0), closeProc(0),
			callbackHandle(0), notifyWParam(0), notifyLParam(0), loops(0), enabled(false), notifyArmed(false),
			active(false), isStatic(false), needsConversion(false), endOfSource(false), overlay(false),
			randomMode(false), fadingIn(false), fadingOut(false), adpcmEncoded(false), adpcmShift(0),
			volume(0), currentVolume(0), randomMin(0), randomRange(0), notifyEndTime(0), initialLoops(0),
			priority(0), notifyPending(false), exempt(false) {}
	};

	struct MidiObject {
		uint16 handle;
		byte *data;
		uint32 size;
		uint16 index;
		uint16 count;
		uint16 readProc;
		uint16 rewindProc;
		uint16 closeProc;
		uint16 callbackHandle;
		uint16 notifyWParam;
		uint16 notifyLParam;
		uint16 tempoNumerator;
		uint16 tempoDenominator;
		int16 loops;
		bool notifyArmed;
		bool channelMuted[TG_AUDIO_V1_2_3_1_MIDI_NUM_CHANNELS];

		uint32 available;
		uint32 readPos;
		uint32 bufferSize;
		uint32 lowWater;
		uint32 consumed;
		uint32 readFromSource;
		uint32 nextEventTime;
		int16 initialLoops;
		bool enabled;
		bool isStatic;
		bool endOfSource;

		MidiObject() : handle(0), data(nullptr), size(0), index(0), count(0), readProc(0), rewindProc(0), closeProc(0),
			callbackHandle(0), notifyWParam(0), notifyLParam(0), tempoNumerator(0), tempoDenominator(0), loops(0),
			notifyArmed(false), available(0), readPos(0), bufferSize(0), lowWater(0), consumed(0), readFromSource(0),
			nextEventTime(0), initialLoops(0), enabled(false), isStatic(false), endOfSource(false) {
			for (uint32 i = 0; i < TG_AUDIO_V1_2_3_1_MIDI_NUM_CHANNELS; i++)
				channelMuted[i] = false;
		}
	};

	// Library
	uint16 libMain();
	bool allocAudioContext();
	void freeAudioContext();
	uint16 readAudioIniInt(const char *key, uint16 defaultValue);
	void readAudioIniSettings();

	// MIDI objects
	MidiObject *allocMidiObject();
	void destroyMidiObject(MidiObject *midi);
	void removeMidiFromArray(MidiObject *midi);
	bool initMidiObjectDataSource(const MidiCreateInfo_v1_2_4 *info, MidiObject *midi);
	bool invokeAndClearCloseCallback(MidiObject *midi);
	void destroyAllMidiObjects();
	MidiObject *validateMidiHandle(uint16 handle);

	// MIDI device
	void probeMidiOutCapabilities();
	bool openMidiOutDevice();
	void setMidiOutVolume(uint32 volume);
	void closeMidiOutDevice();
	bool openAndConfigureMidiOut();
	void resetMidiOutDevice();
	void sendMidiAllNotesOff(byte channel);

	// MIDI playback
	bool startMidiSequence(const byte *params, MidiObject *midi);
	void postMidiCompletionMessage(MidiObject *midi);
	void stopMidiSequence(MidiObject *midi);
	bool midiSequenceEndOrLoop(MidiObject *midi);
	virtual void midiSequencerStep(MidiObject *midi);

	// MIDI streaming
	void pollMidiPlayback();
	void refillStreamedMidiObjects();
	uint16 compactAndRefillMidiBuffer(MidiObject *midi);
	uint16 rewindMidiSource(MidiObject *midi);
	void rewindMidiObject(MidiObject *midi);
	void updateMidiEnabledFlags(uint16 on);

	// Master volume and fades
	void masterFadeTick();
	void masterFadeDownTick() override;

	// Wave device
	bool probeWaveOutCapabilities();
	int16 findWaveFormatIndexExact(uint16 mask, const WaveFormat *format);
	int16 findBestWaveFormatIndex(const WaveFormat *format);
	bool applyWaveFormatIndex(const WaveFormat *format);
	bool allocateWaveOutBuffers();
	void freeWaveOutBuffers();
	virtual bool finishWaveOutOpen();
	bool finishWaveOutOpenSync();
	bool openWaveOutDevice(const WaveFormat *format);
	void destroyAllWaveObjectsAndCloseDevice();
	virtual void resetWaveOutQueue();
	virtual void retireBuffer();
	bool retireBufferAndRefill();
	uint16 mixAllWaves16Bit();
	uint16 mixAllWaves8Bit();
	virtual void pumpAudioPlayback();
	void resetWaveQueueAndEnableAllWaves(uint16 on) override;
	void updateWaveEnabledFlags(uint16 on);
	void pauseAllPlayback(uint16 paused);

	// Mixers
	uint16 mixSound8Bit(uint16 mixCount, uint16 bytes, byte *destination, SoundObject *sound);
	uint16 mixSound16Bit(uint16 mixCount, uint16 bytes, byte *destination, SoundObject *sound);

	// Wave sounds
	SoundObject *allocWaveObject();
	uint16 removeWaveFromArray(SoundObject *sound);
	virtual bool initWaveObjectFromDescriptor(const SoundCreateInfo_v1_2 *info, SoundObject *sound);
	SoundObject *findWaveObject(uint16 handle);
	bool doesWaveNeedConversion(const SoundObject *sound);
	void computeRandomizedRepeatDelay(SoundObject *sound, uint32 now);
	void setupWaveFadeIn(SoundObject *sound, uint32 milliseconds, byte volume);
	void setupWaveFadeOut(SoundObject *sound, uint32 milliseconds, byte volume, uint16 mode);
	virtual bool startWavePlayback(SoundObject *sound, const byte *params);
	void scheduleRandomRepeatPlayback(SoundObject *sound, const byte *params);
	virtual void activateWaveObject(SoundObject *sound);
	void deactivateAllWaves();
	virtual void finishWaveObject(SoundObject *sound);
	virtual void stopWaveObject(SoundObject *sound, const byte *params);

	// Sound streaming
	bool soundEndOrLoop(SoundObject *sound);
	uint16 rewindSourceStream(SoundObject *sound);
	void prepareWaveChunk(SoundObject *sound, uint16 outputBytes);
	void rewindSound(SoundObject *sound);
	uint16 compactAndRefillSoundBuffer(SoundObject *sound);
	virtual bool closeSoundSource(SoundObject *sound);
	bool ensureAdpcmScratchBuffer(const SoundObject *sound, uint32 sourceBytes);
	bool ensureResampleScratchBuffer(uint32 size);
	uint32 computeSourceBytesForOutput(const SoundObject *sound, uint32 outputBytes);
	uint32 decodeImaAdpcm(bool stereo, uint32 count, const byte *source, byte *destination, AdpcmState *state);

	// CD audio
	uint16 getCurrentTask() const;
	uint16 getCdOwnerTask() const;
	void muteCDAudio();
	uint16 stopCDIfOwned();

	bool _libMainDone = false;
	bool _contextCreated = false;
	uint16 _hwnd = 0;
	bool _mute = false;
	uint16 _iniWaveFormatMask = TG_AUDIO_V1_2_DEFAULT_FORMAT_MASK;
	uint16 _maxWaveBuffers = TG_AUDIO_V1_2_DEFAULT_WAVE_BUFFERS;

	// Wave device
	bool _noWaveDevice = false;
	bool _waveDeviceIsSync = false;
	uint32 _waveOutCapsSupport = 0;
	uint16 _userFormatMask = 0;
	bool _userVolumeSet = false;
	uint32 _userVolume = 0;
	WaveRingEntry _waveRing[TG_AUDIO_V1_2_MAX_WAVE_BUFFERS];
	byte *_adpcmScratch = nullptr;
	uint32 _adpcmScratchSize = 0;
	byte *_resampleScratch = nullptr;
	uint32 _resampleScratchSize = 0;

	// Wave sounds
	SoundObject *_sounds[TG_AUDIO_V1_2_SOUND_TABLE_SIZE] = {};
	uint16 _soundCount = 0;
	uint16 _activeSounds = 0;
	uint16 _nextSoundHandle = 1;
	int16 _maxPriority = 0;

	// CD audio
	uint32 _cdMuteSavedVolume = 0;

	// MIDI
	MidiObject *_midiObjects[TG_AUDIO_V1_2_3_MAX_MIDI_OBJECTS] = {};
	uint16 _midiCount = 0;
	uint16 _nextMidiHandle = 1;
	MidiObject *_currentMidi = nullptr;
	bool _paused = false;
	uint16 _streamedMidiCount = 0;
	uint32 _midiServiceTime = 0;

	// MIDI device
	MidiSequencer *_midiOut = nullptr;
	bool _noMidiDevice = false;
	uint32 _midiDeviceVolume = 0xFFFFFFFF;
	uint32 _midiOriginalVolume = 0;
	uint32 _midiUserVolume = 0;
	bool _midiUserVolumeSet = false;

private:
	TopgunEngine_v1_2 *engine() const;
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_AUDIO_AUDIO_V1_2_H
