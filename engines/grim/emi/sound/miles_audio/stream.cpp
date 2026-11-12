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

#include "common/debug.h"
#include "common/endian.h"
#include "common/memstream.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "audio/decoders/mp3.h"

#include "engines/grim/emi/sound/miles_audio/driver.h"

namespace Grim {

static const int kStreamNoLoopEnd = 0x7FFFFFFF;
static const int kMp3HeaderSearchBytes = 65536;
static const int kStreamPrimeFrames = 6;
static const int kStreamLookaheadFrames = 2;
static const int kStreamSkipSamples = 2304;
static const int kStreamFadeSlots = 18;
static const int kStreamFadeSlotFrames = 32;
static const int kStreamMaxEmptySegments = 4;

static const int kMp3Bitrates[2][15] = {
	{ 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 },
	{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160 }
};

static const int kMp3Rates[3][3] = {
	{ 11025, 12000, 8000 },
	{ 22050, 24000, 16000 },
	{ 44100, 48000, 32000 }
};

EMIMilesStream *EMIMilesDriver::openStream(Common::SeekableReadStream *file) {
#ifdef USE_MAD
	if (!file)
		return nullptr;

	int dataOffset = 0;
	int dataSize = (int)file->size();

	file->seek(0);

	if (file->readUint32BE() == MKTAG('R', 'I', 'F', 'F')) {
		file->readUint32LE();

		if (file->readUint32BE() == MKTAG('W', 'A', 'V', 'E')) {
			bool foundData = false;

			while (file->pos() + 8 <= file->size()) {
				uint32 chunkId = file->readUint32BE();
				uint32 chunkSize = file->readUint32LE();

				if (chunkId == MKTAG('d', 'a', 't', 'a')) {
					dataOffset = (int)file->pos();
					dataSize = (int)chunkSize;
					foundData = true;

					break;
				}

				file->seek((int64)chunkSize + (chunkSize & 1), SEEK_CUR);
			}

			if (!foundData) {
				warning("EMIMilesDriver::openStream(): no data chunk in the stream");
				delete file;

				return nullptr;
			}
		}
	}

	if ((int64)dataOffset + dataSize > file->size()) {
		warning("EMIMilesDriver::openStream(): the data chunk runs past the end of the file");
		dataSize = (int)file->size() - dataOffset;
	}

	byte *data = new byte[dataSize > 0 ? dataSize : 1];
	int *offsets = nullptr;
	int frameCount = 0;
	int bitrate = 0;
	int rate = 0;
	int samples = 0;
	bool stereo = false;
	bool valid = false;

	if (dataSize > 0) {
		file->seek(dataOffset);

		if (file->read(data, dataSize) == (uint32)dataSize) {
			int searchEnd = MIN(dataSize, kMp3HeaderSearchBytes);

			for (int start = 0; start < searchEnd; start++) {
				if (data[start] != 0xFF)
					continue;

				int count = scanFrames(data, dataSize, start, nullptr, &bitrate, &rate, &stereo, &samples);

				if (count >= 2) {
					offsets = new int[count + 1];
					frameCount = scanFrames(data, dataSize, start, offsets, &bitrate, &rate, &stereo, &samples);
					valid = true;

					break;
				}
			}
		}
	}

	delete file;

	if (!valid) {
		warning("EMIMilesDriver::openStream(): no MP3 frames found");
		delete[] data;

		return nullptr;
	}

	Common::StackLock lock(_mutex);

	int index = -1;

	for (int i = 0; i < kSampleCount; i++) {
		if (_samples[i].status == kSampleFree) {
			index = i;

			break;
		}
	}

	if (index < 0) {
		Common::String slots;

		for (int i = 0; i < kSampleCount; i++)
			slots += Common::String::format(" %d%s", _samples[i].status, _streams[i] ? "S" : "");

		warning("EMIMilesDriver::openStream(): no free sample slot, slots:%s", slots.c_str());
		delete[] data;
		delete[] offsets;

		return nullptr;
	}

	EMIMilesStream *stream = new EMIMilesStream;
	memset(stream, 0, sizeof(EMIMilesStream));

	int channels = stereo ? 2 : 1;
	int frames = (int)((int64)rate * kStreamBufferMilliseconds / 1000);

	stream->sample = &_samples[index];
	stream->data = data;
	stream->frameOffsets = offsets;
	stream->frameCount = frameCount;
	stream->samplesPerFrame = samples;
	stream->channels = channels;
	stream->dataSize = dataSize;
	stream->bytesPerSecond = bitrate / 8;
	stream->rate = rate;
	stream->stereo = stereo;
	stream->loopStart = 0;
	stream->loopEnd = kStreamNoLoopEnd;
	stream->loopCount = 1;
	stream->pcmCapacity = MAX(frames, 1) * channels * (int)sizeof(int16);
	stream->pcm[0] = new byte[stream->pcmCapacity];
	stream->pcm[1] = new byte[stream->pcmCapacity];
	stream->stage = new byte[stream->pcmCapacity];

	sampleInit(stream->sample);
	sampleSetType(stream->sample, kFormat16Bit | (stereo ? kFormatStereo : 0));
	stream->sample->rate = rate;
	stream->sample->stream = stream;
	_streams[index] = stream;

	return stream;
#else
	warning("EMIMilesDriver::openStream(): ScummVM was built without MP3 support");
	delete file;

	return nullptr;
#endif
}

void EMIMilesDriver::closeStream(EMIMilesStream *stream) {
	if (!stream)
		return;

	Common::StackLock lock(_mutex);

	EMIMilesSample *sample = stream->sample;
	int index = (int)(sample - _samples);

	streamDestroyDecoder(stream);
	sampleEnd(sample);
	destroyFilter(sample);
	sample->stream = nullptr;
	sample->status = kSampleFree;
	_streams[index] = nullptr;

	delete[] stream->pcm[0];
	delete[] stream->pcm[1];
	delete[] stream->stage;
	delete[] stream->data;
	delete[] stream->frameOffsets;
	delete stream;
}

void EMIMilesDriver::startStream(EMIMilesStream *stream) {
	if (!stream)
		return;

	Common::StackLock lock(_mutex);

	startStreamInternal(stream);
}

void EMIMilesDriver::pauseStream(EMIMilesStream *stream, bool paused) {
	if (!stream)
		return;

	Common::StackLock lock(_mutex);

	if (paused) {
		if (getStreamStatusInternal(stream) == kSamplePlaying) {
			sampleStop(stream->sample);
			stream->startFlags |= kStreamPaused | kStreamPausedWhilePlaying;
		} else {
			stream->startFlags |= kStreamPaused;
		}
	} else if ((stream->startFlags & 7) != 0) {
		if (stream->startFlags & kStreamPausedWhilePlaying)
			sampleResume(stream->sample);

		stream->startFlags &= ~(kStreamPaused | kStreamPausedWhilePlaying);
		streamService(stream);
	} else {
		startStreamInternal(stream);
	}
}

int EMIMilesDriver::getStreamStatus(EMIMilesStream *stream) {
	if (!stream)
		return -1;

	Common::StackLock lock(_mutex);

	return getStreamStatusInternal(stream);
}

void EMIMilesDriver::setStreamVolume(EMIMilesStream *stream, int volume) {
	if (!stream)
		return;

	Common::StackLock lock(_mutex);

	sampleSetVolume(stream->sample, volume);
}

void EMIMilesDriver::setStreamPan(EMIMilesStream *stream, int pan) {
	if (!stream)
		return;

	Common::StackLock lock(_mutex);

	sampleSetPan(stream->sample, pan);
}

void EMIMilesDriver::setStreamLoopCount(EMIMilesStream *stream, int count) {
	if (!stream)
		return;

	Common::StackLock lock(_mutex);

	stream->loopCount = count;
}

void EMIMilesDriver::setStreamLoopBlock(EMIMilesStream *stream, int start, int end) {
	if (!stream)
		return;

	Common::StackLock lock(_mutex);

	if (start != 0 || end != -1) {
		stream->loopStart = start;
		stream->loopEnd = end == -1 ? stream->dataSize : end;
	} else {
		stream->loopStart = 0;
		stream->loopEnd = kStreamNoLoopEnd;
	}
}

void EMIMilesDriver::setStreamPosition(EMIMilesStream *stream, int bytes) {
	if (!stream)
		return;

	Common::StackLock lock(_mutex);

	setStreamPositionInternal(stream, bytes);
}

void EMIMilesDriver::setStreamPositionSynced(EMIMilesStream *stream, int bytes, uint stamp) {
	if (!stream)
		return;

	Common::StackLock lock(_mutex);

	setStreamPositionInternal(stream, bytes);

	stream->syncStamp = stamp;
	stream->syncPending = true;
}

int EMIMilesDriver::getStreamPosition(EMIMilesStream *stream) {
	if (!stream)
		return -1;

	Common::StackLock lock(_mutex);

	return getStreamPositionInternal(stream);
}

int EMIMilesDriver::getStreamPositionStamped(EMIMilesStream *stream, uint *stamp) {
	if (!stream)
		return -1;

	Common::StackLock lock(_mutex);

	*stamp = _segmentCounter;

	return getStreamPositionInternal(stream);
}

void EMIMilesDriver::setStreamMsPosition(EMIMilesStream *stream, int milliseconds) {
	if (!stream)
		return;

	Common::StackLock lock(_mutex);

	int64 position = (int64)((double)stream->bytesPerSecond * (double)milliseconds * 0.001);

	setStreamPositionInternal(stream, (int)position);
}

void EMIMilesDriver::getStreamMsPosition(EMIMilesStream *stream, int *total, int *current) {
	if (!stream)
		return;

	Common::StackLock lock(_mutex);

	int bytesPerSecond = stream->bytesPerSecond;

	if (total)
		*total = bytesPerSecond ? (int)(int64)((double)stream->dataSize * 1000.0 / (double)bytesPerSecond) : 1;

	if (current)
		*current = bytesPerSecond ? (int)(int64)((double)getStreamPositionInternal(stream) * 1000.0 / (double)bytesPerSecond) : 1;
}

void EMIMilesDriver::streamServiceAll() {
	for (int i = 0; i < kSampleCount; i++) {
		if (_streams[i])
			streamService(_streams[i]);
	}
}

void EMIMilesDriver::streamService(EMIMilesStream *stream) {
	if (!stream->serviceActive)
		return;

	for (;;) {
		if (stream->stageSize == 0 && stream->dataEndState == 0)
			stream->stageSize = streamFill(stream);

		if (!streamFeed(stream))
			break;
	}
}

bool EMIMilesDriver::streamFeed(EMIMilesStream *stream) {
	if ((stream->stageSize == 0 && stream->dataEndState != 1) || stream->startFlags != kStreamStartRequested)
		return false;

	EMIMilesSample *sample = stream->sample;
	int index = sampleBufferReady(sample);

	if (index == -1)
		return false;

	stream->bufferSequence[index] = stream->sequenceCounter++;
	stream->bufferBase[index] = stream->stageBase;

	if (stream->stageSize > 0)
		memcpy(stream->pcm[index], stream->stage, stream->stageSize);

	sampleLoadBuffer(sample, index, stream->pcm[index], stream->stageSize);

	stream->started = true;

	if (stream->dataEndState == 1 && stream->stageSize == 0)
		stream->dataEndState = 2;

	stream->stageSize = 0;

	return true;
}

int EMIMilesDriver::streamFill(EMIMilesStream *stream) {
	int maxSamples = stream->pcmCapacity / (int)sizeof(int16);
	int16 *dst = (int16 *)stream->stage;
	int written = 0;
	int emptySegments = 0;
	bool baseSet = false;

	if (stream->syncPending && !stream->decoder) {
		int64 elapsed = (int64)(uint)(_segmentCounter - stream->syncStamp);

		stream->position += (int)(elapsed * _segmentFrames * stream->bytesPerSecond / _outputRate);
		stream->syncPending = false;
	}

	while (written < maxSamples) {
		if (!stream->decoder && !streamBeginSegment(stream))
			break;

		if (!baseSet) {
			stream->stageBase = stream->segmentStart + streamBytesFromFrames(stream, stream->segmentFrames);
			baseSet = true;
		}

		bool finished = false;

		written += streamDecode(stream, dst + written, maxSamples - written, &finished);

		if (finished) {
			debug(1, "EMIMilesDriver::streamFill(): segment %d..%d out %d limit %d", stream->segmentStart, stream->segmentEnd, stream->segmentOut, stream->segmentLimit);

			stream->position = stream->segmentEnd;
			streamDestroyDecoder(stream);
			baseSet = false;

			if (stream->segmentOut == 0)
				emptySegments++;
			else
				emptySegments = 0;

			if (emptySegments > kStreamMaxEmptySegments) {
				warning("EMIMilesDriver::streamFill(): the loop block produces no audio");
				stream->dataEndState = 1;

				break;
			}

			if (!streamEndSegment(stream))
				break;

			if (written > 0)
				break;
		}
	}

	if (written == 0 && stream->dataEndState == 0)
		stream->dataEndState = 1;

	return written * (int)sizeof(int16);
}

bool EMIMilesDriver::streamBeginSegment(EMIMilesStream *stream) {
#ifdef USE_MAD
	for (int attempt = 0; attempt < 4; attempt++) {
		int end = stream->dataSize;

		if (stream->loopCount != 1 && stream->loopEnd < end)
			end = stream->loopEnd;

		if (stream->position >= 0 && stream->position < end) {
			const int *offsets = stream->frameOffsets;
			int position = stream->position;

			if (position < offsets[stream->frameCount]) {
				int channels = stream->channels;
				int samples = stream->samplesPerFrame;

				int low = 0;
				int high = stream->frameCount - 1;

				while (low < high) {
					int middle = (low + high + 1) / 2;

					if (offsets[middle] <= position)
						low = middle;
					else
						high = middle - 1;
				}

				int first = low;

				low = first + 1;
				high = stream->frameCount;

				while (low < high) {
					int middle = (low + high) / 2;

					if (offsets[middle] >= end)
						high = middle;
					else
						low = middle + 1;
				}

				int last = low;
				int lookahead = MIN((int)kStreamLookaheadFrames, stream->frameCount - last);
				int prime = MAX(first - kStreamPrimeFrames, 0);
				int decoded = streamProbe(stream, prime, first);
				bool firstDecodable = decoded > 0;
				int primeOutput = firstDecodable ? decoded - 1 : 0;
				int realOutput = last - first;
				int fraction = 0;

				if (firstDecodable) {
					if (position > offsets[first])
						fraction = (int)((int64)(position - offsets[first]) * samples / (offsets[first + 1] - offsets[first]));
				} else if (!stream->primeWarned) {
					warning("EMIMilesDriver::streamBeginSegment(): frame %d cannot be decoded after priming", first);
					stream->primeWarned = true;
				}

				stream->decoder = Audio::makePacketizedMP3Stream((uint)channels, (uint)stream->rate);
				stream->segmentStart = position;
				stream->segmentEnd = end;
				stream->segmentFed = offsets[prime];
				stream->segmentFrames = 0;
				stream->segmentSkip = (primeOutput * samples + fraction) * channels;
				stream->segmentFade = primeOutput > 0 || fraction > 0;
				stream->segmentOut = 0;
				stream->terminatorQueued = false;

				if (lookahead > 0) {
					stream->segmentFeedEnd = offsets[last + lookahead];
					stream->segmentLimit = (realOutput * samples - fraction) * channels;
				} else {
					stream->segmentFeedEnd = stream->dataSize;
					stream->segmentLimit = -1;
				}

				return true;
			}

			stream->position = end;
		}

		if (!streamEndSegment(stream))
			return false;
	}

	warning("EMIMilesDriver::streamBeginSegment(): the loop block is empty");
#endif

	stream->dataEndState = 1;

	return false;
}

bool EMIMilesDriver::streamEndSegment(EMIMilesStream *stream) {
	if (stream->position == stream->loopEnd) {
		if (stream->loopCount == 1) {
			stream->loopEnd = kStreamNoLoopEnd;
		} else {
			if (stream->loopCount)
				stream->loopCount--;

			stream->position = stream->loopStart;

			return true;
		}
	}

	if (stream->position == stream->dataSize) {
		if (stream->loopCount == 1) {
			stream->dataEndState = 1;

			return false;
		}

		if (stream->loopCount)
			stream->loopCount--;

		stream->position = 0;

		return true;
	}

	if (stream->position < 0 || stream->position > stream->dataSize || stream->position > stream->loopEnd) {
		warning("EMIMilesDriver::streamEndSegment(): the stream position is outside the data");
		stream->loopEnd = kStreamNoLoopEnd;
		stream->position = CLIP(stream->position, 0, stream->dataSize);
	}

	return true;
}

int EMIMilesDriver::streamDecode(EMIMilesStream *stream, int16 *dst, int maxSamples, bool *finished) {
	int written = 0;
	int16 scratch[kStreamSkipSamples];

	*finished = false;

	while (written < maxSamples) {
		int wanted = maxSamples - written;
		int16 *target = dst + written;
		bool skipping = stream->segmentSkip > 0;

		if (skipping) {
			wanted = MIN(wanted, MIN(stream->segmentSkip, kStreamSkipSamples));
			target = scratch;
		} else if (stream->segmentLimit >= 0) {
			int left = stream->segmentLimit - stream->segmentOut - written;

			if (left <= 0) {
				*finished = true;

				break;
			}

			wanted = MIN(wanted, left);
		}

		int got = stream->decoder->readBuffer(target, wanted);

		if (skipping) {
			stream->segmentSkip -= got;
		} else {
			if (stream->segmentFade) {
				int firstSample = stream->segmentOut + written;

				for (int i = 0; i < got; i++) {
					int slot = ((firstSample + i) / stream->channels) / kStreamFadeSlotFrames;

					if (slot >= kStreamFadeSlots) {
						stream->segmentFade = false;

						break;
					}

					target[i] = (int16)((double)target[i] * sin(M_PI * (slot + 0.5) / (2 * kStreamFadeSlots)));
				}
			}

			written += got;
		}

		if (got == wanted)
			continue;

		if (stream->segmentFed < stream->segmentFeedEnd) {
			int count = MIN((int)kStreamPacketBytes, stream->segmentFeedEnd - stream->segmentFed);

			stream->decoder->queuePacket(new Common::MemoryReadStream(stream->data + stream->segmentFed, (uint32)count, DisposeAfterUse::NO));
			stream->segmentFed += count;
		} else if (!stream->terminatorQueued) {
			stream->decoder->queuePacket(new Common::MemoryReadStream(&stream->terminator, 0));
			stream->terminatorQueued = true;
		} else {
			*finished = true;

			break;
		}
	}

	stream->segmentFrames += written / stream->channels;
	stream->segmentOut += written;

	return written;
}

int EMIMilesDriver::streamProbe(const EMIMilesStream *stream, int from, int first) const {
#ifdef USE_MAD
	int end = MIN(first + 2, stream->frameCount);
	int size = stream->frameOffsets[end] - stream->frameOffsets[from];
	int frameSamples = stream->samplesPerFrame * stream->channels;
	int total = (end - from) * frameSamples;
	int16 *out = new int16[total];
	byte terminator = 0;
	bool terminated = false;
	int written = 0;

	Audio::PacketizedAudioStream *decoder = Audio::makePacketizedMP3Stream((uint)stream->channels, (uint)stream->rate);

	decoder->queuePacket(new Common::MemoryReadStream(stream->data + stream->frameOffsets[from], (uint32)size, DisposeAfterUse::NO));

	while (written < total) {
		int wanted = total - written;
		int got = decoder->readBuffer(out + written, wanted);

		written += got;

		if (got == wanted)
			break;

		if (terminated)
			break;

		decoder->queuePacket(new Common::MemoryReadStream(&terminator, 0));
		terminated = true;
	}

	delete decoder;
	delete[] out;

	return written / frameSamples;
#else
	return 0;
#endif
}

int EMIMilesDriver::scanFrames(const byte *data, int size, int start, int *offsets, int *bitrate, int *rate, bool *stereo, int *samples) {
	int count = 0;
	int position = start;

	while (position + 4 <= size) {
		const byte *header = data + position;

		if (header[0] != 0xFF || (header[1] & 0xE0) != 0xE0)
			break;

		int version = (header[1] >> 3) & 3;
		int layer = (header[1] >> 1) & 3;
		int bitrateIndex = header[2] >> 4;
		int rateIndex = (header[2] >> 2) & 3;

		if (version == 1 || layer != 1 || bitrateIndex == 0 || bitrateIndex == 15 || rateIndex == 3)
			break;

		int family = version == 0 ? 0 : (version == 2 ? 1 : 2);
		int frameBitrate = kMp3Bitrates[version == 3 ? 0 : 1][bitrateIndex] * 1000;
		int frameRate = kMp3Rates[family][rateIndex];
		int frameSamples = version == 3 ? 1152 : 576;
		bool frameStereo = (header[3] >> 6) != 3;
		int length = (version == 3 ? 144 : 72) * frameBitrate / frameRate + ((header[2] >> 1) & 1);

		if (position + length > size)
			break;

		if (count == 0) {
			*bitrate = frameBitrate;
			*rate = frameRate;
			*stereo = frameStereo;
			*samples = frameSamples;
		} else if (frameRate != *rate || frameSamples != *samples || frameStereo != *stereo) {
			break;
		}

		if (offsets)
			offsets[count] = position;

		count++;
		position += length;
	}

	if (offsets && count > 0)
		offsets[count] = position;

	return count;
}

void EMIMilesDriver::streamDestroyDecoder(EMIMilesStream *stream) {
	if (stream->decoder) {
		delete stream->decoder;
		stream->decoder = nullptr;
	}
}

int EMIMilesDriver::streamBytesFromFrames(const EMIMilesStream *stream, int frames) const {
	return (int)((int64)frames * stream->bytesPerSecond / stream->rate);
}

void EMIMilesDriver::startStreamInternal(EMIMilesStream *stream) {
	stream->serviceActive = true;

	if (stream->started) {
		sampleStop(stream->sample);
		setStreamPositionInternal(stream, 0);
	}

	stream->startFlags = kStreamStartRequested;
}

void EMIMilesDriver::setStreamPositionInternal(EMIMilesStream *stream, int bytes) {
	EMIMilesSample *sample = stream->sample;

	if (stream->started) {
		int rate = sample->rate;
		int volume = sample->volume;
		int pan = sample->pan;

		sampleEnd(sample);
		sampleInit(sample);
		sampleSetType(sample, kFormat16Bit | (stream->stereo ? kFormatStereo : 0));
		sample->rate = rate;
		sampleSetVolume(sample, volume);
		sampleSetPan(sample, pan);
	}

	stream->position = bytes;
	stream->syncPending = false;
	streamDestroyDecoder(stream);
	stream->stageSize = 0;
	stream->started = false;

	if (stream->startFlags != kStreamStartRequested)
		stream->startFlags = 0;

	stream->dataEndState = 0;
}

int EMIMilesDriver::getStreamPositionInternal(EMIMilesStream *stream) {
	EMIMilesSample *sample = stream->sample;
	int index = 0;

	if ((int)stream->bufferSequence[0] >= (int)stream->bufferSequence[1] || sample->position[0] == sample->length[0])
		index = 1;

	int frameBytes = stream->stereo ? 4 : 2;

	return stream->bufferBase[index] + streamBytesFromFrames(stream, sample->position[index] / frameBytes);
}

int EMIMilesDriver::getStreamStatusInternal(EMIMilesStream *stream) {
	if (stream->startFlags != kStreamStartRequested || stream->started)
		return stream->sample->status;

	return kSamplePlaying;
}

} // End of namespace Grim
