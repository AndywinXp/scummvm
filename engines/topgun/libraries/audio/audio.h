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
	~AudioLibrary();

	uint16 audioInitLibrary(uint16 version);
	void audioExitLibrary();
	void audioService();
	uint32 audioCommand(uint16 command, uint16 handle, uint32 value, void *block);
	uint16 audioActivate(uint16 active);
	uint32 audioResampleWave(const WaveFormat *destinationFormat, const WaveFormat *sourceFormat,
	                         uint32 sourceBytes, byte *destinationBuffer, const byte *sourceBuffer);
	byte *waveLoadFile(const Common::String &fileName, WaveFormat *outFormat, uint32 *outSize);
	bool waveSaveFile(const Common::String &fileName, const WaveFormat *format,
	                  const byte *data, uint32 size);
	uint16 wavePlayFile(const Common::String &fileName, uint16 flags);
	uint16 audioSetOwner(uint16 owner);

private:
	enum Command {
		kCmdCdIsOpen = 101,
		kCmdCdGetPosition = 105,
		kCmdCdGetVolume = 106,
		kCmdCdGetTrackCount = 107,
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

	enum {
		kFirstCdCommand = 101,
		kLastCdCommand = 115,
		kFirstMasterCommand = 130,
		kLastMasterCommand = 131,
		kFirstMidiCommand = 150,
		kLastMidiCommand = 152,
		kFirstWaveCommand = 200,
		kLastWaveCommand = 213,
		kFirstSoundCommand = 300,
		kLastSoundCommand = 308,
		kDataBufferSize = 0,
		kWaveBufferSize = 0,
		kWaveFormatMask = 5,
		kDefaultWaveBufferSize = 0x2000,
		kHeaderChunkSize = 20,
		kFormatChunkSize = 16,
		kNumOutputFormats = 12,
		kNumAuxDevices = 1,
		kAuxTechnologyCd = 1,
		kAuxTechnologyMaster = 8,
		kFirstAudioTrack = 2,
		kFramesPerSecond = 75,
		kPollInterval = 200,
		kFadeTickInterval = 200,
		kMaxVolumeLevel = 0x10,
		kLoadFlag = 1,
		kInMemoryFlag = 2,
		kBackgroundFlag = 0x8000
	};

	enum {
		kDevOpen = 0x803,
		kDevClose = 0x804,
		kDevPlay = 0x806,
		kDevStop = 0x808,
		kDevSet = 0x80D,
		kDevStatus = 0x814,
		kDevWait = 2,
		kDevFrom = 4,
		kDevTo = 8,
		kDevStatusItem = 0x100,
		kDevTrack = 0x10,
		kDevSetTimeFormat = 0x400,
		kDevOpenType = 0x2000,
		kDevOpenElement = 0x200,
		kStatusLength = 1,
		kStatusPosition = 2,
		kStatusNumberOfTracks = 3,
		kFormatTmsf = 10
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

		DeviceParms() : deviceId(0), timeFormat(0), item(0), track(0), value(0), from(0), to(0) {}
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
	void masterFadeDownTick();
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
	void retireBufferAndRefill();
	void pumpAudioPlayback();
	void allocateWaveOutBuffers();
	void freeWaveOutBuffers();
	bool findBestWaveFormatIndex();
	void closeWaveOutDevice();
	bool openWaveOutDevice();
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
	uint16 findCdAuxDevice();
	void pollCDEndAndClose();
	uint16 closeCDAudioDevice();
	void resetWaveQueueAndEnableAllWaves(uint16 on);
	uint32 getExtractedTrackFrames(uint32 track);
	void applyCdVolume(uint32 volume);

	// MIDI
	void playDefaultMidiFile();
	bool sendMidiDeviceCommand(uint16 message, uint32 flags, DeviceParms &parms);
	uint16 playMidiFile(const Common::String &fileName);
	void stopAndCloseMidi();
	void shutdownMidi();

	TopgunEngine *_engine;
	uint16 _initCount = 0;
	uint16 _owner = 0;
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
	WaveRingEntry _waveRing[WaveOutStream::kRingSize];
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
