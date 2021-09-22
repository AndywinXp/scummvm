/* ScummVM - Graphic Adventure Engine
*
* ScummVM is the legal property of its developers, whose names
* are too numerous to list here. Please refer to the COPYRIGHT
* file distributed with this source distribution.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

#include "scumm/dimuse_v2/dimuse_v2.h"

namespace Scumm {

int DiMUSE_v2::streamerInit() {
	for (int l = 0; l < MAX_STREAMS; l++) {
		_streams[l].soundId = 0;
	}
	_lastStreamLoaded = NULL;
	return 0;
}

DiMUSEStream *DiMUSE_v2::streamerAllocateSound(int soundId, int bufId, int maxRead) {
	DiMUSESoundBuffer *bufInfoPtr = _filesHandler->getBufInfo(bufId);
	if (!bufInfoPtr) {
		debug(5, "DiMUSE_v2::streamerAlloc(): ERROR: couldn't get buffer info");
		return NULL;
	}

	if ((bufInfoPtr->bufSize / 4) <= maxRead) {
		debug(5, "DiMUSE_v2::streamerAlloc(): ERROR: maxRead too big for buffer");
		return NULL;
	}

	for (int l = 0; l < MAX_STREAMS; l++) {
		if (_streams[l].soundId && _streams[l].bufId == bufId) {
			debug(5, "DiMUSE_v2::streamerAlloc(): ERROR: stream bufId %d already in use", bufId);
			return NULL;
		}
	}

	for (int l = 0; l < MAX_STREAMS; l++) {
		if (!_streams[l].soundId) {
			_streams[l].endOffset = _filesHandler->seek(soundId, 0, SEEK_END, bufId);
			_streams[l].curOffset = 0;
			_streams[l].soundId = soundId;
			_streams[l].bufId = bufId;
			_streams[l].buf = bufInfoPtr->buffer;
			_streams[l].bufFreeSize = bufInfoPtr->bufSize - maxRead - 4;
			_streams[l].loadSize = bufInfoPtr->loadSize;
			_streams[l].criticalSize = bufInfoPtr->criticalSize;
			_streams[l].maxRead = maxRead;
			_streams[l].loadIndex = 0;
			_streams[l].readIndex = 0;
			_streams[l].paused = 0;
			return &_streams[l];
		}
	}
	debug(5, "DiMUSE_v2::streamerAlloc(): ERROR: no spare streams");
	return NULL;
}

int DiMUSE_v2::streamerClearSoundInStream(DiMUSEStream *streamPtr) {
	streamPtr->soundId = 0;
	if (_lastStreamLoaded == streamPtr) {
		_lastStreamLoaded = 0;
	}
	return 0;
}

int DiMUSE_v2::streamerProcessStreams() {
	dispatchPredictFirstStream();
	DiMUSEStream *stream1 = NULL;
	DiMUSEStream *stream2 = NULL;

	for (int l = 0; l < MAX_STREAMS; l++) {
		if ((_streams[l].soundId) && (!_streams[l].paused)) {
			if (stream2) {
				if (stream1) {
					debug(5, "DiMUSE_v2::streamerProcessStreams(): WARNING: three streams in use");
				} else {
					stream1 = &_streams[l];
				}
			} else {
				stream2 = &_streams[l];
			}
		}
	}

	if (!stream1) {
		if (stream2)
			streamerFetchData(stream2);
		return 0;
	}

	if (!stream2) {
		if (stream1)
			streamerFetchData(stream1);
		return 0;
	}

	int size1 = stream1->loadIndex - stream1->readIndex;
	if (size1 < 0) {
		size1 += stream1->bufFreeSize;
	}

	int size2 = stream2->loadIndex - stream2->readIndex;
	if (size1 < 0) {
		size2 += stream2->bufFreeSize;
	}

	// Check if we've reached or surpassed criticalSize
	int critical1 = (size1 >= stream1->criticalSize);
	int critical2 = (size2 >= stream2->criticalSize);

	if (!critical1) {
		if (!critical2) {
			if (stream1 == _lastStreamLoaded) {
				streamerFetchData(stream1);
				streamerFetchData(stream2);
				return 0;
			} else {
				streamerFetchData(stream2);
				streamerFetchData(stream1);
				return 0;
			}
		} else {
			streamerFetchData(stream1);
			return 0;
		}
	}

	if (!critical2) {
		streamerFetchData(stream2);
		return 0;
	} else {
		if (stream1 == _lastStreamLoaded) {
			streamerFetchData(stream1);
		} else {
			streamerFetchData(stream2);
		}
		return 0;
	}
}

uint8 *DiMUSE_v2::streamerReAllocReadBuffer(DiMUSEStream *streamPtr, int reallocSize) {
	int size = streamPtr->loadIndex - streamPtr->readIndex;

	if (size < 0)
		size += streamPtr->bufFreeSize;

	if (size < reallocSize || streamPtr->maxRead < reallocSize)
		return 0;

	if (streamPtr->bufFreeSize - streamPtr->readIndex < reallocSize) {
		memcpy(&streamPtr->buf[streamPtr->bufFreeSize], streamPtr->buf, reallocSize - streamPtr->bufFreeSize + streamPtr->readIndex + 4);
	}

	int readIndex_tmp = streamPtr->readIndex;
	uint8 *ptr = &streamPtr->buf[readIndex_tmp];
	streamPtr->readIndex = readIndex_tmp + reallocSize;
	if (streamPtr->bufFreeSize <= readIndex_tmp + reallocSize)
		streamPtr->readIndex = readIndex_tmp + reallocSize - streamPtr->bufFreeSize;

	return ptr;
}

uint8 *DiMUSE_v2::streamerCopyBufferAbsolute(DiMUSEStream *streamPtr, int offset, int size) {
	int value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufFreeSize;
	if (offset + size > value || streamPtr->maxRead < size)
		return 0;
	int offsetReadIndex = offset + streamPtr->readIndex;
	if (offsetReadIndex >= streamPtr->bufFreeSize) {
		offsetReadIndex -= streamPtr->bufFreeSize;
	}

	if (streamPtr->bufFreeSize - offsetReadIndex < size) {
		memcpy(&streamPtr->buf[streamPtr->bufFreeSize], streamPtr->buf, size + offsetReadIndex - streamPtr->bufFreeSize);
	}

	return &streamPtr->buf[offsetReadIndex];
}

int DiMUSE_v2::streamerSetIndex1(DiMUSEStream *streamPtr, int offset) {
	_streamerBailFlag = 1;
	int value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufFreeSize;
	if (offset > value)
		return -1;
	int offsetReadIndex = streamPtr->readIndex + offset;
	streamPtr->readIndex = offsetReadIndex;
	if (streamPtr->bufFreeSize <= offsetReadIndex) {
		streamPtr->readIndex = offsetReadIndex - streamPtr->bufFreeSize;
	}
	return 0;
}

int DiMUSE_v2::streamerSetIndex2(DiMUSEStream *streamPtr, int offset) {
	_streamerBailFlag = 1;
	int value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufFreeSize;
	if (value < offset)
		return -1;
	int offsetReadIndex = streamPtr->readIndex + offset;
	streamPtr->loadIndex = offsetReadIndex;
	if (offsetReadIndex >= streamPtr->bufFreeSize) {
		streamPtr->loadIndex = offsetReadIndex - streamPtr->bufFreeSize;
	}
	return 0;
}

int DiMUSE_v2::streamerGetFreeBuffer(DiMUSEStream *streamPtr) {
	int freeBufferSize = streamPtr->loadIndex - streamPtr->readIndex;
	if (freeBufferSize < 0)
		freeBufferSize += streamPtr->bufFreeSize;
	return freeBufferSize;
}

int DiMUSE_v2::streamerSetSoundToStreamWithCurrentOffset(DiMUSEStream *streamPtr, int soundId, int currentOffset) {
	_streamerBailFlag = 1;
	streamPtr->soundId = soundId;
	streamPtr->curOffset = currentOffset;
	streamPtr->endOffset = 0;
	streamPtr->paused = 0;
	if (_lastStreamLoaded == streamPtr) {
		_lastStreamLoaded = NULL;
	}
	return 0;
}

int DiMUSE_v2::streamerQueryStream(DiMUSEStream *streamPtr, int *bufSize, int *criticalSize, int *freeSpace, int *paused) {
	dispatchPredictFirstStream();
	*bufSize = streamPtr->bufFreeSize;
	*criticalSize = streamPtr->criticalSize;
	int value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufFreeSize;
	*freeSpace = value;
	*paused = streamPtr->paused;
	return 0;
}

int DiMUSE_v2::streamerFeedStream(DiMUSEStream *streamPtr, uint8 *srcBuf, int sizeToFeed, int paused) {
	int size = streamPtr->readIndex - streamPtr->loadIndex;
	if (size <= 0)
		size += streamPtr->bufFreeSize;
	if (sizeToFeed > (size - 4)) {
		// Don't worry, judging from the intro of The Dig, this is totally expected
		// to happen, and when it does, the output matches the EXE behavior
		debug(5, "DiMUSE_v2::streamerFeedStream(): WARNING: buffer overflow");
		_streamerBailFlag = 1;

		int newTentativeSize = sizeToFeed - (size - 4) - (sizeToFeed - (size - 4)) % 12 + 12;
		size = streamPtr->loadIndex - streamPtr->readIndex;
		if (size < 0)
			size += streamPtr->bufFreeSize;
		if (size >= newTentativeSize) {
			streamPtr->readIndex = newTentativeSize + streamPtr->readIndex;
			if (streamPtr->bufFreeSize <= streamPtr->readIndex)
				streamPtr->readIndex -= streamPtr->bufFreeSize;
		}
	}

	if (sizeToFeed > 0) {
		do {
			size = streamPtr->bufFreeSize - streamPtr->loadIndex;
			if (size >= sizeToFeed) {
				size = sizeToFeed;
			}
			sizeToFeed -= size;
			memcpy(&streamPtr->buf[streamPtr->loadIndex], srcBuf, size);
			srcBuf += size;

			int val = size + streamPtr->loadIndex;
			streamPtr->curOffset += size;
			streamPtr->loadIndex += size;
			if (val >= streamPtr->bufFreeSize) {
				streamPtr->loadIndex = val - streamPtr->bufFreeSize;
			}
		} while (sizeToFeed > 0);
	}

	streamPtr->paused = paused;
	return 0;
}

int DiMUSE_v2::streamerFetchData(DiMUSEStream *streamPtr) {
	if (streamPtr->endOffset == 0) {
		streamPtr->endOffset = _filesHandler->seek(streamPtr->soundId, 0, SEEK_END, streamPtr->bufId);
	}

	int size = streamPtr->readIndex - streamPtr->loadIndex;
	if (size <= 0)
		size += streamPtr->bufFreeSize;

	int loadSize = streamPtr->loadSize;
	int remainingSize = streamPtr->endOffset - streamPtr->curOffset;

	if (loadSize >= size - 4) {
		loadSize = size - 4;
	}

	if (loadSize >= remainingSize) {
		loadSize = remainingSize;
	}

	if (remainingSize <= 0) {
		streamPtr->paused = 1;

		// Pad the buffer
		streamPtr->buf[streamPtr->loadIndex] = 127;
		streamPtr->loadIndex++;
		streamPtr->buf[streamPtr->loadIndex] = 127;
		streamPtr->loadIndex++;
		streamPtr->buf[streamPtr->loadIndex] = 127;
		streamPtr->loadIndex++;
		streamPtr->buf[streamPtr->loadIndex] = 127;
		streamPtr->loadIndex++;
	}

	int actualAmount;
	int requestedAmount;

	while (1) {
		if (loadSize <= 0)
			return 0;

		requestedAmount = streamPtr->bufFreeSize - streamPtr->loadIndex;
		if (requestedAmount >= loadSize) {
			requestedAmount = loadSize;
		}

		if (_filesHandler->seek(streamPtr->soundId, streamPtr->curOffset, SEEK_SET, streamPtr->bufId) != streamPtr->curOffset) {
			debug(5, "DiMUSE_v2::streamerFetchData(): ERROR: invalid seek in streamer (%d)", streamPtr->curOffset);
			streamPtr->paused = 1;
			return 0;
		}

		_streamerBailFlag = 0;
		Common::StackLock lock(_mutex);
		//waveOutDecreaseSlice();
		//debug(5, "streamer_fetchData() called waveOutDecreaseSlice(): %d", _waveSlicingHalted);
		actualAmount = _filesHandler->read(streamPtr->soundId, &streamPtr->buf[streamPtr->loadIndex], requestedAmount, streamPtr->bufId);
		//waveOutIncreaseSlice();
		//debug(5, "streamer_fetchData() called waveOutIncreaseSlice(): %d", _waveSlicingHalted);
		Common::StackLock unlock(_mutex);
		if (_streamerBailFlag != 0)
			return 0;

		loadSize -= actualAmount;
		streamPtr->curOffset += actualAmount;
		_lastStreamLoaded = streamPtr;

		int newLoadIndex = actualAmount + streamPtr->loadIndex;
		streamPtr->loadIndex = newLoadIndex;
		if (newLoadIndex >= streamPtr->bufFreeSize) {
			streamPtr->loadIndex = newLoadIndex - streamPtr->bufFreeSize;
		}
	}

	debug(5, "DiMUSE_v2::streamerFetchData(): ERROR: unable to load the correct amount of data (req=%d, act=%d)", requestedAmount, actualAmount);
	_lastStreamLoaded = NULL;
	return 0;
}

} // End of namespace Scumm
