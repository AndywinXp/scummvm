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

#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/miles_audio/driver.h"
#include "engines/grim/emi/miles_audio/reverb.h"
#include "engines/grim/emi/miles_audio/ps2/spu2interpolation.h"
#include "engines/grim/emi/miles_audio/ps2/spu2reverb.h"

namespace Grim {

static const int kDefaultPan = 64;
static const int kDefaultSampleRate = 11025;
static const int kSpuEffectEndAddress = 0x1FFFFF;
static const int kSpuDepthStep = 100;
static const float kSpuMaxDepth = 32767.0f;
static const float kSpuMaxDelay = 127.0f;

EMIMilesDriver::EMIMilesDriver(LMemoryServices *memoryServices, bool isPs2) {
	_memoryServices = memoryServices;
	_isPs2 = isPs2;

	// SPU2 mixes at 48 kHz, while Miles on PC mixes at 22050 Hz
	if (_isPs2) {
		_outputRate = kSpu2OutputRate;
		_segmentFrames = kSpu2SegmentFrames;
	} else {
		_outputRate = kOutputRate;
		_segmentFrames = kSegmentFrames;
	}

	_segmentSamples = _segmentFrames * 2;
	_spu2Reverb = nullptr;
	_mixer = nullptr;
	_open = false;
	_released = false;
	_masterVolume = kMaxVolume;
	_mixAhead = kDefaultMixAhead;
	_fifoReadSegment = 0;
	_fifoReadOffset = 0;
	_fifoSegments = 0;
	_segmentCounter = 0;
	_mixSourceLimit = nullptr;

	_fifo = new int16[kMaxSegments * _segmentSamples];
	memset(_fifo, 0, kMaxSegments * _segmentSamples * sizeof(int16));
	memset(_build, 0, sizeof(_build));
	memset(_effectBuild, 0, sizeof(_effectBuild));

	for (int i = 0; i < kSampleCount; i++) {
		memset(&_samples[i], 0, sizeof(EMIMilesSample));
		_samples[i].status = kSampleFree;
		_streams[i] = nullptr;
	}

	if (_isPs2)
		initSpuEffect();
}

EMIMilesDriver::~EMIMilesDriver() {
	close();

	for (int i = 0; i < kSampleCount; i++) {
		if (_streams[i])
			closeStream(_streams[i]);
	}

	for (int i = 0; i < kSampleCount; i++)
		destroyFilter(&_samples[i]);

	delete[] _fifo;
	_fifo = nullptr;

	if (_spu2Reverb) {
		delete _spu2Reverb;
		_spu2Reverb = nullptr;
	}
}

void *EMIMilesDriver::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void EMIMilesDriver::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

bool EMIMilesDriver::open(Audio::Mixer *mixer) {
	if (!mixer || !mixer->isReady())
		return false;

	{
		Common::StackLock lock(_mutex);

		if (_open)
			return true;

		_mixer = mixer;
		_open = true;
	}

	mixer->playStream(Audio::Mixer::kPlainSoundType, &_handle, this, -1, Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO);

	return true;
}

void EMIMilesDriver::close() {
	Audio::Mixer *mixer = nullptr;

	{
		Common::StackLock lock(_mutex);

		if (!_open)
			return;

		mixer = _mixer;
		_mixer = nullptr;
		_open = false;
	}

	mixer->stopHandle(_handle);
	_handle = Audio::SoundHandle();
}

void EMIMilesDriver::setMasterVolume(int volume) {
	Common::StackLock lock(_mutex);

	_masterVolume = volume;

	for (int i = 0; i < kSampleCount; i++) {
		if (_samples[i].status != kSampleFree)
			computeScales(&_samples[i]);
	}
}

int EMIMilesDriver::getMasterVolume() {
	Common::StackLock lock(_mutex);

	return _masterVolume;
}

void EMIMilesDriver::setMixAhead(int segments) {
	Common::StackLock lock(_mutex);

	_mixAhead = segments;
}

void EMIMilesDriver::releaseHandle() {
	Common::StackLock lock(_mutex);

	if (_released)
		return;

	for (int i = 0; i < kSampleCount; i++) {
		if (_samples[i].status == kSamplePlaying)
			_samples[i].status = kSamplePlayingReleased;
	}

	_released = true;
}

void EMIMilesDriver::reacquireHandle() {
	Common::StackLock lock(_mutex);

	if (!_released)
		return;

	for (int i = 0; i < kSampleCount; i++) {
		if (_samples[i].status == kSamplePlayingReleased)
			_samples[i].status = kSamplePlaying;
	}

	_released = false;
}

int EMIMilesDriver::getActiveSampleCount() {
	Common::StackLock lock(_mutex);

	int count = 0;

	for (int i = 0; i < kSampleCount; i++) {
		if (_samples[i].status == kSamplePlaying || _samples[i].status == kSamplePlayingReleased)
			count++;
	}

	return count;
}

EMIMilesSample *EMIMilesDriver::allocateSample() {
	Common::StackLock lock(_mutex);

	for (int i = 0; i < kSampleCount; i++) {
		if (_samples[i].status == kSampleFree) {
			sampleInit(&_samples[i]);

			return &_samples[i];
		}
	}

	return nullptr;
}

void EMIMilesDriver::releaseSample(EMIMilesSample *sample) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	destroyFilter(sample);
	sample->status = kSampleFree;
}

void EMIMilesDriver::initSample(EMIMilesSample *sample) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	sampleInit(sample);
}

void EMIMilesDriver::setSampleAddress(EMIMilesSample *sample, const byte *address, uint length) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	sample->buffer[0] = address;
	sample->length[0] = (int)length;
	sample->buffer[1] = nullptr;
	sample->length[1] = 0;
}

void EMIMilesDriver::setSampleType(EMIMilesSample *sample, int format) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	sampleSetType(sample, format);
}

void EMIMilesDriver::setSampleRate(EMIMilesSample *sample, int rate) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	sample->rate = rate;
}

void EMIMilesDriver::setSampleLoopCount(EMIMilesSample *sample, int count) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	sample->loopCount = count;
}

int EMIMilesDriver::getSampleLoopCount(EMIMilesSample *sample) {
	if (!sample)
		return 0;

	Common::StackLock lock(_mutex);

	return sample->loopCount;
}

void EMIMilesDriver::setSampleLoopBlock(EMIMilesSample *sample, int start, int end) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	uint32 granularity = (uint32)getFrameBytes(sample->format);

	sample->loopStart = (int)(granularity * (((uint32)start + granularity / 2) / granularity));
	sample->loopEnd = end;
}

void EMIMilesDriver::setSampleVolume(EMIMilesSample *sample, int volume) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	sampleSetVolume(sample, volume);
}

void EMIMilesDriver::setSamplePan(EMIMilesSample *sample, int pan) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	sampleSetPan(sample, pan);
}

void EMIMilesDriver::setSampleSpuVolume(EMIMilesSample *sample, int left, int right) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	sample->spuVolume = true;
	sample->spuVolumeL = left;
	sample->spuVolumeR = right;

	computeScales(sample);
}

void EMIMilesDriver::setSampleEffectSend(EMIMilesSample *sample, bool enabled) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	sample->effectSend = enabled;
}

void EMIMilesDriver::startSample(EMIMilesSample *sample) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	if (sample->status == kSampleFree)
		return;

	int current = sample->current;

	if (sample->length[current] == 0 || !sample->buffer[current])
		return;

	sample->position[current] = 0;
	sample->serviceMode = 1;
	sample->status = _released ? kSamplePlayingReleased : kSamplePlaying;

	// Starting a voice restarts its pitch counter
	sample->spuCounter = 0;
	sample->spuPending = 1;
	memset(sample->spuHistoryL, 0, sizeof(sample->spuHistoryL));
	memset(sample->spuHistoryR, 0, sizeof(sample->spuHistoryR));
}

void EMIMilesDriver::stopSample(EMIMilesSample *sample) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	sampleStop(sample);
}

void EMIMilesDriver::resumeSample(EMIMilesSample *sample) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	sampleResume(sample);
}

void EMIMilesDriver::endSample(EMIMilesSample *sample) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	sampleEnd(sample);
}

int EMIMilesDriver::getSampleStatus(EMIMilesSample *sample) {
	if (!sample)
		return 0;

	Common::StackLock lock(_mutex);

	return sample->status;
}

void EMIMilesDriver::getSampleMsPosition(EMIMilesSample *sample, int *total, int *current) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	int64 bytesPerSecond = (int64)sample->rate * getFrameBytes(sample->format);
	int buffer = sample->current;

	// The original divides by zero here
	if (bytesPerSecond == 0) {
		if (total)
			*total = 0;

		if (current)
			*current = 0;

		return;
	}

	if (total)
		*total = (int)(int64)((double)(uint32)sample->length[buffer] * 1000.0 / (double)bytesPerSecond);

	if (current)
		*current = (int)(int64)((double)(uint32)sample->position[buffer] * 1000.0 / (double)bytesPerSecond);
}

void EMIMilesDriver::setSampleMsPosition(EMIMilesSample *sample, int milliseconds) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	int64 bytesPerSecond = (int64)sample->rate * getFrameBytes(sample->format);
	int64 position = (int64)((double)(uint32)bytesPerSecond * (double)milliseconds * 0.001);

	sampleSetPosition(sample, (int)position);
}

void EMIMilesDriver::setSamplePosition(EMIMilesSample *sample, int position) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	sampleSetPosition(sample, position);
}

int EMIMilesDriver::getSamplePosition(EMIMilesSample *sample) {
	if (!sample)
		return 0;

	Common::StackLock lock(_mutex);

	return sample->position[sample->current];
}

void EMIMilesDriver::setSampleFilter(EMIMilesSample *sample, bool enabled) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	destroyFilter(sample);

	if (enabled)
		sample->filter = EMIMilesFilter::create(_memoryServices, _outputRate);
}

void EMIMilesDriver::setSampleFilterEnvironment(EMIMilesSample *sample, float environment) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	if (sample->filter)
		sample->filter->setEnvironment(environment);
}

void EMIMilesDriver::setSampleFilterTime(EMIMilesSample *sample, float milliseconds) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	if (sample->filter)
		sample->filter->setTime(milliseconds);
}

void EMIMilesDriver::setSampleFilterMix(EMIMilesSample *sample, float mix) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	if (sample->filter)
		sample->filter->setMix(mix);
}

void EMIMilesDriver::setSampleFilterDamping(EMIMilesSample *sample, float hertz) {
	if (!sample)
		return;

	Common::StackLock lock(_mutex);

	if (sample->filter)
		sample->filter->setDamping(hertz);
}

int EMIMilesDriver::readBuffer(int16 *buffer, const int numSamples) {
	Common::StackLock lock(_mutex);

	int written = 0;

	while (written < numSamples) {
		if (_fifoSegments == 0)
			pushSegment();

		int count = MIN(_segmentSamples - _fifoReadOffset, numSamples - written);

		memcpy(buffer + written, _fifo + _fifoReadSegment * _segmentSamples + _fifoReadOffset, count * sizeof(int16));

		written += count;
		_fifoReadOffset += count;

		if (_fifoReadOffset == _segmentSamples) {
			_fifoReadOffset = 0;
			_fifoReadSegment = (_fifoReadSegment + 1) % kMaxSegments;
			_fifoSegments--;
		}
	}

	service();

	return numSamples;
}

bool EMIMilesDriver::isStereo() const {
	return true;
}

int EMIMilesDriver::getRate() const {
	return _outputRate;
}

bool EMIMilesDriver::endOfData() const {
	return false;
}

void EMIMilesDriver::service() {
	int target = CLIP(_mixAhead, 0, (int)kMaxSegments - 1);

	while (_fifoSegments < target + 1)
		pushSegment();
}

void EMIMilesDriver::pushSegment() {
	int writeSegment = (_fifoReadSegment + _fifoSegments) % kMaxSegments;

	mixSegment(_fifo + writeSegment * _segmentSamples);

	_fifoSegments++;
}

void EMIMilesDriver::mixSegment(int16 *out) {
	memset(_build, 0, sizeof(_build));

	if (_spu2Reverb)
		memset(_effectBuild, 0, sizeof(_effectBuild));

	streamServiceAll();

	for (int i = 0; i < kSampleCount; i++) {
		if (_samples[i].status == kSamplePlaying)
			advanceSample(&_samples[i]);
	}

	if (_spu2Reverb)
		mixSpuEffect();

	copyBuildBuffer(_build, out, _segmentSamples);

	_segmentCounter++;
}

void EMIMilesDriver::initSpuEffect() {
	_spu2Reverb = new EMISpu2Reverb();

	// LECAL places the effect area of core 0 at the top of the SPU2 memory and clears it
	_spu2Reverb->setEffectEndAddress(kSpuEffectEndAddress);
	_spu2Reverb->setEffectAttr(EMISpu2Reverb::kModeClear, 0, 0, 0, 0);
	_spu2Reverb->setEffectEnable(true);
	_spu2Reverb->setEffectVolume(0, 0);
}

void EMIMilesDriver::mixSpuEffect() {
	for (int i = 0; i < _segmentSamples; i += 2) {
		int32 left = 0;
		int32 right = 0;

		_spu2Reverb->process(_effectBuild[i] >> 11, _effectBuild[i + 1] >> 11, &left, &right);

		_build[i] += left * (1 << 11);
		_build[i + 1] += right * (1 << 11);
	}
}

int EMIMilesDriver::setSpuReverb(int mode, int core, float depthLeft, float depthRight, float delay, float feedback) {
	if ((uint)mode >= EMISpu2Reverb::kModeCount) {
		warning("EMIMilesDriver::setSpuReverb(): invalid reverb mode %d", mode);

		return 1;
	}

	if (depthLeft < -1.0f || !(depthLeft <= 1.0f)) {
		warning("EMIMilesDriver::setSpuReverb(): left depth %f out of range", depthLeft);

		return 1;
	}

	if (depthRight < -1.0f || !(depthRight <= 1.0f)) {
		warning("EMIMilesDriver::setSpuReverb(): right depth %f out of range", depthRight);

		return 1;
	}

	if (delay < 0.0f || !(delay <= 1.0f)) {
		warning("EMIMilesDriver::setSpuReverb(): delay %f out of range", delay);

		return 1;
	}

	if (feedback < 0.0f || !(feedback <= 1.0f)) {
		warning("EMIMilesDriver::setSpuReverb(): feedback %f out of range", feedback);

		return 1;
	}

	if ((uint)core >= 2) {
		warning("EMIMilesDriver::setSpuReverb(): illegal core %d", core);

		return 1;
	}

	int targetLeft = convertSpuValue(kSpuMaxDepth * depthLeft);
	int targetRight = convertSpuValue(kSpuMaxDepth * depthRight);
	int delayValue = convertSpuValue(kSpuMaxDelay * delay);
	int feedbackValue = convertSpuValue(kSpuMaxDelay * feedback);

	// Only the effect of core 0 is emulated, and the game never sets another core
	if (core != 0 || !_spu2Reverb)
		return 0;

	Common::StackLock lock(_mutex);

	_spu2Reverb->setEffectEnable(false);
	_spu2Reverb->setEffectVolume(0, 0);

	if (mode == 0) {
		// The registers of the previous mode stay, so the effect keeps running silently
		_spu2Reverb->setEffectEnable(true);
		_spu2Reverb->setEffectVolume(0, 0);

		return 0;
	}

	_spu2Reverb->setEffectAttr(mode, 0, 0, delayValue, feedbackValue);
	_spu2Reverb->setEffectEnable(true);

	// The original ramps the volume up with one remote call per step, so only the last step is kept
	int left = 0;
	int right = 0;
	int lastLeft = 0;
	int lastRight = 0;

	while (left < targetLeft && right < targetRight) {
		lastLeft = left;
		lastRight = right;

		if (left < targetLeft) {
			left += kSpuDepthStep;

			if (targetLeft < left)
				left = targetLeft;
		}

		if (right < targetRight) {
			right += kSpuDepthStep;

			// The original compares the right step with the left target
			if (targetLeft < right)
				right = targetRight;
		}
	}

	_spu2Reverb->setEffectVolume(lastLeft, lastRight);

	return 0;
}

int EMIMilesDriver::convertSpuValue(float value) {
	if (value < 0.0f)
		value = 0.0f;

	return (int)value;
}

void EMIMilesDriver::advanceSample(EMIMilesSample *sample) {
	int dst = 0;
	int emptyJumps = 0;
	bool bufferEnded = false;
	bool finished = false;

	while (!finished) {
		int current = sample->current;
		int remaining = sample->length[current] - sample->position[current];

		if (sample->loopCount != 1) {
			if (sample->loopEnd != -1 && (uint32)sample->loopEnd < (uint32)sample->length[current])
				remaining = sample->loopEnd - sample->position[current];
		}

		if (remaining > 0) {
			if (_isPs2)
				dst = mixSpuVoice(sample, remaining, dst);
			else
				dst = mixChunk(sample, remaining, dst);

			emptyJumps = 0;

			if (dst >= _segmentSamples)
				finished = true;

			continue;
		}

		bufferEnded = true;

		if (sample->loopCount != 1) {
			if (++emptyJumps > 1) {
				warning("EMIMilesDriver::advanceSample(): the loop block is empty");
				sample->status = kSampleDone;

				break;
			}

			sample->position[current] = sample->loopStart;

			if (sample->loopCount)
				sample->loopCount--;

			if (dst >= _segmentSamples)
				finished = true;

			continue;
		}

		if (sample->serviceMode != 2) {
			sample->status = kSampleDone;

			break;
		}

		int other = current ^ 1;

		if (sample->bufferEmpty[other]) {
			sample->status = kSampleDone;

			break;
		}

		if (sample->length[other] == 0 || sample->position[other] != 0)
			break;

		sample->current = other;

		if (dst >= _segmentSamples)
			finished = true;
	}

	if (bufferEnded && sample->stream)
		streamService(sample->stream);
}

int EMIMilesDriver::mixChunk(EMIMilesSample *sample, int remainingBytes, int dstOffset) {
	if (sample->rate < 0) {
		warning("EMIMilesDriver::mixChunk(): invalid sample rate %d", sample->rate);

		return _segmentSamples;
	}

	uint flags = kMixOutputStereo;

	if (sample->format & kFormatStereo)
		flags |= kMixSourceStereo;

	if (sample->format & kFormat16Bit)
		flags |= kMix16Bit;

	if (sample->scaleL != kUnscaledGain || sample->scaleR != kUnscaledGain)
		flags |= kMixScaled;

	int ratio = (int)(((int64)sample->rate * 65536 * 2 + _outputRate) / (2 * _outputRate));

	if (ratio > 0x10000 + kResampleTolerance || ratio < 0x10000 - kResampleTolerance)
		flags |= kMixResample;

	const byte *base = sample->buffer[sample->current];
	const byte *src = base + sample->position[sample->current];
	const byte *srcEnd = src + remainingBytes;
	int32 *dst = _build + dstOffset;
	const int32 *dstEnd = _build + _segmentSamples;

	if (dstEnd - dst <= 0 || srcEnd - src <= 0)
		return _segmentSamples;

	flags |= kMixInterpolate;

	if (sample->rate == _outputRate)
		flags &= ~kMixInterpolate;

	_mixSourceLimit = base + sample->length[sample->current];

	bool handled = false;

	if (sample->filter)
		handled = sample->filter->process(&src, &sample->srcFract, srcEnd, &dst, dstEnd, &sample->prevL, &sample->prevR, ratio, sample->scaleL, sample->scaleR, sample->reverbScale, this, flags);

	if (!handled)
		mixerMerge(&src, &sample->srcFract, srcEnd, &dst, dstEnd, &sample->prevL, &sample->prevR, ratio, sample->scaleL, sample->scaleR, flags);

	sample->position[sample->current] = (int)(src - base);

	return (int)(dst - _build);
}

int EMIMilesDriver::mixSpuVoice(EMIMilesSample *sample, int remainingBytes, int dstOffset) {
	if (sample->rate < 0) {
		warning("EMIMilesDriver::mixSpuVoice(): invalid sample rate %d", sample->rate);

		return _segmentSamples;
	}

	MixFormat format;

	format.is16 = (sample->format & kFormat16Bit) != 0;
	format.srcStereo = (sample->format & kFormatStereo) != 0;
	format.scaled = sample->scaleL != kUnscaledGain || sample->scaleR != kUnscaledGain;
	format.scaleL = sample->scaleL;
	format.scaleR = sample->scaleR;
	format.frameBytes = getFrameBytes(sample->format);

	const byte *base = sample->buffer[sample->current];
	const byte *src = base + sample->position[sample->current];
	const byte *srcEnd = src + remainingBytes;
	int32 *dst = _build + dstOffset;
	const int32 *dstEnd = _build + _segmentSamples;
	int32 *send = nullptr;

	// Every voice of core 0 also feeds the reverb
	if (sample->effectSend)
		send = _effectBuild + dstOffset;

	_mixSourceLimit = base + sample->length[sample->current];

	// LECAL sets the pitch register like this, where 0x1000 plays one sample per output frame
	int pitch = (int)(((int64)sample->rate << EMISpu2Interpolation::kPitchShift) / EMISpu2Interpolation::kOutputRate);

	if (pitch > EMISpu2Interpolation::kMaxPitch)
		pitch = EMISpu2Interpolation::kMaxPitch;

	while (dst < dstEnd) {
		while (sample->spuPending > 0 && src < srcEnd) {
			int32 left;
			int32 right;

			fetchFrame(src, format, &left, &right);

			for (int i = 0; i < 3; i++) {
				sample->spuHistoryL[i] = sample->spuHistoryL[i + 1];
				sample->spuHistoryR[i] = sample->spuHistoryR[i + 1];
			}

			sample->spuHistoryL[3] = left;
			sample->spuHistoryR[3] = right;

			src += format.frameBytes;
			sample->spuPending--;
		}

		if (sample->spuPending > 0)
			break;

		int32 outLeft = EMISpu2Interpolation::interpolate(sample->spuCounter, sample->spuHistoryL);
		int32 outRight = outLeft;

		if (format.srcStereo)
			outRight = EMISpu2Interpolation::interpolate(sample->spuCounter, sample->spuHistoryR);

		emitFrame(dst, outLeft, outRight, format);
		dst += 2;

		if (send) {
			emitFrame(send, outLeft, outRight, format);
			send += 2;
		}

		sample->spuCounter += pitch;
		sample->spuPending += (int)(sample->spuCounter >> EMISpu2Interpolation::kPitchShift);
		sample->spuCounter &= EMISpu2Interpolation::kFractionMask;
	}

	sample->position[sample->current] = (int)(src - base);

	return (int)(dst - _build);
}

void EMIMilesDriver::computeScales(EMIMilesSample *sample) {
	// PS2 voice registers scale linearly up to 0x3FFF
	if (sample->spuVolume) {
		int left = CLIP(sample->spuVolumeL, 0, (int)kMaxSpuVolume);
		int right = CLIP(sample->spuVolumeR, 0, (int)kMaxSpuVolume);

		sample->scaleL = (int)((int64)left * _masterVolume / kMaxVolume) >> kSpuVolumeShift;
		sample->scaleR = (int)((int64)right * _masterVolume / kMaxVolume) >> kSpuVolumeShift;

		return;
	}

	sample->volume = CLIP(sample->volume, 0, (int)kMaxVolume);
	sample->pan = CLIP(sample->pan, 0, (int)kMaxVolume);

	int volume = (int)CLIP<int64>((int64)sample->volume * _masterVolume / kMaxVolume, 0, kMaxVolume);

	int leftIndex = kMaxVolume - sample->pan;
	int rightIndex = sample->pan;
	int leftTable = leftIndex <= 62 ? leftIndex * 2 : 128;
	int rightTable = rightIndex <= 62 ? rightIndex * 2 : 128;

	sample->scaleL = volume * leftTable / 8;
	sample->scaleR = volume * rightTable / 8;

	if (sample->scaleL >= 2032)
		sample->scaleL = kUnscaledGain;

	if (sample->scaleR >= 2032)
		sample->scaleR = kUnscaledGain;

	sample->reverbScale = 16 * volume;

	if (sample->reverbScale >= 2032)
		sample->reverbScale = kUnscaledGain;
}

int EMIMilesDriver::getFrameBytes(int format) const {
	return ((format & kFormat16Bit) ? 2 : 1) * ((format & kFormatStereo) ? 2 : 1);
}

void EMIMilesDriver::sampleInit(EMIMilesSample *sample) {
	sample->status = kSampleFree;

	destroyFilter(sample);

	sample->buffer[0] = nullptr;
	sample->buffer[1] = nullptr;
	sample->length[0] = 0;
	sample->length[1] = 0;
	sample->position[0] = 0;
	sample->position[1] = 0;
	sample->bufferEmpty[0] = false;
	sample->bufferEmpty[1] = true;
	sample->srcFract = 0;
	sample->prevL = 0;
	sample->prevR = 0;
	sample->current = 0;
	sample->bufferCursor = -2;
	sample->loopCount = 1;
	sample->loopStart = 0;
	sample->loopEnd = -1;
	sample->format = 0;
	sample->rate = kDefaultSampleRate;
	sample->volume = kMaxVolume;
	sample->pan = kDefaultPan;
	sample->spuVolume = false;
	sample->spuVolumeL = 0;
	sample->spuVolumeR = 0;
	sample->effectSend = true;
	sample->spuCounter = 0;
	sample->spuPending = 1;
	memset(sample->spuHistoryL, 0, sizeof(sample->spuHistoryL));
	memset(sample->spuHistoryR, 0, sizeof(sample->spuHistoryR));

	computeScales(sample);

	sample->status = kSampleDone;
}

void EMIMilesDriver::sampleSetType(EMIMilesSample *sample, int format) {
	sample->format = format & (kFormat16Bit | kFormatStereo);
}

void EMIMilesDriver::sampleSetVolume(EMIMilesSample *sample, int volume) {
	if (sample->volume != volume) {
		sample->volume = volume;
		computeScales(sample);
	}
}

void EMIMilesDriver::sampleSetPan(EMIMilesSample *sample, int pan) {
	if (sample->pan != pan) {
		sample->pan = pan;
		computeScales(sample);
	}
}

void EMIMilesDriver::sampleStop(EMIMilesSample *sample) {
	if (sample->status == kSamplePlaying)
		sample->status = kSampleStopped;
}

void EMIMilesDriver::sampleResume(EMIMilesSample *sample) {
	if (sample->status == kSampleStopped) {
		sample->status = _released ? kSamplePlayingReleased : kSamplePlaying;
	} else if (sample->status == kSampleDone) {
		int current = sample->current;

		if (sample->length[current] != 0 && sample->buffer[current]) {
			sample->serviceMode = 1;
			sample->status = _released ? kSamplePlayingReleased : kSamplePlaying;
		}
	}
}

void EMIMilesDriver::sampleEnd(EMIMilesSample *sample) {
	if (sample->status != kSampleFree && sample->status != kSampleDone)
		sample->status = kSampleDone;
}

void EMIMilesDriver::sampleLoadBuffer(EMIMilesSample *sample, int index, const byte *address, int length) {
	sample->bufferEmpty[index] = length == 0;
	sample->buffer[index] = address;
	sample->length[index] = length;
	sample->position[index] = 0;

	if (length != 0 && sample->status != kSamplePlaying) {
		sample->serviceMode = 2;
		sample->status = _released ? kSamplePlayingReleased : kSamplePlaying;
	}
}

int EMIMilesDriver::sampleBufferReady(EMIMilesSample *sample) {
	if (sample->bufferCursor == -2) {
		sample->bufferEmpty[1] = false;
		sample->bufferCursor = -1;

		return 0;
	}

	int current = sample->current;

	if (sample->bufferCursor == -1) {
		sample->bufferCursor = current;

		return 1;
	}

	if (sample->bufferCursor == current)
		return -1;

	sample->bufferCursor = current;

	return current ^ 1;
}

void EMIMilesDriver::sampleSetPosition(EMIMilesSample *sample, int position) {
	uint32 granularity = (uint32)getFrameBytes(sample->format);

	sample->position[sample->current] = (int)(granularity * (((uint32)position + granularity / 2) / granularity));
}

void EMIMilesDriver::destroyFilter(EMIMilesSample *sample) {
	if (sample->filter) {
		delete sample->filter;
		sample->filter = nullptr;
	}
}

} // End of namespace Grim
