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

#ifndef GRIM_EMI_MILES_AUDIO_DRIVER_H
#define GRIM_EMI_MILES_AUDIO_DRIVER_H

#include "common/mutex.h"
#include "common/scummsys.h"
#include "common/stream.h"

#include "audio/audiostream.h"
#include "audio/mixer.h"

namespace Grim {

class EMIMilesDriver;
class EMIMilesFilter;
class EMISpu2Reverb;
struct EMIMilesStream;

struct EMIMilesSample {
	int status;
	const byte *buffer[2];
	int length[2];
	int position[2];
	bool bufferEmpty[2];
	uint32 srcFract;
	int32 prevL;
	int32 prevR;
	int current;
	int bufferCursor;
	int loopCount;
	int loopStart;
	int loopEnd;
	int format;
	int rate;
	int volume;
	int pan;
	int scaleL;
	int scaleR;
	int reverbScale;
	bool spuVolume;
	int spuVolumeL;
	int spuVolumeR;
	int serviceMode;
	EMIMilesFilter *filter;
	EMIMilesStream *stream;
	bool effectSend;
	uint32 spuCounter;
	int spuPending;
	int32 spuHistoryL[4];
	int32 spuHistoryR[4];
};

struct EMIMilesStream {
	EMIMilesSample *sample;
	byte *data;
	int *frameOffsets;
	int frameCount;
	int samplesPerFrame;
	int channels;
	int dataSize;
	int bytesPerSecond;
	int rate;
	bool stereo;
	int loopStart;
	int loopEnd;
	int loopCount;
	int position;
	Audio::PacketizedAudioStream *decoder;
	int segmentStart;
	int segmentEnd;
	int segmentFed;
	int segmentFrames;
	int segmentFeedEnd;
	int segmentLimit;
	int segmentSkip;
	bool segmentFade;
	int segmentOut;
	bool terminatorQueued;
	byte terminator;
	bool primeWarned;
	uint syncStamp;
	bool syncPending;
	int dataEndState;
	bool serviceActive;
	bool started;
	int startFlags;
	byte *pcm[2];
	byte *stage;
	int pcmCapacity;
	int stageSize;
	int stageBase;
	int bufferBase[2];
	uint bufferSequence[2];
	uint sequenceCounter;
};

class EMIMilesDriver : public Audio::AudioStream {
public:
	enum {
		kSampleCount = 16,
		kOutputRate = 22050,
		kSegmentFrames = 180,
		kSegmentSamples = kSegmentFrames * 2,
		kSpu2OutputRate = 48000,
		kSpu2SegmentFrames = 392,
		kMaxSegmentSamples = kSpu2SegmentFrames * 2,
		kMaxSegments = 96,
		kDefaultMixAhead = 8,
		kMaxVolume = 127,
		kMaxSpuVolume = 0x3FFF,
		kSpuVolumeShift = 3,
		kUnscaledGain = 2047,
		kResampleTolerance = 655
	};

	enum {
		kSampleFree = 1,
		kSampleDone = 2,
		kSamplePlaying = 4,
		kSampleStopped = 8,
		kSamplePlayingReleased = 16
	};

	enum {
		kFormat16Bit = 1,
		kFormatStereo = 2
	};

	enum {
		kMixOutputStereo = 1,
		kMix16Bit = 2,
		kMixInterpolate = 4,
		kMixSourceStereo = 8,
		kMixScaled = 16,
		kMixResample = 32,
		kMixBigEndian = 64
	};

	enum {
		kStreamStartRequested = 1,
		kStreamPaused = 8,
		kStreamPausedWhilePlaying = 16,
		kStreamPacketBytes = 8192,
		kStreamBufferMilliseconds = 250
	};

	EMIMilesDriver(bool isPs2);
	~EMIMilesDriver() override;

	void *operator new(size_t size);
	void operator delete(void *block);

	bool open(Audio::Mixer *mixer);
	void close();

	void setMasterVolume(int volume);
	int getMasterVolume();
	void setMixAhead(int segments);
	void releaseHandle();
	void reacquireHandle();
	int getActiveSampleCount();

	EMIMilesSample *allocateSample();
	void releaseSample(EMIMilesSample *sample);
	void initSample(EMIMilesSample *sample);
	void setSampleAddress(EMIMilesSample *sample, const byte *address, uint length);
	void setSampleType(EMIMilesSample *sample, int format);
	void setSampleRate(EMIMilesSample *sample, int rate);
	void setSampleLoopCount(EMIMilesSample *sample, int count);
	int getSampleLoopCount(EMIMilesSample *sample);
	void setSampleLoopBlock(EMIMilesSample *sample, int start, int end);
	void setSampleVolume(EMIMilesSample *sample, int volume);
	void setSamplePan(EMIMilesSample *sample, int pan);
	void setSampleSpuVolume(EMIMilesSample *sample, int left, int right);
	void setSampleEffectSend(EMIMilesSample *sample, bool enabled);
	void startSample(EMIMilesSample *sample);
	void stopSample(EMIMilesSample *sample);
	void resumeSample(EMIMilesSample *sample);
	void endSample(EMIMilesSample *sample);
	int getSampleStatus(EMIMilesSample *sample);
	void getSampleMsPosition(EMIMilesSample *sample, int *total, int *current);
	void setSampleMsPosition(EMIMilesSample *sample, int milliseconds);
	void setSamplePosition(EMIMilesSample *sample, int position);
	int getSamplePosition(EMIMilesSample *sample);

	void setSampleFilter(EMIMilesSample *sample, bool enabled);
	void setSampleFilterEnvironment(EMIMilesSample *sample, float environment);
	void setSampleFilterTime(EMIMilesSample *sample, float milliseconds);
	void setSampleFilterMix(EMIMilesSample *sample, float mix);
	void setSampleFilterDamping(EMIMilesSample *sample, float hertz);

	int setSpuReverb(int mode, int core, float depthLeft, float depthRight, float delay, float feedback);

	EMIMilesStream *openStream(Common::SeekableReadStream *file);
	void closeStream(EMIMilesStream *stream);
	void startStream(EMIMilesStream *stream);
	void pauseStream(EMIMilesStream *stream, bool paused);
	int getStreamStatus(EMIMilesStream *stream);
	void setStreamVolume(EMIMilesStream *stream, int volume);
	void setStreamPan(EMIMilesStream *stream, int pan);
	void setStreamLoopCount(EMIMilesStream *stream, int count);
	void setStreamLoopBlock(EMIMilesStream *stream, int start, int end);
	void setStreamPosition(EMIMilesStream *stream, int bytes);
	void setStreamPositionSynced(EMIMilesStream *stream, int bytes, uint stamp);
	int getStreamPosition(EMIMilesStream *stream);
	int getStreamPositionStamped(EMIMilesStream *stream, uint *stamp);
	void setStreamMsPosition(EMIMilesStream *stream, int milliseconds);
	void getStreamMsPosition(EMIMilesStream *stream, int *total, int *current);

	void mixerMerge(const byte **src, uint32 *srcFract, const byte *srcEnd, int32 **dst, const int32 *dstEnd, int32 *prevL, int32 *prevR, int ratio, int scaleL, int scaleR, uint flags);

	int readBuffer(int16 *buffer, const int numSamples) override;
	bool isStereo() const override;
	int getRate() const override;
	bool endOfData() const override;

private:
	struct MixFormat {
		bool is16;
		bool srcStereo;
		bool scaled;
		int scaleL;
		int scaleR;
		int frameBytes;
	};

	void service();
	void pushSegment();
	void mixSegment(int16 *out);
	void initSpuEffect();
	void mixSpuEffect();
	int mixSpuVoice(EMIMilesSample *sample, int remainingBytes, int dstOffset);
	static int convertSpuValue(float value);
	void advanceSample(EMIMilesSample *sample);
	int mixChunk(EMIMilesSample *sample, int remainingBytes, int dstOffset);
	void computeScales(EMIMilesSample *sample);
	int getFrameBytes(int format) const;

	void sampleInit(EMIMilesSample *sample);
	void sampleSetType(EMIMilesSample *sample, int format);
	void sampleSetVolume(EMIMilesSample *sample, int volume);
	void sampleSetPan(EMIMilesSample *sample, int pan);
	void sampleStop(EMIMilesSample *sample);
	void sampleResume(EMIMilesSample *sample);
	void sampleEnd(EMIMilesSample *sample);
	void sampleLoadBuffer(EMIMilesSample *sample, int index, const byte *address, int length);
	int sampleBufferReady(EMIMilesSample *sample);
	void sampleSetPosition(EMIMilesSample *sample, int position);
	void destroyFilter(EMIMilesSample *sample);

	void mixCopy(const byte **src, const byte *srcEnd, int32 **dst, const int32 *dstEnd, const MixFormat &format);
	void mixInterpolate(const byte **src, uint32 *srcFract, const byte *srcEnd, int32 **dst, const int32 *dstEnd, int32 *prevL, int32 *prevR, int ratio, const MixFormat &format);
	void mixAverage(const byte **src, uint32 *srcFract, const byte *srcEnd, int32 **dst, const int32 *dstEnd, int32 *prevL, int32 *prevR, int ratio, const MixFormat &format);
	void fetchFrame(const byte *src, const MixFormat &format, int32 *left, int32 *right) const;
	static void emitFrame(int32 *dst, int32 left, int32 right, const MixFormat &format);
	static void copyBuildBuffer(const int32 *build, int16 *out, int count);

	void streamService(EMIMilesStream *stream);
	void streamServiceAll();
	bool streamFeed(EMIMilesStream *stream);
	int streamFill(EMIMilesStream *stream);
	bool streamBeginSegment(EMIMilesStream *stream);
	bool streamEndSegment(EMIMilesStream *stream);
	int streamDecode(EMIMilesStream *stream, int16 *dst, int maxSamples, bool *finished);
	int streamProbe(const EMIMilesStream *stream, int from, int first) const;
	void streamDestroyDecoder(EMIMilesStream *stream);
	int streamBytesFromFrames(const EMIMilesStream *stream, int frames) const;
	void startStreamInternal(EMIMilesStream *stream);
	void setStreamPositionInternal(EMIMilesStream *stream, int bytes);
	int getStreamPositionInternal(EMIMilesStream *stream);
	int getStreamStatusInternal(EMIMilesStream *stream);
	static int scanFrames(const byte *data, int size, int start, int *offsets, int *bitrate, int *rate, bool *stereo, int *samples);

	Common::Mutex _mutex;
	bool _isPs2;
	int _outputRate;
	int _segmentFrames;
	int _segmentSamples;
	EMISpu2Reverb *_spu2Reverb;
	Audio::Mixer *_mixer;
	Audio::SoundHandle _handle;
	bool _open;
	bool _released;
	int _masterVolume;
	int _mixAhead;

	EMIMilesSample _samples[kSampleCount];
	EMIMilesStream *_streams[kSampleCount];

	int32 _build[kMaxSegmentSamples];
	int32 _effectBuild[kMaxSegmentSamples];
	int16 *_fifo;
	int _fifoReadSegment;
	int _fifoReadOffset;
	int _fifoSegments;
	uint _segmentCounter;

	const byte *_mixSourceLimit;
};

} // End of namespace Grim

#endif
