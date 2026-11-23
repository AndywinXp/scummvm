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

#ifndef TOPGUN_LIBRARIES_AUDIO_AUDIO_H
#define TOPGUN_LIBRARIES_AUDIO_AUDIO_H

#include "audio/mixer.h"
#include "common/array.h"
#include "common/file.h"
#include "common/random.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "common/stream.h"

#include "topgun/libraries/audio/waveout.h"

#define TG_AUDIO_FIRST_CD_COMMAND        101
#define TG_AUDIO_LAST_CD_COMMAND         115
#define TG_AUDIO_FIRST_MASTER_COMMAND    130
#define TG_AUDIO_LAST_MASTER_COMMAND     131
#define TG_AUDIO_FIRST_MIDI_COMMAND      150
#define TG_AUDIO_LAST_MIDI_COMMAND       152
#define TG_AUDIO_FIRST_WAVE_COMMAND      200
#define TG_AUDIO_LAST_WAVE_COMMAND       213
#define TG_AUDIO_FIRST_SOUND_COMMAND     300
#define TG_AUDIO_LAST_SOUND_COMMAND      308
#define TG_AUDIO_DATA_BUFFER_SIZE        0
#define TG_AUDIO_WAVE_BUFFER_SIZE        0
#define TG_AUDIO_WAVE_FORMAT_MASK        5
#define TG_AUDIO_WAVE_FORMAT_MASK_V1_1    0x0D
#define TG_AUDIO_DEFAULT_WAVE_BUFFER_SIZE 0x2000
#define TG_AUDIO_HEADER_CHUNK_SIZE       20
#define TG_AUDIO_FORMAT_CHUNK_SIZE       16
#define TG_AUDIO_NUM_OUTPUT_FORMATS      12
#define TG_AUDIO_NUM_AUX_DEVICES         1
#define TG_AUDIO_AUX_TECHNOLOGY_CD       1
#define TG_AUDIO_AUX_TECHNOLOGY_MASTER   8
#define TG_AUDIO_FIRST_AUDIO_TRACK       2
#define TG_AUDIO_FRAMES_PER_SECOND       75
#define TG_AUDIO_POLL_INTERVAL           200
#define TG_AUDIO_FADE_TICK_INTERVAL      200
#define TG_AUDIO_MAX_VOLUME_LEVEL        0x10
#define TG_AUDIO_LOAD_FLAG               1
#define TG_AUDIO_IN_MEMORY_FLAG          2
#define TG_AUDIO_BACKGROUND_FLAG         0x8000

#define TG_AUDIO_DEV_OPEN                0x803
#define TG_AUDIO_DEV_CLOSE               0x804
#define TG_AUDIO_DEV_PLAY                0x806
#define TG_AUDIO_DEV_STOP                0x808
#define TG_AUDIO_DEV_SET                 0x80D
#define TG_AUDIO_DEV_STATUS              0x814
#define TG_AUDIO_DEV_WAIT                2
#define TG_AUDIO_DEV_FROM                4
#define TG_AUDIO_DEV_TO                  8
#define TG_AUDIO_DEV_STATUS_ITEM         0x100
#define TG_AUDIO_DEV_TRACK               0x10
#define TG_AUDIO_DEV_SET_TIME_FORMAT     0x400
#define TG_AUDIO_DEV_OPEN_TYPE           0x2000
#define TG_AUDIO_DEV_OPEN_ELEMENT        0x200
#define TG_AUDIO_STATUS_LENGTH           1
#define TG_AUDIO_STATUS_POSITION         2
#define TG_AUDIO_STATUS_NUMBER_OF_TRACKS 3
#define TG_AUDIO_FORMAT_TMSF             10

namespace Topgun {

class MidiSequencer;
class TopgunEngine;

struct WaveFormat {
	uint16 tag;
	uint16 channels;
	uint32 sampleRate;
	uint32 averageBytesPerSecond;
	uint16 blockAlign;
	uint16 bitsPerSample;

	WaveFormat() : tag(0), channels(0), sampleRate(0), averageBytesPerSecond(0), blockAlign(0), bitsPerSample(0) {}
};

struct SoundCreateInfo {
	uint16 waveInfo;
	Common::SeekableReadStream *fileStream;
	const byte *header;

	SoundCreateInfo() : waveInfo(0), fileStream(nullptr), header(nullptr) {}
};

class AudioLibrary {
public:
	AudioLibrary(TopgunEngine *engine);
	virtual ~AudioLibrary();

	uint16 audioInitLibrary(uint16 version);
	virtual void audioExitLibrary();
	virtual void audioService();
	uint32 audioCommand(uint16 command, uint16 handle, uint32 value, void *block);
	uint16 audioActivate(uint16 active);
	uint32 audioResampleWave(const WaveFormat *destinationFormat, const WaveFormat *sourceFormat,
	                         uint32 sourceBytes, byte *destinationBuffer, const byte *sourceBuffer);
	byte *waveLoadFile(const Common::String &fileName, WaveFormat *outFormat, uint32 *outSize);
	bool waveSaveFile(const Common::String &fileName, const WaveFormat *format,
	                  const byte *data, uint32 size);
	uint16 wavePlayFile(const Common::String &fileName, uint16 flags);
	uint16 audioSetOwner(uint16 owner);
	void audioSetMute(uint16 muted);

protected:
	enum Command {
		kCmdCdIsOpen = 101,
		kCmdCdGetPosition = 105,
		kCmdCdGetVolume = 106,
		kCmdCdGetTrackCount = 107,
		kCmdCdPlayFadeIn = 102,
		kCmdCdFadeOut = 103,
		kCmdCdPlayFrom = 109,
		kCmdCdPlayRange = 110,
		kCmdCdPlayTracks = 111,
		kCmdCdSetVolume = 114,
		kCmdCdClose = 115,
		kCmdMasterGetVolume = 130,
		kCmdMasterSetVolume = 131,
		kCmdMidiPlayDefault = 150,
		kCmdMidiPlayFile = 151,
		kCmdMidiStop = 152,
		kCmdWaveClose = 200,
		kCmdWaveCreate = 201,
		kCmdWaveDestroyAll = 202,
		kCmdWavePause = 203,
		kCmdWaveGetVolume = 204,
		kCmdWaveGetFormats = 205,
		kCmdWaveGetChosenFormat = 206,
		kCmdWaveOpen = 207,
		kCmdWaveStartGroup = 208,
		kCmdWaveSetVolume = 209,
		kCmdWaveStopGroup = 210,
		kCmdWaveFadeOut = 212,
		kCmdWaveFadeUp = 213,
		kCmdSoundDestroy = 300,
		kCmdSoundGetSourceSize = 301,
		kCmdSoundGetVolume = 302,
		kCmdSoundGetFormat = 303,
		kCmdSoundIsActive = 304,
		kCmdSoundStart = 305,
		kCmdSoundRewind = 306,
		kCmdSoundSetVolume = 307,
		kCmdSoundStop = 308
	};

	struct DeviceParms {
		uint16 deviceId;
		uint32 timeFormat;
		uint32 item;
		uint32 track;
		uint32 value;
		uint32 from;
		uint32 to;
		Common::String fileName;
		const byte *midiData;
		uint32 midiSize;

		DeviceParms() : deviceId(0), timeFormat(0), item(0), track(0), value(0), from(0), to(0), midiData(nullptr), midiSize(0) {}
	};

	struct ConvertState {
		byte source16;
		byte destination16;
		byte sourceMono;
		byte destinationMono;
		byte decimateFactor;
		byte interpolateFactor;
		byte decimateShift;
		byte interpolateShift;
		byte mode;

		ConvertState() : source16(0), destination16(0), sourceMono(0), destinationMono(0), decimateFactor(1),
			interpolateFactor(1), decimateShift(0), interpolateShift(0), mode(0) {}
	};

	struct WaveObject {
		uint16 handle;
		bool active;
		bool suspended;
		bool streaming;
		bool needsConversion;
		bool endOfStream;
		bool busy;
		bool backgroundGroup;
		bool randomRepeat;
		bool fadingIn;
		bool fadingOut;
		byte loops;
		byte volume;
		byte currentVolume;
		byte randomMin;
		byte randomRange;
		byte *data;
		uint32 readPos;
		uint32 available;
		uint32 size;
		byte *stage;
		byte *stageCurrent;
		uint32 stageAvailable;
		uint32 stageSize;
		uint32 sourceSize;
		uint32 sourceRead;
		uint32 sourceStart;
		uint32 fadeReload;
		uint32 fadeCount;
		uint32 nextTrigger;
		uint16 waveInfo;
		Common::SeekableReadStream *fileStream;
		WaveFormat format;
		ConvertState convert;

		WaveObject() : handle(0), active(false), suspended(false), streaming(false), needsConversion(false),
			endOfStream(false), busy(false), backgroundGroup(false), randomRepeat(false), fadingIn(false),
			fadingOut(false), loops(0), volume(0), currentVolume(0), randomMin(0), randomRange(0),
			data(nullptr), readPos(0), available(0), size(0), stage(nullptr), stageCurrent(nullptr),
			stageAvailable(0), stageSize(0), sourceSize(0), sourceRead(0), sourceStart(0), fadeReload(0),
			fadeCount(0), nextTrigger(0), waveInfo(0), fileStream(nullptr) {}
	};

	// Dispatchers
	uint32 dispatchCDCommand(uint16 command, uint16 handle, uint32 value, void *block);
	uint32 dispatchMidiCommand(uint16 command, uint16 handle, uint32 value, void *block);
	uint32 dispatchMasterVolumeCommand(uint16 command, uint16 handle, uint32 value, void *block);
	uint32 dispatchWaveDeviceCommand(uint16 command, uint16 handle, uint32 value, void *block);
	uint32 dispatchWaveSoundCommand(uint16 handle, uint16 command, uint32 value, void *block);
	uint16 auxDeviceTechnology(uint16 device) const;
	static void readWaveFormat(const byte *source, WaveFormat &format);
	static void writeWaveFormat(byte *destination, const WaveFormat &format);

	// Master volume and fades
	uint32 addClampedStereoVolumeDelta(uint16 deltaLeft, uint16 deltaRight, uint32 volume);
	void averageStereoVolumes(uint32 volumeA, uint32 volumeB);
	uint32 getMasterVolume();
	uint16 setMasterVolume(uint32 volume);
	uint16 setMasterVolumeEmulated(uint32 volume);
	void restoreMasterVolume();
	uint16 findMasterAuxDevice();
	void masterFadeUpTick();
	virtual void masterFadeDownTick();
	uint16 startMasterFadeUp(uint16 duration);
	uint16 startMasterFadeOut(uint16 duration);

	// Format conversion
	static uint32 convertPcmMode0Mono8ToStereo8(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed);
	static uint32 convertPcmMode1Mono8ToStereo16(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed);
	static uint32 convertPcmMode2Mono16ToStereo8(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed);
	static uint32 convertPcmMode3Mono16ToStereo16(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed);
	static uint32 convertPcmMode4Stereo8ToMono8(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed);
	static uint32 convertPcmMode5Stereo8ToMono16(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed);
	static uint32 convertPcmMode6Stereo16ToMono8(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed);
	static uint32 convertPcmMode7Stereo16ToMono16(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed);
	static uint32 convertPcmMode8Mono8ToMono16(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed);
	static uint32 convertPcmMode9Mono16ToMono8(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed);
	static void selectConvertMode(ConvertState *state);
	void resampleDecimateRead(const ConvertState *state, const byte *&source);
	void resampleDecimateEmit(const ConvertState *state, byte *&destination);
	void resampleInterpolateRead(const ConvertState *state, const byte *&source);
	void resampleInterpolateEmit(const ConvertState *state, byte *&destination);
	uint32 convertInitState(ConvertState *state, const WaveFormat *destinationFormat, const WaveFormat *sourceFormat, uint32 sourceBytes);
	uint32 convertSampleRate(ConvertState *state, byte *destination, uint32 destinationCapacity, const byte *source, uint32 sourceBytes, uint32 *consumed);
	uint16 convertWaveDataInPlace(WaveObject *wave);

	// Mixer and wave device
	bool soundEndOrLoop(WaveObject *wave);
	void mixSound8Bit(WaveObject *wave);
	void mixSound16Bit(WaveObject *wave);
	void mixAllWavesAndQueueBuffer();
	bool retireBufferAndRefill();
	void pumpAudioPlayback();
	void allocateWaveOutBuffers();
	void freeWaveOutBuffers();
	bool findBestWaveFormatIndex(const WaveFormat *wanted);
	void closeWaveOutDevice();
	bool openWaveOutDevice(const WaveFormat *wanted);
	uint16 pauseAllPlayback(uint16 resume);
	void setWaveOutVolume(uint32 volume);
	void applyWaveOutVolume(uint32 volume);
	void resetWaveOutQueue();
	bool probeWaveOutCapabilities();
	void destroyAllWaveObjectsAndCloseDevice();

	// Wave sounds
	WaveObject *allocWaveObject();
	void setupWaveFadeIn(WaveObject *wave, uint16 seconds, uint16 volume);
	void setupWaveFadeOut(WaveObject *wave, uint16 seconds, uint16 currentVolume);
	uint16 audioCreateWave(SoundCreateInfo *info, uint16 flags);
	void destroyAllWaveObjects();
	void destroyWaveObject(WaveObject *wave);
	int findWaveIndex(const WaveObject *wave) const;
	WaveObject *findWaveByHandle(uint16 handle) const;
	uint16 removeWaveFromArray(WaveObject *wave);
	uint16 initWaveObjectFromDescriptor(WaveObject *wave, SoundCreateInfo *info, uint16 flags);
	void computeRandomizedRepeatDelay(WaveObject *wave, uint32 now);
	void startWavesBySelector(byte *params);
	void startWavePlayback(WaveObject *wave, const byte *params);
	void rewindSound(WaveObject *wave);
	void stopWavesBySelector(byte *params);
	void stopWaveObject(WaveObject *wave, const byte *params);
	void finishWaveObject(WaveObject *wave);
	uint32 refillSoundBuffer(WaveObject *wave, byte *destination, uint32 count);
	uint32 refillSoundBufferConverted(WaveObject *wave, byte *destination, uint32 count);
	uint32 compactAndRefillSoundBuffer(WaveObject *wave);
	bool hasOpenSource(const WaveObject *wave) const;
	int32 readSource(WaveObject *wave, byte *destination, uint32 count);
	int32 seekSource(WaveObject *wave, int32 offset, int16 origin);
	void closeSource(WaveObject *wave);
	void closeCreateInfoSource(SoundCreateInfo *info);

	// Wave files
	uint32 findWaveDataChunkSize(Common::SeekableReadStream *file);
	Common::File *openWaveRiffFile(const Common::String &fileName);
	bool readWaveFormatChunk(Common::SeekableReadStream *file, byte *format);

	// CD audio
	uint8 getCDTrackCount();
	uint32 getCDCurrentPosition();
	uint32 readCDVolumeLevels();
	bool sendCdDeviceCommand(uint16 message, uint32 flags, DeviceParms &parms);
	bool openCDAudioDevice();
	uint16 startCDPlayback(uint32 from, uint32 to);
	uint16 playCDTrackRangeByNumber(uint16 first, uint16 last);
	uint16 setCDAudioVolume(uint32 volume);
	void releaseCDDeviceIfOwned();
	uint16 startCDFadeIn(uint16 milliseconds, uint32 from);
	uint16 startCDFadeOut(uint16 milliseconds);
	void cdFadeTick();
	void profileCDStart();
	uint16 findCdAuxDevice();
	void pollCDEndAndClose();
	virtual uint16 closeCDAudioDevice();
	virtual void resetWaveQueueAndEnableAllWaves(uint16 on);
	uint32 getExtractedTrackFrames(uint32 track);
	void applyCdVolume(uint32 volume);

	// MIDI
	void playDefaultMidiFile();
	bool sendMidiDeviceCommand(uint16 message, uint32 flags, DeviceParms &parms);
	uint16 playMidiFile(const Common::String &fileName);
	void stopAndCloseMidi();
	void shutdownMidi();

	// Output formats: bit of WAVE_FORMAT, rate, channels, bytes per sample
	static const uint16 kFormatBits[TG_AUDIO_NUM_OUTPUT_FORMATS];
	static const uint16 kFormatRates[TG_AUDIO_NUM_OUTPUT_FORMATS];
	static const byte kFormatChannels[TG_AUDIO_NUM_OUTPUT_FORMATS];
	static const byte kFormatBytesPerSample[TG_AUDIO_NUM_OUTPUT_FORMATS];

	TopgunEngine *_engine;
	uint16 _initCount = 0;
	uint16 _owner = 0;
	bool _muted = false;
	bool _disableWaveVolume = false;
	bool _cdProfileFirst = false;
	bool _cdSlowStart = false;
	Common::RandomSource _randomSource{"TopgunAudio"};

	// Wave sounds
	Common::Array<WaveObject *> _waveObjects;
	uint16 _nextWaveHandle = 1;
	byte _activeSoundCount = 0;

	// Wave device
	Audio::SoundHandle _waveOutHandle;
	WaveOutStream *_waveOutStream = nullptr;
	bool _waveRunning = false;
	bool _waveOut8Bit = false;
	byte _buffersInFlight = 0;
	uint32 _waveBufferSize = 0;
	uint32 _streamBufferSize = 0;
	uint32 _refillThreshold = 0;
	uint32 _ringTail = 0;
	uint32 _ringHead = 0;
	WaveRingEntry _waveRing[TG_WAVEOUT_RING_SIZE];
	uint32 _waveOutCapsFormats = 0;
	uint32 _chosenFormatMask = 0;
	WaveFormat _waveFormat;
	uint32 _hardwareWaveVolume = 0xFFFFFFFF;
	uint16 _waveVolumeLeft = 0;
	uint16 _waveVolumeRight = 0;
	uint16 _savedWaveVolumeLeft = 0;
	uint16 _savedWaveVolumeRight = 0;
	uint32 _mixSoundCount = 0;

	// Master volume and fades
	byte _masterFadeMode = 0;
	uint16 _fadeTargetVolumeLeft = 0;
	uint16 _fadeTargetVolumeRight = 0;
	uint32 _fadeNextTick = 0;
	uint16 _fadeVolumeStepLeft = 0;
	uint16 _fadeVolumeStepRight = 0;
	uint16 _fadeStepsRemaining = 0;
	bool _masterAuxPresent = false;
	uint16 _masterAuxId = 0;
	uint32 _masterAuxVolume = 0xFFFFFFFF;
	uint32 _masterAuxSavedVolume = 0;
	uint16 _masterAvgVolumeLeft = 0;
	uint16 _masterAvgVolumeRight = 0;

	// Resampler
	uint32 _resampleOutputBytesRemaining = 0;
	uint32 _resampleInputBytesRemaining = 0;
	int32 _resampleAccumL = 0;
	int32 _resampleNextSampleL = 0;
	int32 _resampleAccumR = 0;
	int32 _resampleNextSampleR = 0;
	int32 _resampleStepL = 0;
	int32 _resampleStepR = 0;

	// CD audio
	uint16 _cdDeviceId = 0;
	bool _cdSuspend = false;
	uint32 _cdLimit = 0;
	uint32 _cdFrom = 0;
	uint32 _cdTo = 0;
	uint32 _cdPos = 0;
	uint32 _cdNextPoll = 0;
	byte _cdFadeMode = 0;
	uint32 _cdFadeNextTick = 0;
	uint16 _cdFadeSteps = 0;
	uint16 _cdFadeStepLeft = 0;
	uint16 _cdFadeStepRight = 0;
	uint16 _cdFadeCurrentLeft = 0;
	uint16 _cdFadeCurrentRight = 0;
	uint16 _cdFadeTargetLeft = 0;
	uint16 _cdFadeTargetRight = 0;
	bool _cdAuxPresent = false;
	uint16 _cdAuxDeviceId = 0;
	uint32 _cdAuxVolume = 0xFFFFFFFF;
	uint32 _cdAuxSavedVolume = 0;
	bool _cdPlayStarted = false;
	uint32 _cdPlayTrack = 0;
	uint32 _cdPlayStartFrame = 0;
	uint32 _cdPlayEndFrame = 0;
	uint32 _cdPlayStartMillis = 0;

	// MIDI
	uint16 _seqDeviceId = 0;
	MidiSequencer *_sequencer = nullptr;
	Common::String _midiFileName;
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_AUDIO_AUDIO_H
